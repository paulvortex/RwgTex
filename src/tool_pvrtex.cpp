////////////////////////////////////////////////////////////////
//
// RwgTex / Imagination PowerVR SDK TexTool support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"
#include "pvrtextool/pvrtextool_lib.h"

#pragma comment(lib, "PVRTexLib.lib")

TexTool TOOL_PVRTEX =
{
	"PVRTex", "PowerVR SDK PVRTexTool", "pvrtex",
	TEXINPUT_RGB | TEXINPUT_RGBA | TEXINPUT_BGR | TEXINPUT_BGRA,
	&PVRTex_Init,
	&PVRTex_Option,
	&PVRTex_Load,
	&PVRTex_Compress,
	&PVRTex_Version,
};

// tool option
pvrtexture::ECompressorQuality pvrtex_quality_prvtc[NUM_PROFILES];
pvrtexture::ECompressorQuality pvrtex_quality_etc[NUM_PROFILES];
bool        pvrtex_dithering;
OptionList  pvrtex_compressionOptionPVRTC[] = 
{ 
	{ "normal", pvrtexture::ePVRTCNormal }, 
	{ "high", pvrtexture::ePVRTCHigh }, 
	{ "best", pvrtexture::ePVRTCBest },
	{ 0 }
};
OptionList  pvrtex_compressionOptionETC[] = 
{ 
	{ "normal", pvrtexture::eETCFastPerceptual }, 
	{ "high", pvrtexture::eETCFastPerceptual }, 
	{ "best", pvrtexture::eETCSlowPerceptual }, 
	{ 0 }
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void PVRTex_Init(void)
{
	RegisterFormat(&F_ETC1, &TOOL_PVRTEX);
	RegisterFormat(&F_ETC2, &TOOL_PVRTEX);
	RegisterFormat(&F_ETC2A, &TOOL_PVRTEX);
	RegisterFormat(&F_ETC2A1, &TOOL_PVRTEX);
	RegisterFormat(&F_EAC1, &TOOL_PVRTEX);
	RegisterFormat(&F_EAC2, &TOOL_PVRTEX);
	RegisterFormat(&F_PVRTC_2BPP_RGB, &TOOL_PVRTEX);
	RegisterFormat(&F_PVRTC_2BPP_RGBA, &TOOL_PVRTEX);
	RegisterFormat(&F_PVRTC_4BPP_RGB, &TOOL_PVRTEX);
	RegisterFormat(&F_PVRTC_4BPP_RGBA, &TOOL_PVRTEX);
	RegisterFormat(&F_PVRTC2_2BPP, &TOOL_PVRTEX);
	RegisterFormat(&F_PVRTC2_4BPP, &TOOL_PVRTEX);
	RegisterFormat(&F_DXT1, &TOOL_PVRTEX);
	RegisterFormat(&F_DXT1A, &TOOL_PVRTEX);
	RegisterFormat(&F_DXT2, &TOOL_PVRTEX);
	RegisterFormat(&F_DXT3, &TOOL_PVRTEX);
	RegisterFormat(&F_DXT4, &TOOL_PVRTEX);
	RegisterFormat(&F_DXT5, &TOOL_PVRTEX);

	// options
	pvrtex_quality_prvtc[PROFILE_FAST] = pvrtexture::ePVRTCNormal;
	pvrtex_quality_prvtc[PROFILE_REGULAR] = pvrtexture::ePVRTCHigh;
	pvrtex_quality_prvtc[PROFILE_BEST] = pvrtexture::ePVRTCBest;
	pvrtex_quality_etc[PROFILE_FAST] = pvrtexture::eETCFastPerceptual;
	pvrtex_quality_etc[PROFILE_REGULAR] = pvrtexture::eETCFastPerceptual;
	pvrtex_quality_etc[PROFILE_BEST] = pvrtexture::eETCSlowPerceptual;
	pvrtex_dithering = false;
}

void PVRTex_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
		{
			pvrtex_quality_prvtc[PROFILE_FAST] = (pvrtexture::ECompressorQuality)OptionEnum(val, pvrtex_compressionOptionPVRTC, pvrtex_quality_prvtc[PROFILE_FAST], TOOL_PVRTEX.name);
			pvrtex_quality_etc[PROFILE_FAST] = (pvrtexture::ECompressorQuality)OptionEnum(val, pvrtex_compressionOptionETC, pvrtex_quality_etc[PROFILE_FAST], TOOL_PVRTEX.name);
		}
		else if (!stricmp(key, "regular"))
		{
			pvrtex_quality_prvtc[PROFILE_REGULAR] = (pvrtexture::ECompressorQuality)OptionEnum(val, pvrtex_compressionOptionPVRTC, pvrtex_quality_prvtc[PROFILE_REGULAR], TOOL_PVRTEX.name);
			pvrtex_quality_etc[PROFILE_REGULAR] = (pvrtexture::ECompressorQuality)OptionEnum(val, pvrtex_compressionOptionETC, pvrtex_quality_etc[PROFILE_REGULAR], TOOL_PVRTEX.name);
		}
		else if (!stricmp(key, "best"))
		{
			pvrtex_quality_prvtc[PROFILE_BEST] = (pvrtexture::ECompressorQuality)OptionEnum(val, pvrtex_compressionOptionPVRTC, pvrtex_quality_prvtc[PROFILE_BEST], TOOL_PVRTEX.name);
			pvrtex_quality_etc[PROFILE_BEST] = (pvrtexture::ECompressorQuality)OptionEnum(val, pvrtex_compressionOptionETC, pvrtex_quality_etc[PROFILE_BEST], TOOL_PVRTEX.name);
		}
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "dither"))
			pvrtex_dithering = OptionBoolean(val) ? 1 : 0;
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void PVRTex_Load(void)
{
	if (CheckParm("-dither"))
		pvrtex_dithering = true;
	// note options
	if (pvrtex_dithering)
		Print("%s tool: enabled dithering\n", TOOL_PVRTEX.name);
}

const char *PVRTex_Version(void)
{
	return PVRTLSTRINGVERSION;
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

pvrtexture::PixelType *pvrtex_pixeltype_etc1            = new pvrtexture::PixelType(ePVRTPF_ETC1);
pvrtexture::PixelType *pvrtex_pixeltype_etc2rgb         = new pvrtexture::PixelType(ePVRTPF_ETC2_RGB);
pvrtexture::PixelType *pvrtex_pixeltype_etc2rgba        = new pvrtexture::PixelType(ePVRTPF_ETC2_RGBA);
pvrtexture::PixelType *pvrtex_pixeltype_etc2rgba1       = new pvrtexture::PixelType(ePVRTPF_ETC2_RGB_A1);
pvrtexture::PixelType *pvrtex_pixeltype_eac1            = new pvrtexture::PixelType(ePVRTPF_EAC_R11);
pvrtexture::PixelType *pvrtex_pixeltype_eac2            = new pvrtexture::PixelType(ePVRTPF_EAC_RG11);
pvrtexture::PixelType *pvrtex_pixeltype_pvrtc_2bpp_rgba = new pvrtexture::PixelType(ePVRTPF_PVRTCI_2bpp_RGBA);
pvrtexture::PixelType *pvrtex_pixeltype_pvrtc_2bpp_rgb  = new pvrtexture::PixelType(ePVRTPF_PVRTCI_2bpp_RGB);
pvrtexture::PixelType *pvrtex_pixeltype_pvrtc_4bpp_rgba = new pvrtexture::PixelType(ePVRTPF_PVRTCI_4bpp_RGBA);
pvrtexture::PixelType *pvrtex_pixeltype_pvrtc_4bpp_rgb  = new pvrtexture::PixelType(ePVRTPF_PVRTCI_4bpp_RGB);
pvrtexture::PixelType *pvrtex_pixeltype_pvrtc2_2bpp     = new pvrtexture::PixelType(ePVRTPF_PVRTCII_2bpp);
pvrtexture::PixelType *pvrtex_pixeltype_pvrtc2_4bpp     = new pvrtexture::PixelType(ePVRTPF_PVRTCII_4bpp);
pvrtexture::PixelType *pvrtex_pixeltype_dxt1            = new pvrtexture::PixelType(ePVRTPF_DXT1);
pvrtexture::PixelType *pvrtex_pixeltype_dxt3            = new pvrtexture::PixelType(ePVRTPF_DXT3);
pvrtexture::PixelType *pvrtex_pixeltype_dxt5            = new pvrtexture::PixelType(ePVRTPF_DXT5);
pvrtexture::PixelType *pvrtex_pixeltype_bgra            = new pvrtexture::PixelType('b','g','r','a',8,8,8,8);
pvrtexture::PixelType *pvrtex_pixeltype_bgr             = new pvrtexture::PixelType('b','g','r',0,8,8,8,0);
pvrtexture::PixelType *pvrtex_pixeltype_rgba            = new pvrtexture::PixelType('r','g','b','a',8,8,8,8);
pvrtexture::PixelType *pvrtex_pixeltype_rgb             = new pvrtexture::PixelType('r','g','b',0,8,8,8,0);

// compress texture
size_t PVRTex_CompressSingleImage(byte *stream, TexEncodeTask *t, int imagewidth, int imageheight, byte *imagedata)
{
	pvrtexture::CPVRTextureHeader header;
	pvrtexture::ECompressorQuality quality;
	pvrtexture::PixelType *pixeltype;
	size_t outsize;

	// prepare
	header.setWidth(imagewidth);
	header.setHeight(imageheight);
	header.setNumMIPLevels(1);
	if (t->image->colorSwap)
	{
		if (t->image->bpp == 4)
			header.setPixelFormat(*pvrtex_pixeltype_bgra);
		else
			header.setPixelFormat(*pvrtex_pixeltype_bgr);
	}
	else
	{
		if (t->image->bpp == 4)
			header.setPixelFormat(*pvrtex_pixeltype_rgba);
		else
			header.setPixelFormat(*pvrtex_pixeltype_rgb);
	}
	if (t->format->block == &B_ETC1)
	{
		quality = pvrtex_quality_etc[tex_profile];
		pixeltype = pvrtex_pixeltype_etc1;
	}
	else if (t->format->block == &B_ETC2)
	{
		quality = pvrtex_quality_etc[tex_profile];
		pixeltype = pvrtex_pixeltype_etc2rgb;
	}
	else if (t->format->block == &B_ETC2A)
	{
		quality = pvrtex_quality_etc[tex_profile];
		pixeltype = pvrtex_pixeltype_etc2rgba;
	}
	else if (t->format->block == &B_ETC2A1)
	{
		quality = pvrtex_quality_etc[tex_profile];
		pixeltype = pvrtex_pixeltype_etc2rgba1;
	}
	else if (t->format->block == &B_EAC1)
	{
		quality = pvrtex_quality_etc[tex_profile];
		pixeltype = pvrtex_pixeltype_eac1;
	}
	else if (t->format->block == &B_EAC2)
	{
		quality = pvrtex_quality_etc[tex_profile];
		pixeltype = pvrtex_pixeltype_eac2;
	}
	else if (t->format->block == &B_PVRTC_2BPP_RGB || t->format->block == &B_PVRTC_2BPP_RGBA)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		if (t->image->hasAlpha)
			pixeltype = pvrtex_pixeltype_pvrtc_2bpp_rgba;
		else
			pixeltype = pvrtex_pixeltype_pvrtc_2bpp_rgb;
	}
	else if (t->format->block == &B_PVRTC_4BPP_RGB || t->format->block == &B_PVRTC_4BPP_RGBA)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		if (t->image->hasAlpha)
			pixeltype = pvrtex_pixeltype_pvrtc_4bpp_rgba;
		else
			pixeltype = pvrtex_pixeltype_pvrtc_4bpp_rgb;
	}
	else if (t->format->block == &B_PVRTC2_2BPP)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		pixeltype = pvrtex_pixeltype_pvrtc2_2bpp;
	}
	else if (t->format->block == &B_PVRTC2_4BPP)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		pixeltype = pvrtex_pixeltype_pvrtc2_4bpp;
	}
	else if (t->format->block == &B_DXT1)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		pixeltype = pvrtex_pixeltype_dxt1;
	}
	else if (t->format->block == &B_DXT2 || t->format->block == &B_DXT3)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		pixeltype = pvrtex_pixeltype_dxt3;
	}
	else if (t->format->block == &B_DXT4 || t->format->block == &B_DXT5)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		pixeltype = pvrtex_pixeltype_dxt5;
	}
	else if (t->format->block == &B_ETC1)
	{
		quality = pvrtex_quality_prvtc[tex_profile];
		pixeltype = pvrtex_pixeltype_etc1;
	}
	else
	{
		Warning("%s : %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name, TOOL_PVRTEX.name);
		return false;
	}

	// compress
	pvrtexture::CPVRTexture texture = pvrtexture::CPVRTexture(header, imagedata);
	Transcode(texture, *pixeltype, texture.getChannelType(), texture.getColourSpace(), quality, pvrtex_dithering);
	outsize = texture.getDataSize(PVRTEX_ALLMIPLEVELS, false, false);
	memcpy(stream, texture.getDataPtr(0, 0, 0), outsize); 

 	return outsize;
}

bool PVRTex_Compress(TexEncodeTask *t)
{
	size_t output_size;

	// compress
	byte *stream = t->stream;
	for (ImageMap *map = t->image->maps; map; map = map->next)
	{
		output_size = PVRTex_CompressSingleImage(stream, t, map->width, map->height, map->data);
		if (output_size)
			stream += output_size;
	}
	return true;
}