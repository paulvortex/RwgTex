////////////////////////////////////////////////////////////////
//
// RwgTex / AMD "The Compressonator" compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"
#ifndef NO_ATITC

#pragma comment(lib, "ATI_Compress_MT.lib")

TexTool TOOL_ATITC =
{
	"AtiTC", "AMD The Compressonator", "ati",
	"DXT, ETC1",
	TEXINPUT_BGR | TEXINPUT_BGRA,
	&ATITC_Init,
	&ATITC_Option,
	&ATITC_Load,
	&ATITC_Compress,
	&ATITC_Version,
};

// tool options
ATI_TC_Speed atitc_compressionSpeed[NUM_PROFILES];
bool         atitc_useAdaptiveWeighting;
OptionList    atitc_compressionOption[] = 
{ 
	{ "superfast", ATI_TC_Speed_SuperFast }, 
	{ "fast", ATI_TC_Speed_SuperFast }, 
	{ "normal", ATI_TC_Speed_Normal },
	{ 0 }
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void ATITC_Init(void)
{
	// DXT
	RegisterFormat(&F_DXT1, &TOOL_ATITC);
	RegisterFormat(&F_DXT1A, &TOOL_ATITC);
	RegisterFormat(&F_DXT2, &TOOL_ATITC);
	RegisterFormat(&F_DXT3, &TOOL_ATITC);
	RegisterFormat(&F_DXT4, &TOOL_ATITC);
	RegisterFormat(&F_DXT5, &TOOL_ATITC);
	RegisterFormat(&F_RXGB, &TOOL_ATITC);
	RegisterFormat(&F_YCG1, &TOOL_ATITC);
	RegisterFormat(&F_YCG2, &TOOL_ATITC);
	RegisterFormat(&F_YCG3, &TOOL_ATITC);
	RegisterFormat(&F_YCG4, &TOOL_ATITC);
	// ETC
	RegisterFormat(&F_ETC1, &TOOL_ATITC);
	// options
	atitc_compressionSpeed[PROFILE_FAST]    = ATI_TC_Speed_SuperFast;
	atitc_compressionSpeed[PROFILE_REGULAR] = ATI_TC_Speed_Normal;
	atitc_compressionSpeed[PROFILE_BEST]    = ATI_TC_Speed_Normal;
	atitc_useAdaptiveWeighting = true;
}

void ATITC_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			atitc_compressionSpeed[PROFILE_FAST] = (ATI_TC_Speed)OptionEnum(val, atitc_compressionOption, atitc_compressionSpeed[PROFILE_REGULAR], TOOL_ATITC.name);
		else if (!stricmp(key, "regular"))
			atitc_compressionSpeed[PROFILE_REGULAR] = (ATI_TC_Speed)OptionEnum(val, atitc_compressionOption, atitc_compressionSpeed[PROFILE_REGULAR], TOOL_ATITC.name);
		else if (!stricmp(key, "best"))
			atitc_compressionSpeed[PROFILE_BEST] = (ATI_TC_Speed)OptionEnum(val, atitc_compressionOption, atitc_compressionSpeed[PROFILE_REGULAR], TOOL_ATITC.name);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "adaptive_weighting"))
			atitc_useAdaptiveWeighting = OptionBoolean(val);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void ATITC_Load(void)
{
	// COMMANDLINEPARM: -noaw: disable adaptive color weighting
	if (CheckParm("-noaw"))
		atitc_useAdaptiveWeighting = false;
	// note options
	if (!atitc_useAdaptiveWeighting)
		Print("%s: disabled adaptive weighting\n", TOOL_ATITC.name);
}

const char *ATITC_Version(void)
{
	static char versionstring[200];
	sprintf(versionstring, "%i.%i", ATI_COMPRESS_VERSION_MAJOR, ATI_COMPRESS_VERSION_MINOR);
	return versionstring;
}

/*
==========================================================================================

  Compression

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

bool ATITC_Compress(TexEncodeTask *t)
{
	ATI_TC_Texture src;
	ATI_TC_Texture dst;
	ATI_TC_CompressOptions options;
	ATI_TC_FORMAT compress;

	memset(&src, 0, sizeof(src));
	memset(&options, 0, sizeof(options));

	// get options
	options.nAlphaThreshold = 127;
	options.nCompressionSpeed = atitc_compressionSpeed[tex_profile];
	options.bUseAdaptiveWeighting = atitc_useAdaptiveWeighting;
	options.bUseChannelWeighting = false;
	options.bDisableMultiThreading = (tex_mode == TEXMODE_DROP_FILE) ? false : true;
	options.dwSize = sizeof(options);
	if (t->format == &F_RXGB)
	{
		options.bUseChannelWeighting = true;
		options.bUseAdaptiveWeighting = false;
		options.fWeightingRed = 0.0;
		options.fWeightingGreen = 0.75;
		options.fWeightingBlue = 0.25;
	}
	else if (t->image->datatype == IMAGE_NORMALMAP)
	{
		options.bUseChannelWeighting = true;
		options.bUseAdaptiveWeighting = false;
		options.fWeightingRed = 0.5f;
		options.fWeightingGreen = 0.5f;
		options.fWeightingBlue = 0.5f;
	}
	if (t->format->block == &B_DXT1)
	{
		if (t->image->hasAlpha)
			options.bDXT1UseAlpha = true;
		else
			options.bDXT1UseAlpha = false;
		compress = ATI_TC_FORMAT_DXT1;
	}
	else if (t->format->block == &B_DXT2 || t->format->block == &B_DXT3)
		compress = ATI_TC_FORMAT_DXT3;
	else if (t->format->block == &B_DXT4 || t->format->block == &B_DXT5)
		compress = ATI_TC_FORMAT_DXT5;
	else if (t->format->block == &B_ETC1)
		compress = ATI_TC_FORMAT_ETC_RGB;
	else
	{
		Warning("%s : %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name, TOOL_ATITC.name);
		return false;
	}

	// init source texture
	src.dwSize = sizeof(src);
	src.dwWidth = t->image->width;
	src.dwHeight = t->image->height;
	src.dwPitch = t->image->width*t->image->bpp;
	if (t->image->bpp == 4)
		src.format = ATI_TC_FORMAT_ARGB_8888;
	else
		src.format = ATI_TC_FORMAT_RGB_888;
	
	// init dest texture
	memset(&dst, 0, sizeof(dst));
	dst.dwSize = sizeof(dst);
	dst.dwWidth = t->image->width;
	dst.dwHeight = t->image->height;
	dst.dwPitch = 0;
	dst.format = compress;

	// compress
	ATI_TC_ERROR res = ATI_TC_OK;
	byte *stream = t->stream;
	dst.pData = stream;
	res = AtiCompressData(&src, &dst, Image_GetData(t->image, NULL), t->image->width, t->image->height, &options, compress, t->image->bpp);
	if (res == ATI_TC_OK)
	{
		stream += dst.dwDataSize;
		// mipmaps
		for (MipMap *mipmap = t->image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			dst.pData = stream;
			res = AtiCompressData(&src, &dst, mipmap->data, mipmap->width, mipmap->height, &options, compress, t->image->bpp);
			if (res != ATI_TC_OK)
				break;
			stream += dst.dwDataSize;
		}
	}

	// end, advance stats
	if (res != ATI_TC_OK)
	{
		Warning("AtiCompress : %s%s.dds - compressor fail (error code %i)", t->file->path.c_str(), t->file->name.c_str(), res);
		return false;
	}
	return true;
}

#endif