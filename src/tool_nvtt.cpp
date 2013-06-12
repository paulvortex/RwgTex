////////////////////////////////////////////////////////////////
//
// RwgTex / NVidia Texture tools compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"
#ifndef NO_NVTT

#pragma comment(lib, "nvtt.lib")

TexTool TOOL_NVTT =
{
	"NVTT", "NVidia Texture Tools", "nvtt",
	"DXT",
	TEXINPUT_BGR | TEXINPUT_BGRA,
	&NvTT_Init,
	&NvTT_Option,
	&NvTT_Load,
	&NvTT_Compress,
	&NvTT_Version,
};

void NvTT_Init(void)
{
	RegisterFormat(&F_DXT1, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT1A, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT2, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT3, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT4, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT5, &TOOL_NVDXTLIB);
	RegisterFormat(&F_RXGB, &TOOL_NVDXTLIB);
}

void NvTT_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
}

void NvTT_Load(void)
{
}

const char *NvTT_Version(void)
{
	static char versionstring[200];
	sprintf(versionstring, "%i.%i", (int)(nvtt::version() / 100), nvtt::version() - ((int)(nvtt::version()) / 100)*100);
	return versionstring;
}

/*
==========================================================================================

  Compression

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

void NvTT_CompressSingleImage(nvtt::InputOptions &inputOptions, nvtt::OutputOptions &outputOptions, byte *data, int width, int height, nvtt::CompressionOptions &compressionOptions)
{
	nvtt::Compressor compressor;

	inputOptions.setTextureLayout(nvtt::TextureType_2D, width, height);
	inputOptions.setMipmapData(data, width, height);
	compressor.process(inputOptions, compressionOptions, outputOptions);
}

bool NvTT_Compress(TexEncodeTask *t)
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
	if (t->image->hasAlpha)
		inputOptions.setAlphaMode(nvtt::AlphaMode_Transparency);
	else
		inputOptions.setAlphaMode(nvtt::AlphaMode_None);
	//if (image->useChannelWeighting)
	//	compressionOptions.setColorWeights(image->weightRed, image->weightGreen, image->weightBlue);
	if (t->format->block == &B_DXT1)
	{
		if (t->image->hasAlpha)
			compressionOptions.setFormat(nvtt::Format_DXT1a);
		else
			compressionOptions.setFormat(nvtt::Format_DXT1);
	}
	else if (t->format->block == &B_DXT3)
		compressionOptions.setFormat(nvtt::Format_DXT3);
	else if (t->format->block == &B_DXT5)
		compressionOptions.setFormat(nvtt::Format_DXT5);
	else
	{
		Warning("NvTT : %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
		return false;
	}
	if (t->image->bpp == 4)
		compressionOptions.setPixelFormat(32, 0x0000FF, 0x00FF00, 0xFF0000, 0xFF000000);
	else
		compressionOptions.setPixelFormat(24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	compressionOptions.setQuality(nvtt::Quality_Production);
	
	// set output parameters
	outputOptions.setOutputHeader(false);
	outputOptions.setErrorHandler(&errorHandler);
	errorHandler.errorCode = nvtt::Error_Unknown;
	outputOptions.setOutputHandler(&outputHandler);
	outputHandler.stream = t->stream; 

	// write base texture and mipmaps
	NvTT_CompressSingleImage(inputOptions, outputOptions, Image_GetData(t->image, NULL), t->image->width, t->image->height, compressionOptions);
	if (errorHandler.errorCode == nvtt::Error_Unknown)
	{
		for (MipMap *mipmap = t->image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			NvTT_CompressSingleImage(inputOptions, outputOptions, mipmap->data, mipmap->width, mipmap->height, compressionOptions);
			if (errorHandler.errorCode != nvtt::Error_Unknown)
				break;
		}
	}
	
	// end, advance stats
	if (errorHandler.errorCode != nvtt::Error_Unknown)
	{
		Warning("NVTT Error: %s%s.dds - %s", t->file->path.c_str(), t->file->name.c_str(), nvtt::errorString(errorHandler.errorCode));
		return false;
	}
	return true;
}

#endif