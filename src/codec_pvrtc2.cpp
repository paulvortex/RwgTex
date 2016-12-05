////////////////////////////////////////////////////////////////
//
// RwgTex / PVRTC texture codec
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "tex.h"
#include "pvrtextool/pvrtextool_lib.h"

TexBlock  B_PVRTC2_2BPP  = { FOURCC('P','T','C','5'), "PTC5", 1, 1, 32 };
TexBlock  B_PVRTC2_4BPP  = { FOURCC('P','T','C','6'), "PTC6", 1, 1, 64 };

TexFormat F_PVRTC2_2BPP  = { FOURCC('P','T','C','5'), "PVRTC2/2BPP", "PVRTC2 2 bits-per-pixel RGBA", "pvrii2", &B_PVRTC2_2BPP, &CODEC_PVRTC2, GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG, GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG, GL_RGBA, 0, FF_ALPHA | FF_SRGB };
TexFormat F_PVRTC2_4BPP  = { FOURCC('P','T','C','6'), "PVRTC2/4BPP", "PVRTC2 4 bits-per-pixel RGBA", "pvrii4", &B_PVRTC2_4BPP, &CODEC_PVRTC2, GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG, GL_RGBA, 0, FF_ALPHA | FF_SRGB };

TexCodec  CODEC_PVRTC2 = 
{
	"PVRTC2", "PowerVR Texture Compression 2", "pvrtc2",
	&CodecPVRTC2_Init,
	&CodecPVRTC2_Option,
	&CodecPVRTC2_Load,
	&CodecPVRTC2_Accept,
	&CodecPVRTC2_Encode,
	&CodecPVRTC2_Decode,
};

void CodecPVRTC2_Init(void)
{
	RegisterTool(&TOOL_PVRTEX, &CODEC_PVRTC2);
}

void CodecPVRTC2_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void CodecPVRTC2_Load(void)
{
}

/*
==========================================================================================

  ENCODING

==========================================================================================
*/

bool CodecPVRTC2_Accept(TexEncodeTask *task)
{
	// PVRTC2 accepts all textures
	return true;
}

void CodecPVRTC2_Encode(TexEncodeTask *task)
{
	// default format is 4BPP
	if (!task->format)
		task->format = &F_PVRTC2_4BPP;

	// default tool
	if (!task->tool)
		task->tool = &TOOL_PVRTEX;
}

/*
==========================================================================================

  DECODING

==========================================================================================
*/

void CodecPVRTC2_Decode(TexDecodeTask *task)
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