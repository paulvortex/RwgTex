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

TexFormat F_DXT1  = { FOURCC('D','X','T','1'), "DXT1", "DirectX Texture Compression 1",  "dxt1", &B_DXT1, &CODEC_DXT, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,  GL_RGB };
TexFormat F_DXT1A = { FOURCC('D','X','T','1'), "DXT1A","DirectX Texture Compression 1A", "dxt1a",&B_DXT1, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, 0, FF_ALPHA | FF_BINARYALPHA };
TexFormat F_DXT2  = { FOURCC('D','X','T','2'), "DXT2", "DirectX Texture Compression 2",  "dxt2", &B_DXT2, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_Premult };
TexFormat F_DXT3  = { FOURCC('D','X','T','3'), "DXT3", "DirectX Texture Compression 3",  "dxt3", &B_DXT3, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, 0, FF_ALPHA };
TexFormat F_DXT4  = { FOURCC('D','X','T','4'), "DXT4", "DirectX Texture Compression 4",  "dxt4", &B_DXT4, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_Premult };
TexFormat F_DXT5  = { FOURCC('D','X','T','5'), "DXT5", "DirectX Texture Compression 5",  "dxt5", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA };
TexFormat F_RXGB  = { FOURCC('R','X','G','B'), "RXGB", "Doom 3 RXGB (swizzled DXT5)",    "rxgb", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_AGBR };
TexFormat F_YCG1  = { FOURCC('Y','C','G','1'), "YCG1", "YCoCg",                          "ycg1", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_YCoCg };
TexFormat F_YCG2  = { FOURCC('Y','C','G','2'), "YCG2", "YCoCg Scaled",                   "ycg2", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_YCoCgScaled };
TexFormat F_YCG3  = { FOURCC('Y','C','G','3'), "YCG3", "YCoCg Gamma 2.0",                "ycg3", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_YCoCg_Gamma2 };
TexFormat F_YCG4  = { FOURCC('Y','C','G','4'), "YCG4", "YCoCg Scaled Gamma 2.0",         "ycg4", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_YCoCgScaled_Gamma2 };

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
	// determine format
	if (!task->format)
	{
		if (task->image->datatype == IMAGE_GRAYSCALE)
			task->format = &F_DXT1;
		else if (task->image->hasAlpha)
			task->format = task->image->hasGradientAlpha ? &F_DXT5 : &F_DXT1A;
		else
			task->format = &F_DXT1;
	}

	// DXT1 with alpha == DXT1A and so on
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
	task->tool = CODEC_DXT.forceTool;
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

	data = Image_GetData(task->image, NULL);
	if (task->format->block == &B_DXT1)
		dxtformat = DDS_COMPRESS_BC1;
	else if (task->format->block == &B_DXT2 || task->format->block == &B_DXT3)
		dxtformat = DDS_COMPRESS_BC2;
	else if (task->format->block == &B_DXT4 || task->format->block == &B_DXT5)
		dxtformat = DDS_COMPRESS_BC3;
	else
		Error("CodecDXT_Decode: block compression type %s not supported\n", task->format->block->name);
	dxt_decompress(data, task->pixeldata, dxtformat, task->pixeldatasize, task->image->width, task->image->height, task->image->bpp, 0);
	task->image->colorSwap = task->colorSwap ? false : true; // dxt_decompress() swaps BGR->RGB
}

/*
==========================================================================================

  SWIZZLED FORMATS

==========================================================================================
*/

// Color premultiplied by alpha (DXT2/DXT3)
void Swizzle_Premult(LoadedImage *image, bool decode)
{
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		while(data < end)
		{
			float mod = 255.0f / (float)data[3];
			data[0] = (byte)min(255, max(0, (data[0] * mod)));
			data[1] = (byte)min(255, max(0, (data[1] * mod)));
			data[2] = (byte)min(255, max(0, (data[2] * mod)));
			data += image->bpp;
		}
		return;
	}
	// encode
	while(data < end)
	{
		float mod = (float)data[3] / 255.0f;
		data[0] = (byte)(data[0] * mod);
		data[1] = (byte)(data[1] * mod);
		data[2] = (byte)(data[2] * mod);
		data += image->bpp;
	}
}

// Doom 3's normalmap trick (store R channel in DXT5 alpha)
void Swizzle_XGBR(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_XGBR: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	// decode
	if (decode)
	{
		while(data < end)
		{
			data[0] = data[3];
			data[3] = 0;
			data += image->bpp;
		}
		Image_ConvertBPP(image, 3);
		return;
	}
	// encode
	while(data < end)
	{
		data[3] = data[2];
		data[2] = 0;
		data += image->bpp;
	}
}

// Doom 3's normalmap trick (store R channel in DXT5 alpha) with alpha stored in R (it seems it is only suitable for binaryalpha)
void Swizzle_AGBR(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_AGBR: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	// decode
	if (decode)
	{
		while(data < end)
		{
			byte saved = data[0];
			data[0] = data[3];
			data[3] = saved;
			data += image->bpp;
		}
		return;
	}
	// encode
	while(data < end)
	{
		byte saved = data[2];
		data[2] = data[3];
		data[3] = saved;
		data += image->bpp;
	}
}

// IdTech5 Chrominance/Luminance swizzle (can store low quality alpha)
void Swizzle_YCoCg(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		while(data < end)
		{
			byte T;
			float Y, Co, Cg, R, G, B;
			const float offset = 0.5f * 256.0f / 255.0f;
			Y  = (float)data[3]/255.0f;
			Co = (float)data[0]/255.0f - offset;
			Cg = (float)data[1]/255.0f - offset;
			R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
			G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
			B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
			data[3] = data[2]; // restore alpha from blue
			data[0] = (byte)(R * 255.0f);
			data[1] = (byte)(G * 255.0f);
			data[2] = (byte)(B * 255.0f);
			if (image->colorSwap == true)
			{
				T = data[2];
				data[2] = data[0];
				data[0] = T;
			}
			data += image->bpp;
		}
		return;
	}
	// encode
	byte Y, Co, Cg;
	while(data < end)
	{
		Y  = ((data[2] + (data[1] << 1) + data[0]) + 2) >> 2;
		Co = ((((data[2] << 1) - (data[0] << 1)) + 2) >> 2) + 128;
		Cg = (((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2) + 128;
		data[0] = data[3];
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		if (image->colorSwap == false)
		{
			Y = data[2];
			data[2] = data[0];
			data[0] = Y;
		}
		data += image->bpp;
	}
}

// YCoCg Unscaled Gamma 2.0
void Swizzle_YCoCg_Gamma2(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		while(data < end)
		{
			byte T;
			float Y, Co, Cg, R, G, B;
			const float offset = 0.5f * 256.0f / 255.0f;
			Y  = (float)data[3]/255.0f;
			Co = (float)data[0]/255.0f - offset;
			Cg = (float)data[1]/255.0f - offset;
			R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
			G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
			B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
			R = R * R;
			G = G * G;
			B = B * B;
			data[3] = data[2]; // restore alpha from blue
			data[0] = (byte)(R * 255.0f);
			data[1] = (byte)(G * 255.0f);
			data[2] = (byte)(B * 255.0f);
			if (image->colorSwap == true)
			{
				T = data[2];
				data[2] = data[0];
				data[0] = T;
			}
			data += image->bpp;

		}
		return;
	}
	// encode
	byte Y, Co, Cg, R, G, B;
	while(data < end)
	{
		R  = (byte)floor(sqrt((float)data[0] / 255.0f) * 255.0f + 0.5f);
		G  = (byte)floor(sqrt((float)data[1] / 255.0f) * 255.0f + 0.5f);
		B  = (byte)floor(sqrt((float)data[2] / 255.0f) * 255.0f + 0.5f);
		Y  = ((B + (G << 1) + R) + 2) >> 2;
		Co = ((((B << 1) - (R << 1)) + 2) >> 2) + 128;
		Cg = (((-B + (G << 1) - R) + 2) >> 2) + 128;
		data[0] = data[3];
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		if (image->colorSwap == false)
		{
			Y = data[2];
			data[2] = data[0];
			data[0] = Y;
		}
		data += image->bpp;
	}
}

// IdTech5 Chrominance/Luminance swizzle - color scaled variant (doesnt store alpha at all)
void Swizzle_YCoCgScaled(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		byte T;
		float Y, Co, Cg, R, G, B, s;
		const float offset = 0.5f * 256.0f / 255.0f;
		while(data < end)
		{
			Y  = (float)data[3]/255.0f;
			Co = (float)data[0]/255.0f;
			Cg = (float)data[1]/255.0f;
			s  = (float)data[2]/255.0f;
			s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
			Co = (Co - offset) * s;
			Cg = (Cg - offset) * s;
			R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
			G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
			B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
			data[0] = (byte)(R * 255.0f);
			data[1] = (byte)(G * 255.0f);
			data[2] = (byte)(B * 255.0f);
			data[3] = 255; // set alpha to 1
			if (image->colorSwap == true)
			{
				T = data[2];
				data[2] = data[0];
				data[0] = T;
			}
			data += image->bpp;
		}
		return;
	}
	// encode YCoCg (scale will be applied during compression)
	byte Y, Co, Cg;
	while(data < end)
	{
		Y  = ((data[2] + (data[1] << 1) + data[0]) + 2) >> 2;
		Co = ((((data[2] << 1) - (data[0] << 1)) + 2) >> 2) + 128;
		Cg = (((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2) + 128;
		data[0] = 0;
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		if (image->colorSwap == false)
		{
			Y = data[2];
			data[2] = data[0];
			data[0] = Y;
		}
		data += image->bpp;
	}
}

// YCoCg Scaled Gamma 2.0
void Swizzle_YCoCgScaled_Gamma2(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		byte T;
		float Y, Co, Cg, R, G, B, s;
		const float offset = 0.5f * 256.0f / 255.0f;
		while(data < end)
		{
			Y  = (float)data[3]/255.0f;
			Co = (float)data[0]/255.0f;
			Cg = (float)data[1]/255.0f;
			s  = (float)data[2]/255.0f;
			s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
			Co = (Co - offset) * s;
			Cg = (Cg - offset) * s;
			R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
			G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
			B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
			R = R * R;
			G = G * G;
			B = B * B;
			data[0] = (byte)(R * 255.0f);
			data[1] = (byte)(G * 255.0f);
			data[2] = (byte)(B * 255.0f);
			data[3] = 255; // set alpha to 1
			if (image->colorSwap == true)
			{
				T = data[2];
				data[2] = data[0];
				data[0] = T;
			}
			data += image->bpp;
		}
		return;
	}
	// encode YCoCg (scale will be applied during compression)
	byte Y, Co, Cg, R, G, B;
	while(data < end)
	{
		R  = (byte)floor(sqrt((float)data[0] / 255.0f) * 255.0f + 0.5f);
		G  = (byte)floor(sqrt((float)data[1] / 255.0f) * 255.0f + 0.5f);
		B  = (byte)floor(sqrt((float)data[2] / 255.0f) * 255.0f + 0.5f);
		Y  = ((B + (G << 1) + R) + 2) >> 2;
		Co = ((((B << 1) - (R << 1)) + 2) >> 2) + 128;
		Cg = (((-B + (G << 1) - R) + 2) >> 2) + 128;
		data[0] = 0;
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		if (image->colorSwap == false)
		{
			Y = data[2];
			data[2] = data[0];
			data[0] = Y;
		}
		data += image->bpp;
	}
}

#if 0

// IdTech5 Chrominance/Luminance swizzle - color scaled variant (doesnt store alpha at all)
void Swizzle_YCoCgScaled(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		// Convert from 2.0 gamma to linear
		float Y, Co, Cg, R, G, B, s;
		const float offset = 0.5f * 256.0f / 255.0f;
		while(data < end)
		{
			Y  = (float)data[3]/255.0f;
			Co = (float)data[0]/255.0f;
			Cg = (float)data[1]/255.0f;
			s  = (float)data[2]/255.0f;
			s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
			printf("DecodedScale: %f %i\n", s, data[2]);
			Co = (Co - offset) * s;
			Cg = (Cg - offset) * s;
			R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
			G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
			B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
			data[0] = (byte)(R * 255.0f);
			data[1] = (byte)(G * 255.0f);
			data[2] = (byte)(B * 255.0f);
			data[3] = 255; // set alpha to 1
			data += image->bpp;
		}
		return;
	}
	// get 
	byte Y, Co, Cg;
	int wblocks = (int)ceil((float)image->width / 4.0f);
	int hblocks = (int)ceil((float)image->height / 4.0f);
	float Co1, Cg1, extent, scale;
	int hb, wb, bw, bh, x, y;
	byte scaleQuantized, *pitch, *src;
	for (hb = 0; hb < hblocks; hb++)
	{
		for (wb = 0; wb < wblocks; wb++)
		{
			// get max extent
			x = wb * 4;
			y = hb * 4;
			bw = min(4, image->width - x);
			bh = min(4, image->height - y);
			extent = 0;
			for (int j = 0; j < bh; ++j)
			{
				for (int i = 0; i < bw; ++i)
				{
					Co1 = (float)((((data[2] << 1) - (data[0] << 1)) + 2) >> 2);
					Cg1 = (float)(((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2);
					extent = max( extent, max( fabs(Co1), fabs(Cg1) ) );
				}
			}
			scale = extent / 128.f;
			printf("scale: %f %f\n", scale, extent);

			//
			#if 0
			0   = 1
			8   = 0,5
			16  = 0,33333333333333333333333333333333
			24  = 0.25
			32  = 0.2
			64  = 0,11111111111111111111111111111111
			128 = 0,05882352941176470588235294117647
			255 = 0,03041825095057034220532319391635
			s  = (float)data[2]/255.0f;
			s = 1.0f / (31,875 * s + 1.0f);
			#endif

			//
			// quantize
			scaleQuantized = (byte)(floor((1.0f / scale) - 0.5f) * 8);
			scale = 1.0f / ((255.0f / 8.0f) * ((float)scaleQuantized/255.0f) + 1.0f);
			printf("EncodedScale: %f %i\n", scale, scaleQuantized);
			// encode
			for (int j = 0; j < bh; ++j)
			{
				pitch = data + (y+j)*image->width*image->bpp;
				for (int i = 0; i < bw; ++i)
				{
					src = pitch + (x + i)*image->bpp;
					Y  = ((src[2] + (src[1] << 1) + src[0]) + 2) >> 2;
					Co = (byte)min(-128, max(((((src[2] << 1) - (src[0] << 1)) + 2) >> 2) / scale, 127)) + 128;
					Cg = (byte)min(-128, max((((-src[2] + (src[1] << 1) - src[0]) + 2) >> 2) / scale, 127)) + 128;
					src[0] = scaleQuantized;
					src[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
					src[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
					src[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
				}
			}
		}
	}
	/*
	// encode
	while(data < end)
	{
		Y  = ((data[2] + (data[1] << 1) + data[0]) + 2) >> 2;
		Co = ((((data[2] << 1) - (data[0] << 1)) + 2) >> 2) + 128;
		Cg = (((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2) + 128;
		data[0] = 0;
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		data += image->bpp;
	}
	*/
}

// convert YCoCg Scaled block
// picked from: http://codedeposit.blogspot.ru/2010/01/pre-linearized-wide-gamut-dxt5-ycocg.html
void YCoCgScaled_ConvertBlock(const byte inPixels[16*3], byte outPixels[16*4])
{
    // Calculate Co and Cg extents
    int extents = 0;
    int n = 0;
    int iY, iCo, iCg;
    int blockCo[16];
    int blockCg[16];
    const byte *px = inPixels;
    for(int i=0;i<16;i++)
    {
        iCo = (px[0]<<1) - (px[2]<<1);
        iCg = (px[1]<<1) - px[0] - px[2];
        if(-iCo > extents) extents = -iCo;
        if( iCo > extents) extents = iCo;
        if(-iCg > extents) extents = -iCg;
        if( iCg > extents) extents = iCg;

        blockCo[n] = iCo;
        blockCg[n++] = iCg;

        px += 3;
    }

    // Co = -510..510
    // Cg = -510..510
    float scaleFactor = 1.0f;
    if (extents > 127)
        scaleFactor = (float)extents * 4.0f / 510.0f;

    // Convert to quantized scalefactor
    unsigned char scaleFactorQuantized = (unsigned char)(ceil((scaleFactor - 1.0f) * 31.0f / 3.0f));
    // Unquantize
    scaleFactor = 1.0f + (float)(scaleFactorQuantized / 31.0f) * 3.0f;
    unsigned char bVal = (unsigned char)((scaleFactorQuantized << 3) | (scaleFactorQuantized >> 2));
    unsigned char *outPx = outPixels;
    n = 0;
    px = inPixels;
    for(int i=0;i<16;i++)
    {
        // Calculate components
        iY = ( px[0] + (px[1]<<1) + px[2] + 2 ) / 4;
        iCo = (int)((blockCo[n] / scaleFactor) + 128);
        iCg = (int)((blockCg[n] / scaleFactor) + 128);

        if(iCo < 0) iCo = 0; else if(iCo > 255) iCo = 255;
        if(iCg < 0) iCg = 0; else if(iCg > 255) iCg = 255;
        if(iY < 0) iY = 0; else if(iY > 255) iY = 255;

        px += 3;

        outPx[0] = (unsigned char)iCo;
        outPx[1] = (unsigned char)iCg;
        outPx[2] = bVal;
        outPx[3] = (unsigned char)iY;
        outPx += 4;
    }
}

// http://codedeposit.blogspot.ru/2010/01/pre-linearized-wide-gamut-dxt5-ycocg.html
// IdTech5 Chrominance/Luminance swizzle - color scaled variant (doesnt store alpha at all)
void Swizzle_YCoCgScaled_WWW(LoadedImage *image, bool decode)
{
	if (!decode)
		Image_ConvertBPP(image, 4);
	else if (image->bpp != 4)
		Error("Swizzle_YCoCg: image have no alpha channel!\n");
	byte *data = FreeImage_GetBits(image->bitmap);
	byte *end = data + image->width*image->height*image->bpp;
	if (decode)
	{
		// Convert from 2.0 gamma to linear
		while(data < end)
		{
			float base[3], scale, mult[4], result[3];

			base[0] = data[3] / 255.0f;
			base[1] = (data[0] - 127) / 255.0f;
			base[2] = (data[1] - 127) / 255.0f;
			scale = (data[2] / 255.0f)*0.75f + 0.25f;
			mult[0] = 1.0f;
			mult[1] = 0.0;
			mult[2] = scale;
			mult[3] = -scale;
			result[0] = ((base[0]*mult[0] + base[1]*mult[2] + base[2]*mult[3]) * 255.0f);
			result[1] = ((base[0]*mult[0] + base[1]*mult[1] + base[2]*mult[2]) * 255.0f);
			result[2] = ((base[0]*mult[0] + base[1]*mult[3] + base[2]*mult[3]) * 255.0f);
			data[0] = (byte)result[0];
			data[1] = (byte)result[1];
			data[2] = (byte)result[2];
			data[3] = 255;
			data += image->bpp;
		}
		/*
		float Y, Co, Cg, R, G, B, s;
		const float offset = 0.5f * 256.0f / 255.0f;
		while(data < end)
		{
			Y  = (float)data[3]/255.0f;
			Co = (float)data[0]/255.0f;
			Cg = (float)data[1]/255.0f;
			s  = (float)data[2]/255.0f;
			s = 1.0f / ((255.0f / 8.0f) * s + 1.0f);
			Co = (Co - offset) * s;
			Cg = (Cg - offset) * s;
			R = Y + Co - Cg; R = (R < 0) ? 0 : (R > 1) ? 1 : R;
			G = Y + Cg;      G = (G < 0) ? 0 : (G > 1) ? 1 : G;
			B = Y - Co - Cg; B = (B < 0) ? 0 : (B > 1) ? 1 : B;
			data[0] = (byte)(R * 255.0f);
			data[1] = (byte)(G * 255.0f);
			data[2] = (byte)(B * 255.0f);
			data[3] = 255; // set alpha to 1
			data += image->bpp;
		}
		*/
		return;
	}
	// encode
	static const int map[] = { 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 0, 1, 2, 3 };
	byte blockIn[16*3], blockOut[16*4];
	int wblocks = (int)ceil((float)image->width / 4.0f);
	int hblocks = (int)ceil((float)image->height / 4.0f);
	int wb, hb, x, y, bw, bh;
	for (hb = 0; hb < hblocks; hb++)
	{
		for (wb = 0; wb < wblocks; wb++)
		{
			// extract block
			x = wb * 4;
			y = hb * 4;
			bw = min(4, image->width - x);
			bh = min(4, image->height - y);
			for (int j = 0; j < 4; ++j)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (i >= bw || j >= bh)
					{
						// todo: get nearest color
						blockIn[j*3*4 + i*3 + 0] = 0;
						blockIn[j*3*4 + i*3 + 1] = 0;
						blockIn[j*3*4 + i*3 + 2] = 0;
						continue;
					}
					blockIn[j*3*4 + i*3 + 0] = data[(y+j)*image->width*4 + (x + i)*4 + 2];
					blockIn[j*3*4 + i*3 + 1] = data[(y+j)*image->width*4 + (x + i)*4 + 1];
					blockIn[j*3*4 + i*3 + 2] = data[(y+j)*image->width*4 + (x + i)*4 + 0];
				}
			}
			// swizzle
			YCoCgScaled_ConvertBlock( blockIn, blockOut );
			// put back
			for (int j = 0; j < 4; ++j)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (i >= bw || j >= bh)
						continue;
					data[(y+j)*image->width*4 + (x + i)*4 + 0] = blockOut[j*4*4 + i*4 + 0];
					data[(y+j)*image->width*4 + (x + i)*4 + 1] = blockOut[j*4*4 + i*4 + 1];
					data[(y+j)*image->width*4 + (x + i)*4 + 2] = blockOut[j*4*4 + i*4 + 2];
					data[(y+j)*image->width*4 + (x + i)*4 + 3] = blockOut[j*4*4 + i*4 + 3];
				}
			}
		}
	}
	/*
	byte Y, Co, Cg;
	while(data < end)
	{
		Y  = ((data[2] + (data[1] << 1) + data[0]) + 2) >> 2;
		Co = ((((data[2] << 1) - (data[0] << 1)) + 2) >> 2) + 128;
		Cg = (((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2) + 128;
		data[0] = 255;
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		data += image->bpp;
	}
	*/
}

#endif