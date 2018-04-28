/*
 * Photos - access, organize and share your photos on GNOME
 * Copyright © 2018 Red Hat, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Based on code from:
 *   + GdkPixbuf
 */


#include "config.h"

#include <string.h>

#include <glib/gi18n.h>

#include "photos-error.h"
#include "photos-gegl-buffer-codec.h"
#include "photos-gegl-buffer-loader.h"
#include "photos-marshalers.h"


enum
{
  SNIFF_BUFFER_SIZE = 0
};

struct _PhotosGeglBufferLoader
{
  GObject parent_instance;
  PhotosGeglBufferCodec *codec;
  gboolean closed;
  guchar header_buf[SNIFF_BUFFER_SIZE];
  gsize header_buf_offset;
};

enum
{
  PROP_0,
};

enum
{
  SIZE_PREPARED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (PhotosGeglBufferLoader, photos_gegl_buffer_loader, G_TYPE_OBJECT);


static PhotosGeglBufferCodec *
photos_gegl_buffer_loader_find_codec (PhotosGeglBufferLoader *self, GError **error)
{
  GIOExtensionPoint *extension_point;
  GList *extensions;
  GList *l;
  PhotosGeglBufferCodec *ret_val = NULL;
  g_autoptr (PhotosGeglBufferCodec) codec = NULL;
  gboolean uncertain;
  g_autofree gchar *content_type = NULL;

  content_type = g_content_type_guess (NULL, self->header_buf, self->header_buf_offset, &uncertain);
  if ((uncertain
       || g_strcmp0 (content_type, "text/plain") == 0
       || g_strcmp0 (content_type, "application/gzip") == 0)
      && self->path != NULL)
    {
      g_free (content_type);
      content_type = g_content_type_guess (self->path, self->header_buf, self->header_buf_offset, NULL);
    }

  extension_point = g_io_extension_point_lookup (PHOTOS_GEGL_BUFFER_CODEC_EXTENSION_POINT_NAME);
  extensions = g_io_extension_point_get_extensions (extension_point);
  for (l = extensions; l != NULL; l = l->next)
    {
      GIOExtension *extension = (GIOExtension *) l->data;
      PhotosGeglBufferCodecClass *buffer_codec_class; /* TODO: use g_autoptr */
      gint i;

      buffer_codec_class = PHOTOS_GEGL_BUFFER_CODEC_CLASS (g_io_extension_ref_class (extension));
      for (i = 0; buffer_codec_class->mime_types[i] != NULL; i++)
        {
          g_autofree gchar *codec_content_type = NULL;

          codec_content_type = g_content_type_from_mime_type (buffer_codec_class->mime_types[i]);
          if (g_content_type_equals (codec_content_type, content_type))
            {
              GType type;

              type = g_io_extension_get_type (extension);
              codec = PHOTOS_GEGL_BUFFER_CODEC (g_object_new (type, NULL));
              break;
            }
        }

      g_type_class_unref (buffer_codec_class);
    }

  if (codec == NULL)
    {
      if (self->path == NULL)
        {
          g_set_error_literal (error, PHOTOS_ERROR, 0, _("Unrecognized image file format"));
        }
      else
        {
          g_autofree gchar *display_name = NULL;

          display_name = g_filename_display_name (self->path);
          g_set_error (error,
                       PHOTOS_ERROR,
                       0,
                       _("Couldn’t recognize the image file format for file “%s”"),
                       display_name);
        }

      goto out;
    }

  ret_val = g_object_ref (codec);

 out:
  return ret_val;
}


static gboolean
photos_gegl_buffer_loader_load_codec (PhotosGeglBufferLoader *self, const gchar *codec_name, GError **error)
{
  gboolean ret_val = FALSE;

  g_return_val_if_fail (self->codec == NULL, FALSE);

  if (codec_name == NULL)
    {
      self->codec = photos_gegl_buffer_loader_find_codec (self, error);
      if (self->codec == NULL)
        goto out;
    }
  else
    {
      GIOExtension *extension;
      GIOExtensionPoint *extension_point;
      GType type;

      extension_point = g_io_extension_point_lookup (PHOTOS_GEGL_BUFFER_CODEC_EXTENSION_POINT_NAME);
      extension = g_io_extension_point_get_extension_by_name (extension_point, codec_name);
      if (extension == NULL)
        goto out;

      type = g_io_extension_get_type (extension);
      self->codec = PHOTOS_GEGL_BUFFER_CODEC (g_object_new (type, NULL));
    }

  if (!photos_gegl_buffer_codec_load_begin (self->codec, error))
    goto out;

  if (self->header_buf_offset > 0)
    {
      if (!photos_gegl_buffer_codec_load_increment (self, self->header_buf, self->header_buf_offset, error))
        goto out;
    }

  ret_val = TRUE;

 out:
  return ret_val;
}


static gsize
photos_gegl_buffer_loader_eat_header (PhotosGeglBufferLoader *self, const guchar *buf, gsize count, GError **error)
{
  gsize n_bytes_to_eat;
  gsize ret_val = 0;

  g_return_val_if_fail (self->header_buf_offset < SNIFF_BUFFER_SIZE, 0);

  n_bytes_to_eat = MIN (SNIFF_BUFFER_SIZE - self->header_buf_offset, count);
  memcpy (self->header_buf + self->header_buf_offset, buf, n_bytes_to_eat);

  self->header_buf_offset += n_bytes_to_eat;
  g_return_val_if_fail (self->header_buf_offset <= SNIFF_BUFFER_SIZE, 0);

  if (self->header_buf_offset == SNIFF_BUFFER_SIZE)
    {
      if (!photos_gegl_buffer_loader_load_codec (self, NULL, error))
        goto out;
    }
  else
    {
      g_return_val_if_fail (n_bytes_to_eat == count, 0);
    }

  ret_val = n_bytes_to_eat;

 out:
  return ret_val;
}


static void
photos_gegl_buffer_loader_constructed (GObject *object)
{
  PhotosGeglBufferLoader *self = PHOTOS_GEGL_BUFFER_LOADER (object);

  G_OBJECT_CLASS (photos_gegl_buffer_loader_parent_class)->constructed (object);
}


static void
photos_gegl_buffer_loader_dispose (GObject *object)
{
  PhotosGeglBufferLoader *self = PHOTOS_GEGL_BUFFER_LOADER (object);

  G_OBJECT_CLASS (photos_gegl_buffer_loader_parent_class)->dispose (object);
}


static void
photos_gegl_buffer_loader_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  PhotosGeglBufferLoader *self = PHOTOS_GEGL_BUFFER_LOADER (object);

  switch (prop_id)
    {
    case PROP_GESTURE:
      g_value_set_object (value, self->gesture);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
photos_gegl_buffer_loader_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  PhotosGeglBufferLoader *self = PHOTOS_GEGL_BUFFER_LOADER (object);

  switch (prop_id)
    {
    case PROP_GESTURE:
      self->gesture = GTK_GESTURE (g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
photos_gegl_buffer_loader_init (PhotosGeglBufferLoader *self)
{
}


static void
photos_gegl_buffer_loader_class_init (PhotosGeglBufferLoaderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructed = photos_gegl_buffer_loader_constructed;
  object_class->dispose = photos_gegl_buffer_loader_dispose;
  object_class->get_property = photos_gegl_buffer_loader_get_property;
  object_class->set_property = photos_gegl_buffer_loader_set_property;

  signals[SIZE_PREPARED] = g_signal_new ("size-prepared",
                                         G_TYPE_FROM_CLASS (class),
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL, /* accumulator */
                                         NULL, /* accu_data */
                                         _photos_marshal_VOID__INT_INT,
                                         G_TYPE_NONE,
                                         2,
                                         G_TYPE_INT,
                                         G_TYPE_INT);
}


PhotosGeglBufferLoader *
photos_gegl_buffer_loader_new (void)
{
  return g_object_new (PHOTOS_TYPE_GEGL_BUFFER_LOADER, NULL);
}


gboolean
photos_gegl_buffer_loader_close (PhotosGeglBufferLoader *self, GError **error)
{
  g_return_val_if_fail (PHOTOS_IS_GEGL_BUFFER_LOADER (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (self->closed)
    goto out;

  if (self->codec == NULL)
    {
      {
        g_autoptr (GError) local_error = NULL;

        if (!photos_gegl_buffer_loader_load_codec (self, NULL, &local_error))
          {
            g_propagate_error (error, g_steal_pointer (&local_error));
            goto out;
          }
      }
    }

  g_return_val_if_fail (PHOTOS_IS_GEGL_BUFFER_CODEC (self->codec), FALSE);

  {
    g_autoptr (GError) local_error = NULL;

    if (!photos_gegl_buffer_codec_load_stop (self->codec, &local_error))
      {
        g_propagate_error (error, g_steal_pointer (&local_error));
        goto out;
      }
  }

  self->closed = TRUE;

 out:
  return self->closed;
}


gboolean
photos_gegl_buffer_loader_write_bytes (PhotosGeglBufferLoader *self,
                                       GBytes *bytes,
                                       GCancellable *cancellable,
                                       GError **error)
{
  gboolean ret_val = FALSE;
  gconstpointer data;
  gsize count;

  g_return_val_if_fail (PHOTOS_IS_GEGL_BUFFER_LOADER (self), FALSE);
  g_return_val_if_fail (bytes != NULL, FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data = g_bytes_get_data (bytes, &count);
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (count > 0, FALSE);

  g_return_val_if_fail (!self->closed, FALSE);

  if (self->codec == NULL)
    {
      gssize eaten;

      eaten = photos_gegl_buffer_loader_eat_header (self, data, count, error);
      if (eaten == 0)
        goto out;

      count -= eaten;
      data += eaten;
    }

  g_return_val_if_fail (count == 0 || self->codec != NULL, FALSE);

  if (count > 0)
    {
      if (!photos_gegl_buffer_codec_load_increment (self->codec, data, count, error))
        goto out;
    }

  ret_val = TRUE;

 out:
  return ret_val;
}
