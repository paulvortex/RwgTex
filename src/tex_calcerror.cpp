////////////////////////////////////////////////////////////////
//
// RwgTex / calc compressino error
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "freeimage.h"

/*
==========================================================================================

Error metric

==========================================================================================
*/

#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)

// RGB -> YUV
#define RGB2Y(R, G, B) CLIP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define RGB2U(R, G, B) CLIP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLIP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)

// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )
#define YUV2R(Y, U, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)

// RGB -> YCbCr
#define CRGB2Y(R, G, B) CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16)
#define CRGB2Cb(R, G, B) CLIP((36962 * (B - CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16) ) >> 16) + 128)
#define CRGB2Cr(R, G, B) CLIP((46727 * (R - CLIP((19595 * R + 38470 * G + 7471 * B ) >> 16) ) >> 16) + 128)

// YCbCr -> RGB
#define CYCbCr2R(Y, Cb, Cr) CLIP( Y + ( 91881 * Cr >> 16 ) - 179 )
#define CYCbCr2G(Y, Cb, Cr) CLIP( Y - (( 22544 * Cb + 46793 * Cr ) >> 16) + 135)
#define CYCbCr2B(Y, Cb, Cr) CLIP( Y + (116129 * Cb >> 16 ) - 226 )

void rgb_to_hsb(int r, int g, int b, double *hsbvals)
{
	double hue, saturation, brightness;

	int cmax = (r > g) ? r : g;
	if (b > cmax) cmax = b;
	int cmin = (r < g) ? r : g;
	if (b < cmin) cmin = b;
	brightness = ((double)cmax) / 255.0f;
	if (cmax != 0)
		saturation = ((double)(cmax - cmin)) / ((double)cmax);
	else
		saturation = 0;
	if (saturation == 0)
		hue = 0;
	else
	{
		double redc =   ((double)(cmax - r)) / ((double)(cmax - cmin));
		double greenc = ((double)(cmax - g)) / ((double)(cmax - cmin));
		double bluec =  ((double)(cmax - b)) / ((double)(cmax - cmin));
		if (r == cmax)
			hue = bluec - greenc;
		else if (g == cmax)
			hue = 2.0f + redc - bluec;
		else
			hue = 4.0f + greenc - redc;
		hue = hue / 6.0f;
		if (hue < 0)
			hue = hue + 1.0f;
	}
	hsbvals[0] = hue;
	hsbvals[1] = saturation;
	hsbvals[2] = brightness;
}

void rgb_to_yuv(int r, int g, int b, double *yuv)
{
	yuv[0] = (double)RGB2Y(r, g, b);
	yuv[1] = (double)RGB2U(r, g, b);
	yuv[2] = (double)RGB2V(r, g, b);
}

TexCalcErrors *AllocErrorCalc()
{
	TexCalcErrors *calc;

	calc = (TexCalcErrors *)mem_alloc(sizeof(TexCalcErrors));
	memset(calc, 0, sizeof(TexCalcErrors));

	return calc;
}

void FreeErrorCalc(TexCalcErrors *calc, bool keepBitmap)
{
	if (calc->image != NULL)
	{
		if (keepBitmap)
			calc->image->bitmap = NULL;
		Image_Delete(calc->image);
	}
	mem_free(calc);
}

// TexCalcErrors
// calculate compression error and store in image
TexCalcErrors *TexCompressionError(TexFormat *format, LoadedImage *compressed, LoadedImage *original, TexErrorMetric metric, bool generateImage)
{
	size_t uncsize, cmpsize, dstsize;
	byte *dst, *dst_data, *unc, *unc_data, *cmp, *cmp_data, *end;
	byte ur, ug, ub, cr, cg, cb;
	int dstpitch, uncpitch, cmppitch, y;
	LoadedImage *destination = NULL;
	TexCalcErrors *calc = AllocErrorCalc();
	int localmetric;

	// get uncompressed data
	unc_data = Image_GetData(original, &uncsize, &uncpitch);
	cmp_data = Image_GetData(compressed, &cmpsize, &cmppitch);
	if (compressed->width != original->width || compressed->height != original->height)
		Error("TexCompressionError: Compressed mismatched original dimensions (width: %i != %i, height: %i != %i)", compressed->width, original->width, compressed->height, original->height);
	if (generateImage)
	{
		destination = Image_Create();
		Image_Generate(destination, original->width, original->height, original->bpp);
		dst_data = Image_GetData(destination, &dstsize, &dstpitch);
	}

	// auto metric
	if (metric == ERRORMETRIC_AUTO)
	{
		if (original->datatype == IMAGE_NORMALMAP)
			metric = ERRORMETRIC_LINEAR;
		else
			metric = ERRORMETRIC_PERCEPTURAL;
	}

	// select local metric
	localmetric = 000;
	if (metric == ERRORMETRIC_LINEAR)
	{
		// compare at linear level
		if (original->sRGB)
		{
			if (compressed->sRGB)
				localmetric = 011; // linear, convert both to linear
			else
				localmetric = 010; // linear, convert first to linear
		}
		else
		{
			if (compressed->sRGB)
				localmetric = 001; // linear, convert second to linear
			else
				localmetric = 000; // linear, no convert
		}
	}
	else if (metric == ERRORMETRIC_PERCEPTURAL)
	{
		// compare at linear level
		if (original->sRGB)
		{
			if (compressed->sRGB)
				localmetric = 100; // perceptural, do nothing
			else
				localmetric = 102; // perceptural, convert second to sRGB
		}
		else
		{
			if (compressed->sRGB)
				localmetric = 120; // perceptural, convert first to sRGB
			else
				localmetric = 122; // perceptural, convert both to sRGB
		}
	}

	// swapped color?
	if (compressed->colorSwap)
	{
		cr = ur = 2;
		cg = ug = 1;
		cb = ub = 0;
		if (compressed->colorSwap != original->colorSwap)
		{
			ur = 0;
			ug = 1;
			ub = 2;
		}
	}
	else
	{
		cr = ur = 0;
		cg = ug = 1;
		cb = ub = 2;
		if (compressed->colorSwap != original->colorSwap)
		{
			ur = 2;
			ug = 1;
			ub = 0;
		}
	}

	// calculate errors
	int err_len = 3;
	int err_pitch = compressed->width * err_len;
	double *errors = (double *)mem_alloc(compressed->width * compressed->height * err_len * sizeof(double));
	double *err, *err_end;
	double all_err[3], cmpf[3], uncf[3]; all_err[0] = all_err[1] = all_err[2] = 0;
	for (y = 0; y < compressed->height; y++)
	{
		unc = unc_data;
		err = errors + y * compressed->width * err_len;
		for (cmp = cmp_data, end = cmp + cmppitch, unc = unc_data; cmp < end; cmp += compressed->bpp, unc += original->bpp, err += err_len)
		{
			cmpf[0] = (double)cmp[cr] / 255.0f;
			cmpf[1] = (double)cmp[cg] / 255.0f;
			cmpf[2] = (double)cmp[cb] / 255.0f;
			uncf[0] = (double)unc[ur] / 255.0f;
			uncf[1] = (double)unc[ug] / 255.0f;
			uncf[2] = (double)unc[ub] / 255.0f;
			// local metrics are used to set the comparison metric:
			// in linear mode textures should be compared as sRGB values (so dark areas errors will be same weight as bright ones)
			// in perceptural mode textures should be compared as linear values (dark ares will have lesser weight)
			switch (localmetric)
			{
				case 000: // linear, no convert
					err[0] = cmpf[0] - uncf[0];
					err[1] = cmpf[1] - uncf[1];
					err[2] = cmpf[2] - uncf[2];
					break;
				case 001: // linear, convert second to linear
					err[0] = cmpf[0] - linear_to_srgb(uncf[0]);
					err[1] = cmpf[1] - linear_to_srgb(uncf[1]);
					err[2] = cmpf[2] - linear_to_srgb(uncf[2]);
					break;
				case 010: // linear, convert first to linear
					err[0] = linear_to_srgb(cmpf[0]) - uncf[0];
					err[1] = linear_to_srgb(cmpf[1]) - uncf[1];
					err[2] = linear_to_srgb(cmpf[2]) - uncf[2];
					break;
				case 011: // linear, convert both to linear
					err[0] = linear_to_srgb(cmpf[0]) - linear_to_srgb(uncf[0]);
					err[1] = linear_to_srgb(cmpf[1]) - linear_to_srgb(uncf[1]);
					err[2] = linear_to_srgb(cmpf[2]) - linear_to_srgb(uncf[2]);
					break;
				case 100: // perceptural, no convert
					err[0] = cmpf[0] - uncf[0];
					err[1] = cmpf[1] - uncf[1];
					err[2] = cmpf[2] - uncf[2];
					break;
				case 102: // perceptural, convert second to sRGB
					err[0] = cmpf[0] - srgb_to_linear(uncf[0]);// * 0.29890f * 3.0f;
					err[1] = cmpf[1] - srgb_to_linear(uncf[1]);// * 0.58700f * 3.0f;
					err[2] = cmpf[2] - srgb_to_linear(uncf[2]);// * 0.11400f * 3.0f;
					break;
				case 120: // perceptural, convert first to sRGB
					err[0] = srgb_to_linear(cmpf[0]) - uncf[0];
					err[1] = srgb_to_linear(cmpf[1]) - uncf[1];
					err[2] = srgb_to_linear(cmpf[2]) - uncf[2];
					break;
				case 122: // perceptural, convert both to sRGB
					err[0] = srgb_to_linear(cmpf[0]) - srgb_to_linear(uncf[0]);
					err[1] = srgb_to_linear(cmpf[1]) - srgb_to_linear(uncf[1]);
					err[2] = srgb_to_linear(cmpf[2]) - srgb_to_linear(uncf[2]);
					break;
				default:
					Error("TexCompressionError: unknown metric %i\n", localmetric);
					break;
			}
			if (compressed->hasAlpha && (format->features & FF_PUNCH_THROUGH_ALPHA))
			{
				// punch-through alpha does break color layer in opaque pixels, so don't count errors there
				err[0] = err[0] * (cmp[3] / 255.0f);
				err[1] = err[1] * (cmp[3] / 255.0f);
				err[2] = err[2] * (cmp[3] / 255.0f);
			}
			all_err[0] += pow(err[0], 2) * 1.0f;
			all_err[1] += pow(err[1], 2) * 1.0f;
			all_err[2] += pow(err[2], 2) * 1.0f;
		}
		cmp_data += cmppitch;
		unc_data += uncpitch;
	}

	// calc average
	all_err[0] = all_err[0] / (compressed->width * compressed->height);
	all_err[1] = all_err[1] / (compressed->width * compressed->height);
	all_err[2] = all_err[2] / (compressed->width * compressed->height);

	// calc dispersion
	double disp[3], all_disp[3]; all_disp[0] = all_disp[1] = all_disp[2] = 0;
	for (y = 0; y < compressed->height; y++)
	{
		for (err = errors + y * err_pitch, err_end = err + err_pitch; err < err_end; err += err_len)
		{
			disp[0] = err[0] - all_err[0];
			disp[1] = err[1] - all_err[1];
			disp[2] = err[2] - all_err[2];
			all_disp[0] += disp[0] * disp[0];
			all_disp[1] += disp[1] * disp[1];
			all_disp[2] += disp[2] * disp[2];
		}
	}

	// calc dispersion and rms
	all_disp[0] = all_disp[0] / (compressed->width * compressed->height);
	all_disp[1] = all_disp[1] / (compressed->width * compressed->height);
	all_disp[2] = all_disp[2] / (compressed->width * compressed->height);

	// calc root mean square
	double all_rms[3];
	all_rms[0] = sqrt(all_disp[0]);
	all_rms[1] = sqrt(all_disp[1]);
	all_rms[2] = sqrt(all_disp[2]);

	// store averaged
	calc->average = (fabs(all_err[0]) + fabs(all_err[1]) + fabs(all_err[2])) * 255.0f;
	calc->dispersion = (all_disp[0] + all_disp[1] + all_disp[2]) * 255.0f;
	calc->rms = (all_rms[0] + all_rms[1] + all_rms[2]) * 255.0f;

	// generate image
	double total[3];
	if (generateImage)
	{
		for (y = 0; y < compressed->height; y++)
		{
			for (dst = dst_data, end = dst + destination->width * destination->bpp, err = errors + y * err_pitch; dst < end; dst += destination->bpp, err += err_len)
			{
				total[0] = 128.0f + fabs(err[0] - all_err[0]) * 3.0f * 255.0f;
				total[1] = 128.0f + fabs(err[1] - all_err[1]) * 3.0f * 255.0f;
				total[2] = 128.0f + fabs(err[2] - all_err[2]) * 3.0f * 255.0f;
				dst[cr] = (byte)CLIP(total[0]);
				dst[cg] = (byte)CLIP(total[1]);
				dst[cb] = (byte)CLIP(total[2]);
				if (destination->bpp == 4)
					dst[3] = 255;
			}
			dst_data += dstpitch;
		}
	}

	mem_free(errors);

	calc->image = destination;

	if (tex_testCompresion)
	{
		Print("\nTotal for %s:\n", OptionEnumName(metric, tex_error_metrics));
		Print("average error: %9.2f\n", calc->average);
		Print("dispersion   : %9.2f\n", calc->dispersion);
		Print("rms          : %9.2f\n", calc->rms);
	}

	return calc;
}

TexCalcErrors *TexCompressionError(char *filename, TexEncodeTask *encodetask, TexErrorMetric metric)
{
	TexDecodeTask task = { 0 };
	TexCalcErrors *calc = NULL;

	if (encodetask->container != NULL)
	{
		DecodeFromEncode(&task, encodetask, filename);
		// decompress
		if (!task.container->fReadHeader(&task))
		{
			task.container->fPrintHeader(task.data);
			Error("TexCompressionError(%s): %s\n", task.filename, task.errorMessage);
		}
		DecompressImage(&task); // only use first level, ignore mipmaps
		Image_LoadFinish(task.image);
		// calc error
		calc = TexCompressionError(task.format, task.image, encodetask->image, metric, false);
		// cleanup
		if (task.image != NULL)
			Image_Delete(task.image);
	}	
	return calc;
}