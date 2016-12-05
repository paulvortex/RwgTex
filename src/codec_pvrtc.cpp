////////////////////////////////////////////////////////////////
//
// RwgTex / PVRTC texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "tex.h"
#include "pvrtextool/pvrtextool_lib.h"

TexBlock  B_PVRTC_2BPP_RGB   = { FOURCC('P','T','C','1'), "PTC1", 1, 1, 32 };
TexBlock  B_PVRTC_2BPP_RGBA  = { FOURCC('P','T','C','2'), "PTC2", 1, 1, 32 };
TexBlock  B_PVRTC_4BPP_RGB   = { FOURCC('P','T','C','3'), "PTC3", 1, 1, 64 };
TexBlock  B_PVRTC_4BPP_RGBA  = { FOURCC('P','T','C','4'), "PTC4", 1, 1, 64 };

TexFormat F_PVRTC_2BPP_RGB   = { FOURCC('P','T','C','1'), "PVRTC/2BPP/RGB",  "PVRTC 2 bits-per-pixel RGB",  "pvr2",  &B_PVRTC_2BPP_RGB,  &CODEC_PVRTC, GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,  GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT,       GL_RGB,  0, FF_SQUARE | FF_SRGB };
TexFormat F_PVRTC_2BPP_RGBA  = { FOURCC('P','T','C','2'), "PVRTC/2BPP/RGBA", "PVRTC 2 bits-per-pixel RGBA", "pvr2a", &B_PVRTC_2BPP_RGBA, &CODEC_PVRTC, GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT, GL_RGBA, 0, FF_SQUARE | FF_ALPHA | FF_SRGB };
TexFormat F_PVRTC_4BPP_RGB   = { FOURCC('P','T','C','3'), "PVRTC/4BPP/RGB",  "PVRTC 4 bits-per-pixel RGB",  "pvr4",  &B_PVRTC_4BPP_RGB,  &CODEC_PVRTC, GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,  GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT,       GL_RGB,  0, FF_SQUARE | FF_SRGB };
TexFormat F_PVRTC_4BPP_RGBA  = { FOURCC('P','T','C','4'), "PVRTC/4BPP/RGBA", "PVRTC 4 bits-per-pixel RGBA", "pvr4a", &B_PVRTC_4BPP_RGBA, &CODEC_PVRTC, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT, GL_RGBA, 0, FF_SQUARE | FF_ALPHA | FF_SRGB };

TexCodec  CODEC_PVRTC = 
{
	"PVRTC", "PowerVR Texture Compression", "pvrtc",
	&CodecPVRTC_Init,
	&CodecPVRTC_Option,
	&CodecPVRTC_Load,
	&CodecPVRTC_Accept,
	&CodecPVRTC_Encode,
	&CodecPVRTC_Decode,
};

void CodecPVRTC_Init(void)
{
	RegisterTool(&TOOL_PVRTEX, &CODEC_PVRTC);
}

void CodecPVRTC_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void CodecPVRTC_Load(void)
{
}

/*
==========================================================================================

  ENCODING

==========================================================================================
*/

bool CodecPVRTC_Accept(TexEncodeTask *task)
{
	// PVRTC only encodes square textures
	// so we detect textures that are too long and discard them, 
	// because non-square textures are upscaled to square size (this make compression useless)
	float f = (float)task->image->width / (float)task->image->height;
	if (f < 0.5f || f > 2.0f)
		return false;
	return true;
}

void CodecPVRTC_Encode(TexEncodeTask *task)
{
	// default format is 4BPP
	if (!task->format)
	{
		if (task->image->hasAlpha)
			task->format = &F_PVRTC_4BPP_RGBA;
		else
			task->format = &F_PVRTC_4BPP_RGB;
	}

	// force alpha or non-alpha format
	if (task->image->hasAlpha)
	{
		if (task->format == &F_PVRTC_2BPP_RGB)
			task->format = &F_PVRTC_2BPP_RGBA;
		else if (task->format == &F_PVRTC_4BPP_RGB)
			task->format = &F_PVRTC_4BPP_RGBA;
	}
	else
	{
		if (task->format == &F_PVRTC_2BPP_RGBA)
			task->format = &F_PVRTC_2BPP_RGB;
		else if (task->format == &F_PVRTC_4BPP_RGBA)
			task->format = &F_PVRTC_4BPP_RGB;
	}

	// default tool
	if (!task->tool)
		task->tool = &TOOL_PVRTEX;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

void CodecPVRTC_Decode(TexDecodeTask *task)
{
	bool do2bit;

	// decoding to RGBA
	int havebpp = task->image->bpp;
	if (havebpp != 4)
		Image_ConvertBPP(task->image, 4);
	if (task->format->block->bitlength == 32)
		do2bit = true;
	else if (task->format->block->bitlength == 64)
		do2bit = false;
	else
		Error("CodecPVRTC_Decode: wrong format '%s' block '%s' bitlength %i\n", task->format->name, task->format->block->name, task->format->block->bitlength);
	
	// decompress
	// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
	byte *data = Image_GetData(task->image, NULL, NULL);
	pvr::PVRTDecompressPVRTC(task->pixeldata, do2bit, task->image->width, task->image->height, data);
	if (havebpp != task->image->bpp)
		Image_ConvertBPP(task->image, havebpp);
}