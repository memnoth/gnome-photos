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

#include <setjmp.h>

#include <glib/gi18n.h>
#include <jpeglib.h>

#include "photos-error.h"
#include "photos-gegl.h"
#include "photos-gegl-buffer-codec-jpeg.h"


typedef struct _PhotosJpegErrorMgr PhotosJpegErrorMgr;
typedef struct _PhotosJpegSourceMgr PhotosJpegSourceMgr;

struct _PhotosJpegErrorMgr
{
  struct jpeg_error_mgr pub;
  GError **error;
  sigjmp_buf setjmp_buffer;
};

struct _PhotosJpegSourceMgr
{
  struct jpeg_source_mgr pub;
  JOCTET buffer[JPEG_PROG_BUF_SIZE];
  glong skip_next;
};

struct _PhotosGeglBufferCodecJpeg
{
  PhotosGeglBufferCodec parent_instance;
  PhotosJpegErrorMgr jerr;
  gboolean decoding;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_source_mgr src;
};


G_DEFINE_TYPE_WITH_CODE (PhotosGeglBufferCodecJpeg, photos_gegl_buffer_codec_jpeg, PHOTOS_TYPE_GEGL_BUFFER_CODEC,
                         photos_gegl_ensure_extension_points ();
                         g_io_extension_point_implement (PHOTOS_GEGL_BUFFER_CODEC_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "jpeg",
                                                         0));


static const gchar *MIME_TYPES[] =
{
  "image/jpeg",
  NULL
};


static void
photos_gegl_buffer_codec_jpeg_error_exit (j_common_ptr cinfo)
{
  PhotosJpegErrorMgr *jerr = (PhotosJpegErrorMgr *) cinfo->err;
  gchar buffer[JMSG_LENGTH_MAX];

  cinfo->err->format_message (cinfo, buffer);
  g_set_error (jerr->error, PHOTOS_ERROR, 0, _("Error interpreting JPEG image file (%s)"), buffer);
  siglongjmp (jerr->setjmp_buffer, 1);
  g_assert_not_reached ();
}


static void
photos_gegl_buffer_codec_jpeg_fill_input_buffer (j_decompress_ptr cinfo)
{
  return FALSE;
}


static void
photos_gegl_buffer_codec_jpeg_init_source (j_decompress_ptr cinfo)
{
}


static void
photos_gegl_buffer_codec_jpeg_output_message (j_common_ptr cinfo)
{
}


static void
photos_gegl_buffer_codec_jpeg_skip_input_data (j_decompress_ptr cinfo, glong num_bytes)
{
}


static void
photos_gegl_buffer_codec_jpeg_term_source (j_decompress_ptr cinfo)
{
}


static gboolean
photos_gegl_buffer_codec_jpeg_load_begin (PhotosGeglBufferCodec *codec, GError **error)
{
  PhotosGeglBufferCodecJpeg *self = PHOTOS_GEGL_BUFFER_CODEC_JPEG (codec);

  g_return_val_if_fail (!self->decoding, FALSE);

  self->cinfo.err = jpeg_std_error (&self->jerr.pub);
  self->jerr.pub.error_exit = photos_gegl_buffer_codec_jpeg_error_exit;
  self->jerr.pub.output_message = photos_gegl_buffer_codec_jpeg_output_message;
  self->jerr.error = error;

  if (sigsetjmp (self->jerr.setjmp_buffer, 1))
    {
      jpeg_destroy_decompress (&self->cinfo);
      self->decoding = FALSE;
      goto out;
    }

  jpeg_create_decompress (&self->cinfo);
  self->decoding = TRUE;

  self->cinfo.src = &self->src;
  self->cinfo.src->init_source = photos_gegl_buffer_codec_jpeg_init_source;
  self->cinfo.src->fill_input_buffer = photos_gegl_buffer_codec_jpeg_fill_input_buffer;
  self->cinfo.src->skip_input_data = photos_gegl_buffer_codec_jpeg_skip_input_data;
  self->cinfo.src->resync_to_restart = jpeg_resync_to_restart;
  self->cinfo.src->term_source = photos_gegl_buffer_codec_jpeg_term_source;
  self->cinfo.src->next_input_byte = NULL;

  self->jerr.error = NULL;

  g_return_val_if_fail (self->decoding, FALSE);

 out:
  return self->decoding;
}


static gboolean
photos_gegl_buffer_codec_jpeg_load_increment (PhotosGeglBufferCodec *codec,
                                              const guchar *buf,
                                              gsize count,
                                              GError **error)
{
  PhotosGeglBufferCodecJpeg *self = PHOTOS_GEGL_BUFFER_CODEC_JPEG (codec);
  gboolean ret_val = FALSE;

  g_return_val_if_fail (self->decoding, FALSE);

  self->jerr.error = error;

  if (sigsetjmp (self->jerr.setjmp_buffer, 1))
    {
      ret_val = FALSE;
      goto out;
    }
}


static gboolean
photos_gegl_buffer_codec_jpeg_load_stop (PhotosGeglBufferCodec *codec, GError **error)
{
}


static void
photos_gegl_buffer_codec_jpeg_constructed (GObject *object)
{
  PhotosGeglBufferCodecJpeg *self = PHOTOS_GEGL_BUFFER_CODEC_JPEG (object);

  G_OBJECT_CLASS (photos_gegl_buffer_codec_jpeg_parent_class)->constructed (object);
}


static void
photos_gegl_buffer_codec_jpeg_dispose (GObject *object)
{
  PhotosGeglBufferCodecJpeg *self = PHOTOS_GEGL_BUFFER_CODEC_JPEG (object);

  G_OBJECT_CLASS (photos_gegl_buffer_codec_jpeg_parent_class)->dispose (object);
}


static void
photos_gegl_buffer_codec_jpeg_finalize (GObject *object)
{
  PhotosGeglBufferCodecJpeg *self = PHOTOS_GEGL_BUFFER_CODEC_JPEG (object);

  if (self->decoding)
    jpeg_destroy_decompress (&self->cinfo);

  G_OBJECT_CLASS (photos_gegl_buffer_codec_jpeg_parent_class)->finalize (object);
}


static void
photos_gegl_buffer_codec_jpeg_init (PhotosGeglBufferCodecJpeg *self)
{
}


static void
photos_gegl_buffer_codec_jpeg_class_init (PhotosGeglBufferCodecJpegClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PhotosGeglBufferCodecClass *buffer_codec_class = PHOTOS_GEGL_BUFFER_CODEC_CLASS (class);

  buffer_codec_class->mime_types = MIME_TYPES;

  object_class->constructed = photos_gegl_buffer_codec_jpeg_constructed;
  object_class->dispose = photos_gegl_buffer_codec_jpeg_dispose;
  object_class->finalize = photos_gegl_buffer_codec_jpeg_finalize;
  buffer_codec_class->load_begin = photos_gegl_buffer_codec_jpeg_load_begin;
  buffer_codec_class->load_increment = photos_gegl_buffer_codec_jpeg_load_increment;
  buffer_codec_class->load_stop = photos_gegl_buffer_codec_jpeg_load_stop;
}
