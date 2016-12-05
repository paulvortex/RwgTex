////////////////////////////////////////////////////////////////
//
// RwgTex / Ericsson's ETCPack support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"
#include "etcpack/etcpack_lib.h"

TexTool TOOL_ETCPACK =
{
	"ETCPack", "Ericsson ETCPack", "etcpack",
	TEXINPUT_RGB | TEXINPUT_RGBA,
	&ETCPack_Init,
	&ETCPack_Option,
	&ETCPack_Load,
	&ETCPack_Compress,
	&ETCPack_Version,
};

// tool options
int        etcpack_speed[NUM_PROFILES];
bool       etcpack_linearcolormetric = false;
bool       etcpack_onlylinearmode = false;
bool       etcpack_onlyplanarmode = false;
bool       etcpack_onlytmode = false;
bool       etcpack_onlyhmode = false;
OptionList etcpack_speedOption[] = 
{ 
	{ "normal", 0 }, 
	{ "exhaustive", 1 }, 
	{ 0 }
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void ETCPack_Init(void)
{
	// ETC1-based
	RegisterFormat(&F_ETC1, &TOOL_ETCPACK);
	// ETC2-based
	RegisterFormat(&F_ETC2, &TOOL_ETCPACK);
	RegisterFormat(&F_ETC2A, &TOOL_ETCPACK);
	RegisterFormat(&F_ETC2A1, &TOOL_ETCPACK);
	RegisterFormat(&F_EAC1, &TOOL_ETCPACK);
	RegisterFormat(&F_EAC2, &TOOL_ETCPACK);
	// options
	etcpack_speed[PROFILE_FAST] = 0;
	etcpack_speed[PROFILE_REGULAR] = 1;
	etcpack_speed[PROFILE_BEST] = 1;
	// ETCPack startup
	etcpack_readCompressParams();
	etcpack_setupAlphaTableAndValtab();
}

void ETCPack_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			etcpack_speed[PROFILE_FAST] = OptionEnum(val, etcpack_speedOption, etcpack_speed[PROFILE_FAST], TOOL_ETCPACK.name);
		else if (!stricmp(key, "regular"))
			etcpack_speed[PROFILE_REGULAR] = OptionEnum(val, etcpack_speedOption, etcpack_speed[PROFILE_REGULAR], TOOL_ETCPACK.name);
		else if (!stricmp(key, "best"))
			etcpack_speed[PROFILE_BEST] = OptionEnum(val, etcpack_speedOption, etcpack_speed[PROFILE_BEST], TOOL_ETCPACK.name);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void ETCPack_Load(void)
{
	etcpack_onlyplanarmode = CheckParm("-etc2onlyplanarmode");
	etcpack_onlylinearmode = CheckParm("-etc2onlylinearmode");
	etcpack_onlytmode = CheckParm("-etc2onlytmode");
	etcpack_onlyhmode  = CheckParm("-etc2onlyhmode");
}


const char *ETCPack_Version(void)
{
	return ETCPACK_VERSION_S;
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

void ETCPack_Prepare(byte *imagedata, int sw, int sh, int imagebpp, byte **prescaled, byte **alpha, int *w, int *h, byte **decoded, int *resized, bool binaryalpha)
{
	byte *tempimg, *tempimg2, *in, *out, *end, *alpha1, *alpha2;
	int x, y, dw, dh;

	dw = (int)(ceil((float)(sw)/4.0f)*4);
	dh = (int)(ceil((float)(sh)/4.0f)*4);

	// get alpha
	if (alpha != NULL)
	{
		// get
		alpha1 = (byte *)mem_alloc(sw * sh);
		if (imagebpp == 4)
		{
			in = imagedata;
			end = in + sh*sw*imagebpp;
			out = alpha1;
			while(in < end)
			{
				*out++ = in[3];
				in += 4;
			}
		}
		else
			memset(alpha1, 255, sw * sh);
		// resize to match div/4
		if (dw != sw || dh != sh)
		{
			alpha2 = (byte *)mem_alloc(dw * dh);
			for (y = 0; y < sh; y++)
			{
				memcpy(alpha2 + dw*y, alpha1 + sw*y, sw);
				for(x = sw; x < dw; x++)
					memcpy(alpha2 + (dw*y + x), alpha1 + (sw*y - 1), 1);
			}
			for(y = sh; y < dh; y++)
				memcpy(alpha2 + y*dw, alpha2 + (sh-1)*dw, dw);
			mem_free(alpha1);
			alpha1 = alpha2;
		}
		// binary alpha
		if (binaryalpha)
		{
			byte *in = alpha1;
			byte *end = alpha1 + dw * dh;
			while(in < end)
				*in++ = (in[0] < tex_binaryAlphaCenter) ? 0 : 255;
		}
		// set
		*alpha = alpha1;
	}

	// get RGB
	// convert
	if (imagebpp != 3)
	{
		tempimg = (byte *)mem_alloc(sw * sh * 3);
		in = imagedata;
		end = in + sh*sw*imagebpp;
		out = tempimg;
		if (imagebpp == 4)
		{
			while(in < end)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				out += 3;
				in += 4;
			}
		}
		else if (imagebpp == 3)
		{
			while(in < end)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				out[3] = 255;
				out += 4;
				in += 3;
			}
		}
		else
			Error("ETCPack_PrepareImage: bad source BPP (%i) and dest BPP 3", imagebpp);
	}
	else
		tempimg = imagedata;

	// resize to match div/4
	if (dw != sw || dh != sh)
	{
		tempimg2 = (byte *)mem_alloc(dw * dh * 3);
		for (y = 0; y < sh; y++)
		{
			memcpy(tempimg2 + dw*y*3, tempimg + sw*y*3, sw*3);
			for(x = sw; x < dw; x++)
				memcpy(tempimg2 + (dw*y + x)*3, tempimg + (sw*y + sw - 1)*3, 3);
		}
		for(y = sh; y < dh; y++)
			memcpy(tempimg2 + dw*y*3, tempimg2 + dw*(sh-1)*3, dw*3);
	}
	else
		tempimg2 = tempimg;

	// return
	*prescaled = tempimg2;
	*resized = (tempimg2 == imagedata) ? 0 : 1;
	if (tempimg != imagedata && tempimg2 != tempimg)
		mem_free(tempimg);
	*decoded = (byte *)mem_alloc(dw * dh * 3);
	*w = dw;
	*h = dh;
}

void ETCPack_Free(byte *imagedata, byte *imagealpha, byte *decoded, int resized)
{
	if (resized != 0)
		mem_free(imagedata);
	if (imagealpha)
		mem_free(imagealpha);
	mem_free(decoded);
}

void ETCPack_WriteColorBlock(byte **stream, unsigned int block1, unsigned int block2)
{
	byte *data = *stream;
	data[0] = (block1 >> 24) & 0xFF;
	data[1] = (block1 >> 16) & 0xFF;
	data[2] = (block1 >> 8) & 0xFF;
	data[3] =  block1 & 0xFF;
	data[4] = (block2 >> 24) & 0xFF;
	data[5] = (block2 >> 16) & 0xFF;
	data[6] = (block2 >> 8) & 0xFF;
	data[7] =  block2 & 0xFF;
	*stream = data + 8;
}

void ETCPack_WriteAlphaBlock(byte **stream, byte *alphadata)
{
	byte *data = *stream;
	memcpy(data, alphadata, 8);
	*stream = data + 8;
}

// compress ETC1 RGB block
void ETCPack_CompressBlockETC1(byte **stream, byte *imagedata, byte *imagealpha, byte *decoded, int w, int h, int x, int y)
{
	unsigned int block1, block2;

	if (etcpack_linearcolormetric)
	{
		if (etcpack_speed[tex_profile])
			etcpack_compressBlockETC1Exhaustive(imagedata, decoded, w, h, x, y, block1, block2);
		else
			etcpack_compressBlockDiffFlipFast(imagedata, decoded, w, h, x, y, block1, block2);
	}
	else
	{
		if (etcpack_speed[tex_profile])
			etcpack_compressBlockETC1ExhaustivePerceptual(imagedata, decoded, w, h, x, y, block1, block2);
		else
			etcpack_compressBlockDiffFlipFastPerceptual(imagedata, decoded, w, h, x, y, block1, block2);
	}
	ETCPack_WriteColorBlock(stream, block1, block2);
}

// compress ETC2 RGB block
void ETCPack_CompressBlockETC2(byte **stream, byte *imagedata, byte *imagealpha, byte *decoded, int w, int h, int x, int y)
{
	unsigned int block1, block2;

	// compress color block
	if (etcpack_linearcolormetric)
	{
		if (etcpack_speed[tex_profile])
			etcpack_compressBlockETC2Exhaustive(imagedata, decoded, w, h, x, y, block1, block2);
		else
			etcpack_compressBlockETC2Fast(imagedata, NULL, decoded, w, h, x, y, block1, block2);
	}
	else
	{
		if (etcpack_speed[tex_profile])
			etcpack_compressBlockETC2ExhaustivePerceptual(imagedata, decoded, w, h, x, y, block1, block2);
		else
			etcpack_compressBlockETC2FastPerceptual(imagedata, decoded, w, h, x, y, block1, block2);
	}
	ETCPack_WriteColorBlock(stream, block1, block2);
}

// compress ETC2 RGBA block
void ETCPack_CompressBlockETC2A(byte **stream, byte *imagedata, byte *imagealpha, byte *decoded, int w, int h, int x, int y)
{
	unsigned int block1, block2;
	byte alphadata[8];

	// compress color block
	if (etcpack_linearcolormetric)
	{
		if (etcpack_speed[tex_profile])
			etcpack_compressBlockETC2Exhaustive(imagedata, decoded, w, h, x, y, block1, block2);
		else
			etcpack_compressBlockETC2Fast(imagedata, NULL, decoded, w, h, x, y, block1, block2);
	}
	else
	{
		if (etcpack_speed[tex_profile])
			etcpack_compressBlockETC2ExhaustivePerceptual(imagedata, decoded, w, h, x, y, block1, block2);
		else
			etcpack_compressBlockETC2FastPerceptual(imagedata, decoded, w, h, x, y, block1, block2);
	}

	// compress and write EAC block
	if (etcpack_speed[tex_profile])
		etcpack_compressBlockAlphaSlow(imagealpha, x, y, w, h, alphadata);
	else
		etcpack_compressBlockAlphaFast(imagealpha, x, y, w, h, alphadata);
	ETCPack_WriteAlphaBlock(stream, alphadata);
	// compress and write ETC2 block
	ETCPack_WriteColorBlock(stream, block1, block2);
}

// compress ETC2 RGBA1 block
void ETCPack_CompressBlockETC2A1(byte **stream, byte *imagedata, byte *imagealpha, byte *decoded, int w, int h, int x, int y)
{
	unsigned int block1, block2;

	// compress color block
	// vortex: for ETC2A1 etcpack provides only non-perceptural fast compression
	etcpack_compressBlockETC2RGBA1(imagedata, imagealpha, decoded, w, h, x, y, block1, block2);
	ETCPack_WriteColorBlock(stream, block1, block2);
}

size_t ETCPack_CompressSingleImage(byte *stream, TexEncodeTask *t, int imagewidth, int imageheight, byte *imagedata, void (*compressBlockFunction)(byte **stream, byte *imagedata, byte *imagealpha, byte *decoded, int w, int h, int x, int y))
{
	byte *data, *dec, *src, *src_alpha, *block_src, *block_src_alpha, *block_dec;
	int resized, x, y, w, h;
	size_t out = 0;
	
	resized = 0;
	data = stream;
	ETCPack_Prepare(imagedata, imagewidth, imageheight, t->image->bpp, &src, &src_alpha, &w, &h, &dec, &resized, (compressBlockFunction == ETCPack_CompressBlockETC2A1) ? true : false);
	block_src = src;
	block_src_alpha = src_alpha;
	block_dec = dec;
	for (y = 0; y < h / 4; y++)
		for (x = 0; x < w / 4; x++)
			compressBlockFunction(&data, block_src, block_src_alpha, block_dec, w, h, x*4, y*4);
	ETCPack_Free(src, src_alpha, dec, resized);
	return data - stream;
}

bool ETCPack_Compress(TexEncodeTask *t)
{
	void (*compressBlockFunction)(byte **stream, byte *imagedata, byte *imagealpha, byte *decoded, int w, int h, int x, int y);
	size_t output_size;

	// options
	if (t->format->block == &B_ETC1)
		compressBlockFunction = ETCPack_CompressBlockETC1;
	else if (t->format->block == &B_ETC2)
		compressBlockFunction = ETCPack_CompressBlockETC2;
	else if (t->format->block == &B_ETC2A)
		compressBlockFunction = ETCPack_CompressBlockETC2A;
	else if (t->format->block == &B_ETC2A1)
		compressBlockFunction = ETCPack_CompressBlockETC2A1;
	else
	{
		Warning("ETCPack: %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
		return 0;
	}

	// compress
	byte *stream = t->stream;
	for (ImageMap *map = t->image->maps; map; map = map->next)
	{
		output_size = ETCPack_CompressSingleImage(stream, t, map->width, map->height, map->data, compressBlockFunction);
		if (output_size)
			stream += output_size;
	}
	return true;
}