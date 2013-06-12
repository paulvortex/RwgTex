////////////////////////////////////////////////////////////////
//
// RwgTex / Crunch lib support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

#pragma comment(lib, "crnlib_VC9.lib")

TexTool TOOL_CRUNCH =
{
	"CrnLib", "Crunch Library", "crunch",
	"DXT",
	TEXINPUT_RGBA,
	&Crunch_Init,
	&Crunch_Option,
	&Crunch_Load,
	&Crunch_Compress,
	&Crunch_Version,
};

// tool options
crn_dxt_quality         crnlib_speed[NUM_PROFILES];
crn_dxt_compressor_type crnlib_compressor_type;
OptionList               crnlib_speedOption[] = 
{ 
	{ "superfast", cCRNDXTQualitySuperFast }, 
	{ "fast", cCRNDXTQualityFast }, 
	{ "normal", cCRNDXTQualityNormal },
	{ "better", cCRNDXTQualityBetter },
	{ "uber", cCRNDXTQualityUber },
	{ 0 }
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void Crunch_Init(void)
{
	RegisterFormat(&F_DXT1, &TOOL_CRUNCH);
	RegisterFormat(&F_DXT1A, &TOOL_CRUNCH);
	RegisterFormat(&F_DXT2, &TOOL_CRUNCH);
	RegisterFormat(&F_DXT3, &TOOL_CRUNCH);
	RegisterFormat(&F_DXT4, &TOOL_CRUNCH);
	RegisterFormat(&F_DXT5, &TOOL_CRUNCH);
	RegisterFormat(&F_RXGB, &TOOL_CRUNCH);

	// options
	crnlib_speed[PROFILE_FAST]    = cCRNDXTQualityFast;
	crnlib_speed[PROFILE_REGULAR] = cCRNDXTQualityBetter;
	crnlib_speed[PROFILE_BEST]    = cCRNDXTQualityUber;
	crnlib_compressor_type        = cCRNDXTCompressorCRN;
}

void Crunch_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			crnlib_speed[PROFILE_FAST] = (crn_dxt_quality)OptionEnum(val, crnlib_speedOption, crnlib_speed[PROFILE_REGULAR], TOOL_CRUNCH.name);
		else if (!stricmp(key, "regular"))
			crnlib_speed[PROFILE_REGULAR] = (crn_dxt_quality)OptionEnum(val, crnlib_speedOption, crnlib_speed[PROFILE_REGULAR], TOOL_CRUNCH.name);
		else if (!stricmp(key, "best"))
			crnlib_speed[PROFILE_BEST] = (crn_dxt_quality)OptionEnum(val, crnlib_speedOption, crnlib_speed[PROFILE_REGULAR], TOOL_CRUNCH.name);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "dxt_compressor"))
		{
			if (!stricmp(val, "crn"))
				crnlib_compressor_type = cCRNDXTCompressorCRN;
			else if (!stricmp(val, "crnf"))
				crnlib_compressor_type = cCRNDXTCompressorCRNF; 
			else if (!stricmp(val, "ryg"))
				crnlib_compressor_type = cCRNDXTCompressorRYG; 
			else
				crnlib_compressor_type = cCRNDXTCompressorCRN; 
		}
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void Crunch_Load(void)
{
	// COMMANDLINEPARM: -crnf: CrnLib: select CRNF compressor
	if (CheckParm("-crnf"))    crnlib_compressor_type = cCRNDXTCompressorCRNF;
	// COMMANDLINEPARM: -ryg:  CrnLib: select RYG compressor
	if (CheckParm("-ryg"))     crnlib_compressor_type = cCRNDXTCompressorRYG;
	// note options
	if (crnlib_compressor_type != cCRNDXTCompressorCRN)
	{
		if (crnlib_compressor_type == cCRNDXTCompressorCRNF)
			Print("%s: using CRNF compressor\n", TOOL_CRUNCH.name);
		else if (crnlib_compressor_type == cCRNDXTCompressorRYG)
			Print("%s: using RYG compressor\n", TOOL_CRUNCH.name);
	}
}

const char *Crunch_Version(void)
{
	static char versionstring[200];
	sprintf(versionstring, "%i.%02i", (CRNLIB_VERSION / 100), CRNLIB_VERSION - (CRNLIB_VERSION / 100)*100);
	return versionstring;
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

size_t Crunch_CompressSingleImage(byte *stream, TexEncodeTask *t, int imagewidth, int imageheight, byte *imagedata)
{
	crn_comp_params options;
	crn_uint32 quality_level = 0;
    crn_uint32 output_size = 0;
	float actual_bitrate = 0;
	byte *output_data;

	// set options
	options.clear();
	options.m_width = imagewidth;
	options.m_height = imageheight;
	options.m_pImages[0][0] = (crn_uint32 *)imagedata;
	options.set_flag(cCRNCompFlagPerceptual, true); // crnlib non-perceptural mode looks worse
	if (t->image->datatype == IMAGE_GRAYSCALE)
		options.set_flag(cCRNCompFlagGrayscaleSampling, true);
	else
		options.set_flag(cCRNCompFlagGrayscaleSampling, false);
	options.set_flag(cCRNCompFlagUseBothBlockTypes, true);
	options.set_flag(cCRNCompFlagHierarchical, true);
	options.m_file_type = cCRNFileTypeDDS;
	options.m_quality_level = cCRNMaxQualityLevel;
	options.m_dxt1a_alpha_threshold = 128;
	options.m_dxt_quality = crnlib_speed[tex_profile];
	options.m_dxt_compressor_type = crnlib_compressor_type;
	options.m_num_helper_threads = (tex_mode == TEXMODE_DROP_FILE) ? (numthreads - 1): 0;
	if (t->format->block == &B_DXT1)
	{
		options.m_format = cCRNFmtDXT1;
		if (t->image->hasAlpha)
			options.set_flag(cCRNCompFlagDXT1AForTransparency, true);
		else
			options.set_flag(cCRNCompFlagDXT1AForTransparency, false);
	}
	else if (t->format->block == &B_DXT3)
		options.m_format = cCRNFmtDXT3;
	else if (t->format->block == &B_DXT5)
		options.m_format = cCRNFmtDXT5;
	else
	{
		Warning("CrnLib: %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
		return 0;
	}

	// compress
	output_data = (byte *)crn_compress(options, output_size, &quality_level, &actual_bitrate);
	if (output_data)
	{
		output_size -= DDS_HEADER_SIZE; // crnlib generates data with DDS header
		memcpy(stream, output_data + DDS_HEADER_SIZE, output_size);
		crn_free_block(output_data);
		return output_size;
	}
	Warning("CrnLib : %s%s.dds - compressor failed", t->file->path.c_str(), t->file->name.c_str());
	return 0;
}

bool Crunch_Compress(TexEncodeTask *t)
{
	size_t output_size;

	// compress
	byte *stream = t->stream;
	output_size = Crunch_CompressSingleImage(stream, t, t->image->width, t->image->height, Image_GetData(t->image, NULL));
	if (output_size)
	{
		// compress mipmaps
		stream += output_size;
		for (MipMap *mipmap = t->image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			output_size = Crunch_CompressSingleImage(stream, t, mipmap->width, mipmap->height, mipmap->data);
			if (output_size)
				stream += output_size;
		}
	}
	return true;
}