////////////////////////////////////////////////////////////////
//
// RwgTex / Etc2Comp compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_TOOL_ETC2COMP_C
#include "main.h"
#include "tex.h"
#include "etc2comp/etc2comp_lib.h"

TexTool TOOL_ETC2COMP =
{
	"Etc2Comp", "Google Etc2Comp compressor", "etc2comp",
	TEXINPUT_RGBA,
	&Etc2Comp_Init,
	&Etc2Comp_Option,
	&Etc2Comp_Load,
	&Etc2Comp_Compress,
	&Etc2Comp_Version,
};

// tool options
float etc2comp_effortLevel[NUM_PROFILES];

/*
==========================================================================================

  Init

==========================================================================================
*/

void Etc2Comp_Init(void)
{
	// ETC1-based
	RegisterFormat(&F_ETC1, &TOOL_ETC2COMP);

	// ETC2-based
	RegisterFormat(&F_ETC2, &TOOL_ETC2COMP);
	RegisterFormat(&F_ETC2A, &TOOL_ETC2COMP);
	RegisterFormat(&F_ETC2A1, &TOOL_ETC2COMP);
	RegisterFormat(&F_EAC1, &TOOL_ETC2COMP);
	RegisterFormat(&F_EAC2, &TOOL_ETC2COMP);

	// options
	etc2comp_effortLevel[PROFILE_FAST] = 10;
	etc2comp_effortLevel[PROFILE_REGULAR] = 40;
	etc2comp_effortLevel[PROFILE_BEST] = 60;
}

void Etc2Comp_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "profiles"))
	{
		if (!stricmp(key, "fast"))
			etc2comp_effortLevel[PROFILE_FAST] = (float)OptionInt(val);
		else if (!stricmp(key, "regular"))
			etc2comp_effortLevel[PROFILE_REGULAR] = (float)OptionInt(val);
		else if (!stricmp(key, "best"))
			etc2comp_effortLevel[PROFILE_BEST] = (float)OptionInt(val);
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void Etc2Comp_Load(void)
{
}

const char *Etc2Comp_Version(void)
{
	return ETC2COMP_GITVERSION;
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

typedef struct compressOptions_t
{
	Etc::Image::Format format;
	Etc::ErrorMetric errorMetric;
	float effortLevel;
	int minThreads;
	int maxThreads;
} compressOptions_t;

// compress texture
size_t Etc2Comp_CompressSingleImage(byte *stream, TexEncodeTask *t, int imagewidth, int imageheight, byte *imagedata, Etc::ColorFloatRGBA *floatPixels, compressOptions_t *options)
{
	// convert byte RGBA to float RGBA
	Etc::ColorFloatRGBA *floatPixel = floatPixels;
	for (byte *dataend = &imagedata[imagewidth * imageheight * 4]; imagedata < dataend; imagedata += 4)
		*floatPixel++ = Etc::ColorFloatRGBA::ConvertFromRGBA8(imagedata[0], imagedata[1], imagedata[2], imagedata[3]);

	// encode ETC2
	Etc::Image image((float *)floatPixels, imagewidth, imageheight, options->errorMetric);
	image.m_bVerboseOutput = false;
	Etc::Image::EncodingStatus status = image.Encode(options->format, options->errorMetric, options->effortLevel, options->minThreads, options->maxThreads);
	if (status >= Etc::Image::EncodingStatus::ERROR_THRESHOLD)
	{
		Error("Etc2Comp: %s%s.dds - compression failed with status %s", t->file->path.c_str(), t->file->name.c_str(), Etc::ImageStatusName(status));
		return 0;
	}
	//if (status >= Etc::Image::EncodingStatus::WARNING_THRESHOLD)
	//	Warning("Etc2Comp: %s%s.dds - %s", t->file->path.c_str(), t->file->name.c_str(), Etc::ImageStatusName(status));

	// write to stream
	size_t encodedBytes = image.GetEncodingBitsBytes();
	memcpy(stream, image.GetEncodingBits(), encodedBytes);

	return encodedBytes;
}

bool Etc2Comp_Compress(TexEncodeTask *t)
{
	size_t output_size;
	compressOptions_t options;

	// set options
	options.minThreads = 1;
	options.maxThreads = 1;
	options.effortLevel = etc2comp_effortLevel[tex_profile];
	if (t->format->block == &B_ETC1)
		options.format = Etc::Image::Format::ETC1;
	else if (t->format->block == &B_ETC2)
		options.format = t->image->sRGB ? Etc::Image::Format::SRGB8 : Etc::Image::Format::RGB8;
	else if (t->format->block == &B_ETC2A)
		options.format = t->image->sRGB ? Etc::Image::Format::SRGBA8 : Etc::Image::Format::RGBA8;
	else if (t->format->block == &B_ETC2A1)
		options.format = t->image->sRGB ? Etc::Image::Format::SRGB8A1 : Etc::Image::Format::RGB8A1;
	else if (t->format->block == &B_EAC1)
		options.format = Etc::Image::Format::R11;
	else if (t->format->block == &B_EAC2)
		options.format = Etc::Image::Format::RG11;
	else
	{
		Warning("Etc2Comp: %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
		return 0;
	}
	if (t->image->datatype == IMAGE_NORMALMAP)
		options.errorMetric = Etc::ErrorMetric::RGBA;
	else
		options.errorMetric = Etc::ErrorMetric::REC709;

	// allocate biggest map to be used for compressing
	size_t biggestMapSize = 0;
	for (ImageMap *map = t->image->maps; map; map = map->next)
		biggestMapSize = max(biggestMapSize, (size_t)(map->width * map->height));
	Etc::ColorFloatRGBA *floatPixels = new Etc::ColorFloatRGBA[biggestMapSize];

	// compress sequentally
	byte *stream = t->stream;
	for (ImageMap *map = t->image->maps; map; map = map->next)
	{
		output_size = Etc2Comp_CompressSingleImage(stream, t, map->width, map->height, map->data, floatPixels, &options);
		if (output_size)
			stream += output_size;
	}

	// finish
	delete floatPixels;
	return true;
}