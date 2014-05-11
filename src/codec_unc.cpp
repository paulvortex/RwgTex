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

TexFormat F_BGRA = { FOURCC('B','G','R','A'), "BGRA", "Uncompressed 32-bit BGRA8888", "bgra", &B_BGRA, &CODEC_BGRA, GL_BGRA, GL_BGRA, GL_UNSIGNED_BYTE, FF_ALPHA };
TexFormat F_BGR6 = { FOURCC('B','G','R','6'), "BGR6", "Uncompressed 24-bit BGRA6666", "bgr6", &B_BGR6, &CODEC_BGRA, GL_BGR,  GL_BGR,  GL_UNSIGNED_BYTE, FF_ALPHA, Swizzle_AlphaInRGB6 };
TexFormat F_BGR3 = { FOURCC('B','G','R','4'), "BGR3", "Uncompressed 24-bit BGRA7773", "bgr3", &B_BGR3, &CODEC_BGRA, GL_BGR,  GL_BGR,  GL_UNSIGNED_BYTE, FF_ALPHA, Swizzle_AlphaInRGB3 };
TexFormat F_BGR1 = { FOURCC('B','G','R','1'), "BGR1", "Uncompressed 24-bit BGRA8871", "bgr1", &B_BGR1, &CODEC_BGRA, GL_BGR,  GL_BGR,  GL_UNSIGNED_BYTE, FF_ALPHA | FF_BINARYALPHA, Swizzle_AlphaInRGB1 };

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
	RegisterTool(&TOOL_BGRA, &CODEC_BGRA);
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
		task->tool = &TOOL_BGRA;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

void CodecBGRA_Decode(TexDecodeTask *task)
{
	byte *data;

	data = Image_GetData(task->image, NULL);
	memcpy(data, task->pixeldata, task->image->width*task->image->height*task->image->bpp);
	task->image->colorSwap = task->colorSwap;
}

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/

// alpha stored in RGB bits, stores 2 bits in each channele, resulting in 666 RGB color, 6-bit alpha
void Swizzle_AlphaInRGB6(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_AlphaInRGB6: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	// decode
	if (decode)
	{
		while(data < end)
		{
			data[3] = (((data[0] & 0xC0) >> 6) + ((data[1] & 0xC0) >> 4) + ((data[2] & 0xC0) >> 2)) * 4;
			data[0] =  (data[0] & 0x3F) * 4;
			data[1] =  (data[1] & 0x3F) * 4;
			data[2] =  (data[2] & 0x3F) * 4;
			data += image->bpp;
		}
		return;
	}
	// encode
	while(data < end)
	{
		byte a  = (byte)(floor((float)data[3] * (63.0f / 255.0f) + 0.5f));
		data[0] = (byte)(floor((float)data[0] * (63.0f / 255.0f) + 0.5f) + ((a & 0x3) << 6));
		data[1] = (byte)(floor((float)data[1] * (63.0f / 255.0f) + 0.5f) + (((a >> 2) & 0x3) << 6));
		data[2] = (byte)(floor((float)data[2] * (63.0f / 255.0f) + 0.5f) + (((a >> 4) & 0x3) << 6));
		data += image->bpp;
	}
	Image_ConvertBPP(image, 3);
}

// alpha stored in RGB bits, stores 1 bits in each channele, resulting in 777 RGB color, 3-bit alpha
void Swizzle_AlphaInRGB3(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_AlphaInRGB3: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	// decode
	if (decode)
	{
		while(data < end)
		{
			data[3] =(((data[0] & 0x80) >> 7) + ((data[1] & 0x80) >> 6) + ((data[2] & 0x80) >> 5)) * 32;
			data[0] =  (data[0] & 0x7F) * 2;
			data[1] =  (data[1] & 0x7F) * 2;
			data[2] =  (data[2] & 0x7F) * 2;
			data += image->bpp;
		}
		return;
	}
	// encode
	while(data < end)
	{
		byte a  = (byte)(floor((float)data[3] * (7.0f   / 255.0f) + 0.5f));
		data[0] = (byte)(floor((float)data[0] * (127.0f / 255.0f) + 0.5f) + ((a & 0x1) << 7));
		data[1] = (byte)(floor((float)data[1] * (127.0f / 255.0f) + 0.5f) + (((a >> 1) & 0x1) << 7));
		data[2] = (byte)(floor((float)data[2] * (127.0f / 255.0f) + 0.5f) + (((a >> 2) & 0x1) << 7));
		data += image->bpp;
	}
	Image_ConvertBPP(image, 3);
}

// 1-bit alpha stored in blue channel
void Swizzle_AlphaInRGB1(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_AlphaInRGB1: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	// decode
	if (decode)
	{
		while(data < end)
		{
			data[3] = (byte)((data[2] & 0x80) * 255.0f/128.0f);
			data[2] = (data[2] & 0x7F) * 2;
			data += image->bpp;
		}
		return;
	}
	// encode
	while(data < end)
	{
		data[2] = min(0x7F, (byte)floor((float)data[2] * (127.0f / 255.0f) + 0.5f) + ((data[3] > tex_binaryAlphaCenter) ? 0x80 : 0));
		data += image->bpp;
	}
	Image_ConvertBPP(image, 3);
}
