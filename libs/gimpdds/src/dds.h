/*
	DDS GIMP plugin

	Copyright (C) 2004 Shawn Kirst <skirst@gmail.com>,
   with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA.
*/

#ifndef DDS_H
#define DDS_H

#ifndef FOURCC
#define FOURCC(a, b, c, d) \
         ((unsigned int)((unsigned int)(a)      ) | \
                        ((unsigned int)(b) <<  8) | \
                        ((unsigned int)(c) << 16) | \
                        ((unsigned int)(d) << 24))
#endif

typedef enum
{
   DDS_COMPRESS_NONE = 0,
   DDS_COMPRESS_BC1,        /* DXT1  */
   DDS_COMPRESS_BC2,        /* DXT3  */
   DDS_COMPRESS_BC3,        /* DXT5  */
   DDS_COMPRESS_BC3N,       /* DXT5n */
   DDS_COMPRESS_BC4,        /* ATI1  */
   DDS_COMPRESS_BC5,        /* ATI2  */
   DDS_COMPRESS_RXGB,       /* DXT5  */
   DDS_COMPRESS_AEXP,       /* DXT5  */
   DDS_COMPRESS_YCOCG,      /* DXT5  */
   DDS_COMPRESS_YCOCGS,     /* DXT5  */
   DDS_COMPRESS_MAX
} DDS_COMPRESSION_TYPE;

typedef enum
{
   DDS_SAVE_SELECTED_LAYER = 0,
   DDS_SAVE_CUBEMAP,
   DDS_SAVE_VOLUMEMAP,
   DDS_SAVE_MAX
} DDS_SAVE_TYPE;

typedef enum
{
   DDS_FORMAT_DEFAULT = 0,
   DDS_FORMAT_RGB8,
   DDS_FORMAT_RGBA8,
   DDS_FORMAT_BGR8,
   DDS_FORMAT_ABGR8,
   DDS_FORMAT_R5G6B5,
   DDS_FORMAT_RGBA4,
   DDS_FORMAT_RGB5A1,
   DDS_FORMAT_RGB10A2,
   DDS_FORMAT_R3G3B2,
   DDS_FORMAT_A8,
   DDS_FORMAT_L8,
   DDS_FORMAT_L8A8,
   DDS_FORMAT_AEXP,
   DDS_FORMAT_YCOCG,
   DDS_FORMAT_MAX
} DDS_FORMAT_TYPE;

typedef enum
{
   DDS_COLOR_DEFAULT = 0,
   DDS_COLOR_DISTANCE,
   DDS_COLOR_LUMINANCE,
   DDS_COLOR_INSET_BBOX,
   DDS_COLOR_MAX
} DDS_COLOR_TYPE;

typedef enum
{
   DDS_MIPMAP_NONE = 0,
   DDS_MIPMAP_GENERATE,
   DDS_MIPMAP_EXISTING,
   DDS_MIPMAP_MAX
} DDS_MIPMAP;

typedef enum
{
   DDS_MIPMAP_FILTER_DEFAULT = 0,
   DDS_MIPMAP_FILTER_NEAREST,
   DDS_MIPMAP_FILTER_BOX,
   DDS_MIPMAP_FILTER_BILINEAR,
   DDS_MIPMAP_FILTER_BICUBIC,
   DDS_MIPMAP_FILTER_LANCZOS,
   DDS_MIPMAP_FILTER_MAX
} DDS_MIPMAP_FILTER;

#endif
