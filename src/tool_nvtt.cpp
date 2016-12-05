////////////////////////////////////////////////////////////////
//
// RwgTex / NVidia Texture tools compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

#pragma comment(lib, "nvtt.lib")

TexTool TOOL_NVTT =
{
	"NVTT", "NVidia Texture Tools", "nvtt",
	TEXINPUT_BGRA,
	&NvTT_Init,
	&NvTT_Option,
	&NvTT_Load,
	&NvTT_Compress,
	&NvTT_Version,
};

// tool option
nvtt::Quality         nvtt_quality[NUM_PROFILES];
bool                  nvtt_dithering;
OptionList            nvtt_compressionOption[] =
{
	{ "fastest", nvtt::Quality_Fastest },
	{ "normal", nvtt::Quality_Normal },
	{ "production", nvtt::Quality_Production },
	{ "highest", nvtt::Quality_Highest },
	{ 0 }
};

void NvTT_Init(void)
{
	RegisterFormat(&F_DXT1, &TOOL_NVTT);
	RegisterFormat(&F_DXT1A, &TOOL_NVTT);
	RegisterFormat(&F_DXT2, &TOOL_NVTT);
	RegisterFormat(&F_DXT3, &TOOL_NVTT);
	RegisterFormat(&F_DXT4, &TOOL_NVTT);
	RegisterFormat(&F_DXT5, &TOOL_NVTT);

	// options
	nvtt_quality[PROFILE_FAST] = nvtt::Quality_Fastest;
	nvtt_quality[PROFILE_REGULAR] = nvtt::Quality_Production;
	nvtt_quality[PROFILE_BEST] = nvtt::Quality_Highest;
	nvtt_dithering = false;
}

void NvTT_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			nvtt_quality[PROFILE_FAST] = (nvtt::Quality)OptionEnum(val, nvtt_compressionOption, nvtt_quality[PROFILE_FAST], TOOL_NVTT.name);
		else if (!stricmp(key, "regular"))
			nvtt_quality[PROFILE_REGULAR] = (nvtt::Quality)OptionEnum(val, nvtt_compressionOption, nvtt_quality[PROFILE_REGULAR], TOOL_NVTT.name);
		else if (!stricmp(key, "best"))
			nvtt_quality[PROFILE_BEST] = (nvtt::Quality)OptionEnum(val, nvtt_compressionOption, nvtt_quality[PROFILE_BEST], TOOL_NVTT.name);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "dither"))
			nvtt_dithering = OptionBoolean(val) ? 1 : 0;
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void NvTT_Load(void)
{
	if (CheckParm("-dither"))
		nvtt_dithering = true;
	// note options
	if (nvtt_dithering)
		Print("%s tool: enabled dithering\n", TOOL_NVTT.name);
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

bool NvTT_Compress_Task(TexEncodeTask *t, nvtt::Quality quality, bool dithering)
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
	else if (t->format->block == &B_DXT2 || t->format->block == &B_DXT3)
		compressionOptions.setFormat(nvtt::Format_DXT3);
	else if (t->format->block == &B_DXT4 || t->format->block == &B_DXT5)
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
	compressionOptions.setQuality(quality);

	// dithering
	compressionOptions.setQuantization(dithering, false, t->image->hasAlpha && t->image->hasGradientAlpha);

	// set output parameters
	outputOptions.setOutputHeader(false);
	outputOptions.setErrorHandler(&errorHandler);
	errorHandler.errorCode = nvtt::Error_Unknown;
	outputOptions.setOutputHandler(&outputHandler);
	outputHandler.stream = t->stream;

	// write base texture and maps
	for (ImageMap *map = t->image->maps; map; map = map->next)
	{
		NvTT_CompressSingleImage(inputOptions, outputOptions, map->data, map->width, map->height, compressionOptions);
		if (errorHandler.errorCode != nvtt::Error_Unknown)
			break;
	}

	// end, advance stats
	if (errorHandler.errorCode != nvtt::Error_Unknown)
	{
		Warning("NVTT Error: %s%s.dds - %s", t->file->path.c_str(), t->file->name.c_str(), nvtt::errorString(errorHandler.errorCode));
		return false;
	}
	return true;
}

bool NvTT_Compress(TexEncodeTask *t)
{
	return NvTT_Compress_Task(t, nvtt_quality[tex_profile], nvtt_dithering);
}

/*
==========================================================================================

NVDXTLIB Legacy Support

==========================================================================================
*/

// tool options
bool              nvdxtlib_dithering;
nvtt::Quality     nvdxtlib_quality[NUM_PROFILES];

void NvTT_NvDXTLib_Init(void)
{
	RegisterFormat(&F_DXT1, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT1A, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT2, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT3, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT4, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT5, &TOOL_NVDXTLIB);
}

void NvTT_NvDXTLib_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			nvdxtlib_quality[PROFILE_FAST] = (nvtt::Quality)OptionEnum(val, nvtt_compressionOption, nvdxtlib_quality[PROFILE_FAST], TOOL_NVDXTLIB.name);
		else if (!stricmp(key, "regular"))
			nvdxtlib_quality[PROFILE_REGULAR] = (nvtt::Quality)OptionEnum(val, nvtt_compressionOption, nvdxtlib_quality[PROFILE_REGULAR], TOOL_NVDXTLIB.name);
		else if (!stricmp(key, "best"))
			nvdxtlib_quality[PROFILE_BEST] = (nvtt::Quality)OptionEnum(val, nvtt_compressionOption, nvdxtlib_quality[PROFILE_BEST], TOOL_NVDXTLIB.name);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "dither"))
			nvdxtlib_dithering = OptionBoolean(val);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void NvTT_NvDXTLib_Load(void)
{
	if (CheckParm("-dither"))
		nvdxtlib_dithering = true;
	// note options
	if (nvdxtlib_dithering)
		Print("%s tool: enabled dithering\n", TOOL_NVDXTLIB.name);
}

bool NvTT_NvDXTLib_Compress(TexEncodeTask *t)
{
	return NvTT_Compress_Task(t, nvdxtlib_quality[tex_profile], nvdxtlib_dithering);
}

TexTool TOOL_NVDXTLIB =
{
	"NvDXTlib", "NVidia DXTlib (deprecated to NVidia Texture Tools)", "nv",
	TEXINPUT_BGRA,
	&NvTT_NvDXTLib_Init,
	&NvTT_NvDXTLib_Option,
	&NvTT_NvDXTLib_Load,
	&NvTT_NvDXTLib_Compress,
	&NvTT_Version,
};
