////////////////////////////////////////////////////////////////
//
// RwgTex / ETC2 texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "tex.h"
#include "etcpack/etcpack_lib.h"

TexBlock  B_ETC2    = { FOURCC('E','T','C','2'), "ETC2",   4, 4, 64  };
TexBlock  B_ETC2A   = { FOURCC('E','T','C','A'), "ETC2A",  4, 4, 128 };
TexBlock  B_ETC2A1  = { FOURCC('E','T','C','P'), "ETC2A1", 4, 4, 64  };
TexBlock  B_EAC1    = { FOURCC('E','A','C','1'), "EAC1",   4, 4, 64  };
TexBlock  B_EAC2    = { FOURCC('E','A','C','2'), "EAC2",   4, 4, 128 };

TexFormat F_ETC2    = { FOURCC('E','T','C','2'), "ETC2",   "ETC2 RGB",                           "etc2",       &B_ETC2,  &CODEC_ETC2, GL_COMPRESSED_RGB8_ETC2,                     GL_RGB };
TexFormat F_ETC2A   = { FOURCC('E','T','C','A'), "ETC2A",  "ETC2 RGBA",                          "etc2a",      &B_ETC2A, &CODEC_ETC2, GL_COMPRESSED_RGBA8_ETC2_EAC,                GL_RGBA, 0, FF_ALPHA };
TexFormat F_ETC2A1  = { FOURCC('E','T','C','P'), "ETC2A1", "ETC2 RGBA with punch-through alpha", "etc2a1",     &B_ETC2A1,&CODEC_ETC2, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_RGBA, 0, FF_ALPHA | FF_BINARYALPHA };
TexFormat F_EAC1    = { FOURCC('E','A','C','1'), "EAC1",   "ETC2 Alpha Compression R",           "eac1",       &B_EAC1,  &CODEC_ETC2, GL_COMPRESSED_R11_EAC,                       GL_RED };
TexFormat F_EAC2    = { FOURCC('E','A','C','2'), "EAC2",   "ETC2 Alpha Compression RG",          "eac2",       &B_EAC2,  &CODEC_ETC2, GL_COMPRESSED_RG11_EAC,                      GL_RG };

TexCodec  CODEC_ETC2 = 
{
	"ETC2", "ETC2 Texture Compression", "etc2",
	&CodecETC2_Init,
	&CodecETC2_Option,
	&CodecETC2_Load,
	&CodecETC2_Accept,
	&CodecETC2_Encode,
	&CodecETC2_Decode
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

// extract 4x4 RGBA block from source image
void CodecETC2_ExtractBlockRGBA(const unsigned char *src, int x, int y, int w, int h, unsigned char *block)
{
	static const int map[] = { 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 0, 1, 2, 3 };
	int bx, by, bw, bh;
   
	bw = min(w - x, 4);
	bh = min(h - y, 4);
	for (int i = 0; i < 4; ++i)
	{
		by = map[(bh - 1) * 4 + i] + y;
		for (int j = 0; j < 4; ++j)
		{
			bx = map[(bw - 1) * 4 + j] + x;
			block[(i*4*4) + (j*4) + 0] = src[(by * (w*4)) + (bx*4) + 0];
			block[(i*4*4) + (j*4) + 1] = src[(by * (w*4)) + (bx*4) + 1];
			block[(i*4*4) + (j*4) + 2] = src[(by * (w*4)) + (bx*4) + 2];
			block[(i*4*4) + (j*4) + 3] = src[(by * (w*4)) + (bx*4) + 3];
		}
	}
}

// extract 4x4 Alpha block from source image
void CodecETC2_ExtractBlockAlpha(const unsigned char *src, int x, int y, int w, int h, unsigned char *block)
{
	static const int map[] = { 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 0, 1, 2, 3 };
	int bx, by, bw, bh;
   
	bw = min(w - x, 4);
	bh = min(h - y, 4);
	for (int i = 0; i < 4; ++i)
	{
		by = map[(bh - 1) * 4 + i] + y;
		for (int j = 0; j < 4; ++j)
		{
			bx = map[(bw - 1) * 4 + j] + x;
			block[(i*4) + j + 3] = src[(by * (w*4)) + (bx*4) + 3];
		}
	}
}

bool CodecETC2_Accept(TexEncodeTask *task)
{
	// ETC2 can encode any data
	return true;
}

void CodecETC2_Encode(TexEncodeTask *task)
{
	// determine format
	if (!task->format)
		task->format = &F_ETC2;
	// select compressor tool
	if (!task->tool)
		task->tool = &TOOL_ETCPACK;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

/*
==========================================================================================

  DECODING

==========================================================================================
*/

// using EtcPack to decode ETC2
void CodecETC2_Decode(TexDecodeTask *task)
{
	byte *data, *stream;
	int x, y, w, h;

	w = task->image->width;
	h = task->image->height;
	data = Image_GetData(task->image, NULL);
	stream = task->pixeldata;
	for (y = 0; y < h / 4; y++)
	{
		for (x = 0; x < w / 4; x++)
		{
			// get block
			unsigned int block1, block2;
			block1 = 0;           block1 |= stream[0];
			block1 = block1 << 8; block1 |= stream[1];
			block1 = block1 << 8; block1 |= stream[2];
			block1 = block1 << 8; block1 |= stream[3];
			block2 = 0;           block2 |= stream[4];
			block2 = block2 << 8; block2 |= stream[5];
			block2 = block2 << 8; block2 |= stream[6];
			block2 = block2 << 8; block2 |= stream[7];
			stream += 8;
			// unpack ETC2 RGB
			decompressBlockETC2c(block1, block2, data, w, h, x*4, y*4, task->image->bpp);
		}
	}
	task->image->colorSwap = false;
}

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/