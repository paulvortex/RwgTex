/*
 * This file is part of the Scale2x project.
 *
 * Copyright (C) 2003, 2004 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

enum
{
	SCALEX_OK,
	SCALEX_BAD_BPP,
	SCALEX_BAD_WIDTH,
	SCALEX_BAD_HEIGHT,
	SCALEX_BAD_SCALE
};
extern char *sxErrorString(int err);

/**
 * Check if the scale implementation is applicable at the given arguments.
 * \param scale Scale factor. 2, 203 (fox 2x3), 204 (for 2x4), 3 or 4.
 * \param bpp Bytes per pixel of the source and destination bitmap.
 * \param width Horizontal size in pixels of the source bitmap.
 * \param height Vertical size in pixels of the source bitmap.
*/
extern int sxCheck(unsigned int scale, unsigned char bpp, unsigned int width, unsigned int height);

/**
 * Apply the Scale effect on a bitmap.
 * This function is simply a common interface for ::scale2x(), ::scale3x() and ::scale4x().
 * \param scale Scale factor. 2, 203 (fox 2x3), 204 (for 2x4), 3 or 4.
 * \param void_dst Pointer at the first pixel of the destination bitmap.
 * \param dst_slice Size in bytes of a destination bitmap row.
 * \param void_src Pointer at the first pixel of the source bitmap.
 * \param src_slice Size in bytes of a source bitmap row.
 * \param bpp Bytes per pixel of the source and destination bitmap.
 * \param width Horizontal size in pixels of the source bitmap.
 * \param height Vertical size in pixels of the source bitmap.
*/
extern void sxScale(unsigned int scale, void* void_dst, unsigned int dst_slice, const void* void_src, unsigned int src_slice, unsigned char bpp, unsigned int width, unsigned int height);