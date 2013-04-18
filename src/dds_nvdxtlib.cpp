////////////////////////////////////////////////////////////////
//
// RWGTEX - NVidia DXTlib compressor support
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

#pragma comment(lib, "nvDXTlibMT.vc8.lib")

/*
==========================================================================================

  DDS compression - nvDXTlib path

==========================================================================================
*/

typedef struct
{
	byte        *stream;
	int          numwrites;
}nvWriteInfo;

NV_ERROR_CODE NvDXTLib_WriteDDS(const void *buffer, size_t count, const MIPMapData *mipMapData, void *userData)
{
	nvWriteInfo *wparm = (nvWriteInfo *)userData;
	if (wparm->numwrites > 1)
	{
		memcpy(wparm->stream, buffer, count);
		wparm->stream = (byte *)(wparm->stream) + count;
	}
	wparm->numwrites++;
	return NV_OK;
}

bool NvDXTlib(byte *stream, FS_File *file, LoadedImage *image, DWORD formatCC)
{
	nvCompressionOptions options;
	nvWriteInfo writeOptions;
	float weights[3];

	// options
	options.SetDefaultOptions();
	options.DoNotGenerateMIPMaps();
	options.SetQuality(kQualityHighest, 100);
	options.user_data = &writeOptions;
	if (image->useChannelWeighting)
	{
		weights[0] = image->weightRed;
		weights[1] = image->weightGreen;
		weights[2] = image->weightBlue;
		options.SetCompressionWeighting(kUserDefinedWeighting, weights);
	}
	if (formatCC == FORMAT_DXT1)
	{
		if (image->hasAlpha)
		{
			options.bForceDXT1FourColors = false;
			options.SetTextureFormat(kTextureTypeTexture2D, kDXT1a);
		}
		else
		{
			options.bForceDXT1FourColors = true;
			options.SetTextureFormat(kTextureTypeTexture2D, kDXT1);
		}
	}
	else if (formatCC == FORMAT_DXT2 || formatCC == FORMAT_DXT3)
		options.SetTextureFormat(kTextureTypeTexture2D, kDXT3);
	else if (formatCC == FORMAT_DXT4 || formatCC == FORMAT_DXT5 || formatCC == FORMAT_RXGB || formatCC == FORMAT_YCG1 || formatCC == FORMAT_YCG2)
		options.SetTextureFormat(kTextureTypeTexture2D, kDXT5);
	else
	{
		Warning("NvDXTlib : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompressionFormatString(formatCC));
		return false;
	}
	memset(&writeOptions, 0, sizeof(writeOptions));
	writeOptions.stream = stream;

	// compress
	NV_ERROR_CODE res = nvDDS::nvDXTcompress(Image_GetData(image), image->width,  image->height,  image->width*image->bpp, image->hasAlpha ? nvBGRA : nvBGR, &options, NvDXTLib_WriteDDS, 0);
	if (res == NV_OK)
	{
		for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			writeOptions.numwrites = 0;
			res = nvDDS::nvDXTcompress(mipmap->data, mipmap->width, mipmap->height,  mipmap->width*image->bpp, image->hasAlpha ? nvBGRA : nvBGR, &options, NvDXTLib_WriteDDS, 0);
			if (res != NV_OK)
				break;
		}
	}
	if (res != NV_OK)
	{
		Warning("NvDXTlib : %s%s.dds error - %s", file->path.c_str(), file->name.c_str(), getErrorString(res));
		return false;
	}
	return true;
}