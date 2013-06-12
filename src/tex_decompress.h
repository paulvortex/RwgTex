// tex_decompress.h
#ifndef H_TEX_DECOMPRESS_H
#define H_TEX_DECOMPRESS_H

#include "tex.h"

// available psnr-calculation metrics
typedef enum
{
	ERRORMETRIC_AUTO,
	ERRORMETRIC_LINEAR,
	ERRORMETRIC_LUMA,
	ERRORMETRIC_CHROMA,
	ERRORMETRIC_HUE,
	ERRORMETRIC_SATURATION,
	ERRORMETRIC_PERCEPTURAL,
	NUM_ERRORMETRICS,
}TexErrorMetric;
#ifdef F_TEX_C
	OptionList tex_error_metrics[] =
	{
		{ "auto",        ERRORMETRIC_AUTO },
		{ "linear",      ERRORMETRIC_LINEAR },
		{ "luma",        ERRORMETRIC_LUMA },
		{ "chroma",      ERRORMETRIC_CHROMA },
		{ "hue",         ERRORMETRIC_HUE },
		{ "saturation",  ERRORMETRIC_SATURATION },
		{ "perceptural", ERRORMETRIC_PERCEPTURAL },
	};
#else
	extern OptionList tex_error_metrics[];
#endif

// file decode task
typedef struct TexDecodeTask_s
{
	// initialized right before shipping task to the tool
	char             *filename;
	byte             *data;
	size_t            datasize;
	TexContainer     *container;
	// initialized by container loader
	TexCodec         *codec;
	TexFormat        *format;
	int               width;
	int               height;
	int               numMipmaps;
	int               hasAlpha;
	bool              colorSwap; // true - BGR, false - RGB
	byte             *pixeldata;
	size_t            pixeldatasize;
	char             *comment;
	// initialized by exporter code
	LoadedImage      *image;
	// error message
	char              errorMessage[4000];
} TexDecodeTask;

// generic
bool  TexDecompress(char *filename);
byte *TexDecompress(char *filename, TexEncodeTask *encodetask, size_t *outdatasize);

#endif