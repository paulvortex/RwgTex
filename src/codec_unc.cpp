////////////////////////////////////////////////////////////////
//
// RwgTex / uncompressed texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "tex.h"


TexBlock  B_BGRA = { FOURCC('B','G','R','A'), "BGRA", 1, 1, 32 };
TexBlock  B_BGR6 = { FOURCC('B','G','R','6'), "BGR6", 1, 1, 24 };
TexBlock  B_BGR3 = { FOURCC('B','G','R','4'), "BGR4", 1, 1, 24 };
TexBlock  B_BGR1 = { FOURCC('B','G','R','1'), "BGR1", 1, 1, 24 };

TexFormat F_BGRA = { FOURCC('B','G','R','A'), "BGRA", "Uncompressed 32-bit BGRA8888", "bgra", &B_BGRA, &CODEC_BGRA, GL_BGRA, GL_SRGB_ALPHA_EXT, GL_BGRA, GL_UNSIGNED_BYTE, FF_ALPHA | FF_SRGB };
TexFormat F_BGR6 = { FOURCC('B','G','R','6'), "BGR6", "Uncompressed 24-bit BGRA6666", "bgr6", &B_BGR6, &CODEC_BGRA, GL_BGR,  GL_SRGB_ALPHA_EXT, GL_BGR,  GL_UNSIGNED_BYTE, FF_ALPHA | FF_SRGB, Swizzle_AlphaInRGB6 };
TexFormat F_BGR3 = { FOURCC('B','G','R','4'), "BGR3", "Uncompressed 24-bit BGRA7773", "bgr3", &B_BGR3, &CODEC_BGRA, GL_BGR,  GL_SRGB_ALPHA_EXT, GL_BGR,  GL_UNSIGNED_BYTE, FF_ALPHA | FF_SRGB, Swizzle_AlphaInRGB3 };
TexFormat F_BGR1 = { FOURCC('B','G','R','1'), "BGR1", "Uncompressed 24-bit BGRA8871", "bgr1", &B_BGR1, &CODEC_BGRA, GL_BGR,  GL_SRGB_ALPHA_EXT, GL_BGR,  GL_UNSIGNED_BYTE, FF_ALPHA | FF_SRGB | FF_PUNCH_THROUGH_ALPHA, Swizzle_AlphaInRGB1 };

TexCodec  CODEC_BGRA = 
{
	"UNC", "Uncompressed BGR/BGRA codec", "unc",
	&CodecBGRA_Init,
	&CodecBGRA_Option,
	&CodecBGRA_Load,
	&CodecBGRA_Accept,
	&CodecBGRA_Encode,
	&CodecBGRA_Decode,
};

void CodecBGRA_Init(void)
{
	RegisterTool(&TOOL_RWGTP, &CODEC_BGRA);
}

void CodecBGRA_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void CodecBGRA_Load(void)
{
}

/*
==========================================================================================

  ENCODING

==========================================================================================
*/

bool CodecBGRA_Accept(TexEncodeTask *task)
{
	// uncompressed format can encode any data
	return true;
}

void CodecBGRA_Encode(TexEncodeTask *task)
{
	if (!task->format)
		task->format = &F_BGRA;
	if (!task->tool)
		task->tool = &TOOL_RWGTP;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

void CodecBGRA_Decode(TexDecodeTask *task)
{
	Image_StoreUnalignedData(task->image, task->pixeldata, task->image->width*task->image->height*task->image->bpp);
	task->image->colorSwap = task->ImageParms.colorSwap;
}

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/

// alpha stored in RGB bits, stores 2 bits in each channele, resulting in 666 RGB color, 6-bit alpha
void Swizzle_AlphaInRGB6(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode)
{
	int y;

	if (bpp != 4)
		Error("Swizzle_AlphaInRGB6: image have no alpha channel!\n");

	// decode
	if (decode)
	{
		for (y = 0; y < height; y++)
		{
			byte *in = data;
			byte *end = in + width*bpp;
			while(data < end)
			{
				in[3] = (((in[0] & 0xC0) >> 6) + ((in[1] & 0xC0) >> 4) + ((in[2] & 0xC0) >> 2)) * 4;
				in[0] = (in[0] & 0x3F) * 4;
				in[1] = (in[1] & 0x3F) * 4;
				in[2] = (in[2] & 0x3F) * 4;
				in += bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	for (y = 0; y < height; y++)
	{
		byte *in = data;
		byte *end = in + width*bpp;
		while(in < end)
		{
			byte a = (byte)(floor((float)in[3] * (63.0f / 255.0f) + 0.5f));
			in[0] = (byte)(floor((float)in[0] * (63.0f / 255.0f) + 0.5f) + ((a & 0x3) << 6));
			in[1] = (byte)(floor((float)in[1] * (63.0f / 255.0f) + 0.5f) + (((a >> 2) & 0x3) << 6));
			in[2] = (byte)(floor((float)in[2] * (63.0f / 255.0f) + 0.5f) + (((a >> 4) & 0x3) << 6));
			in += bpp;
		}
		data += pitch;
	}
}

// alpha stored in RGB bits, stores 1 bits in each channele, resulting in 777 RGB color, 3-bit alpha
void Swizzle_AlphaInRGB3(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode)
{
	int y;

	if (bpp != 4)
		Error("Swizzle_AlphaInRGB3: image have no alpha channel!\n");

	// decode
	if (decode)
	{
		for (y = 0; y < height; y++)
		{
			byte *in = data;
			byte *end = in + width*bpp;
			while(in < end)
			{
				in[3] =(((in[0] & 0x80) >> 7) + ((in[1] & 0x80) >> 6) + ((in[2] & 0x80) >> 5)) * 32;
				in[0] = (in[0] & 0x7F) * 2;
				in[1] = (in[1] & 0x7F) * 2;
				in[2] = (in[2] & 0x7F) * 2;
				in += bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	for (y = 0; y < height; y++)
	{
		byte *in = data;
		byte *end = in + width*bpp;
		while(in < end)
		{
			byte a = (byte)(floor((float)in[3] * (7.0f   / 255.0f) + 0.5f));
			in[0] = (byte)(floor((float)in[0] * (127.0f / 255.0f) + 0.5f) + ((a & 0x1) << 7));
			in[1] = (byte)(floor((float)in[1] * (127.0f / 255.0f) + 0.5f) + (((a >> 1) & 0x1) << 7));
			in[2] = (byte)(floor((float)in[2] * (127.0f / 255.0f) + 0.5f) + (((a >> 2) & 0x1) << 7));
			in += bpp;
		}
		data += pitch;
	}
}

// 1-bit alpha stored in blue channel
void Swizzle_AlphaInRGB1(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode)
{
	int y;

	if (bpp != 4)
		Error("Swizzle_AlphaInRGB1: image have no alpha channel!\n");

	// decode
	if (decode)
	{
		for (y = 0; y < height; y++)
		{
			byte *in = data;
			byte *end = in + width*bpp;
			while(in < end)
			{
				in[3] = (byte)((in[2] & 0x80) * 255.0f/128.0f);
				in[2] = (in[2] & 0x7F) * 2;
				in += bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	for (y = 0; y < height; y++)
	{
		byte *in = data;
		byte *end = in + width*bpp;
		while(in < end)
		{
			in[2] = min(0x7F, (byte)floor((float)in[2] * (127.0f / 255.0f) + 0.5f) + ((in[3] > tex_binaryAlphaCenter) ? 0x80 : 0));
			in += bpp;
		}
		data += pitch;
	}
}