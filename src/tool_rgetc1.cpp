////////////////////////////////////////////////////////////////
//
// RwgTex / Rg-ETC1 compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"
#include "rg_etc1/rg_etc1.h"

TexTool TOOL_RGETC1 =
{
	"RgETC1", "Rg-Etc1 Packer", "rgetc1",
	TEXINPUT_RGBA,
	&RgEtc1_Init,
	&RgEtc1_Option,
	&RgEtc1_Load,
	&RgEtc1_Compress,
	&RgEtc1_Version,
};

// tool option
rg_etc1::etc1_quality rgetc1_quality[NUM_PROFILES];
bool                  rgetc1_dithering;
OptionList            rgetc1_compressionOption[] = 
{ 
	{ "low", rg_etc1::cLowQuality }, 
	{ "medium", rg_etc1::cMediumQuality }, 
	{ "high", rg_etc1::cHighQuality },
	{ 0 }
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void RgEtc1_Init(void)
{
	RegisterFormat(&F_ETC1, &TOOL_RGETC1);

	// options
	rgetc1_quality[PROFILE_FAST] = rg_etc1::cLowQuality;
	rgetc1_quality[PROFILE_REGULAR] = rg_etc1::cHighQuality;
	rgetc1_quality[PROFILE_BEST] = rg_etc1::cHighQuality;
	rgetc1_dithering = false;
}

void RgEtc1_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			rgetc1_quality[PROFILE_FAST] = (rg_etc1::etc1_quality)OptionEnum(val, rgetc1_compressionOption, rgetc1_quality[PROFILE_FAST], TOOL_RGETC1.name);
		else if (!stricmp(key, "regular"))
			rgetc1_quality[PROFILE_REGULAR] = (rg_etc1::etc1_quality)OptionEnum(val, rgetc1_compressionOption, rgetc1_quality[PROFILE_REGULAR], TOOL_RGETC1.name);
		else if (!stricmp(key, "best"))
			rgetc1_quality[PROFILE_BEST] = (rg_etc1::etc1_quality)OptionEnum(val, rgetc1_compressionOption, rgetc1_quality[PROFILE_BEST], TOOL_RGETC1.name);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "dither"))
			rgetc1_dithering = OptionBoolean(val) ? 1 : 0;
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void RgEtc1_Load(void)
{
	if (CheckParm("-dither"))
		rgetc1_dithering = true;
	// note options
	if (rgetc1_dithering)
		Print("%s tool: enabled dithering\n", TOOL_RGETC1.name);
}

const char *RgEtc1_Version(void)
{
	return "1.04"; // from rg_etc1.cpp
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

// compress texture
size_t RgEtc1_CompressSingleImage(byte *stream, TexEncodeTask *t, int imagewidth, int imageheight, byte *imagedata, int pitch, rg_etc1::etc1_pack_params &options)
{
	unsigned int block[16];

	rg_etc1::pack_etc1_block_init();
	for (int y = 0; y < imageheight / 4; y++)
	{
		for (int x = 0; x < imagewidth / 4; x++)
		{
			// extract block 
			CodecETC1_ExtractBlockRGBA(imagedata, x * 4, y * 4, imagewidth, imageheight, pitch, (unsigned char*)block);
			// pack block
			rg_etc1::pack_etc1_block(stream, block, options);
			stream += 8;
		}
	}
	return imagewidth*imageheight/2;
}

bool RgEtc1_Compress(TexEncodeTask *t)
{
	size_t output_size;
	rg_etc1::etc1_pack_params options;

	// RgEtc1 requires 32-bit images to have all alpha  == 255
	Image_SetAlpha(t->image, 255);

	// set parameters
	options.clear();
	options.m_dithering = rgetc1_dithering;
	options.m_quality = rgetc1_quality[tex_profile];

	// compress
	byte *stream = t->stream;
	for (ImageMap *map = t->image->maps; map; map = map->next)
	{
		output_size = RgEtc1_CompressSingleImage(stream, t, map->width, map->height, map->data, map->width*t->image->bpp, options);
		if (output_size)
			stream += output_size;
	}
	return true;
}