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

#include "photos-gegl-buffer-codec.h"
#include "photos-marshalers.h"


struct _PhotosGeglBufferCodecPrivate
{
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


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PhotosGeglBufferCodec, photos_gegl_buffer_codec, G_TYPE_OBJECT);


static void
photos_gegl_buffer_codec_constructed (GObject *object)
{
  PhotosGeglBufferCodec *self = PHOTOS_GEGL_BUFFER_CODEC (object);
  PhotosGeglBufferCodecPrivate *priv;

  priv = photos_gegl_buffer_codec_get_instance_private (self);

  G_OBJECT_CLASS (photos_gegl_buffer_codec_parent_class)->constructed (object);
}


static void
photos_gegl_buffer_codec_dispose (GObject *object)
{
  PhotosGeglBufferCodec *self = PHOTOS_GEGL_BUFFER_CODEC (object);
  PhotosGeglBufferCodecPrivate *priv;

  priv = photos_gegl_buffer_codec_get_instance_private (self);

  G_OBJECT_CLASS (photos_gegl_buffer_codec_parent_class)->dispose (object);
}


static void
photos_gegl_buffer_codec_finalize (GObject *object)
{
  PhotosGeglBufferCodec *self = PHOTOS_GEGL_BUFFER_CODEC (object);
  PhotosGeglBufferCodecPrivate *priv;

  priv = photos_gegl_buffer_codec_get_instance_private (self);

  G_OBJECT_CLASS (photos_gegl_buffer_codec_parent_class)->finalize (object);
}


static void
photos_gegl_buffer_codec_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  PhotosGeglBufferCodec *self = PHOTOS_GEGL_BUFFER_CODEC (object);
  PhotosGeglBufferCodecPrivate *priv;

  priv = photos_gegl_buffer_codec_get_instance_private (self);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
photos_gegl_buffer_codec_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  PhotosGeglBufferCodec *self = PHOTOS_GEGL_BUFFER_CODEC (object);
  PhotosGeglBufferCodecPrivate *priv;

  priv = photos_gegl_buffer_codec_get_instance_private (self);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
photos_gegl_buffer_codec_init (PhotosGeglBufferCodec *self)
{
  PhotosGeglBufferCodecPrivate *priv;

  priv = photos_gegl_buffer_codec_get_instance_private (self);
}


static void
photos_gegl_buffer_codec_class_init (PhotosGeglBufferCodecClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructed = photos_gegl_buffer_codec_constructed;
  object_class->dispose = photos_gegl_buffer_codec_dispose;
  object_class->finalize = photos_gegl_buffer_codec_finalize;
  object_class->get_property = photos_gegl_buffer_codec_get_property;
  object_class->set_property = photos_gegl_buffer_codec_set_property;

  signals[SIZE_PREPARED] = g_signal_new ("size-prepared",
                                         G_TYPE_FROM_CLASS (class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (PhotosGeglBufferCodecClass, size_prepared),
                                         NULL, /* accumulator */
                                         NULL, /* accu_data */
                                         _photos_marshal_VOID__INT_INT,
                                         G_TYPE_NONE,
                                         2,
                                         G_TYPE_INT,
                                         G_TYPE_INT);
}


gboolean
photos_gegl_buffer_codec_load_begin (PhotosGeglBufferCodec *self, GError **error)
{
  gboolean ret_val;

  g_return_val_if_fail (PHOTOS_IS_GEGL_BUFFER_CODEC (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret_val = PHOTOS_GEGL_BUFFER_CODEC_GET_CLASS (self)->load_begin (self, error);
  return ret_val;
}


gboolean
photos_gegl_buffer_codec_load_increment (PhotosGeglBufferCodec *self,
                                         const guchar *buf,
                                         gsize count,
                                         GError **error)
{
  gboolean ret_val;

  g_return_val_if_fail (PHOTOS_IS_GEGL_BUFFER_CODEC (self), FALSE);
  g_return_val_if_fail (buf != NULL, FALSE);
  g_return_val_if_fail (count > 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret_val = PHOTOS_GEGL_BUFFER_CODEC_GET_CLASS (self)->load_increment (self, buf, count, error);
  return ret_val;
}


gboolean
photos_gegl_buffer_codec_load_stop (PhotosGeglBufferCodec *self, GError **error)
{
  gboolean ret_val;

  g_return_val_if_fail (PHOTOS_IS_GEGL_BUFFER_CODEC (self), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret_val = PHOTOS_GEGL_BUFFER_CODEC_GET_CLASS (self)->load_stop (self, error);
  return ret_val;
}
