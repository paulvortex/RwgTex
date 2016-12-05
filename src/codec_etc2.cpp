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

TexFormat F_ETC2    = { FOURCC('E','T','C','2'), "ETC2",   "ETC2 RGB",    "etc2rgb",   &B_ETC2,  &CODEC_ETC2, GL_COMPRESSED_RGB8_ETC2,                     GL_COMPRESSED_SRGB8_ETC2,                     GL_RGBA, 0, FF_SRGB };
TexFormat F_ETC2A   = { FOURCC('E','T','C','A'), "ETC2A",  "ETC2 RGBA",   "etc2rgba",  &B_ETC2A, &CODEC_ETC2, GL_COMPRESSED_RGBA8_ETC2_EAC,                GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_RGBA, 0, FF_ALPHA | FF_SRGB };
TexFormat F_ETC2A1  = { FOURCC('E','T','C','P'), "ETC2A1", "ETC2 RGBA1",  "etc2rgba1", &B_ETC2A1,&CODEC_ETC2, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_RGBA, 0, FF_ALPHA | FF_BINARYALPHA | FF_SRGB };
TexFormat F_EAC1    = { FOURCC('E','A','C','1'), "EAC1",   "ETC2 R",      "eac1",      &B_EAC1,  &CODEC_ETC2, GL_COMPRESSED_R11_EAC,                       0,                                            GL_RED };
TexFormat F_EAC2    = { FOURCC('E','A','C','2'), "EAC2",   "ETC2 RG",     "eac2",      &B_EAC2,  &CODEC_ETC2, GL_COMPRESSED_RG11_EAC,                      0,                                            GL_RG };
   
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
	RegisterTool(&TOOL_PVRTEX, &CODEC_ETC2);
	RegisterTool(&TOOL_ETC2COMP, &CODEC_ETC2);
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
	// determine format
	if (!task->format)
	{
		if (task->image->datatype == IMAGE_GRAYSCALE)
			task->format = &F_ETC2;
		else if (task->image->hasAlpha)
			task->format = task->image->hasGradientAlpha ? &F_ETC2A : &F_ETC2A;
		else
			task->format = &F_ETC2;
	}

	// ETC2 with alpha == ETC2A
	if (task->image->hasAlpha)
	{
		if (task->format == &F_ETC2)
			task->format = &F_ETC2A;
	}
	else
	{
		// ETC2A with no alpha = ETC2
		if (task->format == &F_ETC2A || task->format == &F_ETC2A1)
			task->format = &F_ETC2;
	}

	// select compressor tool
	if (!task->tool)
		task->tool = &TOOL_ETC2COMP;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

// read color block from data stream
void readColorBlockETC(byte **stream, unsigned int &block1, unsigned int &block2)
{
	byte *data = *stream;
	block1 = 0;           block1 |= data[0];
	block1 = block1 << 8; block1 |= data[1];
	block1 = block1 << 8; block1 |= data[2];
	block1 = block1 << 8; block1 |= data[3];
	block2 = 0;           block2 |= data[4];
	block2 = block2 << 8; block2 |= data[5];
	block2 = block2 << 8; block2 |= data[6];
	block2 = block2 << 8; block2 |= data[7];
	*stream = data + 8;
}

// using EtcPack to decode ETC2
void CodecETC2_Decode(TexDecodeTask *task)
{
	byte *data, *stream, rgba[4*4*4], *lb;
	unsigned int block1, block2;
	int x, y, w, h, bpp;
	int lx, ly, lw, lh;

	// init
	w = task->image->width;
	h = task->image->height;
	bpp = task->image->bpp;
	data = (byte *)mem_alloc(w * h * bpp);
	stream = task->pixeldata;

	// decode
	if (task->format->block == &B_ETC2)
	{
		for (y = 0; y < h; y+=4)
		{
			for (x = 0; x < w; x+=4)
			{
				readColorBlockETC(&stream, block1, block2);
				etcpack_decompressBlockETC2c(block1, block2, rgba, 4, 4, 0, 0, 4);
				lb = rgba;
				lh = min(y + 4, h) - y;
				lw = min(x + 4, w) - x;
				for (ly = 0; ly < lh; ly++,lb+=4*4)
					for(lx = 0; lx < lw; lx++)
						memcpy(data + (w*(y + ly) + x + lx)*bpp, lb + lx*4, 3);
			}
		}
	}
	else if (task->format->block == &B_ETC2A)
	{
		for (y = 0; y < h; y+=4)
		{
			for (x = 0; x < w; x+=4)
			{
				// EAC block + ETC2 RGB block
				etcpack_decompressBlockAlphaC(stream, rgba+3, 4, 4, 0, 0, 4);
				stream += 8;
				readColorBlockETC(&stream, block1, block2);
				etcpack_decompressBlockETC2c(block1, block2, rgba, 4, 4, 0, 0, 4);
				lb = rgba;
				lh = min(y + 4, h) - y;
				lw = min(x + 4, w) - x;
				for (ly = 0; ly < lh; ly++,lb+=4*4)
					for(lx = 0; lx < lw; lx++)
						memcpy(data + (w*(y + ly) + x + lx)*bpp, lb + lx*4, 4);
			}
		}
	}
	else if (task->format->block == &B_ETC2A1)
	{
		for (y = 0; y < h; y+=4)
		{
			for (x = 0; x < w; x+=4)
			{
				// ETC2 RGB/punchthrough alpha block 
				readColorBlockETC(&stream, block1, block2);
				etcpack_decompressBlockETC21BitAlphaC(block1, block2, rgba, NULL, 4, 4, 0, 0, 4);
				lb = rgba;
				lh = min(y + 4, h) - y;
				lw = min(x + 4, w) - x;
				for (ly = 0; ly < lh; ly++,lb+=4*4)
					for(lx = 0; lx < lw; lx++)
						memcpy(data + (w*(y + ly) + x + lx)*bpp, lb + lx*4, 4);
			}
		}
	}
	else
		Error("CodecETC2_Decode: block %s not supported\n", task->format->block->name);

	// store decoded image
	Image_StoreUnalignedData(task->image, data, w*h*bpp);
	free(data);
	task->image->colorSwap = false;
}

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/