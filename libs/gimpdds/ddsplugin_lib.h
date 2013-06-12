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

#ifndef __DDSPLUGIN_H
#define __DDSPLUGIN_H

#define DDS_PLUGIN_VERSION_MAJOR     2
#define DDS_PLUGIN_VERSION_MINOR     2
#define DDS_PLUGIN_VERSION_REVISION  1

int dxt_decompress(unsigned char *dst, unsigned char *src, int format, unsigned int size, unsigned int width, unsigned int height, int bpp, int normals);

void compress_DXT1(unsigned char *dst, const unsigned char *src, int w, int h, int type, int dither, int alpha);
void compress_DXT3(unsigned char *dst, const unsigned char *src, int w, int h, int type, int dither);
void compress_DXT5(unsigned char *dst, const unsigned char *src, int w, int h, int type, int dither);
void compress_BC4(unsigned char *dst, const unsigned char *src, int w, int h);
void compress_BC5(unsigned char *dst, const unsigned char *src, int w, int h);
void compress_YCoCg(unsigned char *dst, const unsigned char *src, int w, int h);

#include "src/dds.h"

#endif
