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

#ifndef PHOTOS_GEGL_BUFFER_CODEC_H
#define PHOTOS_GEGL_BUFFER_CODEC_H

#include <gegl.h>

G_BEGIN_DECLS

#define PHOTOS_GEGL_BUFFER_CODEC_EXTENSION_POINT_NAME "photos-gegl-buffer-codec"

#define PHOTOS_TYPE_GEGL_BUFFER_CODEC (photos_gegl_buffer_codec_get_type ())
G_DECLARE_DERIVABLE_TYPE (PhotosGeglBufferCodec, photos_gegl_buffer_codec, PHOTOS, GEGL_BUFFER_CODEC, GObject);

typedef struct _PhotosGeglBufferCodecPrivate PhotosGeglBufferCodecPrivate;

struct _PhotosGeglBufferCodecClass
{
  GObjectClass parent_class;

  GStrv mime_types;

  /* virtual methods */
  gboolean    (*load_begin)                 (PhotosGeglBufferCodec *self, GError **error);
  gboolean    (*load_increment)             (PhotosGeglBufferCodec *self,
                                             const guchar *buf,
                                             gsize count,
                                             GError **error);
  gboolean    (*load_stop)                  (PhotosGeglBufferCodec *self, GError **error);

  /* signals */
  void        (*size_prepared)              (PhotosGeglBufferCodec *self, gint width, gint height);
};

gboolean            photos_gegl_buffer_codec_load_begin                (PhotosGeglBufferCodec *self,
                                                                        GError *error);

gboolean            photos_gegl_buffer_codec_load_increment            (PhotosGeglBufferCodec *self,
                                                                        const guchar *buf,
                                                                        gsize count,
                                                                        GError *error);

gboolean            photos_gegl_buffer_codec_load_stop                 (PhotosGeglBufferCodec *self,
                                                                        GError *error);

G_END_DECLS

#endif /* PHOTOS_GEGL_BUFFER_CODEC_H */
