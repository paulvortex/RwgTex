////////////////////////////////////////////////////////////////
//
// RWGTEX - NVidia Texture tools compressor support
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

#pragma comment(lib, "nvtt.lib")

/*
==========================================================================================

  DDS compression - NVidia Texture Tools Path

==========================================================================================
*/

class nvttOutputHandler : public nvtt::OutputHandler
{
public:
	byte *stream;
	void beginImage(int size, int width, int height, int depth, int face, int miplevel)
	{
	}
	bool writeData(const void *data, int size)
	{
		memcpy(stream, data, size);
		stream += size;
		return true;
	}
};

class nvttErrorHandler : public nvtt::ErrorHandler
{
public : 
	nvtt::Error errorCode;
	void error(nvtt::Error e)
	{
		errorCode = e;
	}
};

void NvTTlibCompressData(nvtt::InputOptions &inputOptions, nvtt::OutputOptions &outputOptions, byte *data, int width, int height, nvtt::CompressionOptions &compressionOptions)
{
	nvtt::Compressor compressor;

	inputOptions.setTextureLayout(nvtt::TextureType_2D, width, height);
	inputOptions.setMipmapData(data, width, height);
	compressor.process(inputOptions, compressionOptions, outputOptions);
}

bool NvTTlib(byte *stream, FS_File *file, LoadedImage *image, DWORD formatCC)
{
	nvtt::CompressionOptions compressionOptions;
	nvtt::OutputOptions outputOptions;
	nvtt::InputOptions inputOptions;
	nvttErrorHandler errorHandler;
	nvttOutputHandler outputHandler;

	inputOptions.reset();
	compressionOptions.reset();
	outputOptions.reset();

	// options
	inputOptions.setMipmapGeneration(false);
	if (image->hasAlpha)
		inputOptions.setAlphaMode(nvtt::AlphaMode_Transparency);
	else
		inputOptions.setAlphaMode(nvtt::AlphaMode_None);
	if (image->useChannelWeighting)
		compressionOptions.setColorWeights(image->weightRed, image->weightGreen, image->weightBlue);
	if (formatCC == FORMAT_DXT1)
	{
		if (image->hasAlpha)
			compressionOptions.setFormat(nvtt::Format_DXT1a);
		else
			compressionOptions.setFormat(nvtt::Format_DXT1);
	}
	else if (formatCC == FORMAT_DXT2 || formatCC == FORMAT_DXT3)
		compressionOptions.setFormat(nvtt::Format_DXT3);
	else if (formatCC == FORMAT_DXT4 || formatCC == FORMAT_DXT5 || formatCC == FORMAT_RXGB || formatCC == FORMAT_YCG1 || formatCC == FORMAT_YCG2)
		compressionOptions.setFormat(nvtt::Format_DXT5);
	else
	{
		Warning("NvTT : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompressionFormatString(formatCC));
		return false;
	}
	if (image->hasAlpha)
		compressionOptions.setPixelFormat(32, 0x0000FF, 0x00FF00, 0xFF0000, 0xFF000000);
	else
		compressionOptions.setPixelFormat(24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	compressionOptions.setQuality(nvtt::Quality_Production);
	
	// set output parameters
	outputOptions.setOutputHeader(false);
	outputOptions.setErrorHandler(&errorHandler);
	errorHandler.errorCode = nvtt::Error_Unknown;
	outputOptions.setOutputHandler(&outputHandler);
	outputHandler.stream = stream; 

	// write base texture and mipmaps
	NvTTlibCompressData(inputOptions, outputOptions, Image_GetData(image), image->width, image->height, compressionOptions);
	if (errorHandler.errorCode == nvtt::Error_Unknown)
	{
		for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			NvTTlibCompressData(inputOptions, outputOptions, mipmap->data, mipmap->width, mipmap->height, compressionOptions);
			if (errorHandler.errorCode != nvtt::Error_Unknown)
				break;
		}
	}
	
	// end, advance stats
	if (errorHandler.errorCode != nvtt::Error_Unknown)
	{
		Warning("NVTT Error: %s%s.dds - %s", file->path.c_str(), file->name.c_str(), nvtt::errorString(errorHandler.errorCode));
		return false;
	}
	return true;
}