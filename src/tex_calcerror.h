// tex_calcerror.h
#ifndef H_TEX_CALCERROR_H
#define H_TEX_CALCERROR_H

#include "tex.h"

// available error metrics
typedef enum
{
	ERRORMETRIC_AUTO,
	ERRORMETRIC_LINEAR,
	ERRORMETRIC_PERCEPTURAL,
	NUM_ERRORMETRICS,
}TexErrorMetric;

#ifdef F_TEX_C
OptionList tex_error_metrics[] =
{
	{ "auto",        ERRORMETRIC_AUTO },
	{ "linear",      ERRORMETRIC_LINEAR },
	{ "perceptural", ERRORMETRIC_PERCEPTURAL },
};
#else
extern OptionList tex_error_metrics[];
#endif

// calc errors task
typedef struct TexCalcErrors_s
{
	double average;
	double dispersion;
	double rms;

	LoadedImage *image;
} TexCalcErrors;

// util
TexCalcErrors *AllocErrorCalc();
void FreeErrorCalc(TexCalcErrors *calc, bool keepBitmap = false);

// generic
TexCalcErrors *TexCompressionError(TexFormat *format, LoadedImage *compressed, LoadedImage *original, TexErrorMetric metric, bool generateImage);
TexCalcErrors *TexCompressionError(char *filename, TexEncodeTask *encodetask, TexErrorMetric metric);

#endif