////////////////////////////////////////////////////////////////
//
// RWGTEX - freeimage helper class
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

size_t fiGetSize(FIBITMAP *bitmap)
{
	size_t size;

	if (!bitmap)
		return 0;
	size = sizeof(FIBITMAP);
	if (FreeImage_HasPixels(bitmap))
		size += FreeImage_GetWidth(bitmap)*FreeImage_GetHeight(bitmap)*FreeImage_GetBPP(bitmap)/8;
	return size;
}

/*
==========================================================================================

  FREE HELP ROUTINES

==========================================================================================
*/

// remove bitmap and set pointer
FIBITMAP *_fiFree(FIBITMAP *bitmap, char *file, int line)
{
	if (bitmap)
		if (_mem_sentinel_free("fiFree", bitmap, file, line))
			FreeImage_Unload(bitmap);
	return NULL;
}

// create empty bitmap
FIBITMAP *fiCreate(int width, int height, int bpp)
{
	FIBITMAP *bitmap;

	bitmap = FreeImage_Allocate(width, height, bpp * 8, 0x00FF0000, 0x0000FF00, 0x000000FF);
	if (!bitmap)
		Error("fiCreate: failed to allocate new bitmap (%ix%i bpp %i)", width, height, bpp);
	mem_sentinel("fiCreate", bitmap, fiGetSize(bitmap));
	return bitmap;
}

// clone bitmap
FIBITMAP *fiClone(FIBITMAP *bitmap)
{
	if (!bitmap)
		return NULL;
	FIBITMAP *cloned = FreeImage_Clone(bitmap);
	mem_sentinel("fiClone", cloned, fiGetSize(cloned));
	return cloned;
}

// rescale bitmap
FIBITMAP *fiRescale(FIBITMAP *bitmap, int width, int height, FREE_IMAGE_FILTER filter, bool removeSource)
{
	FIBITMAP *scaled;

	scaled = FreeImage_Rescale(bitmap, width, height, filter);
	if (!scaled)
		Error("fiRescale: failed to rescale bitmap to %ix%i", width, height);
	if (removeSource)
		fiFree(bitmap);
	mem_sentinel("fiRescale", scaled, fiGetSize(scaled));
	return scaled;
}

// bind bitmap to image
bool fiBindToImage(FIBITMAP *bitmap, LoadedImage *image, FREE_IMAGE_FORMAT format)
{
	image->bitmap = fiFree(image->bitmap);
	image->width = 0;
	image->height = 0;
	image->colorSwap = false;

	if (!bitmap)
		return false;
	if (!FreeImage_HasPixels(bitmap))
	{
		fiFree(bitmap);
		return false;
	}

	// fill image
	image->colorSwap = true;
	if (format == FIF_TARGA || format == FIF_BMP || format == FIF_JPEG)
		FreeImage_FlipVertical(bitmap);
	image->bitmap = bitmap;
	image->width = FreeImage_GetWidth(image->bitmap);
	image->height = FreeImage_GetHeight(image->bitmap);
	return true;
}

// load bitmap from memory
bool fiLoadData(FREE_IMAGE_FORMAT format, FS_File *file, byte *data, size_t datasize, LoadedImage *image)
{
	FIMEMORY *memory;

	// get format
	if (format == FIF_UNKNOWN)
		format = FreeImage_GetFIFFromFilename(file->ext.c_str());
	if (format == FIF_UNKNOWN)
	{
		Warning("%s%s.%s : FreeImage unabled to load file (unknown format)\n", file->path.c_str(), file->name.c_str(), file->ext.c_str());
		return NULL;
	}
	if (!FreeImage_FIFSupportsReading(format))
	{
		Warning("%s%s.%s : FreeImage is not supporting loading of this format (%i)\n", file->path.c_str(), file->name.c_str(), file->ext.c_str(), format);
		return NULL;
	}

	// load from memory
	memory = FreeImage_OpenMemory(data, datasize);
	FIBITMAP *bitmap = FreeImage_LoadFromMemory(format, memory, 0);
	FreeImage_CloseMemory(memory);
	mem_sentinel("fiLoadData", bitmap, fiGetSize(bitmap));
	return fiBindToImage(bitmap, image, format);
}

// load bitmap from raw data
bool fiLoadDataRaw(int width, int height, int bpp, byte *data, size_t datasize, byte *palette, bool dataIsBGR, LoadedImage *image)
{
	FIBITMAP *bitmap;
	byte *bits;

	if (width < 1 || height < 1 || !data)
		return NULL;
	// create image
	bitmap = fiCreate(width, height, bpp);
	if (!bitmap)
		return NULL;
	// fill pixels
	bits = FreeImage_GetBits(bitmap);
	if ((size_t)(width*height*bpp) > datasize)
	{
		fiFree(bitmap);
		Warning("fiLoadDataRaw : failed to read stream (unexpected end of data)\n");
		return NULL;
	}
	// fill colors
	memcpy(bits, data, width*height*bpp);
	// swap colors if needed (FreeImage loads as BGR)
	if (!dataIsBGR)
	{
		if (bpp == 3 || bpp == 4)
		{
			byte *end = data + width*height*bpp;
			while(data < end)
			{
				bits[0] = data[2];
				bits[2] = data[0];
				data += bpp;
				bits += bpp;
			}
		}
		else
			Error("fiLoadDataRaw: bpp %i not supporting color swap", bpp);
	}
	// fill colormap
	// FreeImage loads as BGR
	if (bpp == 1 && palette)
	{
		RGBQUAD *pal = FreeImage_GetPalette(bitmap);
		if (pal)
		{
			int i;
			if (dataIsBGR)
			{
				// load standart
				for (i = 0; i < 256; i++)
				{
					pal[i].rgbRed = palette[i*3 + 0];
					pal[i].rgbGreen = palette[i*3 + 1];
					pal[i].rgbBlue = palette[i*3 + 2];
				}
			}
			else
			{
				// load swapped
				for (i = 0; i < 256; i++)
				{
					pal[i].rgbRed = palette[i*3 + 2];
					pal[i].rgbGreen = palette[i*3 + 1];
					pal[i].rgbBlue = palette[i*3 + 0];
				}
			}
		}
	}
	return fiBindToImage(bitmap, image);
}

// load bitmap from file
bool fiLoadFile(FREE_IMAGE_FORMAT format, const char *filename, LoadedImage *image)
{
	if (format == FIF_UNKNOWN)
		format = FreeImage_GetFIFFromFilename(filename);
	if (format == FIF_UNKNOWN)
	{
		Warning("%s : FreeImage unabled to load file (unknown format)\n", filename);
		return NULL;
	}
	if (!FreeImage_FIFSupportsReading(format))
	{
		Warning("%s : FreeImage is not supporting loading of this format (%i)\n", filename, format);
		return NULL;
	}
	FIBITMAP *bitmap = FreeImage_Load(format, filename, 0);
	mem_sentinel("fiLoadFile", bitmap, fiGetSize(bitmap));
	return fiBindToImage(bitmap, image, format);
}

// save bitmap to file
bool fiSave(FIBITMAP *bitmap, FREE_IMAGE_FORMAT format, const char *filename)
{
	if (format == FIF_UNKNOWN)
		format = FreeImage_GetFIFFromFilename(filename);
	if (format == FIF_UNKNOWN)
	{
		Warning("%s : FreeImage unabled to save file (unknown format)\n", filename);
		return false;
	}
	if (!FreeImage_FIFSupportsWriting(format))
	{
		Warning("%s : FreeImage is not supporting riting of this format (%i)\n", filename, format);
		return false;
	}
	return FreeImage_Save(format, bitmap, filename) ? true : false;
}

/*
==========================================================================================

  COMBINE

==========================================================================================
*/

// combine two bitmaps
void fiCombine(FIBITMAP *source, FIBITMAP *combine, FREE_IMAGE_COMBINE mode, float blend, bool destroyCombine)
{
	if (!source || !combine)
		return;

	// check
	int cbpp = FreeImage_GetBPP(combine)/8;
	int sbpp = FreeImage_GetBPP(source)/8;
	if (cbpp != 1 && cbpp != 3 && cbpp != 4)
		Error("fiCombine: combined bitmap should be 8, 24 or 32-bit");
	if (sbpp != 1 && sbpp != 3 && sbpp != 4)
		Error("fiCombine: source bitmap should be 8, 24 or 32-bit");
	if (FreeImage_GetWidth(source) != FreeImage_GetWidth(combine) || FreeImage_GetHeight(source) != FreeImage_GetHeight(combine))
		Error("fiCombine: source and blend bitmaps having different width/height/BPP");

	// combine
	float rb = 1 - blend;
	byte *in = FreeImage_GetBits(combine);
	byte *end = in + FreeImage_GetWidth(combine)*FreeImage_GetHeight(combine)*cbpp;
	byte *out = FreeImage_GetBits(source);
	switch(mode)
	{
		case COMBINE_RGB:
			if (sbpp != 3 && sbpp != 4)
				Warning("fiCombine(COMBINE_RGB): source bitmap should be RGB or RGBA");
			else if (cbpp != 3 && cbpp != 4)
				Warning("fiCombine(COMBINE_RGB): combined bitmap should be RGB or RGBA");
			else
			{
				while(in < end)
				{
					out[0] = (byte)floor(out[0]*rb + in[0]*blend + 0.5);
					out[1] = (byte)floor(out[1]*rb + in[1]*blend + 0.5);
					out[2] = (byte)floor(out[2]*rb + in[2]*blend + 0.5);
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		case COMBINE_ALPHA:
			if (sbpp != 4)
				Warning("fiCombine(COMBINE_ALPHA): source bitmap should be RGBA");
			else if (cbpp != 4)
				Warning("fiCombine(COMBINE_ALPHA): combined bitmap should be RGBA");
			else
			{
				while(in < end)
				{
					out[3] = (byte)floor(out[3]*rb + in[3]*blend + 0.5);
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		case COMBINE_R_TO_ALPHA:
			if (sbpp != 4)
				Warning("fiCombine(COMBINE_R_TO_ALPHA): source bitmap should be RGBA");
			else
			{
				while(in < end)
				{
					out[3] = (byte)floor(out[3]*rb + in[0]*blend + 0.5);
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		case COMBINE_ALPHA_TO_RGB:
			if (sbpp != 3 && sbpp != 4)
				Warning("fiCombine(COMBINE_ALPHA_TO_RGB): source bitmap should be RGB or RGBA");
			else if (cbpp != 4)
				Warning("fiCombine(COMBINE_ALPHA_TO_RGB): combined bitmap should be RGBA");
			else
			{
				while(in < end)
				{
					out[0] = (byte)floor(out[0]*rb + in[3]*blend + 0.5);
					out[1] = (byte)floor(out[0]*rb + in[3]*blend + 0.5);
					out[2] = (byte)floor(out[0]*rb + in[3]*blend + 0.5);
					out[3] = 255;
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		case COMBINE_ADD:
			if (sbpp != 3 && sbpp != 4)
				Warning("fiCombine(COMBINE_ADD): source bitmap should be RGB or RGBA");
			else if (cbpp != 3 && cbpp != 4)
				Warning("fiCombine(COMBINE_ADD): combined bitmap should be RGB or RGBA");
			else
			{
				while(in < end)
				{
					out[0] = (byte)max(0, min(floor(out[0] + in[0]*blend), 255));
					out[1] = (byte)max(0, min(floor(out[1] + in[1]*blend), 255));
					out[2] = (byte)max(0, min(floor(out[2] + in[2]*blend), 255));
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		case COMBINE_MIN:
			if (sbpp != 3 && sbpp != 4)
				Warning("fiCombine(COMBINE_MIN): source bitmap should be RGB or RGBA");
			else if (cbpp != 3 && cbpp != 4)
				Warning("fiCombine(COMBINE_MIN): combined bitmap should be RGB or RGBA");
			else
			{
				while(in < end)
				{
					out[0] = min(out[0], in[0]);
					out[1] = min(out[1], in[1]);
					out[2] = min(out[2], in[2]);
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		case COMBINE_MAX:
			if (sbpp != 3 && sbpp != 4)
				Warning("fiCombine(COMBINE_MAX): source bitmap should be RGB or RGBA");
			else if (cbpp != 3 && cbpp != 4)
				Warning("fiCombine(COMBINE_MAX): combined bitmap should be RGB or RGBA");
			else
			{
				while(in < end)
				{
					out[0] = max(out[0], in[0]);
					out[1] = max(out[1], in[1]);
					out[2] = max(out[2], in[2]);
					out += sbpp;
					in  += cbpp;
				}
			}
			break;
		default:
			Warning("fiCombine: bad mode");
			break;
	}
	if (destroyCombine)
		fiFree(combine);
}

// converts image to requested BPP
FIBITMAP *fiConvertBPP(FIBITMAP *bitmap, int want_bpp)
{
	FIBITMAP *converted;

	// check if needs conversion
	int bpp = FreeImage_GetBPP(bitmap) / 8;
	if (bpp == want_bpp)
		return bitmap;

	// convert
	if (want_bpp == 3)
		converted = FreeImage_ConvertTo24Bits(bitmap);
	else if (want_bpp == 4)
		converted = FreeImage_ConvertTo32Bits(bitmap);
	else
		Error("fiConvertBPP: bad bpp %i", bpp);
	if (!converted)
	{
		Warning("fiConvertBPP: conversion failed");
		return bitmap;
	}
	if (converted == bitmap)
		return bitmap;
	fiFree(bitmap);
	mem_sentinel("fiConvertBPP", converted, fiGetSize(converted));
	return converted;
}

// converts image to requested type
FIBITMAP *fiConvertType(FIBITMAP *bitmap, FREE_IMAGE_TYPE want_type)
{
	// check if needs conversion
	FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);
	if (type == want_type)
		return bitmap;

	// convert
	FIBITMAP *converted = FreeImage_ConvertToType(bitmap, want_type);
	if (!converted)
	{
		Warning("fiConvertType: conversion failed");
		return bitmap;
	}
	if (converted == bitmap)
		return bitmap;
	fiFree(bitmap);
	mem_sentinel("fiConvertType", converted, fiGetSize(converted));
	return converted;
}

/*
==========================================================================================
 
 SCALE2x

==========================================================================================
*/

// scale bitmap with scale2x
FIBITMAP *fiScale2x(byte *data, int width, int height, int bpp, int scaler, bool freeData)
{
	if (sxCheck(scaler, bpp, width, height) != SCALEX_OK)
		return NULL;

	FIBITMAP *scaled = fiCreate(width*scaler, height*scaler, bpp);
	sxScale(scaler, FreeImage_GetBits(scaled), width*scaler*bpp, data, width*bpp, bpp, width, height);
	if (freeData)
		mem_free(data);

	// bug: 1-byte pics sometimes get crapped, convert to 32 bits
	if (bpp == 1)
	{
		FIBITMAP *fixed = fiCreate(width*scaler, height*scaler, 4);

		byte *in = FreeImage_GetBits(scaled);
		byte *end = in + width*scaler*height*scaler;
		byte *out = FreeImage_GetBits(fixed);
		while(in < end)
		{
			out[0] = in[0];
			out[1] = in[0];
			out[2] = in[0];
			out[3] = in[0];
			out   += 4;
			in    += 1;
		}
		fiFree(scaled);

		return fixed;
	}

	return scaled;
}

FIBITMAP *fiScale2x(FIBITMAP *bitmap, int scaler, bool freeSource)
{
	FIBITMAP *scaled = fiScale2x(FreeImage_GetBits(bitmap), FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap), FreeImage_GetBPP(bitmap)/8, scaler, false);
	if (!scaled)
		return fiClone(bitmap);
	if (freeSource)
		fiFree(bitmap);
	return scaled;
}

/*
==========================================================================================

  BLUR AND SHARPEN

  picked from Developer's ImageLib

==========================================================================================
*/

// apply a custom filter matrix to bitmap
FIBITMAP *fiFilter(FIBITMAP *bitmap, double *m, double scale, double bias, int iteractions, bool removeSource)
{
    int	i, t, x, y, c, lastx, lasty, ofs[9];
	FIBITMAP *filtered, *filtered2, *temp;
	double n;

	if (!bitmap)
		return NULL;

	int width = FreeImage_GetWidth(bitmap);
	int height = FreeImage_GetHeight(bitmap);
	int bpp  = FreeImage_GetBPP(bitmap) / 8;
	int pitch = width * bpp;

	// check bitmap type
	if (bpp != 1 && bpp != 3 && bpp != 4)
	{
		Warning("fiFilter: only supported for 8, 24 or 32-bit bitmaps");
		return fiClone(bitmap);
	}

	// many iteractinos requires 2 buffers
	filtered = fiClone(bitmap);
	if (iteractions > 1)
	{
		if (removeSource)
			filtered2 = bitmap;
		else
			filtered2 = fiClone(bitmap);
	}
	
	// do the job
	byte *in = FreeImage_GetBits(bitmap);
	byte *out = FreeImage_GetBits(filtered);
	for ( ; iteractions > 0; iteractions--)
	{
		lastx = width  - 1;
		lasty = height - 1;
		for (y = 1; y < lasty; y++)
		{

			for (x = 1; x < lastx; x++)
			{
				ofs[4] = ((y  ) * width + (x  )) * bpp;
				ofs[0] = ((y-1) * width + (x-1)) * bpp;
				ofs[1] = ((y-1) * width + (x  )) * bpp;
				ofs[2] = ((y-1) * width + (x+1)) * bpp;
				ofs[3] = ((y  ) * width + (x-1)) * bpp;
				ofs[5] = ((y  ) * width + (x+1)) * bpp;
				ofs[6] = ((y+1) * width + (x-1)) * bpp;
				ofs[7] = ((y+1) * width + (x  )) * bpp;
				ofs[8] = ((y+1) * width + (x-1)) * bpp;
				// sample 0 channel
				n = in[ofs[0]]*m[0] + in[ofs[1]]*m[1] + in[ofs[2]]*m[2] + in[ofs[3]]*m[3] + in[ofs[4]]*m[4] + in[ofs[5]]*m[5] + in[ofs[6]]*m[6] + in[ofs[7]]*m[7] + in[ofs[8]]*m[8];
				t = (unsigned int)fabs((n/scale) + bias);
				out[ofs[4]] = min(t, 255);
				// sample rest channels
				for (c = 1; c < bpp; c++)
				{
					n = in[ofs[0]+c]*m[0] + in[ofs[1]+c]*m[1] + in[ofs[2]+c]*m[2] + in[ofs[3]+c]*m[3] + in[ofs[4]+c]*m[4] + in[ofs[5]+c]*m[5] + in[ofs[6]+c]*m[6] + in[ofs[7]+c]*m[7] + in[ofs[8]+c]*m[8];
					t = (unsigned int)fabs(n/scale + bias);
					out[ofs[4]+c] = min(t, 255);
				}
			}
		}

		// copy 4 corners
		for (c = 0; c < bpp; c++)
		{
			out[c] = in[c];
			out[pitch - bpp + c] = in[pitch - bpp + c];
			out[(height - 1) * pitch + c] = in[(height - 1) * pitch + c];
			out[height * pitch - bpp + c] = in[height * pitch - bpp + c];
		}

		// if we only copy the edge pixels, then they receive no filtering, making them
		// look out of place after several passes of an image.  So we filter the edge
		// rows/columns, duplicating the edge pixels for one side of the "matrix"

		// first row
		for (x = 1; x < (width - 1); x++)
		{
			for (c = 0; c < bpp; c++)
			{
				n = in[(x-1)*bpp+c]*m[0] + in[x*bpp+c]*m[1] + in[(x+1)*bpp+c]*m[2] + in[(x-1)*bpp+c]*m[3] + in[x*bpp+c]*m[4] + in[(x+1)*bpp+c]*m[5] + in[(width+(x-1))*bpp+c]*m[6] + in[(width+(x))*bpp+c]*m[7] + in[(width+(x-1))*bpp+c]*m[8];
				t = (unsigned int)fabs(n/scale + bias);
				out[x*bpp+c] = min(t, 255);
			}
		}

		// last row
		y = (height - 1) * pitch;
		for (x = 1; x < (width - 1); x++)
		{
			for (c = 0; c < bpp; c++)
			{
				n = in[y-pitch+(x-1)*bpp+c]*m[0] + in[y-pitch+x*bpp+c]*m[1] + in[y-pitch+(x+1)*bpp+c]*m[2] + in[y+(x-1)*bpp+c]*m[3] + in[y+x*bpp+c]*m[4] + in[y+(x+1)*bpp+c]*m[5] + in[y+(x-1)*bpp+c]*m[6] + in[y+x*bpp+c]*m[7] + in[y+(x-1)*bpp+c]*m[8];
				t = (unsigned int)fabs(n/scale + bias);
				out[y+x*bpp+c] = min(t, 255);
			}
		}

		// left side
		for (i = 1, y = pitch; i < height-1; i++, y += pitch)
		{
			for (c = 0; c < bpp; c++)
			{
				n=in[y-pitch+c]*m[0] + in[y-pitch+bpp+c]*m[1] + in[y-pitch+2*bpp+c]*m[2] + in[y+c]*m[3] + in[y+bpp+c]*m[4] + in[y+2*bpp+c]*m[5] + in[y+pitch+c]*m[6] + in[y+pitch+bpp+c]*m[7] + in[y+pitch+2*bpp+c]*m[8];
				t = (unsigned int)fabs(n/scale + bias);
				out[y + c] = min(t, 255);
			}
		}

		// right side
		for (i = 1, y = pitch*2-bpp; i < height-1; i++, y += pitch)
		{
			for (c = 0; c < bpp; c++)
			{
				n = in[y-pitch+c]*m[0] + in[y-pitch+bpp+c]*m[1] + in[y-pitch+2*bpp+c]*m[2] + in[y+c]*m[3] + in[y+bpp+c]*m[4] + in[y+2*bpp+c]*m[5] + in[y+pitch+c]*m[6] + in[y+pitch+bpp+c]*m[7] + in[y+pitch+2*bpp+c]*m[8];
				t = (unsigned int)fabs(n/scale + bias);
				out[y+c] = min(t, 255);
			}
		}

		// swap buffers
		if (out == FreeImage_GetBits(filtered))
		{
			in = FreeImage_GetBits(filtered);
			out = FreeImage_GetBits(filtered2);
		}
		else
		{
			in = FreeImage_GetBits(filtered2);
			out = FreeImage_GetBits(filtered);
		}
	}

	// remove source pic
	temp = (out == FreeImage_GetBits(filtered)) ? filtered2 : filtered;
	if (temp == filtered)
	{
		if (filtered2 != bitmap)
		{
			fiFree(filtered2);
			if (removeSource)
				fiFree(bitmap);
		}
		else if (removeSource)
			fiFree(bitmap);
	}
	else
	{
		if (filtered != bitmap)
		{
			fiFree(filtered);
			if (removeSource)
				fiFree(bitmap);
		}
		else if (removeSource)
			fiFree(bitmap);
	}
	return temp;
}

// apply gaussian blur to bitmap
FIBITMAP *fiBlur(FIBITMAP *bitmap, int iteractions, bool removeSource)
{
	double k[9], scale = 0;
	int i;

	if (!bitmap)
		return NULL;

	// check bitmap type
	int bpp = FreeImage_GetBPP(bitmap) / 8;
	if (bpp != 1 && bpp != 3 && bpp != 4)
	{
		Warning("fiBlur: only supported for 8, 24 or 32-bit bitmaps");
		return fiClone(bitmap);
	}

	// create kernel
	k[0] = 1.0f; k[1] = 2.0f; k[2] = 1.0f; 
	k[3] = 2.0f; k[4] = 4.0f; k[5] = 2.0f; 
	k[6] = 1.0f; k[7] = 2.0f; k[8] = 1.0f;
	for (i = 0; i < 9; i++)
		scale += k[i];

	// blur
	return fiFilter(bitmap, k, scale, scale * 0.0625, iteractions, removeSource);
}

// apply sharpen
// factor < 1 blurs, > 1 sharpens
FIBITMAP *fiSharpen(FIBITMAP *bitmap, float factor, int iteractions, bool removeSource)
{
	byte *in, *out, *end;
	double k[9], scale = 0;

	// check bitmap type
	int bpp = FreeImage_GetBPP(bitmap) / 8;
	if (bpp != 1 && bpp != 3 && bpp != 4)
	{
		Warning("fiSharpen: only supported for 8, 24 or 32-bit bitmaps");
		return fiClone(bitmap);
	}

	// get blurred image
	k[0] = 1.0f; k[1] = 2.0f; k[2] = 1.0f; 
	k[3] = 2.0f; k[4] = 4.0f; k[5] = 2.0f; 
	k[6] = 1.0f; k[7] = 2.0f; k[8] = 1.0f;
	for (int i = 0; i < 9; i++)
		scale += k[i];
	FIBITMAP *blurred = fiFilter(bitmap, k, scale, scale * 0.0625, 2, false);

	// sharpen
	FIBITMAP *sharpened = fiClone(bitmap);
	float rf = 1 - factor;
	for ( ; iteractions > 0; iteractions--)
	{
		in = FreeImage_GetBits(blurred);
		end = in + FreeImage_GetWidth(blurred)*FreeImage_GetHeight(blurred)*bpp;
		out = FreeImage_GetBits(sharpened);
		while(in < end)
		{
			*out = min(255, max(0, (int)(*in*rf + *out*factor)));
			out++;
			in++;
		}
	}
	fiFree(blurred);
	if (removeSource)
		fiFree(bitmap);

	return sharpened;
}

/*
==========================================================================================

  FIX TRANSPARENT PIXELS FOR ALPHA BLENDING

==========================================================================================
*/

// fill rgb data of transparent pixels from non-transparent neighbors to fix 'black aura' effect
FIBITMAP *fiFixTransparentPixels(FIBITMAP *bitmap)
{
	byte *data, *out, *pixel, *npix;
	int x, y, nc, nx, ny, w, h;
	float rgb[3], b, br;

	// check bitmap type
	if (FreeImage_GetBPP(bitmap) != 32)
	{
		Warning("fiSharpen: only supported 32-bit bitmaps");
		return fiClone(bitmap);
	}

	FIBITMAP *filled = fiClone(bitmap);
	data = FreeImage_GetBits(bitmap);
	out  = FreeImage_GetBits(filled);
	w    = FreeImage_GetWidth(bitmap);
	h    = FreeImage_GetHeight(bitmap);
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			pixel = data + w*4*y + x*4;
			if (pixel[3] < 255)
			{
				// fill from neighbors
				nc = 0;
				rgb[0] = 0.0f;
				rgb[1] = 0.0f;
				rgb[2] = 0.0f;
				#define fill(_x,_y) nx = (_x < 0) ? (w - 1) : (_x >= w) ? 0 : _x; ny = (_y < 0) ? (h - 1) : (_y >= h) ? 0 : _y; npix = data + w*4*ny + nx*4; if (npix[3] > opt_binaryAlphaMin) { nc++; rgb[0] += (float)npix[0]; rgb[1] += (float)npix[1]; rgb[2] += (float)npix[2]; }
				fill(x-1,y-1)
				fill(x  ,y-1)
				fill(x+1,y-1)
				fill(x-1,y  )
				fill(x+1,y  )
				fill(x-1,y+1)
				fill(x  ,y+1)
				fill(x+1,y+1)
				#undef fill
				if (nc)
				{
					b = 1.0f - (float)pixel[3] / 255.0f;
					br = 1.0f - b;
					npix = out + w*4*y + x*4;
					*npix++ = (byte)min(255, (pixel[0]*br + (rgb[0]/nc)*b));
					*npix++ = (byte)min(255, (pixel[1]*br + (rgb[1]/nc)*b));
					*npix++ = (byte)min(255, (pixel[2]*br + (rgb[2]/nc)*b));
				}
			}
		}
	}
	return filled;
}