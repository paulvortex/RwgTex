////////////////////////////////////////////////////////////////
//
// RwgTex / DXT texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

TexBlock  B_DXT1  = { "DXT1", 4, 4, 64  };
TexBlock  B_DXT3  = { "DXT3", 4, 4, 128 };
TexBlock  B_DXT5  = { "DXT5", 4, 4, 128 };

TexFormat F_DXT1  = { FOURCC('D','X','T','1'), "DXT1", "DirectX Texture Compression 1",  "dxt1", &B_DXT1, &CODEC_DXT, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,  GL_RGB };
TexFormat F_DXT1A = { FOURCC('D','X','T','1'), "DXT1A","DirectX Texture Compression 1A", "dxt1a",&B_DXT1, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, 0, FF_ALPHA | FF_BINARYALPHA };
TexFormat F_DXT2  = { FOURCC('D','X','T','2'), "DXT2", "DirectX Texture Compression 2",  "dxt2", &B_DXT3, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_Premult };
TexFormat F_DXT3  = { FOURCC('D','X','T','3'), "DXT3", "DirectX Texture Compression 3",  "dxt3", &B_DXT3, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, 0, FF_ALPHA };
TexFormat F_DXT4  = { FOURCC('D','X','T','4'), "DXT4", "DirectX Texture Compression 4",  "dxt4", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_Premult };
TexFormat F_DXT5  = { FOURCC('D','X','T','5'), "DXT5", "DirectX Texture Compression 5",  "dxt5", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA };
TexFormat F_RXGB  = { FOURCC('R','X','G','B'), "RXGB", "Doom 3 RXGB (swizzled DXT5)",    "rxgb", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_AGBR };
TexFormat F_YCG1  = { FOURCC('Y','C','G','1'), "YCG1", "YCoCg Unscaled",                 "ycg1", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_YCoCg };
TexFormat F_YCG2  = { FOURCC('Y','C','G','2'), "YCG2", "YCoCg Scaled",                   "ycg2", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_YCoCgScaled };
TexFormat F_YCG3  = { FOURCC('Y','C','G','3'), "YCG3", "YCoCg Unscaled Gamma 2.0",       "ycg3", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, FF_ALPHA, Swizzle_YCoCg_Gamma2 };
TexFormat F_YCG4  = { FOURCC('Y','C','G','4'), "YCG4", "YCoCg Scaled Gamma 2.0",         "ycg4", &B_DXT5, &CODEC_DXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 0, 0, Swizzle_YCoCgScaled_Gamma2 };

TexCodec  CODEC_DXT = 
{
	"DXT", "DirectX Texture Compression", "dxt",
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

	// specific formats only supported by a few compressors
	if (task->format == &F_YCG1 || task->format == &F_YCG2 || task->format == &F_YCG3 || task->format == &F_YCG4)
		task->tool = &TOOL_GIMPDDS;
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
	else if (task->format->block == &B_DXT3)
		dxtformat = DDS_COMPRESS_BC2;
	else if (task->format->block == &B_DXT5)
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
			data += image->bpp;
		}
		Image_ConvertBPP(image, 3);
		return;
	}
	// encode
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
			data += image->bpp;
		}
		Image_ConvertBPP(image, 3);
		return;
	}
	// encode
	byte Y, Co, Cg;
	while(data < end)
	{
		Y  = ((data[2] + (data[1] << 1) + data[0]) + 2) >> 2;
		Co = ((((data[2] << 1) - (data[0] << 1)) + 2) >> 2) + 128;
		Cg = (((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2) + 128;
		Y = (byte)floor(sqrt((float)Y/255.0f) + 0.5f);        
		Co = (byte)floor(sqrt((float)Co/255.0f) + 0.5f);    
		Cg = (byte)floor(sqrt((float)Cg/255.0f) + 0.5f); 
		data[0] = 255;
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
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
		return;
	}
	// encode
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
			data += image->bpp;
		}
		return;
	}
	// encode
	byte Y, Co, Cg, R, G, B;
	while(data < end)
	{
		R  = (byte)floor(sqrt((float)data[0] / 255.0f) * 255.0f);
		G  = (byte)floor(sqrt((float)data[1] / 255.0f) * 255.0f);
		B  = (byte)floor(sqrt((float)data[2] / 255.0f) * 255.0f);
		Y  = ((B + (G << 1) + R) + 2) >> 2;
		Co = ((((B << 1) - (R << 1)) + 2) >> 2) + 128;
		Cg = (((-B + (G << 1) - R) + 2) >> 2) + 128;
		data[0] = 255;
		data[1] = (Cg > 255 ? 255 : (Cg < 0 ? 0 : Cg));
		data[2] = (Co > 255 ? 255 : (Co < 0 ? 0 : Co));
		data[3] = (Y  > 255 ? 255 : (Y  < 0 ? 0 :  Y));
		data += image->bpp;
	}
}