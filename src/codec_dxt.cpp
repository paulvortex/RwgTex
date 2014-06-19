////////////////////////////////////////////////////////////////
//
// RwgTex / DXT texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

TexBlock  B_DXT1  = { FOURCC('D','X','T','1'), "DXT1", 4, 4, 64  };
TexBlock  B_DXT2  = { FOURCC('D','X','T','2'), "DXT2", 4, 4, 128 };
TexBlock  B_DXT3  = { FOURCC('D','X','T','3'), "DXT3", 4, 4, 128 };
TexBlock  B_DXT4  = { FOURCC('D','X','T','4'), "DXT4", 4, 4, 128 };
TexBlock  B_DXT5  = { FOURCC('D','X','T','5'), "DXT5", 4, 4, 128 };

TexFormat F_DXT1  = { FOURCC('D','X','T','1'), "DXT1", "DirectX Texture Compression 1",  "dxt1", &B_DXT1, &CODEC_DXT, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,  GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,       GL_RGB,  0, FF_SRGB };
TexFormat F_DXT1A = { FOURCC('D','X','T','1'), "DXT1A","DirectX Texture Compression 1A", "dxt1a",&B_DXT1, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_RGBA, 0, FF_ALPHA | FF_BINARYALPHA | FF_SRGB };
TexFormat F_DXT2  = { FOURCC('D','X','T','2'), "DXT2", "DirectX Texture Compression 2",  "dxt2", &B_DXT2, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, 0, FF_ALPHA | FF_SRGB, Swizzle_Premult };
TexFormat F_DXT3  = { FOURCC('D','X','T','3'), "DXT3", "DirectX Texture Compression 3",  "dxt3", &B_DXT3, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, 0, FF_ALPHA | FF_SRGB };
TexFormat F_DXT4  = { FOURCC('D','X','T','4'), "DXT4", "DirectX Texture Compression 4",  "dxt4", &B_DXT4, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA | FF_SRGB, Swizzle_Premult };
TexFormat F_DXT5  = { FOURCC('D','X','T','5'), "DXT5", "DirectX Texture Compression 5",  "dxt5", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA | FF_SRGB, NULL, F_SWIZZLED_DXT5};

// swizzled formats
TexFormat F_DXT5_RXGB  = { FOURCC('R','X','G','B'), "RXGB", "Doom 3 RXGB (swizzled DXT5)",            "rxgb", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_SRGB, Swizzle_AGBR };
TexFormat F_DXT5_YCG1  = { FOURCC('Y','C','G','1'), "YCG1", "YCoCg (swizzled DXT5)",                  "ycg1", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA | FF_SRGB, Swizzle_YCoCg };
TexFormat F_DXT5_YCG2  = { FOURCC('Y','C','G','2'), "YCG2", "YCoCg Scaled (swizzled DXT5)",           "ycg2", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_SRGB, Swizzle_YCoCgScaled };
TexFormat F_DXT5_YCG3  = { FOURCC('Y','C','G','3'), "YCG3", "YCoCg Gamma 2.0 (swizzled DXT5)",        "ycg3", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_YCoCg_Gamma2 };
TexFormat F_DXT5_YCG4  = { FOURCC('Y','C','G','4'), "YCG4", "YCoCg Scaled Gamma 2.0 (swizzled DXT5)", "ycg4", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_YCoCgScaled_Gamma2 };
TexFormat *F_SWIZZLED_DXT5[] =
{
	&F_DXT5_RXGB,
	&F_DXT5_YCG1,
	&F_DXT5_YCG2,
	&F_DXT5_YCG3,
	&F_DXT5_YCG4,
	NULL
};

TexCodec  CODEC_DXT = 
{
	"DXT", "DXT Texture Compression", "dxt",
	&CodecDXT_Init,
	&CodecDXT_Option,
	&CodecDXT_Load,
	&CodecDXT_Accept,
	&CodecDXT_Encode,
	&CodecDXT_Decode,
};

void CodecDXT_Init(void)
{
	RegisterTool(&TOOL_NVDXTLIB, &CODEC_DXT);
	RegisterTool(&TOOL_GIMPDDS, &CODEC_DXT);
	RegisterTool(&TOOL_CRUNCH, &CODEC_DXT);
	RegisterTool(&TOOL_PVRTEX, &CODEC_DXT);
#ifndef NO_ATITC
	RegisterTool(&TOOL_ATITC, &CODEC_DXT);
#endif
#ifndef NO_NVTT
	RegisterTool(&TOOL_NVTT, &CODEC_DXT);
#endif
}

void CodecDXT_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void CodecDXT_Load(void)
{
	if (!CODEC_DXT.forceTool)
		Print("%s codec: using hybrid (autoselect) compressor\n", CODEC_DXT.name);
}

/*
==========================================================================================

  ENCODING

==========================================================================================
*/

bool CodecDXT_Accept(TexEncodeTask *task)
{
	// DXT can encode any data
	return true;
}

void CodecDXT_Encode(TexEncodeTask *task)
{
	// select format
	if (!task->format)
	{
		if (task->image->datatype == IMAGE_GRAYSCALE)
			task->format = &F_DXT1;
		else if (task->image->hasAlpha)
			task->format = task->image->hasGradientAlpha ? &F_DXT5 : &F_DXT1A;
		else
			task->format = &F_DXT1;
	}

	// DXT1 with alpha == DXT1A
	if (task->image->hasAlpha)
	{
		if (task->format == &F_DXT1)
			task->format = &F_DXT1A;
	}
	else
	{
		// DXT1A with no alpha = DXT1
		if (task->format == &F_DXT1A)
			task->format = &F_DXT1;
		// check if selected DXT3/DXT5 and image have no alpha information
		if (task->format == &F_DXT3 || task->format == &F_DXT5)
			task->format = &F_DXT1;
	}

	// select compressor tool
	if (!task->tool)
	{
		// hybrid mode, pick best
		// NVidia tool compresses normalmaps better than ATI
		// while ATI wins over general DXT1 and DXT5
#ifndef NO_ATITC
		if (task->image->datatype == IMAGE_NORMALMAP)
			task->tool = &TOOL_NVDXTLIB;
		else
			task->tool = &TOOL_ATITC;
#else
		task->tool = &TOOL_NVDXTLIB;
#endif
	}
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

// using GimpDDS to decode DXT
void CodecDXT_Decode(TexDecodeTask *task)
{
	byte *data;
	int dxtformat;
	size_t size;

	size = task->image->width * task->image->height * task->image->bpp;
	data = (byte *)mem_alloc(size);
	if (task->format->block == &B_DXT1)
		dxtformat = DDS_COMPRESS_BC1;
	else if (task->format->block == &B_DXT2 || task->format->block == &B_DXT3)
		dxtformat = DDS_COMPRESS_BC2;
	else if (task->format->block == &B_DXT4 || task->format->block == &B_DXT5)
		dxtformat = DDS_COMPRESS_BC3;
	else
		Error("CodecDXT_Decode: block compression type %s not supported\n", task->format->block->name);
	dxt_decompress(data, task->pixeldata, dxtformat, task->pixeldatasize, task->image->width, task->image->height, task->image->bpp, 0);
	Image_StoreUnalignedData(task->image, data, size);
	free(data);
	task->image->colorSwap = task->ImageParms.colorSwap ? false : true; // dxt_decompress() swaps BGR->RGB
}

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/

// Color premultiplied by alpha (DXT2/DXT3)
void Swizzle_Premult(LoadedImage *image, bool decode)
{
	int pitch, y;

	byte *data = Image_GetData(image, NULL, &pitch);
	if (decode)
	{
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				float mod = 255.0f / (float)in[3];
				in[0] = (byte)min(255, max(0, (in[0] * mod)));
				in[1] = (byte)min(255, max(0, (in[1] * mod)));
				in[2] = (byte)min(255, max(0, (in[2] * mod)));
				in += image->bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			float mod = (float)in[3] / 255.0f;
			in[0] = (byte)(in[0] * mod);
			in[1] = (byte)(in[1] * mod);
			in[2] = (byte)(in[2] * mod);
			in += image->bpp;
		}
		data += pitch;
	}
}

// Doom 3's normalmap trick (store R channel in DXT5 alpha)
void Swizzle_XGBR(LoadedImage *image, bool decode)
{
	int pitch, y;

	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_XGBR: image have no alpha channel!\n");
	byte *data = Image_GetData(image, NULL, &pitch);
	// decode
	if (decode)
	{
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				in[0] = in[3];
				in[3] = 0;
				in += image->bpp;
			}
			data += pitch;
		}
		Image_ConvertBPP(image, 3);
		return;
	}
	// encode
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			in[3] = in[2];
			in[2] = 0;
			in += image->bpp;
		}
		data += pitch;
	}
}

// Doom 3's normalmap trick (store R channel in DXT5 alpha) with alpha stored in R (it seems it is only suitable for binaryalpha)
void Swizzle_AGBR(LoadedImage *image, bool decode)
{
	int pitch, y;

	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_AGBR: image have no alpha channel!\n");
	byte *data = Image_GetData(image, NULL, &pitch);
	// decode
	if (decode)
	{
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				byte saved = in[0];
				in[0] = in[3];
				in[3] = saved;
				in += image->bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	byte saved;
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			if (image->colorSwap == false)
			{
				saved = in[0];
				in[0] = in[3];
				in[3] = saved;
			}
			else
			{
				saved = in[2];
				in[2] = in[3];
				in[3] = saved;
			}
			in += image->bpp;
		}
		data += pitch;
	}
}

// IdTech5 Chrominance/Luminance swizzle (can store low quality alpha)
void Swizzle_YCoCg(LoadedImage *image, bool decode)
{
	int pitch, y;

	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = Image_GetData(image, NULL, &pitch);
	if (decode)
	{
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				byte T;
				float Y, Co, Cg, R, G, B;
				const float offset = 0.5f * 256.0f / 255.0f;
				Y  = (float)in[3]/255.0f;
				Co = (float)in[0]/255.0f - offset;
				Cg = (float)in[1]/255.0f - offset;
				R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
				G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
				B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
				in[3] = in[2]; // restore alpha from blue
				in[0] = (byte)(R * 255.0f);
				in[1] = (byte)(G * 255.0f);
				in[2] = (byte)(B * 255.0f);
				if (image->colorSwap == true)
				{
					T = in[2];
					in[2] = in[0];
					in[0] = T;
				}
				in += image->bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	byte Y, Co, Cg;
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			Y  = ((in[2] + (in[1] << 1) + in[0]) + 2) >> 2;
			Co = ((((in[2] << 1) - (in[0] << 1)) + 2) >> 2) + 128;
			Cg = (((-in[2] + (in[1] << 1) - in[0]) + 2) >> 2) + 128;
			in[0] = in[3];
			in[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
			in[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
			in[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
			if (image->colorSwap == false)
			{
				Y = in[2];
				in[2] = in[0];
				in[0] = Y;
			}
			in += image->bpp;
		}
		data += pitch;
	}
}

// YCoCg Unscaled Gamma 2.0
void Swizzle_YCoCg_Gamma2(LoadedImage *image, bool decode)
{
	int pitch, y;

	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = Image_GetData(image, NULL, &pitch);
	if (decode)
	{
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				byte T;
				float Y, Co, Cg, R, G, B;
				const float offset = 0.5f * 256.0f / 255.0f;
				Y  = (float)in[3]/255.0f;
				Co = (float)in[0]/255.0f - offset;
				Cg = (float)in[1]/255.0f - offset;
				R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
				G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
				B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
				R = R * R;
				G = G * G;
				B = B * B;
				in[3] = in[2]; // restore alpha from blue
				in[0] = (byte)(R * 255.0f);
				in[1] = (byte)(G * 255.0f);
				in[2] = (byte)(B * 255.0f);
				if (image->colorSwap == true)
				{
					T = in[2];
					in[2] = in[0];
					in[0] = T;
				}
				in += image->bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode
	byte Y, Co, Cg, R, G, B;
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			R  = (byte)floor(sqrt((float)in[0] / 255.0f) * 255.0f + 0.5f);
			G  = (byte)floor(sqrt((float)in[1] / 255.0f) * 255.0f + 0.5f);
			B  = (byte)floor(sqrt((float)in[2] / 255.0f) * 255.0f + 0.5f);
			Y  = ((B + (G << 1) + R) + 2) >> 2;
			Co = ((((B << 1) - (R << 1)) + 2) >> 2) + 128;
			Cg = (((-B + (G << 1) - R) + 2) >> 2) + 128;
			in[0] = in[3];
			in[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
			in[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
			in[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
			if (image->colorSwap == false)
			{
				Y = in[2];
				in[2] = in[0];
				in[0] = Y;
			}
			in += image->bpp;
		}
		data += pitch;
	}
}

// IdTech5 Chrominance/Luminance swizzle - color scaled variant (doesnt store alpha at all)
void Swizzle_YCoCgScaled(LoadedImage *image, bool decode)
{
	int pitch, y;

	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = Image_GetData(image, NULL, &pitch);
	if (decode)
	{
		byte T;
		float Y, Co, Cg, R, G, B, s;
		const float offset = 0.5f * 256.0f / 255.0f;
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				Y  = (float)in[3]/255.0f;
				Co = (float)in[0]/255.0f;
				Cg = (float)in[1]/255.0f;
				s  = (float)in[2]/255.0f;
				s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
				Co = (Co - offset) * s;
				Cg = (Cg - offset) * s;
				R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
				G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
				B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
				in[0] = (byte)(R * 255.0f);
				in[1] = (byte)(G * 255.0f);
				in[2] = (byte)(B * 255.0f);
				in[3] = 255; // set alpha to 1
				if (image->colorSwap == true)
				{
					T = in[2];
					in[2] = in[0];
					in[0] = T;
				}
				in += image->bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode YCoCg (scale will be applied during compression)
	byte Y, Co, Cg;
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			Y  = ((in[2] + (in[1] << 1) + in[0]) + 2) >> 2;
			Co = ((((in[2] << 1) - (in[0] << 1)) + 2) >> 2) + 128;
			Cg = (((-in[2] + (in[1] << 1) - in[0]) + 2) >> 2) + 128;
			in[0] = 0;
			in[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
			in[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
			in[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
			if (image->colorSwap == false)
			{
				Y = in[2];
				in[2] = in[0];
				in[0] = Y;
			}
			in += image->bpp;
		}
		data += pitch;
	}
}

// YCoCg Scaled Gamma 2.0
void Swizzle_YCoCgScaled_Gamma2(LoadedImage *image, bool decode)
{
	int pitch, y;

	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = Image_GetData(image, NULL, &pitch);
	if (decode)
	{
		byte T;
		float Y, Co, Cg, R, G, B, s;
		const float offset = 0.5f * 256.0f / 255.0f;
		for (y = 0; y < image->height; y++)
		{
			byte *in = data;
			byte *end = in + image->width*image->bpp;
			while(in < end)
			{
				Y  = (float)in[3]/255.0f;
				Co = (float)in[0]/255.0f;
				Cg = (float)in[1]/255.0f;
				s  = (float)in[2]/255.0f;
				s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
				Co = (Co - offset) * s;
				Cg = (Cg - offset) * s;
				R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
				G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
				B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
				R = R * R;
				G = G * G;
				B = B * B;
				in[0] = (byte)(R * 255.0f);
				in[1] = (byte)(G * 255.0f);
				in[2] = (byte)(B * 255.0f);
				in[3] = 255; // set alpha to 1
				if (image->colorSwap == true)
				{
					T = in[2];
					in[2] = in[0];
					in[0] = T;
				}
				in += image->bpp;
			}
			data += pitch;
		}
		return;
	}
	// encode YCoCg (scale will be applied during compression)
	byte Y, Co, Cg, R, G, B;
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + image->width*image->bpp;
		while(in < end)
		{
			R  = (byte)floor(sqrt((float)in[0] / 255.0f) * 255.0f + 0.5f);
			G  = (byte)floor(sqrt((float)in[1] / 255.0f) * 255.0f + 0.5f);
			B  = (byte)floor(sqrt((float)in[2] / 255.0f) * 255.0f + 0.5f);
			Y  = ((B + (G << 1) + R) + 2) >> 2;
			Co = ((((B << 1) - (R << 1)) + 2) >> 2) + 128;
			Cg = (((-B + (G << 1) - R) + 2) >> 2) + 128;
			in[0] = 0;
			in[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
			in[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
			in[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
			if (image->colorSwap == false)
			{
				Y = in[2];
				in[2] = in[0];
				in[0] = Y;
			}
			in += image->bpp;
		}
		data += pitch;
	}
}