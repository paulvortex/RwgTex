////////////////////////////////////////////////////////////////
//
// RwgTex / texture decompression engine
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

void rgb_to_hsb(int r, int g, int b, float *hsbvals)
{
	float hue, saturation, brightness;

	int cmax = (r > g) ? r : g;
	if (b > cmax) cmax = b;
	int cmin = (r < g) ? r : g;
	if (b < cmin) cmin = b;
	brightness = ((float) cmax) / 255.0f;
	if (cmax != 0)
		saturation = ((float) (cmax - cmin)) / ((float) cmax);
	else
		saturation = 0;
	if (saturation == 0)
		hue = 0;
	else
	{
		float redc = ((float) (cmax - r)) / ((float) (cmax - cmin));
		float greenc = ((float) (cmax - g)) / ((float) (cmax - cmin));
		float bluec = ((float) (cmax - b)) / ((float) (cmax - cmin));
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

void rgb_to_yuv(int r, int g, int b, float *yuv)
{
	yuv[0] = (float)RGB2Y(r, g, b);
	yuv[1] = (float)RGB2U(r, g, b);
	yuv[2] = (float)RGB2V(r, g, b);
}

void CalculateCompressionError(LoadedImage *compressed, LoadedImage *original, LoadedImage *destination, TexErrorMetric metric)
{
	size_t uncsize, cmpsize, dstsize;
	byte *dst, *dst_data, *unc, *unc_data, *cmp, *cmp_data, *end;
	byte ur, ug, ub, cr, cg, cb;
	int dstpitch, uncpitch, cmppitch, y;
	
	dst_data = Image_GetData(destination, &dstsize, &dstpitch);
	unc_data = Image_GetData(original, &uncsize, &uncpitch);
	cmp_data = Image_GetData(compressed, &cmpsize, &cmppitch);
	if (compressed->width != original->width || compressed->height != original->height)
		Error("Compressed mismatched original dimensions (width: %i != %i, height: %i != %i)", compressed->width, original->width, compressed->height, original->height); 
	if (destination->width != original->width || destination->height != original->height)
		Error("Destination image mismatched original dimensions (width: %i != %i, height: %i != %i)", destination->width, original->width, destination->height, original->height); 
	// auto metric
	if (metric == ERRORMETRIC_AUTO)
	{
		if (original->datatype == IMAGE_NORMALMAP)
			metric = ERRORMETRIC_LINEAR;
		else
			metric = ERRORMETRIC_PERCEPTURAL;
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
	// calculate 
	if (metric == ERRORMETRIC_LINEAR)
	{
		for (y = 0; y < original->height; y++)
		{
			cmp = cmp_data;
			unc = unc_data;
			dst = dst_data;
			end = cmp + compressed->width * compressed->bpp;
			while(cmp < end)
			{
				float error = fabs((float)cmp[cr] - (float)unc[ur])
							+ fabs((float)cmp[cg] - (float)unc[ug]) 
							+ fabs((float)cmp[cb] - (float)unc[ub]);
				dst[cr] = dst[cg] = dst[cb] = (byte)min(255, max(0, floor(error * 3)));
				cmp += compressed->bpp;
				unc += original->bpp;
				dst += destination->bpp;
			}
			cmp_data += cmppitch;
			unc_data += uncpitch;
			dst_data += dstpitch;
		}
		compressed->datatype = IMAGE_GRAYSCALE;
		return;
	}
	// color metric (hue)
	if (metric == ERRORMETRIC_HUE)
	{
		float chsb[3], uhsb[3];
		for (y = 0; y < original->height; y++)
		{
			cmp = cmp_data;
			unc = unc_data;
			dst = dst_data;
			end = cmp + compressed->width * compressed->bpp;
			while(cmp < end)
			{
				rgb_to_hsb(cmp[cr], cmp[cg], cmp[cb], chsb);
				rgb_to_hsb(unc[ur], unc[ug], unc[ub], uhsb);
				float error = (chsb[0] - uhsb[0]) * 10;
				if (error > 0)
					dst[cr] = (byte)min(255, max(0, floor(fabs(error))));
				else
					dst[cb] = (byte)min(255, max(0, floor(fabs(error))));
				cmp += compressed->bpp;
				unc += original->bpp;
				dst += destination->bpp;
			}
			cmp_data += cmppitch;
			unc_data += uncpitch;
			dst_data += dstpitch;
		}
		compressed->datatype = IMAGE_GRAYSCALE;
		return;
	}
	// color metric (saturation)
	if (metric == ERRORMETRIC_SATURATION)
	{
		float chsb[3], uhsb[3];
		for (y = 0; y < original->height; y++)
		{
			cmp = cmp_data;
			unc = unc_data;
			dst = dst_data;
			end = cmp + compressed->width * compressed->bpp;
			while(cmp < end)
			{
				rgb_to_hsb(cmp[cr], cmp[cg], cmp[cb], chsb);
				rgb_to_hsb(unc[ur], unc[ug], unc[ub], uhsb);
				float error = (chsb[1] - uhsb[1]) * 10;
				if (error > 0)
					dst[cr] = (byte)min(255, max(0, floor(fabs(error))));
				else
					dst[cb] = (byte)min(255, max(0, floor(fabs(error))));
				cmp += compressed->bpp;
				unc += original->bpp;
				dst += destination->bpp;
			}
			cmp_data += cmppitch;
			unc_data += uncpitch;
			dst_data += dstpitch;
		}
		compressed->datatype = IMAGE_GRAYSCALE;
		return;
	}
	// perceptural metric (luminance only)
	if (metric == ERRORMETRIC_LUMA)
	{
		float cyuv[3], uyuv[3];
		for (y = 0; y < original->height; y++)
		{
			cmp = cmp_data;
			unc = unc_data;
			dst = dst_data;
			end = cmp + compressed->width * compressed->bpp;
			while(cmp < end)
			{
				rgb_to_yuv(cmp[cr], cmp[cg], cmp[cb], cyuv);
				rgb_to_yuv(unc[ur], unc[ug], unc[ub], uyuv);
				float error = fabs(cyuv[0] - uyuv[0]) * 10;
				dst[cr] = dst[cg] = dst[cb] = (byte)min(255, max(0, floor(error)));
				cmp += compressed->bpp;
				unc += original->bpp;
				dst += destination->bpp;
			}
			cmp_data += cmppitch;
			unc_data += uncpitch;
			dst_data += dstpitch;
		}
		compressed->datatype = IMAGE_GRAYSCALE;
		return;
	}
	// perceptural metric (chrominance only)
	if (metric == ERRORMETRIC_CHROMA)
	{
		float cyuv[3], uyuv[3];
		for (y = 0; y < original->height; y++)
		{
			cmp = cmp_data;
			unc = unc_data;
			dst = dst_data;
			end = cmp + compressed->width * compressed->bpp;
			while(cmp < end)
			{
				rgb_to_yuv(cmp[cr], cmp[cg], cmp[cb], cyuv);
				rgb_to_yuv(unc[ur], unc[ug], unc[ub], uyuv);
				float error = fabs(cyuv[1] - uyuv[1]) * 10
							+ fabs(cyuv[2] - uyuv[2]) * 10;
				dst[cr] = dst[cg] = dst[cb] = (byte)min(255, max(0, floor(error)));
				cmp += compressed->bpp;
				unc += original->bpp;
				dst += destination->bpp;
			}
			cmp_data += cmppitch;
			unc_data += uncpitch;
			dst_data += dstpitch;
		}
		compressed->datatype = IMAGE_GRAYSCALE;
		return;
	}
	// perceptural metric (convert to eye space becure calculating error)
	if (metric == ERRORMETRIC_PERCEPTURAL)
	{
		float cyuv[3], uyuv[3];
		for (y = 0; y < original->height; y++)
		{
			cmp = cmp_data;
			unc = unc_data;
			dst = dst_data;
			end = cmp + compressed->width * compressed->bpp;
			while(cmp < end)
			{
				rgb_to_yuv(cmp[cr], cmp[cg], cmp[cb], cyuv);
				rgb_to_yuv(unc[ur], unc[ug], unc[ub], uyuv);
				float error = fabs(cyuv[0] - uyuv[0]) * 8
							+ fabs(cyuv[1] - uyuv[1])
							+ fabs(cyuv[2] - uyuv[2]);
				dst[cr] = dst[cg] = dst[cb] = (byte)min(255, max(0, floor(error)));
				cmp += compressed->bpp;
				unc += original->bpp;
				dst += destination->bpp;
			}
			cmp_data += cmppitch;
			unc_data += uncpitch;
			dst_data += dstpitch;
		}
		compressed->datatype = IMAGE_GRAYSCALE;
		return;
	}
	Error("CalculateCompressionError: bad metric %i\n", metric);
}

/*
==========================================================================================

  Decompression

==========================================================================================
*/

void Decompress(TexDecodeTask *task, bool exportWholeFileWithMipLevels, LoadedImage *originalImage)
{
	char outfile[MAX_FPATH], filepath[MAX_FPATH];
	int levels;

	// decompress
	if (!task->container->fReadHeader(task))
	{
		task->container->fPrintHeader(task->data);
		Error("TexDecompress(%s): %s\n", task->filename, task->errorMessage);
	}

	// COMMANDLINEPARM: -nounswizzle: disable unswizzling of zwizzled formats to allow inspection of whats really stored in file
	bool nounswizzle = CheckParm("-nounswizzle");

	// create image
	task->image = Image_Create();
	task->image->width = task->width;
	task->image->height = task->height;

	// decode base image and maps
	levels = 1 + task->numMipmaps;
	for (int level = 0; level < levels; level++)
	{
		// decompress (and unswizzle swizzled format)
		task->image->sRGB = task->ImageParms.sRGB;
		task->image->datatype = task->ImageParms.isNormalmap ? IMAGE_NORMALMAP : IMAGE_COLOR;
		task->image->bpp = compressedTextureBPP(task->image, task->format, task->container);
		task->image->bitmap = fiCreate(task->image->width, task->image->height, task->image->bpp, "Decompress");
		size_t compressedSize = compressedTextureSize(task->image, task->format, task->container, true, false);
		if (compressedSize > task->pixeldatasize)
			Error("TexDecompress(%s): image data %i is lesser than estimated data size %i\n", task->filename, task->pixeldatasize, compressedSize);
		if (task->codec->fDecode)
			task->codec->fDecode(task);
		else
			Error("TexDecompress(%s): %s codec does not support decoding of %s format\n", task->filename, task->codec->name, task->format->name);
		if (!nounswizzle)
		{
			// two ways of sRGB handling for swizzled formats
			// 1. unswizzled color, then convert sRGB -> Linear
			// 2. convert sRGB->linear then unswizzle
			if (task->format->features & FF_SWIZZLE_INTERNAL_SRGB)
			{
				// unswizzle
				if (task->format->colorSwizzle)
				{
					int pitch;
					byte *data = Image_GetData(task->image, NULL, &pitch);
					task->format->colorSwizzle(data, task->image->width, task->image->height, pitch, task->image->bpp, task->image->colorSwap, task->image->sRGB, true);
				}
				// convert to linear color
				Image_ConvertSRGB(task->image, false); 
			}
			else
			{
				// convert to linear color
				Image_ConvertSRGB(task->image, false); 
				// unswizzle
				if (task->format->colorSwizzle)
				{
					int pitch;
					byte *data = Image_GetData(task->image, NULL, &pitch);
					task->format->colorSwizzle(data, task->image->width, task->image->height, pitch, task->image->bpp, task->image->colorSwap, task->image->sRGB, true);
				}
			}
			// remove reserved alpha
			Image_ConvertBPP(task->image, (task->format->features & FF_ALPHA) ? 4 : 3);
		}
		Image_LoadFinish(task->image);

		// rescale to original size
		// todo: make work with tex_testCompresionError?
		if (originalImage && tex_testCompresion_keepSize && !exportWholeFileWithMipLevels)
		{
			if (originalImage->width != originalImage->loadedState.width || originalImage->height != originalImage->loadedState.height)
			{
				FIBITMAP *scaled = fiRescale(task->image->bitmap, originalImage->loadedState.width, originalImage->loadedState.height, FILTER_LANCZOS3, false);
				bool oldColorSwap = task->image->colorSwap; // fiBindToImage assumes image is loaded as BGR, we should keep it as is
				fiBindToImage(scaled, task->image);
				task->image->colorSwap = oldColorSwap;
			}
		}

		// calculate errors
		if (originalImage && tex_testCompresionError)
		{
			if (!tex_testCompresionAllErrors)
				CalculateCompressionError(task->image, originalImage, task->image, tex_errorMetric);
			else
			{
				LoadedImage *ext = Image_Create();
				Image_Generate(ext, originalImage->width, originalImage->height, originalImage->bpp);
				for (TexErrorMetric metric = ERRORMETRIC_AUTO; metric < NUM_ERRORMETRICS; metric = (TexErrorMetric)(metric + 1))
				{
					CalculateCompressionError(task->image, originalImage, ext, tex_errorMetric);
					// export
					StripFileExtension(task->filename, filepath);
					sprintf(outfile, "%s_error_%s.tga", filepath, OptionEnumName(metric, tex_error_metrics));
					Image_ExportTarga(task->image, outfile);
				}
				Image_Delete(ext);
			}
		}

		// export
		StripFileExtension(task->filename, filepath);
		if (level > 0)
			sprintf(outfile, "%s_%i.tga", filepath, level);
		else
			sprintf(outfile, "%s.tga", filepath);
		if (exportWholeFileWithMipLevels)
		{
			Image_ExportTarga(task->image, outfile);
			task->pixeldata += compressedSize;
			task->pixeldatasize -= compressedSize;
		}
		else
		{
			task->data = Image_ExportTarga(task->image, &task->datasize);
			task->pixeldata = NULL;
			task->pixeldatasize = 0;
		}
		fiFree(task->image->bitmap);
		task->image->bitmap = NULL;
		
		// next mip level
		if (task->image->width < 2 || !exportWholeFileWithMipLevels)
			break;
		task->image->width = task->image->width / 2;
		task->image->height = task->image->height / 2;
	}
	if (task->pixeldatasize > 0)
		Warning("TexDecompress(%s): image data contains %i tail bytes\n", task->filename, task->pixeldatasize);
	
	// cleanup
	Image_Delete(task->image);
	task->image = NULL;
	if (task->comment)
	{
		mem_free(task->comment);
		task->comment = NULL;
	}
}

byte *TexDecompress(char *filename, TexEncodeTask *encodetask, size_t *outdatasize)
{
	TexDecodeTask task = { 0 };

	if (!encodetask->container)
		return false;
	task.filename = filename;
	task.container = encodetask->container;
	task.data = encodetask->stream;
	task.datasize = encodetask->streamLen;
	task.ImageParms.sRGB = encodetask->image->maps->sRGB;
	task.ImageParms.isNormalmap = (encodetask->image->datatype == IMAGE_NORMALMAP) ? true : false;
	task.ImageParms.hasAverageColor = encodetask->image->hasAverageColor;
	task.ImageParms.averagecolor[0] = encodetask->image->averagecolor[0]; 
	task.ImageParms.averagecolor[1] = encodetask->image->averagecolor[1]; 
	task.ImageParms.averagecolor[2] = encodetask->image->averagecolor[2]; 
	task.ImageParms.colorSwap = encodetask->image->colorSwap;
	task.ImageParms.hasAlpha = encodetask->image->hasAlpha;
	Decompress(&task, false, encodetask->image);
	*outdatasize = task.datasize;
	return task.data;
} 

bool TexDecompress(char *filename)
{
	TexContainer *container;
	TexDecodeTask task = { 0 };

	container = findContainerForFile(filename, NULL, 0);
	if (!container)
		return false;
	task.filename = filename;
	task.container = container;
	Print("Decompressing %s file...\n", task.container->name);
	task.datasize = LoadFile(filename, &task.data);
	Decompress(&task, true, NULL);
	mem_free(task.data);
	Print("Decompression finished!\n");
	return true;
}