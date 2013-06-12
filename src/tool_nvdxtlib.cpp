////////////////////////////////////////////////////////////////
//
// RwgTex / NVidia DXTlib compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

#include "nvlibs/inc/dxtlib/dxtlib.h"
#pragma comment(lib, "nvDXTlibMT.vc8.lib")

TexTool TOOL_NVDXTLIB =
{
	"NvDXTlib", "NVidia DXTlib", "nv",
	"DXT",
	TEXINPUT_BGR | TEXINPUT_BGRA,
	&NvDXTLib_Init,
	&NvDXTLib_Option,
	&NvDXTLib_Load,
	&NvDXTLib_Compress,
	&NvDXTLib_Version,
};

// tool options
bool             nvdxtlib_dithering;
nvQualitySetting nvdxtlib_quality[NUM_PROFILES];
OptionList        nvdxtlib_speedOption[] = 
{ 
	{ "fastest", kQualityFastest }, 
	{ "normal", kQualityNormal },
	{ "production", kQualityProduction },
	{ "highest", kQualityHighest },
	{ 0 }
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void NvDXTLib_Init(void)
{
	RegisterFormat(&F_DXT1, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT1A, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT2, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT3, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT4, &TOOL_NVDXTLIB);
	RegisterFormat(&F_DXT5, &TOOL_NVDXTLIB);
	RegisterFormat(&F_RXGB, &TOOL_NVDXTLIB);

	// options
	nvdxtlib_quality[PROFILE_FAST] = kQualityNormal;
	nvdxtlib_quality[PROFILE_REGULAR] = kQualityProduction;
	nvdxtlib_quality[PROFILE_BEST] = kQualityHighest;
}

void NvDXTLib_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			nvdxtlib_quality[PROFILE_FAST] = (nvQualitySetting)OptionEnum(val, nvdxtlib_speedOption, nvdxtlib_quality[PROFILE_REGULAR], TOOL_NVDXTLIB.name);
		else if (!stricmp(key, "regular"))
			nvdxtlib_quality[PROFILE_REGULAR] = (nvQualitySetting)OptionEnum(val, nvdxtlib_speedOption, nvdxtlib_quality[PROFILE_REGULAR], TOOL_NVDXTLIB.name);
		else if (!stricmp(key, "best"))
			nvdxtlib_quality[PROFILE_BEST] = (nvQualitySetting)OptionEnum(val, nvdxtlib_speedOption, nvdxtlib_quality[PROFILE_REGULAR], TOOL_NVDXTLIB.name);
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

void NvDXTLib_Load(void)
{
	if (CheckParm("-dither"))
		nvdxtlib_dithering = true;
	// note options
	if (nvdxtlib_dithering)
		Print("%s tool: enabled dithering\n", TOOL_NVDXTLIB.name);
}

const char *NvDXTLib_Version(void)
{
	return GetDXTCVersion()+8;
}

/*
==========================================================================================

  Compression

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

bool NvDXTLib_Compress(TexEncodeTask *t)
{
	nvCompressionOptions options;
	nvWriteInfo writeOptions;

	// options
	options.SetDefaultOptions();
	options.DoNotGenerateMIPMaps();
	options.SetQuality(nvdxtlib_quality[tex_profile], 200);
	options.bDitherColor = nvdxtlib_dithering;
	options.user_data = &writeOptions;
	if (t->format->block == &B_DXT1)
	{
		if (t->image->hasAlpha)
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
	else if (t->format->block == &B_DXT3)
		options.SetTextureFormat(kTextureTypeTexture2D, kDXT3);
	else if (t->format->block == &B_DXT5)
		options.SetTextureFormat(kTextureTypeTexture2D, kDXT5);
	else
	{
		Warning("NvDXTlib : %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
		return false;
	}
	memset(&writeOptions, 0, sizeof(writeOptions));
	writeOptions.stream = t->stream;

	// compress
	NV_ERROR_CODE res = nvDDS::nvDXTcompress(Image_GetData(t->image, NULL), t->image->width, t->image->height, t->image->width*t->image->bpp, (t->image->bpp == 4) ? nvBGRA : nvBGR, &options, NvDXTLib_WriteDDS, 0);
	if (res == NV_OK)
	{
		for (MipMap *mipmap = t->image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			writeOptions.numwrites = 0;
			res = nvDDS::nvDXTcompress(mipmap->data, mipmap->width, mipmap->height,  mipmap->width*t->image->bpp, (t->image->bpp == 4) ? nvBGRA : nvBGR, &options, NvDXTLib_WriteDDS, 0);
			if (res != NV_OK)
				break;
		}
	}
	if (res != NV_OK)
	{
		Warning("NvDXTlib : %s%s.dds error - %s", t->file->path.c_str(), t->file->name.c_str(), getErrorString(res));
		return false;
	}
	return true;
}