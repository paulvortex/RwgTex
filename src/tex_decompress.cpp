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

  Decompression

==========================================================================================
*/

size_t DecompressImage(TexDecodeTask *task)
{
	size_t compressedSize;

	// create image (if need to)
	if (task->image == NULL)
	{
		task->image = Image_Create();
		task->image->width = task->width;
		task->image->height = task->height;
	}

	// decompress (and unswizzle swizzled format)
	task->image->sRGB = task->ImageParms.sRGB;
	task->image->datatype = task->ImageParms.isNormalmap ? IMAGE_NORMALMAP : IMAGE_COLOR;
	task->image->bpp = compressedTextureBPP(task->image, task->format, task->container);
	task->image->bitmap = fiCreate(task->image->width, task->image->height, task->image->bpp, "Decompress");
	compressedSize = compressedTextureSize(task->image, task->format, task->container, true, false);
	if (compressedSize > task->pixeldatasize)
		Error("Decompress_Image(%s): image data %i is lesser than estimated data size %i\n", task->filename, task->pixeldatasize, compressedSize);
	if (task->codec->fDecode)
		task->codec->fDecode(task);
	else
		Error("Decompress_Image(%s): %s codec does not support decoding of %s format\n", task->filename, task->codec->name, task->format->name);

	// image params from header
	task->image->hasAlpha = task->image->bpp == 4 || task->ImageParms.hasAlpha;

	return compressedSize;
}

void UnswizzleImage(TexDecodeTask *task)
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

void Decompress(TexDecodeTask *task, bool exportWholeFileWithMipLevels, LoadedImage *original)
{
	char outfile[MAX_FPATH], filepath[MAX_FPATH];
	int levels;

	// decompress
	if (!task->container->fReadHeader(task))
	{
		task->container->fPrintHeader(task->data);
		Error("TexDecompress(%s): %s\n", task->filename, task->errorMessage);
	}

	// COMMANDLINEPARM: -nounswizzle: disable unswizzling of swizzled formats to allow inspection of whats really stored in file
	bool nounswizzle = CheckParm("-nounswizzle");

	// decode base image and maps
	levels = 1 + task->numMipmaps;
	for (int level = 0; level < levels; level++)
	{
		// decompress
		size_t compressedSize = DecompressImage(task);

		// calculate errors
		if (original != NULL && tex_testCompresionError)
		{
			if (!tex_testCompresionAllErrors)
			{
				TexCalcErrors *calc = TexCompressionError(task->format, task->image, original, tex_errorMetric, true);
				fiBindToImage(calc->image->bitmap, task->image);
				FreeErrorCalc(calc, true);
			}
			else
			{
				for (TexErrorMetric metric = ERRORMETRIC_LINEAR; metric < NUM_ERRORMETRICS; metric = (TexErrorMetric)(metric + 1))
				{
					TexCalcErrors *calc = TexCompressionError(task->format, task->image, original, metric, true);
					StripFileExtension(task->filename, filepath);
					sprintf(outfile, "%s_error_%s.tga", filepath, OptionEnumName(metric, tex_error_metrics));
					Image_ExportTarga(calc->image, outfile);
					FreeErrorCalc(calc);
				}
			}
		}

		// rescale to original size
		if (original != NULL && tex_testCompresion_keepSize && !exportWholeFileWithMipLevels)
		{
			if (original->width != original->loadedState.width || original->height != original->loadedState.height)
			{
				FIBITMAP *scaled = fiRescale(task->image->bitmap, original->loadedState.width, original->loadedState.height, FILTER_BILINEAR, false);
				bool oldColorSwap = task->image->colorSwap; // fiBindToImage assumes image is loaded as BGR, we should keep it as is
				fiBindToImage(scaled, task->image);
				task->image->colorSwap = oldColorSwap;
			}
		}

		// unswizzle color
		if (!nounswizzle)
			UnswizzleImage(task);

		// finished loading
		Image_LoadFinish(task->image);

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

void DecodeFromEncode(TexDecodeTask *out, TexEncodeTask *in, char *filename)
{
	out->filename = filename;
	out->container = in->container;
	out->data = in->stream;
	out->datasize = in->streamLen;
	out->ImageParms.sRGB = in->image->maps->sRGB;
	out->ImageParms.isNormalmap = (in->image->datatype == IMAGE_NORMALMAP) ? true : false;
	out->ImageParms.hasAverageColor = in->image->hasAverageColor;
	out->ImageParms.averagecolor[0] = in->image->averagecolor[0];
	out->ImageParms.averagecolor[1] = in->image->averagecolor[1];
	out->ImageParms.averagecolor[2] = in->image->averagecolor[2];
	out->ImageParms.colorSwap = in->image->colorSwap;
	out->ImageParms.hasAlpha = in->image->hasAlpha;
	out->image = NULL;
}

byte *TexDecompress(char *filename, TexEncodeTask *encodetask, size_t *outdatasize)
{
	TexDecodeTask task = { 0 };

	if (!encodetask->container)
		return false;
	DecodeFromEncode(&task, encodetask, filename);
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
	task.image = NULL;
	task.filename = filename;
	task.container = container;
	Print("Decompressing %s file...\n", task.container->name);
	task.datasize = LoadFile(filename, &task.data);
	Decompress(&task, true, NULL);
	mem_free(task.data);
	Print("Decompression finished!\n");
	return true;
}