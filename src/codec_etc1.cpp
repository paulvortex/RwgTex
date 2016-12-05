////////////////////////////////////////////////////////////////
//
// RwgTex / ETC1 texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "tex.h"
#include "etcpack/etcpack_lib.h"

TexBlock  B_ETC1 = { FOURCC('E','T','C','1'), "ETC1", 4, 4, 64 };

TexFormat F_ETC1 = { FOURCC('E','T','C','1'), "ETC1", "ETC1", "etc1", &B_ETC1, &CODEC_ETC1, GL_COMPRESSED_ETC1_RGB8_OES, 0, GL_RGB };

TexCodec  CODEC_ETC1 = 
{
	"ETC1", "ETC1 Texture Compression", "etc1",
	&CodecETC1_Init,
	&CodecETC1_Option,
	&CodecETC1_Load,
	&CodecETC1_Accept,
	&CodecETC1_Encode,
	&CodecETC1_Decode,
};

void CodecETC1_Init(void)
{
	RegisterTool(&TOOL_ETCPACK, &CODEC_ETC1);
#ifndef NO_ATITC
	RegisterTool(&TOOL_ATITC, &CODEC_ETC1);
#endif
	RegisterTool(&TOOL_RGETC1, &CODEC_ETC1);
	RegisterTool(&TOOL_PVRTEX, &CODEC_ETC1);
	RegisterTool(&TOOL_ETC2COMP, &CODEC_ETC1);
}

void CodecETC1_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}


void CodecETC1_Load(void)
{
}

/*
==========================================================================================

  ENCODING

==========================================================================================
*/

bool CodecETC1_Accept(TexEncodeTask *task)
{
	// ETC1 can encode only RGB
	if (task->image->hasAlpha)
		return false;
	return true;
}

// extract 4x4 RGBA block from source image
void CodecETC1_ExtractBlockRGBA(const unsigned char *src, int x, int y, int w, int h, int pitch, unsigned char *block)
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
			block[(i*4*4) + (j*4) + 0] = src[(by * pitch) + (bx*4) + 0];
			block[(i*4*4) + (j*4) + 1] = src[(by * pitch) + (bx*4) + 1];
			block[(i*4*4) + (j*4) + 2] = src[(by * pitch) + (bx*4) + 2];
			block[(i*4*4) + (j*4) + 3] = src[(by * pitch) + (bx*4) + 3];
		}
	}
}

void CodecETC1_Encode(TexEncodeTask *task)
{
	// select format
	if (!task->format)
		task->format = &F_ETC1;
	// select compressor tool
	if (!task->tool)
		task->tool = &TOOL_ETC2COMP;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

// using EtcPack to decode ETC1
void CodecETC1_Decode(TexDecodeTask *task)
{
	byte *data, *stream;
	int x, y, w, h;
	size_t size;

	w = task->image->width;
	h = task->image->height;
	size = w * h * task->image->bpp;
	data = (byte *)mem_alloc(size);
	stream = task->pixeldata;
	for (y = 0; y < h / 4; y++)
	{
		for (x = 0; x < w / 4; x++)
		{
			// etcpack path
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
			// ETC2 is backwards compatible, which means that an ETC2-capable decompressor
			// can also handle old ETC1 textures without any problems.
			etcpack_decompressBlockETC2c(block1, block2, data, w, h, x*4, y*4, task->image->bpp);
		}
	}
	Image_StoreUnalignedData(task->image, data, size);
	free(data);
	task->image->colorSwap = false;
}