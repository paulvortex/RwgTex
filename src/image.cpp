////////////////////////////////////////////////////////////////
//
// RwgTex / image loading and processing
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_IMAGE_C

#include "main.h"
#include "freeimage.h"
#include "omnilib/dpomnilib.h"
#include "scale2x.h"
#include "scalexBR.h"
#include "tex.h"

using namespace omnilib;

/*
==========================================================================================

  IMAGE LOADING 

==========================================================================================
*/

void FreeImageBitmap(LoadedImage *image)
{
	if (!image->bitmap)
		return;
	fiFree(image->bitmap);
	image->bitmap = NULL;
}

void FreeImageMaps(LoadedImage *image)
{
	ImageMap *map, *next;

	if (!image->maps)
		return;
	for (map = image->maps; map; map = next)
	{
		next = map->next;
		if (map->data && map->external == false)
			mem_free(map->data);
		mem_free(map);
	}
	image->maps = NULL;
}

void ClearImage(LoadedImage *image)
{
	FreeImageBitmap(image);
	FreeImageMaps(image);
	memset(image->texname, 0, 128); 
	image->useTexname = false;
}

LoadedImage *Image_Create(void)
{
	LoadedImage *image;

	image = (LoadedImage *)mem_alloc(sizeof(LoadedImage));
	memset(image, 0, sizeof(LoadedImage));
	ClearImage(image);
	return image;
}

void Image_Unload(LoadedImage *image)
{
	LoadedImage *frame, *next;

	ClearImage(image);
	// remove frames
	for (frame = image->next; frame != NULL; frame = next)
	{
		next = frame->next;
		ClearImage(frame);
		mem_free(frame);
	}
	image->next = NULL;
}

void Image_Delete(LoadedImage *image)
{
	Image_Unload(image);
	mem_free(image);
}

/*
==========================================================================================

  IMAGE DATA PROCESS 

==========================================================================================
*/

// ImageData_SwapRB
// swap reg and blue channels
void ImageData_SwapRB(byte *data, int width, int height, int pitch, int bpp)
{
	byte *in, *end, saved;
	int y;

	for (y = 0; y < height; y++)
	{
		in = data;
		end = in + width*bpp;
		while(in < end)
		{
			saved = in[0];
			in[0] = in[2];
			in[2] = saved;
			in += bpp;
		}
		data += pitch;
	}
}

// ImageData_ConvertSRGB
// convert RGB->sRGB colorspace
#define linear_to_srgb(c) (((c) < 0.0031308f) ? (c) * 12.92f : 1.055f * (float)pow((c), 1.0f/2.4f) - 0.055f)
#define srgb_to_linear(c) (((c) <= 0.04045f) ? (c) * (1.0f / 12.92f) : (float)pow(((c) + 0.055f)*(1.0f/1.055f), 2.4f))
void ImageData_ConvertSRGB(byte *data, int width, int height, int pitch, int bpp, bool srcSRGB, bool dstSRGB)
{
	byte *in, *end, *lines;
	int y;

	if (!data)
		return;
	if (srcSRGB == dstSRGB)
		return;

	if (dstSRGB == true)
	{
		// linear->sRGB
		lines = data;
		for (y = 0; y < height; y++)
		{
			in = lines;
			end = in + width*bpp;
			while(in < end)
			{
				in[0] = (byte)floor(linear_to_srgb((float)in[0] / 255.0f) * 255.0f + 0.5f);
				in[1] = (byte)floor(linear_to_srgb((float)in[1] / 255.0f) * 255.0f + 0.5f);
				in[2] = (byte)floor(linear_to_srgb((float)in[2] / 255.0f) * 255.0f + 0.5f);
				in += bpp;
			}
			lines += pitch;
		}
		return;
	}
	// sRGB->linear
	lines = data;
	for (y = 0; y < height; y++)
	{
		in = lines;
		end = in + width*bpp;
		while(in < end)
		{
			in[0] = (byte)floor(srgb_to_linear((float)in[0] / 255.0f) * 255.0f + 0.5f);
			in[1] = (byte)floor(srgb_to_linear((float)in[1] / 255.0f) * 255.0f + 0.5f);
			in[2] = (byte)floor(srgb_to_linear((float)in[2] / 255.0f) * 255.0f + 0.5f);
			in += bpp;
		}
		lines += pitch;
	}
}

// probe linear image for 16-bit color sRGB conversion, if overall quality will increase, return true
// smart conversion: if sRGB colorspace will cause a quality increase (i.e. image contains many dark pixels), convert it
bool ImageData_ProbeLinearToSRGB_16bit(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap)
{
	int h;
	float grayscale;
	size_t lowpixels = 0, highpixels = 0;
	byte *in, *end;
	if (rgbSwap)
	{
		for (h = 0; h < height; h++)
		{
			in = data;
			end = in + width * bpp;
			while(in < end)
			{
				grayscale = (float)in[0] * 0.299f + (float)in[1] * 0.587f + (float)in[2] * 0.114f;
				if (bpp == 4 && in[3] == 0)
					grayscale = 0;
				if (grayscale > 0) // skip black and transparent pixels
				{
					if (grayscale > 30)
						highpixels++;
					else
						lowpixels++;
				}
				in += bpp;
			}
			data += pitch;
		}
	}
	else
	{
		for (h = 0; h < height; h++)
		{
			in = data;
			end = in + width * bpp;
			while(in < end)
			{
				grayscale = (float)in[2] * 0.299f + (float)in[1] * 0.587f + (float)in[0] * 0.114f;
				if (bpp == 4 && in[3] == 0)
					grayscale = 0;
				if (grayscale > 0) // skip black and transparent pixels
				{
					if (grayscale > 30) // pixels < 30 have 2.5-4x more precision 
						highpixels++;
					else
						lowpixels++;
				}
				in += bpp;
			}
			data += pitch;
		}
	}
	//printf("lowpixels %i, highpixels %i\n", lowpixels, highpixels); 
	if (lowpixels > highpixels)
		return true;
	return false;
}

/*
==========================================================================================

  IMAGE PROCESS
  this processes all filters requested by LoadedImage container

==========================================================================================
*/

// convert bits-per-pixel
void Image_ConvertBPP(LoadedImage *image, int bpp)
{
	if (!image)
		return;
	if (image->bpp == bpp)
		return;
	image->bitmap = fiConvertBPP(image->bitmap, bpp, 256, NULL);
	image->converted = true;
	if (image->bpp < 4)
	{
		if (bpp == 4)
		{
			image->hasAlpha = true;
			image->hasGradientAlpha = false;
		}
	}
	else if (bpp < 4)
	{
		image->hasAlpha = false;
		image->hasGradientAlpha = false;
	}
	image->bpp = FreeImage_GetBPP(image->bitmap)/8;
}

// convert RGB->sRGB
void Image_ConvertSRGB(LoadedImage *image, bool useSRGB)
{
	byte *data;
	int pitch;

	if (!image->bitmap)
		return;
	if (useSRGB == image->sRGB)
		return;
	data = fiGetData(image->bitmap, &pitch);
	ImageData_ConvertSRGB(data, image->width, image->height, pitch, image->bpp, image->sRGB, useSRGB);
	image->sRGB = useSRGB;
}

// swap RGB->BGR
void Image_SwapColors(LoadedImage *image, bool swappedColor)
{
	byte *data;
	int pitch;

	if (!image->bitmap)
		return;
	if (swappedColor == image->colorSwap)
		return;
	data = fiGetData(image->bitmap, &pitch);
	ImageData_SwapRB(data, image->width, image->height, pitch, image->bpp);
	image->colorSwap = swappedColor;
}

// calculate image averaged color
void Image_CalcAverageColor(LoadedImage *image)
{
	double avgcolor[3];
	long samples;
	int w, r, g, b, pitch, y;

	samples = 0;
	avgcolor[0] = 0;
	avgcolor[1] = 0;
	avgcolor[2] = 0;
	byte *data = fiGetData(image->bitmap, &pitch);
	if (image->colorSwap == true)
	{
		r = 2;
		g = 1;
		b = 0;
	}
	else
	{
		r = 0;
		g = 1;
		b = 2;
	}
	w = FreeImage_GetWidth(image->bitmap);
	for (y = 0; y < image->height; y++)
	{
		byte *in = data;
		byte *end = in + w*image->bpp;
		while(in < end)
		{
			// only use not-black pixels
			if (in[0] != 0 || in[1] != 0 || in[2] != 0)
			{
				avgcolor[0] += (float)in[r] / 255.0f;
				avgcolor[1] += (float)in[g] / 255.0f;
				avgcolor[2] += (float)in[b] / 255.0f;
				samples++;
			}
			in += image->bpp;
		}
		data += pitch;
	}
	if (samples)
	{
		avgcolor[0] = min(1.0f, max(0.0f, avgcolor[0] / samples));
		avgcolor[1] = min(1.0f, max(0.0f, avgcolor[1] / samples));
		avgcolor[2] = min(1.0f, max(0.0f, avgcolor[2] / samples));
	}

	// store
	image->hasAverageColor = true;
	image->averagecolor[0] = (byte)(avgcolor[0] * 255.0f);
	image->averagecolor[1] = (byte)(avgcolor[1] * 255.0f);
	image->averagecolor[2] = (byte)(avgcolor[2] * 255.0f);
}

void Image_FreeMaps(LoadedImage *image)
{
	if (image->maps)
		FreeImageMaps(image);
}

int NextPowerOfTwo(int n) 
{ 
    if ( n <= 1 ) return n;
    double d = n-1; 
    return 1 << ((((int*)&d)[1]>>20)-1022); 
}

// scale for 4x and then backscale 1/2 for better quality
void Image_Scale2x_Super2x(LoadedImage *image, bool makePowerOfTwo)
{
	byte *in, *end, *out;
	byte palette[1024];

	if (!image->bitmap)
		return;

	// scale2x does not allows BPP = 3
	// convert to 4 (and convert back after finish)
	if (image->bpp != 4)
		Image_ConvertBPP(image, 4);

	// check if we can scale
	if (sxCheck(4, image->bpp, image->width, image->height) != SCALEX_OK)
		return;

	// get our desired sizes
	int w = image->width;
	int h = image->height;
	int nw = w * 2;
	int nh = h * 2;
	if (makePowerOfTwo)
	{
		nw = NextPowerOfTwo(nw);
		nh = NextPowerOfTwo(nh);
	}

	// save out alpha to be scaled in separate pass
	// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
	byte *alpha = NULL;
	if (image->hasAlpha)
	{
		alpha = (byte *)mem_alloc(w * h);
		in  = fiGetData(image->bitmap, NULL);
		end = in + w*h*4;
		out = alpha;
		while(in < end)
		{
			*out++ = in[3];
			if (in[3] < tex_binaryAlphaMin)
			{
				// filter out colors for transparent pixels for better scaling
				in[0] = 0;
				in[1] = 0;
				in[2] = 0;
			}
			in[3]  = 255;
			in += 4;
		}
	}

	// get palette
	FIBITMAP *paletted = fiConvertBPP(fiClone(image->bitmap), 1, 0, NULL);
	if (!fiGetPalette(paletted, palette, 256))
		Warning("Image_Scale2x_Super2x: fiGetPalette failed!");
	fiFree(paletted);

	// scale with lanczos, restore palette
	FIBITMAP *lanczos = fiRescale(image->bitmap, nw, nh, FILTER_LANCZOS3, false);
	lanczos = fiConvertBPP(lanczos, 1, 256, palette);
	lanczos = fiConvertBPP(lanczos, 4, 0, NULL);
	lanczos = fiSharpen(lanczos, 1.1f, 3, true);

	// scale with scale 4x, then backscale to 2x
	FIBITMAP *rescaled = fiScale2x(image->bitmap, 4, false);
	rescaled = fiRescale(rescaled, nw, nh, FILTER_LANCZOS3, true);
	fiBindToImage(rescaled, image);

	// blend lanczos over scale2x
	fiCombine(image->bitmap, lanczos, COMBINE_RGB, 0.4f, true);

	// scale alpha as a second pass
	if (!alpha)
		Image_ConvertBPP(image, 3);
	else
	{
		FIBITMAP *alpha_scaled = fiRescale(fiScale2x(alpha, w, w, h, 1, 4, true), nw, nh, FILTER_CATMULLROM, true);
		alpha_scaled = fiSharpen(alpha_scaled, 1.2f, 1, true);
		fiCombine(image->bitmap, alpha_scaled, COMBINE_R_TO_ALPHA, 1.0, true);
		fiBindToImage(fiFixTransparentPixels(image->bitmap), image);
	}
	image->scaled = true;
}

// scale for 2x
void Image_Scale2x(LoadedImage *image, int factor)
{
	if (!image->bitmap)
		return;

	// scale2x does not allows BPP = 3
	// convert to 4 (and convert back after finish)
	if (image->bpp != 4)
		Image_ConvertBPP(image, 4);

	// check if we can scale
	if (sxCheck(factor, image->bpp, image->width, image->height) != SCALEX_OK)
		return;

	// scale
	// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
	int w = image->width;
	int h = image->height;
	FIBITMAP *scaled = fiCreate(w*factor, h*factor, 4);
	byte *data_scaled = fiGetData(scaled, NULL);
	byte *data_bitmap = fiGetData(image->bitmap, NULL);
	sxScale(factor, data_scaled, w*factor*4, data_bitmap, w*4, 4, w, h);
	fiBindToImage(scaled, image);

	// finish
	if (!image->hasAlpha)
		Image_ConvertBPP(image, 3);
	image->scaled = true;
}

// scale using xBR scaler
void Image_ScalexBR(LoadedImage *image, int factor)
{
	xbrz::ScalerCfg scalerconfig;

	// create scaler config
	memcpy(&scalerconfig, &xbrz::DefaultScalerCfg, sizeof(xbrz::ScalerCfg));
	scalerconfig.luminanceWeight_ = 0.75;
	//scalerconfig.equalColorTolerance_ = 1;
	//scalerconfig.steepDirectionThreshold = 1;

	// create scaled image
	FIBITMAP *scaled = fiCreate(image->width*factor, image->height*factor, 4);

	// save out alpha to be scaled in separate pass
	// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
	if (image->hasAlpha)
	{
		// extract alpha as RGB image
		byte *alpha = (byte *)mem_alloc(image->width * image->height * 4);
		byte *in = fiGetData(image->bitmap, NULL);
		byte *end = in + image->width*image->height*4;
		byte *out = alpha;
		while(in < end)
		{
			out[0] = in[3];
			out[1] = in[3];
			out[2] = in[3];
			out[3] = 0;
			in += 4;
			out += 4;
		}
		// alpha scale
		xbrz::scale(factor, (uint32_t *)alpha, (uint32_t *)fiGetData(scaled, NULL), image->width, image->height, scalerconfig, 0, image->height);
		mem_free(alpha);
		// RGB scale
		Image_SetAlpha(image, 0);
		FIBITMAP *rgb_scaled = fiCreate(image->width*factor, image->height*factor, 4);
		xbrz::scale(factor, (uint32_t *)fiGetData(image->bitmap, NULL), (uint32_t *)fiGetData(rgb_scaled, NULL), image->width, image->height, scalerconfig, 0, image->height);
		// combine and return
		fiCombine(rgb_scaled, scaled, COMBINE_R_TO_ALPHA, 1, true);
		fiBindToImage(rgb_scaled, image);
		image->scaled = true;
		return;
	}

	// RGB scale
	Image_ConvertBPP(image, 4);
	Image_SetAlpha(image, 0);
	xbrz::scale(factor, (uint32_t *)fiGetData(image->bitmap, NULL), (uint32_t *)fiGetData(scaled, NULL), image->width, image->height, scalerconfig, 0, image->height);
	fiBindToImage(scaled, image);
	Image_ConvertBPP(image, 3);
	image->scaled = true;
}

// Image_ScalexBR_Super2x
// scale to 4x then backscale to 2x using xBR scaling
void Image_ScalexBR_Super2x(LoadedImage *image, bool makePowerOfTwo)
{
	FIBITMAP *backscale1;

	int dstwidth = image->width * 2;
	int dstheight = image->height * 2;
	if (makePowerOfTwo)
	{
		dstwidth = NextPowerOfTwo(dstwidth);
		dstheight = NextPowerOfTwo(dstheight);
	}
	Image_ScalexBR(image, 4);
	if (image->bpp == 4)
	{
		// backscale RGB with interpolation
		backscale1 = fiRescale(image->bitmap, dstwidth, dstheight, FILTER_CATMULLROM, false);
		// backscale alpha with no interpolation
		FIBITMAP *backscale2 = fiRescaleNearestNeighbor(image->bitmap, dstwidth, dstheight, false);
		// combine
		fiCombine(backscale1, backscale2, COMBINE_ALPHA, 1, true);
	}
	else
	{
		// backscale RGB with interpolation
		backscale1 = fiRescale(image->bitmap, dstwidth, dstheight, FILTER_CATMULLROM, false);
	}
	fiBindToImage(backscale1, image);
}

// scale image by 2x using different scale technique
void Image_ScaleBy2(LoadedImage *image, ImageScaler scaler, bool makePowerOfTwo)
{
	if (!image->bitmap)
		return;

	if (scaler == IMAGE_SCALER_SUPER2X)
	{
		Image_Scale2x_Super2x(image, makePowerOfTwo);
		return;
	}
	if (scaler == IMAGE_SCALER_SCALE2X)
	{
		Image_Scale2x(image, 2);
		return;
	}
	if (scaler == IMAGE_SCALER_XBRZ)
	{
		Image_ScalexBR(image, 2);
		return;
	}
	if (scaler == IMAGE_SCALER_SBRZ)
	{
		Image_ScalexBR_Super2x(image, makePowerOfTwo);
		return;
	}

	// a FreeImage scaler
	FREE_IMAGE_FILTER filter;
	     if (scaler == IMAGE_SCALER_BOX)        filter = FILTER_BOX;
	else if (scaler == IMAGE_SCALER_BILINEAR)   filter = FILTER_BILINEAR;
	else if (scaler == IMAGE_SCALER_BICUBIC)    filter = FILTER_BICUBIC;
	else if (scaler == IMAGE_SCALER_BSPLINE)    filter = FILTER_BSPLINE;
	else if (scaler == IMAGE_SCALER_CATMULLROM) filter = FILTER_CATMULLROM;
	else if (scaler == IMAGE_SCALER_LANCZOS)    filter = FILTER_LANCZOS3;
	else
	{
		Warning("Image_Scale: bad scaler");
		return;
	}

	int w = image->width * 2;
	int h = image->height * 2;
	if (makePowerOfTwo)
	{
		w = NextPowerOfTwo(w);
		w = NextPowerOfTwo(w);
	}
	fiBindToImage(fiRescale(image->bitmap, w, h, filter, false), image);
	image->scaled = true;
}

// scale image by 4x using different scale technique
void Image_ScaleBy4(LoadedImage *image, ImageScaler scaler, ImageScaler scaler2, bool makePowerOfTwo)
{
	// instand 4x scale
	if (scaler == IMAGE_SCALER_XBRZ && scaler2 == IMAGE_SCALER_XBRZ)
	{
		Image_ScalexBR(image, 4);
		return;
	}
	if (scaler == IMAGE_SCALER_SCALE2X && scaler2 == IMAGE_SCALER_SCALE2X)
	{
		Image_Scale2x(image, 4);
		return;
	}
	// apply 2x scale two times
	Image_ScaleBy2(image, scaler, false);
	Image_ScaleBy2(image, scaler2, makePowerOfTwo);
}

// scale for 4x and then backscale 1/2 for best quality
void Image_MakeDimensions(LoadedImage *image, bool powerOfTwo, bool square)
{
	int w, h;

	if (!image->bitmap)
		return;
	w = image->width;
	h = image->height;
	if (powerOfTwo)
	{
		w = NextPowerOfTwo(w);
		h = NextPowerOfTwo(h);
	}
	if (square)
	{
		if (w < h)
			w = h;
		if (w > h)
			h = w;
	}
	if (w != image->width || h != image->height)
		fiBindToImage(fiRescale(image->bitmap, w, h, FILTER_LANCZOS3, false), image);
}

// make image's alpha binary
void Image_MakeAlphaBinary(LoadedImage *image, int thresh)
{
	if (!image->bitmap)
		return;
	if (image->bpp != 4)
		return;
	if (image->hasGradientAlpha == false)
		return;
	
	// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
	byte *in = fiGetData(image->bitmap, NULL);
	byte *end = in + image->width * image->height * image->bpp;
	while(in < end)
	{
		in[3] = (in[3] < tex_binaryAlphaCenter) ? 0 : 255;
		in += 4;
	}
	image->hasGradientAlpha = false;
	image->swizzled = true;
}

// force alpha to certain value
void Image_SetAlpha(LoadedImage *image, byte value)
{
	Image_ConvertBPP(image, 4);
	// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
	byte *data = fiGetData(image->bitmap, NULL);
	byte *end = data + FreeImage_GetWidth(image->bitmap)*FreeImage_GetHeight(image->bitmap)*image->bpp;
	while(data < end)
	{
		data[3] = value;
		data += image->bpp;
	}
	image->swizzled = true;
}

byte *Image_GetData(LoadedImage *image, size_t *datasize, int *pitch)
{
	size_t bitmapsize;

	if (!image->bitmap)
		return NULL;
	bitmapsize = (FreeImage_GetBPP(image->bitmap)/8)*FreeImage_GetWidth(image->bitmap)*FreeImage_GetHeight(image->bitmap);
	if (datasize)
		*datasize = bitmapsize;
	// compare real data size and structure
	if ((image->bpp*image->width*image->height) != bitmapsize)
		Error("Image_GetData: local bpp/width/height %i/%i/%i not match bitmap parameters %i/%i/%i", image->bpp, image->width, image->height, FreeImage_GetBPP(image->bitmap)/8, FreeImage_GetWidth(image->bitmap), FreeImage_GetHeight(image->bitmap));
	return fiGetData(image->bitmap, pitch);
}

byte *Image_GetUnalignedData(LoadedImage *image, size_t *datasize, bool *data_allocated, bool force_allocate)
{
	byte *data;

	data = fiGetUnalignedData(image->bitmap, data_allocated, force_allocate);
	if (datasize)
		*datasize = image->width*image->height*image->bpp;
	return data;
}

void Image_StoreUnalignedData(LoadedImage *image, byte *dataptr, size_t datasize)
{
	if (!image->bitmap)
		return;
	fiStoreUnalignedData(image->bitmap, dataptr, image->width, image->height, image->bpp);
}

void Image_FreeUnalignedData(byte *dataptr, bool data_allocated)
{
	fiFreeUnalignedData(dataptr, data_allocated);
}

byte *Image_GenerateTarga(size_t *outsize, int width, int height, int bpp, byte *data, bool flip, bool rgb, bool grayscale)
{
	byte *tga, *pixels;
	byte sr, sg, sb;
	size_t tgasize;
	int i;

	tgasize = 18 + width*height*bpp;
	tga = (byte *)mem_alloc(tgasize);
	memset(tga, 0, 18);
	tga[2]  = 2;
	tga[12] = (width >> 0) & 0xFF;
	tga[13] = (width >> 8) & 0xFF;
	tga[14] = (height >> 0) & 0xFF;
	tga[15] = (height >> 8) & 0xFF;
	tga[16] = bpp * 8;
	pixels = tga + 18;
	if (rgb)
	{
		sr = 2;
		sg = 1;
		sb = 0;
	}
	else
	{
		sr = 0;
		sg = 1;
		sb = 2;
	}
	if (grayscale)
		sb = sg = sr;
	if (!flip)
	{
		byte *out = pixels;
		for (i = 0; i < height; i++)
		{
			byte *in = data + i * width * bpp;
			byte *end = in + width * bpp;
			while(in < end)
			{
				out[0] = in[sr];
				out[1] = in[sg];
				out[2] = in[sb];
				if (bpp == 4)
					out[3] = in[3];
				out+=bpp;
				in+=bpp;
			}
		}
	}
	else
	{
		byte *out = pixels;
		for (i = height-1; i >=0; i--)
		{
			byte *in = data + i * width * bpp;
			byte *end = in + width * bpp;
			while(in < end)
			{
				out[0] = in[sr];
				out[1] = in[sg];
				out[2] = in[sb];
				if (bpp == 4)
					out[3] = in[3];
				out+=bpp;
				in+=bpp;
			}
		}
	}
	// write file
	*outsize = tgasize;
	return tga;
}

bool Image_Save(LoadedImage *image, char *filename)
{
	return fiSave(image->bitmap, FIF_UNKNOWN, filename);
}

byte *Image_ExportTarga(LoadedImage *image, size_t *tgasize)
{
	byte *data;
	bool data_allocated;
	size_t datasize;

	data = Image_GetUnalignedData(image, &datasize, &data_allocated, false);
	byte *tga = Image_GenerateTarga(tgasize, FreeImage_GetWidth(image->bitmap), FreeImage_GetHeight(image->bitmap), FreeImage_GetBPP(image->bitmap) / 8, data, true, image->colorSwap ? false : true, (image->datatype == IMAGE_GRAYSCALE) ? true : false);
	Image_FreeUnalignedData(data, data_allocated);
	return tga;
}

bool Image_ExportTarga(LoadedImage *image, char *filename)
{
	byte *tga;
	size_t tgasize;
	FILE *f;

	tga = Image_ExportTarga(image, &tgasize);
	f = fopen(filename, "wb");
	if (f)
	{
		fwrite(tga, tgasize, 1, f);
		fclose(f);
	}
	mem_free(tga);
	return true;
}

/*
==========================================================================================

  IMAGE LOAD

==========================================================================================
*/

// create procedural image
void Image_Generate(LoadedImage *image, int width, int height, int bpp)
{
	Image_Unload(image);
	image->bitmap = fiCreate(width, height, bpp);
	image->width = width;
	image->height = height;
	image->bpp = bpp;
	Image_LoadFinish(image);
}

// returns true if image was changed since latest load
bool Image_Changed(LoadedImage *image)
{
	LoadedImage *frame;

	for (frame = image; frame != NULL; frame = frame->next)
		if (memcmp(&frame->loadedState, &image->width, sizeof(ImageState)))
			return true;
	return false;
}

void Image_LoadFinish(LoadedImage *image)
{
	if (!image->bitmap)
		return;

	// we want bitmap image type
	image->bitmap = fiConvertType(image->bitmap, FIT_BITMAP);

	// check color type (only RGB and RGBA allowed)
	FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(image->bitmap);
	image->bpp = FreeImage_GetBPP(image->bitmap) / 8;
	if (colorType == FIC_PALETTE)
	{
		if (FreeImage_IsTransparent(image->bitmap))
			Image_ConvertBPP(image, 4);
		else
			Image_ConvertBPP(image, 3);
	}
	else if (image->bpp != 3 && image->bpp != 4)
		Image_ConvertBPP(image, 3);

	// check alpha
	image->hasAlpha = false;
	image->hasGradientAlpha = false;
	if (image->bpp == 4)
	{
		int num_grad = 0;
		int need_grad = (int)(image->width*image->height*(100.0f - tex_binaryAlphaThreshold)/100.0f);
		long pixels = image->width*image->height*image->bpp;
		// vortex: since BPP is 4, data is always properly aligned, so we dont need pitch
		byte *data = fiGetData(image->bitmap, NULL);
		image->hasAlpha = true;
		if (!tex_detectBinaryAlpha)
			image->hasGradientAlpha = true;
		else
		{
			for (long i = 0; i < pixels; i+= image->bpp)
			{
				if (data[i+3] >= tex_binaryAlphaMin && data[i+3] <= tex_binaryAlphaMax)
					num_grad++;
				if (num_grad > need_grad)
				{
					image->hasGradientAlpha = true;
					break;
				}
			}
		}
	}

	// calc average color
	Image_CalcAverageColor(image);

	// save the loaded state (for comparison if image was altered)
	memcpy(&image->loadedState, &image->width, sizeof(ImageState));
}

byte quake_palette[768] = { 0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,243,147,255,247,199,255,255,255,159,91,83 };

// a quake sprite loader
void LoadImage_QuakeSprite(FS_File *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	MetaSprite_t *sprite;
	MetaSpritePic_t *pic;
	int i;

	sprite = olLoadSprite(filedata, filesize);
	if (sprite->errormsg[0])
		Warning("%s%s.%s : failed to load Quake sprite: %s", file->path.c_str(), file->name.c_str(), file->ext.c_str(), sprite->errormsg);
	else
	{
		// load frames
		LoadedImage *frame = image;
		for (i = 0; i < sprite->numPics; i++)
		{
			pic = sprite->pics[i];
			if (pic->bpp != 4 && pic->bpp != 1)
				Warning("%s%s.%s : failed to load Quake sprite frame %i: bpp %i not supported", file->path.c_str(), file->name.c_str(), file->ext.c_str(), i, pic->bpp);

			// fill data
			frame->useTexname = true;
			sprintf(frame->texname, "%s.%s_%i", file->name.c_str(), file->ext.c_str(), i);
			frame->width = pic->width;
			frame->height = pic->height;
			frame->filesize = pic->width*pic->height*pic->bpp;
			if (pic->bpp == 1)
				fiLoadDataRaw(frame->width, frame->height, 1, pic->pixels, frame->width * frame->height, pic->colormap->palette, false, frame);
			else
				fiLoadDataRaw(frame->width, frame->height, 4, pic->pixels, frame->width * frame->height * 4, NULL, false, frame);
			Image_LoadFinish(frame);
			// go next frame
			if ((i+1) < sprite->numPics)
			{
				frame->next = Image_Create();
				frame = frame->next;
			}
		}
	}
	mem_free(filedata);
}

// a quake bsp stored textures loader
void LoadImage_QuakeBSP(FS_File *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	unsigned int lump_ofs = *(unsigned int *)(filedata + 20);
	unsigned int lump_size = *(unsigned int *)(filedata + 24);

	if ((size_t)(lump_ofs + lump_size) <= filesize)
	{
		byte *buf = filedata + lump_ofs;
		int   numtextures = *(int *)(buf);
		int  *textureoffsets = (int *)(buf + 4); 
		int   texnum = 0;
		LoadedImage *tex = image;
		while(texnum < numtextures)
		{
			// there could be null textures
			if (textureoffsets[texnum] > 0)
			{
				char *texname = (char *)(buf + textureoffsets[texnum]);
				int  texwidth = *(int *)(buf + textureoffsets[texnum] + 16); 
				int texheight = *(int *)(buf + textureoffsets[texnum] + 20); 
				int texmipofs = *(int *)(buf + textureoffsets[texnum] + 24); 
				// fill data
				tex->filesize = texwidth * texheight;
				if (texname[0] == '*')
					texname[0] = '#';
				tex->useTexname = true;
				sprintf(tex->texname, "%s/%s", file->name.c_str(), texname);
				if (texwidth < 0 || texwidth > 32768 || texheight < 0 || texheight > 32768)
					Error("LoadImage_QuakeBSP(%s) bogus texture size: %ix%i", tex->texname, texwidth, texheight);
				fiLoadDataRaw(texwidth, texheight, 1, (byte *)(buf + textureoffsets[texnum] + texmipofs), texwidth * texheight, quake_palette, false, tex);
				Image_LoadFinish(tex);
				// set next texture
				tex->next = Image_Create();
				tex = tex->next;
			}
			texnum++;
		}
	}



	mem_free(filedata);
}

void LoadImage_Generic(FS_File *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	image->filesize = filesize;
	fiLoadData(FIF_UNKNOWN, file, filedata, filesize, image);
	mem_free(filedata);
	Image_LoadFinish(image);
}

void Image_Load(FS_File *file, LoadedImage *image)
{
	size_t filesize;
	byte *filedata;

	filedata = FS_LoadFile(file, &filesize);
	if (!filedata)
		return;

	ClearImage(image);
	unsigned int fourCC = *(unsigned int *)filedata;
	if (fourCC == FOURCC('I','D','S','P'))
		LoadImage_QuakeSprite(file, filedata, filesize, image);
	else if (fourCC == 29)
		LoadImage_QuakeBSP(file, filedata, filesize, image);
	else
		LoadImage_Generic(file, filedata, filesize, image);
}

/*
==========================================================================================

  OMNILIB BINDING

==========================================================================================
*/

// omnilib dynamic memory wrapper - malloc
void *omnilib_malloc(size_t size, char *file, int line)
{
	return _mem_alloc(size, file, line);
}

// omnilib dynamic memory  - realloc
void *omnilib_realloc(void *buf, size_t size, char *file, int line)
{
	return _mem_realloc(buf, size, file, line);
}

// omnilib dynamic memory  - free
void omnilib_free(void *buf, char *file, int line)
{
	return _mem_free(&buf, file, line);
}

// omnilib message wrapper
void omnilib_print_message(int level, char *msg)
{
	if (level == 0)
		Print(msg);
	else if (level == 1)
		Verbose(msg);
	else if (level == 2)
		Warning(msg);
}

// omnilib error wrapper
void omnilib_error(char *msg)
{
	Error(msg);
}

/*
==========================================================================================

  COMMON

==========================================================================================
*/

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message)
{
	Warning("FreeImage: %s", message);
}

void Image_Init(void)
{
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif
	FreeImage_SetOutputMessage(FreeImageErrorHandler);

	// init omnilib
	OmnilibSetMemFunc(omnilib_malloc, omnilib_realloc, omnilib_free);
	OmnilibSetMessageFunc(omnilib_print_message, omnilib_error);

	// test sRGB table
	//int i;
	//printf("sRGB table:\n");
	//for (i = 0; i < 256; i++)
	//{
	//	byte c = (byte)floor(linear_to_srgb((float)i / 255.0f) * 255.0f + 0.5f);
	//	float p = (i == 0) ? 0 : (linear_to_srgb((float)(i+1) / 255.0f) * 255.0f + 0.5f - linear_to_srgb((float)i / 255.0f) * 255.0f + 0.5f);
	//	printf("%i: %i %f\n", i, c, p);
	//}
}

void Image_Shutdown(void)
{
#ifdef FREEIMAGE_LIB
	FreeImage_Deinitialise();
#endif
}

void Image_PrintModules(void)
{
	Print(" FreeImage %s\n", FreeImage_GetVersion());
}