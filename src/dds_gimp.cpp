////////////////////////////////////////////////////////////////
//
// RWGTEX - GIMP DDS plugin compressor support
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////

#include "main.h"
#include "dds.h"

/*
==========================================================================================

  DDS compression - GIMP DDS plugin

==========================================================================================
*/

typedef struct gimpdds_options_s
{
	DDS_COMPRESSION_TYPE compressionType;
	DDS_COLOR_TYPE colorBlockMethod;
	int dithering;
} gimpdds_options_t;

unsigned int GimpGetCompressedSize(int width, int height, int bpp, int level, int num, int format)
{
	int w, h, n = 0;
	unsigned int size = 0;

	w = width >> level;
	h = height >> level;
	w = max(1, w);
	h = max(1, h);
	w <<= 1;
	h <<= 1;

	while(n < num && (w != 1 || h != 1))
	{
		if(w > 1) w >>= 1;
		if(h > 1) h >>= 1;
		if(format == DDS_COMPRESS_NONE)
			size += (w * h);
		else
			size += ((w + 3) >> 2) * ((h + 3) >> 2);
		++n;
	}
	if(format == DDS_COMPRESS_NONE)
		size *= bpp;
	else
	{
		if(format == DDS_COMPRESS_BC1 || format == DDS_COMPRESS_BC4)
			size *= 8;
		else
			size *= 16;
	}
	return(size);
}

int GimpCompressData(byte *dst, byte *src, int w, int h, gimpdds_options_t *options)
{
	switch(options->compressionType)
    {
		case DDS_COMPRESS_BC1:
			compress_DXT1(dst, src, w, h, options->colorBlockMethod, options->dithering, 1);
			break;
		case DDS_COMPRESS_BC2:
			compress_DXT3(dst, src, w, h, options->colorBlockMethod, options->dithering);
			break;
		case DDS_COMPRESS_BC3:
		case DDS_COMPRESS_BC3N:
		case DDS_COMPRESS_RXGB:
		case DDS_COMPRESS_AEXP:
		case DDS_COMPRESS_YCOCG:
			compress_DXT5(dst, src, w, h, options->colorBlockMethod, options->dithering);
			break;
		case DDS_COMPRESS_BC4:
			compress_BC4(dst, src, w, h);
            break;
		case DDS_COMPRESS_BC5:
			compress_BC5(dst, src, w, h);
			break;
		case DDS_COMPRESS_YCOCGS:
			compress_YCoCg(dst, src, w, h);
			break;
		default:
            compress_DXT5(dst, src, w, h, options->colorBlockMethod, options->dithering);
            break;
	}
	return GimpGetCompressedSize(w, h, 0, 0, 1, options->compressionType);
}

bool GimpDDS(byte *stream, FS_File *file, LoadedImage *image, DWORD formatCC)
{
	gimpdds_options_t options;
	byte *data;
	size_t res;

	// get options
	if (formatCC == FORMAT_DXT1)
		options.compressionType = DDS_COMPRESS_BC1;
	else if (formatCC == FORMAT_DXT3)
		options.compressionType = DDS_COMPRESS_BC2;
	else if (formatCC == FORMAT_DXT5)
		options.compressionType = DDS_COMPRESS_BC3;
	else if (formatCC == FORMAT_YCG1)
		options.compressionType = DDS_COMPRESS_YCOCG;
	else if (formatCC == FORMAT_YCG2)
		options.compressionType = DDS_COMPRESS_YCOCGS;
	else
	{
		Warning("GimpDDS : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompressionFormatString(formatCC));
		return false;
	}
	options.colorBlockMethod = DDS_COLOR_DEFAULT;
	options.dithering = 0;

	// compress
	data = stream;
	res = GimpCompressData(data, Image_GetData(image), image->width, image->height, &options);
	if (res >= 0)
	{
		data += res;
		// mipmaps
		for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			res = GimpCompressData(data, mipmap->data, mipmap->width, mipmap->height, &options);
			if (res < 0)
				break;
			data += res;
		}
	}

	// end, advance stats
	if (res < 0)
	{
		Warning("GimpDDS : %s%s.dds - compressor fail (error code %i)", file->path.c_str(), file->name.c_str(), res);
		return false;
	}
	return true;
}