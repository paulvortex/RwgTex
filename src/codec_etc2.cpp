////////////////////////////////////////////////////////////////
//
// RwgTex / ETC2 texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "tex.h"

TexBlock  B_ETC2  = { "ETC2", 4, 4, 64 };
TexBlock  B_ETC2A = { "ETC2A", 4, 4, 128 };
   
TexFormat F_ETC2_RGB    = { FOURCC('E','T','2','1'), "ETC2/RGB",   "ETC2 RGB",                           "etc2-rgb",   &B_ETC2,  &CODEC_ETC2, GL_COMPRESSED_RGB8_ETC2,                     GL_RGB };
TexFormat F_ETC2_RGBA   = { FOURCC('E','T','2','2'), "ETC2/RGBA",  "ETC2 RGBA",                          "etc2-rgba",  &B_ETC2A, &CODEC_ETC2, GL_COMPRESSED_RGBA8_ETC2_EAC,                GL_RGBA, 0, FF_ALPHA };
TexFormat F_ETC2_RGBA1  = { FOURCC('E','T','2','3'), "ETC2/RGBA1", "ETC2 RGBA with punch-through alpha", "etc2-rgba1", &B_ETC2A, &CODEC_ETC2, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_RGBA, 0, FF_ALPHA | FF_BINARYALPHA };
TexFormat F_EAC1        = { FOURCC('E','A','C','1'), "EAC1",       "ETC2 Alpha Compression R",           "eac1",       &B_ETC2,  &CODEC_ETC2, GL_COMPRESSED_R11_EAC,                       GL_RED };
TexFormat F_EAC2        = { FOURCC('E','A','C','2'), "EAC2",       "ETC2 Alpha Compression RG",          "eac2",       &B_ETC2A, &CODEC_ETC2, GL_COMPRESSED_RG11_EAC,                      GL_RG };

TexCodec  CODEC_ETC2 = 
{
	"ETC2", "Ericsson Texture Compression 2", "etc2",
	&CodecETC2_Init,
	&CodecETC2_Option,
	&CodecETC2_Load,
	&CodecETC2_Accept,
	&CodecETC2_Encode,
};

void CodecETC2_Init(void)
{
	RegisterTool(&TOOL_ETCPACK, &CODEC_ETC2);
}

void CodecETC2_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void CodecETC2_Load(void)
{
}

/*
==========================================================================================

  ENCODING

==========================================================================================
*/

bool CodecETC2_Accept(TexEncodeTask *task)
{
	// ETC2 can encode any data
	return true;
}

void CodecETC2_Encode(TexEncodeTask *task)
{
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/