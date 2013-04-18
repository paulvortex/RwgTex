////////////////////////////////////////////////////////////////
//
// RWGDDS - image loading
// coded by Pavel [VorteX] Timofeyev and placed to public domain
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////

#include "main.h"

#include "freeimage.h"

#include "scale2x.h"

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

void FreeImageMipmaps(LoadedImage *image)
{
	MipMap *mipmap, *nextmip;

	if (!image->mipMaps)
		return;
	for (mipmap = image->mipMaps; mipmap; mipmap = nextmip)
	{
		nextmip = mipmap->nextmip;
		mem_free(mipmap->data);
		mem_free(mipmap);
	}
	image->mipMaps = NULL;
}

void ClearImage(LoadedImage *image)
{
	FreeImageBitmap(image);
	FreeImageMipmaps(image);
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

  IMAGE PROCESS
  this processes all filters requested by LoadedImage container

==========================================================================================
*/

void ConvertToBPP(LoadedImage *image, int bpp)
{
	image->bitmap = fiConvertBPP(image->bitmap, bpp);
	image->bpp = FreeImage_GetBPP(image->bitmap)/8;
}

// swap RGB->BGR, optional swizzle for smart texture formats
// assumes that image is already in RGB/RGBA format
void Image_ConvertColorsForCompression(LoadedImage *image, bool swappedColor, COLORSWIZZLE swizzleColor, bool forceBGRA)
{
	byte *data, *end, y, co, cg;
	bool doSwap;

	if (!image->bitmap)
		return;

	// prepare colors
	doSwap = (swappedColor != image->colorSwap);
	// premodulate color requires 32-bite texture (otherwize alpha is 255)
	if (swizzleColor == IMAGE_COLORSWIZZLE_PREMODULATE)
		if (image->bpp != 4)
			swizzleColor = IMAGE_COLORSWIZZLE_NONE;
	// RXGB, YCoCg requires alpha to be presented
	if (swizzleColor == IMAGE_COLORSWIZZLE_XGBR || swizzleColor == IMAGE_COLORSWIZZLE_AGBR || swizzleColor == IMAGE_COLORSWIZZLE_YCOCG || forceBGRA)
	{
		if (image->bpp != 4)
		{
			ConvertToBPP(image, 4);
			image->hasAlpha = true;
		}
	}
	// do not allow double color swizzle as it would freak thing out
	if (image->colorSwizzle != IMAGE_COLORSWIZZLE_NONE)
		swizzleColor = IMAGE_COLORSWIZZLE_NONE;

	// get start & end pointers
	data = FreeImage_GetBits(image->bitmap);
	end  = data + FreeImage_GetWidth(image->bitmap)*FreeImage_GetHeight(image->bitmap)*image->bpp;

	// process
	if (swizzleColor == IMAGE_COLORSWIZZLE_PREMODULATE)
	{
		if (doSwap)
		{
			// swap, premodulate
			while(data < end)
			{
				float mod = (float)data[3] / 255.0f;
				byte saved = data[0];
				data[0] = (byte)(data[2] * mod);
				data[1] = (byte)(data[1] * mod);
				data[2] = (byte)(saved   * mod);
				data += image->bpp;
			}
		}
		else
		{
			// no swap, premodulate
			while(data < end)
			{
				float mod = (float)data[3] / 255.0f;
				data[0] = (byte)(data[0] * mod);
				data[1] = (byte)(data[1] * mod);
				data[2] = (byte)(data[2] * mod);
				data += image->bpp;
			}
		}
	}
	else if (swizzleColor == IMAGE_COLORSWIZZLE_XGBR)
	{
		if (doSwap)
		{
			// swap, swizzle
			while(data < end)
			{
				byte saved = data[0];
				data[0] = data[2];
				data[2] = 0;
				data[3] = saved;
				data += image->bpp;
			}
		}
		else
		{
			// no swap, swizzle
			while(data < end)
			{
				data[3] = data[2];
				data[2] = 0;
				data += image->bpp;
			}
		}
	}
	else if (swizzleColor == IMAGE_COLORSWIZZLE_YCOCG)
	{
		if (doSwap)
		{
			// swap, swizzle
			while(data < end)
			{
				y  = ((data[0] + (data[1] << 1) + data[2]) + 2) >> 2;
				co = ((((data[0] << 1) - (data[2] << 1)) + 2) >> 2) + 128;
				cg = (((-data[0] + (data[1] << 1) - data[2]) + 2) >> 2) + 128;
				data[0] = 255;
			    data[1] = (cg > 255 ? 255 : (cg < 0 ? 0 : cg));
			    data[2] = (co > 255 ? 255 : (co < 0 ? 0 : co));
			    data[3] = (y  > 255 ? 255 : (y  < 0 ? 0 :  y));
				data += image->bpp;
			}
		}
		else
		{
			// no swap, swizzle
			while(data < end)
			{
				y  = ((data[2] + (data[1] << 1) + data[0]) + 2) >> 2;
				co = ((((data[2] << 1) - (data[0] << 1)) + 2) >> 2) + 128;
				cg = (((-data[2] + (data[1] << 1) - data[0]) + 2) >> 2) + 128;
				data[0] = 255;
			    data[1] = (cg > 255 ? 255 : (cg < 0 ? 0 : cg));
			    data[2] = (co > 255 ? 255 : (co < 0 ? 0 : co));
			    data[3] = (y  > 255 ? 255 : (y  < 0 ? 0 :  y));
				data += image->bpp;
			}
		}
	}
	else if (swizzleColor == IMAGE_COLORSWIZZLE_AGBR)
	{
		if (doSwap)
		{
			// swap, swizzle
			while(data < end)
			{
				byte saved = data[0];
				data[0] = data[2];
				data[2] = data[3];
				data[3] = saved;
				data += image->bpp;
			}
		}
		else
		{
			// no swap, swizzle
			while(data < end)
			{
				byte saved = data[2];
				data[2] = data[3];
				data[3] = saved;
				data += image->bpp;
			}
		}
	}
	else if (doSwap)
	{
		// swap R<>B
		while(data < end)
		{
			byte saved = data[0];
			data[0] = data[2];
			data[2] = saved;
			data += image->bpp;
		}
	}
	else
	{
		// image already in format we want it to be
	}
	image->colorSwap = swappedColor;
	image->colorSwizzle = swizzleColor;
}

void Image_GenerateMipmaps(LoadedImage *image)
{
	MipMap *mipmap;
	int s, w, h, l;
		
	if (image->mipMaps)
		FreeImageMipmaps(image);

	s = min(image->width, image->height);
	w = image->width;
	h = image->height;
	l = 0;
	while(s > 1)
	{
		l++;
		w = w / 2;
		h = h / 2;
		s = s / 2;
		if (image->mipMaps)
		{
			mipmap->nextmip = (MipMap *)mem_alloc(sizeof(MipMap));
			mipmap = mipmap->nextmip;
		}
		else
		{
			image->mipMaps  = (MipMap *)mem_alloc(sizeof(MipMap));
			mipmap = image->mipMaps;
		}
		memset(mipmap, 0, sizeof(MipMap));
		mipmap->nextmip = NULL;
		FIBITMAP *mipbitmap = fiRescale(image->bitmap, w, h, FILTER_LANCZOS3, false);
		mipmap->width = w;
		mipmap->height = h;
		mipmap->level = l;
		mipmap->datasize = mipmap->width*mipmap->height*image->bpp;
		mipmap->data = (byte *)mem_alloc(mipmap->datasize);
		memcpy(mipmap->data, FreeImage_GetBits(mipbitmap), mipmap->datasize);
		if (mipbitmap != image->bitmap)
			fiFree(mipbitmap);
	}
}

int NextPowerOfTwo(int n) 
{ 
    if ( n <= 1 ) return n;
    double d = n-1; 
    return 1 << ((((int*)&d)[1]>>20)-1022); 
}

// scale for 4x and then backscale 1/2 for best quality
void Image_Scale2x_Super2x(LoadedImage *image, bool makePowerOfTwo)
{
	byte *in, *end, *out;

	if (!image->bitmap)
		return;

	// scale2x does not allows BPP = 3
	// convert to 4 (and convert back after finish)
	if (image->bpp != 4)
		ConvertToBPP(image, 4);

	// check if we can scale
	if (sxCheck(4, image->bpp, image->width, image->height) != SCALEX_OK)
	{
		//Warning("Image_Scale2x_Super2x: cannot scale image %ix%i bpp %i\n", image->width, image->height, image->bpp);
		return;
	}

	// get out desired sizes
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
	byte *alpha = NULL;
	if (image->hasAlpha)
	{
		alpha = (byte *)mem_alloc(w * h);
		in  = FreeImage_GetBits(image->bitmap);
		end = in + w*h*4;
		out = alpha;
		while(in < end)
		{
			*out++ = in[3];
			if (in[3] < opt_binaryAlphaMin)
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

	// scale with lanczos
	FIBITMAP *lanczos = fiRescale(image->bitmap, nw, nh, FILTER_LANCZOS3, false);

	// scale with scale 2x and backscale to desired size
	fiBindToImage(fiRescale(fiScale2x(image->bitmap, 4, false), nw, nh, FILTER_LANCZOS3, true), image);

	// blend lanczos over scale2x
	fiCombine(image->bitmap, lanczos, COMBINE_RGB, 0.4f, true);

	// scale alpha as a second pass
	if (!alpha)
		ConvertToBPP(image, 3);
	else
	{
		FIBITMAP *alpha_scaled = fiRescale(fiScale2x(alpha, w, h, 1, 4, true), nw, nh, FILTER_CATMULLROM, true);
		alpha_scaled = fiSharpen(alpha_scaled, 1.2f, 1, true);
		fiCombine(image->bitmap, alpha_scaled, COMBINE_R_TO_ALPHA, 1.0, true);
		fiBindToImage(fiFixTransparentPixels(image->bitmap), image);
	}

	image->scale = image->scale * 2;
}

// scale for 2x
void Image_Scale2x_Scale2x(LoadedImage *image)
{
	if (!image->bitmap)
		return;

	// scale2x does not allows BPP = 3
	// convert to 4 (and convert back after finish)
	if (image->bpp != 4)
		ConvertToBPP(image, 4);

	// check if we can scale
	if (sxCheck(2, image->bpp, image->width, image->height) != SCALEX_OK)
	{
		//Warning("Image_Scale2x_Scale2x: cannot scale image %ix%i bpp %i\n", image->width, image->height, image->bpp);
		return;
	}

	// scale
	int w = image->width;
	int h = image->height;
	FIBITMAP *scaled = fiCreate(w*2, h*2, 4);
	sxScale(2, FreeImage_GetBits(scaled), w*2*4, FreeImage_GetBits(image->bitmap), w*4, 4, w, h);
	fiBindToImage(scaled, image);

	// finish
	if (!image->hasAlpha)
		ConvertToBPP(image, 3);
	image->scale = image->scale * 2;
}

// scale image using different scale technique
void Image_Scale2x(LoadedImage *image, SCALER scaler, bool makePowerOfTwo)
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
		Image_Scale2x_Scale2x(image);
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
	image->scale = image->scale * 2;
}

// scale for 4x and then backscale 1/2 for best quality
void Image_MakePowerOfTwo(LoadedImage *image)
{
	int w, h;

	if (!image->bitmap)
		return;
	w = NextPowerOfTwo(image->width);
	h = NextPowerOfTwo(image->height);
	if (w == image->width && h == image->height)
		return;
	fiBindToImage(fiRescale(image->bitmap, w, h, FILTER_LANCZOS3, false), image);
}

// make image's alpha binary
void Image_MakeAlphaBinary(LoadedImage *image, int thresh)
{
	if (!image->bitmap)
		return;
	if (!image->hasAlpha)
		return;
	if (image->bpp != 4)
		return;
	
	byte *in = FreeImage_GetBits(image->bitmap);
	byte *end = in + image->width * image->height * image->bpp;
	while(in < end)
	{
		in[3] = (in[3] < opt_binaryAlphaCenter) ? 0 : 255;
		in += 4;
	}
	image->hasGradientAlpha = false;
}

byte *Image_GetData(LoadedImage *image)
{
	if (!image->bitmap)
		return NULL;
	return (byte *)FreeImage_GetBits(image->bitmap);
}


size_t Image_GetDataSize(LoadedImage *image)
{
	return image->width * image->height * image->bpp;
}

bool Image_WriteTarga(char *filename, int width, int height, int bpp, byte *data, bool flip)
{
	byte tga[18];
	FILE *f;
	int i;

	f = fopen(filename, "wb");
	if (!f)
		return false;
	memset(tga, 0, 18);
	tga[2]  = 2;
	tga[12] = (width >> 0) & 0xFF;
	tga[13] = (width >> 8) & 0xFF;
	tga[14] = (height >> 0) & 0xFF;
	tga[15] = (height >> 8) & 0xFF;
	tga[16] = bpp * 8;
	fwrite(tga, 18, 1, f);
	if (!flip)
		fwrite(data, width * height * bpp, 1, f);
	else
	{
		byte *pixels = (byte *)mem_alloc(width * height * bpp);
		byte *out = pixels;
		if (bpp == 4)
		{
			for (i = height-1; i >=0; i--)
			{
				byte *in = data + i * width * bpp;
				byte *end = in + width * bpp;
				while(in < end)
				{
					out[0] = in[2];
					out[1] = in[1];
					out[2] = in[0];
					out[3] = in[3];
					out+=4;
					in+=4;
				}
			}
		}
		else if (bpp == 3)
		{
			for (i = height-1; i >=0; i--)
			{
				byte *in = data + i * width * bpp;
				byte *end = in + width * bpp;
				while(in < end)
				{
					out[0] = in[2];
					out[1] = in[1];
					out[2] = in[0];
					out+=3;
					in+=3;
				}
			}
		}
		fwrite(pixels, width * height * bpp, 1, f);
		mem_free(pixels);
	}
	fclose(f);
	return true;
}

bool Image_Save(LoadedImage *image, char *filename)
{
	return fiSave(image->bitmap, FIF_UNKNOWN, filename);
}

bool Image_Test(LoadedImage *image, char *filename)
{
	return Image_WriteTarga(filename, FreeImage_GetWidth(image->bitmap), FreeImage_GetHeight(image->bitmap), FreeImage_GetBPP(image->bitmap) / 8, FreeImage_GetBits(image->bitmap), true);
}

/*
==========================================================================================

  IMAGE LOAD

==========================================================================================
*/

void LoadFinish(LoadedImage *image)
{
	if (!image->bitmap)
		return;

	image->scale = 1;

	// we want bitmap image type
	image->bitmap = fiConvertType(image->bitmap, FIT_BITMAP);

	// check color type (only RGB and RGBA allowed)
	FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(image->bitmap);
	image->bpp = FreeImage_GetBPP(image->bitmap) / 8;
	if (colorType == FIC_PALETTE)
	{
		if (FreeImage_IsTransparent(image->bitmap))
			ConvertToBPP(image, 4);
		else
			ConvertToBPP(image, 3);
	}
	else if (image->bpp != 3 && image->bpp != 4)
		ConvertToBPP(image, 3);

	// check alpha
	image->hasAlpha = false;
	image->hasGradientAlpha = false;
	if (image->bpp == 4)
	{
		int  num_grad = 0;
		int  need_grad = (image->width + image->height) / 4;
		long pixels = image->width*image->height*4;
		byte *data = FreeImage_GetBits(image->bitmap);
		image->hasAlpha = true;
		if (!opt_detectBinaryAlpha)
			image->hasGradientAlpha = true;
		else
		{
			for (long i = 0; i < pixels; i+= 4)
			{
				if (data[i+3] < opt_binaryAlphaMax && data[i+3] > opt_binaryAlphaMin)
					num_grad++;
				if (num_grad > need_grad)
				{
					image->hasGradientAlpha = true;
					break;
				}
			}
		}
	}
}

byte quake_palette[768] = { 0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,243,147,255,247,199,255,255,255,159,91,83 };

// a quake sprite loader
void LoadImage_QuakeSprite(FS_File *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	byte *buf;

	if (filesize < 36 || *(unsigned int *)(filedata + 4) != 32)
		Warning("%s%s.%s : only supports loading of Quake SPR32", file->path.c_str(), file->name.c_str(), file->ext.c_str());
	else
	{
		// load frames
		int framenum = 0;
		int numframes = *(int *)(filedata + 24);
		LoadedImage *frame = image;
		filesize -= 36;
		buf = filedata + 36;
		while(1)
		{
			// group sprite?
			if (*(unsigned int *)(buf) == 1) 
				buf += *(unsigned int *)(buf + 4) * 4 + 4;
			// fill data
			frame->useTexname = true;
			sprintf(frame->texname, "%s.%s_%i", file->name.c_str(), file->ext.c_str(), framenum);
			frame->width  = *(unsigned int *)(buf + 12);
			frame->height = *(unsigned int *)(buf + 16);
			frame->filesize = frame->width*frame->height*4;
			filesize -= 20; buf += 20;
			fiLoadDataRaw(frame->width, frame->height, 4, buf, filesize, NULL, false, frame);
			LoadFinish(frame);
			// go next frame
			framenum++;
			filesize -= frame->filesize;
			     buf += frame->filesize;
			if (framenum >= numframes)
				break;
			frame->next = Image_Create();
			frame = frame->next;
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
				fiLoadDataRaw(texwidth, texheight, 1, (byte *)(buf + textureoffsets[texnum] + texmipofs), texwidth * texheight, quake_palette, false, tex);
				if (texname[0] == '*')
					texname[0] = '#';
				tex->useTexname = true;
				sprintf(tex->texname, "%s/%s", file->name.c_str(), texname);
				LoadFinish(tex);
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
	LoadFinish(image);
}

void Image_Load(FS_File *file, LoadedImage *image)
{
	unsigned int formatCC;
	size_t filesize;
	byte *filedata;

	filedata = FS_LoadFile(file, &filesize);
	if (!filedata)
		return;

	ClearImage(image);
	formatCC = *(unsigned int *)filedata;
	if (formatCC == MAKEFOURCC('I','D','S','P'))
		LoadImage_QuakeSprite(file, filedata, filesize, image);
	else if (formatCC == 29)
		LoadImage_QuakeBSP(file, filedata, filesize, image);
	else
		LoadImage_Generic(file, filedata, filesize, image);
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