////////////////////////////////////////////////////////////////
//
// RWGTEX - AMD "The Compressonator" DDS compressor support
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

#pragma comment(lib, "ATI_Compress_MT.lib")

/*
==========================================================================================

  DDS compression - ATI Compressor path

==========================================================================================
*/

// compress single image
ATI_TC_ERROR AtiCompressData(ATI_TC_Texture *src, ATI_TC_Texture *dst, byte *data, int width, int height, ATI_TC_CompressOptions *options, ATI_TC_FORMAT compressFormat, int bpp)
{

	// fill source pixels, swap rgb->bgr
	// premodulate color if requested
	src->dwWidth = width;
	src->dwHeight = height;
	src->dwPitch = width * bpp;
	src->dwDataSize = width * height * bpp;
	src->pData = data;
	
	// fill dst texture
	dst->dwWidth = width;
	dst->dwHeight = height;
	dst->dwPitch = 0;
	dst->format = compressFormat;
	dst->dwDataSize = ATI_TC_CalculateBufferSize(dst);

	// convert
	return ATI_TC_ConvertTexture(src, dst, options, NULL, NULL, NULL);
}

bool AtiCompress(byte *stream, FS_File *file, LoadedImage *image, DWORD formatCC)
{
	ATI_TC_Texture src;
	ATI_TC_Texture dst;
	ATI_TC_CompressOptions options;
	ATI_TC_FORMAT compress;

	memset(&src, 0, sizeof(src));
	memset(&options, 0, sizeof(options));

	// get options
	options.nAlphaThreshold = 127;
	options.nCompressionSpeed = ATI_TC_Speed_Normal;
	options.bUseAdaptiveWeighting = true;
	options.bUseChannelWeighting = false;
	options.dwSize = sizeof(options);
	if (image->useChannelWeighting)
	{
		options.bUseChannelWeighting = true;
		options.fWeightingRed = image->weightRed;
		options.fWeightingGreen = image->weightGreen;
		options.fWeightingBlue = image->weightBlue;
	}
	if (formatCC == FORMAT_DXT1)
	{
		if (image->hasAlpha)
			options.bDXT1UseAlpha = true;
		else
			options.bDXT1UseAlpha = false;
		compress = ATI_TC_FORMAT_DXT1;
	}
	else if (formatCC == FORMAT_DXT2 || formatCC == FORMAT_DXT3)
		compress = ATI_TC_FORMAT_DXT3;
	else if (formatCC == FORMAT_DXT4 || formatCC == FORMAT_DXT5 || formatCC == FORMAT_RXGB || formatCC == FORMAT_YCG1 || formatCC == FORMAT_YCG2)
		compress = ATI_TC_FORMAT_DXT5;
	else
	{
		Warning("AtiCompress : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompressionFormatString(formatCC));
		return false;
	}

	// init source texture
	src.dwSize = sizeof(src);
	src.dwWidth = image->width;
	src.dwHeight = image->height;
	src.dwPitch = image->width*image->bpp;
	if (image->hasAlpha)
		src.format = ATI_TC_FORMAT_ARGB_8888;
	else
		src.format = ATI_TC_FORMAT_RGB_888;
	
	// init dest texture
	memset(&dst, 0, sizeof(dst));
	dst.dwSize = sizeof(dst);
	dst.dwWidth = image->width;
	dst.dwHeight = image->height;
	dst.dwPitch = 0;
	dst.format = compress;

	// compress
	ATI_TC_ERROR res = ATI_TC_OK;
	dst.pData = stream;
	res = AtiCompressData(&src, &dst, Image_GetData(image), image->width, image->height, &options, compress, image->bpp);
	if (res == ATI_TC_OK)
	{
		stream += dst.dwDataSize;
		// mipmaps
		for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			dst.pData = stream;
			res = AtiCompressData(&src, &dst, mipmap->data, mipmap->width, mipmap->height, &options, compress, image->bpp);
			if (res != ATI_TC_OK)
				break;
			stream += dst.dwDataSize;
		}
	}

	// end, advance stats
	if (res != ATI_TC_OK)
	{
		Warning("AtiCompress : %s%s.dds - compressor fail (error code %i)", file->path.c_str(), file->name.c_str(), res);
		return false;
	}
	return true;
}