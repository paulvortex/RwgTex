// freeimage.h

#pragma once

// freeimage lib
#include <freeimage.h>
#pragma comment(lib, "FreeImageDLL.lib")

// blend modes
typedef enum
{
	COMBINE_RGB,
	COMBINE_ALPHA,
	COMBINE_R_TO_ALPHA,
	COMBINE_ALPHA_TO_RGB,
	COMBINE_ADD,
	COMBINE_MIN,
	COMBINE_MAX
}FREE_IMAGE_COMBINE;

// helper functions

// remove bitmap and set pointer
FIBITMAP *_fiFree(FIBITMAP *bitmap, char *file, int line);
#define fiFree(bitmap) _fiFree(bitmap, __FILE__, __LINE__)

// create empty bitmap
FIBITMAP *fiCreate(int width, int height, int bpp);

// clone bitmap
FIBITMAP *fiClone(FIBITMAP *bitmap);

// rescale bitmap
FIBITMAP *fiRescale(FIBITMAP *bitmap, int width, int height, FREE_IMAGE_FILTER filter, bool removeSource);

// bind bitmap to image
bool fiBindToImage(FIBITMAP *bitmap, LoadedImage *image, FREE_IMAGE_FORMAT format = FIF_UNKNOWN);

// load bitmap from memory
bool fiLoadData(FREE_IMAGE_FORMAT format, FS_File *file, byte *data, size_t datasize, LoadedImage *image);

// load bitmap from raw data
bool fiLoadDataRaw(int width, int height, int bpp, byte *data, size_t datasize, byte *palette, bool dataIsBGR, LoadedImage *image);

// load bitmap from file
bool fiLoadFile(FREE_IMAGE_FORMAT format, const char *filename, LoadedImage *image);

// save bitmap to file
bool fiSave(FIBITMAP *bitmap, FREE_IMAGE_FORMAT format, const char *filename);

// combine two bitmaps
void fiCombine(FIBITMAP *source, FIBITMAP *combine, FREE_IMAGE_COMBINE mode, float blend, bool destroyCombine);

// converts image to requested BPP
FIBITMAP *fiConvertBPP(FIBITMAP *bitmap, int want_bpp);

// converts image to requested type
FIBITMAP *fiConvertType(FIBITMAP *bitmap, FREE_IMAGE_TYPE want_type);

// scale bitmap with scale2x
FIBITMAP *fiScale2x(byte *data, int width, int height, int bpp, int scaler, bool freeData);
FIBITMAP *fiScale2x(FIBITMAP *bitmap, int scaler, bool freeSource);

// apply a custom filter matrix to bitmap
FIBITMAP *fiFilter(FIBITMAP *bitmap, double *m, double scale, double bias, int iteractions, bool removeSource);

// apply gaussian blur to bitmap
FIBITMAP *fiBlur(FIBITMAP *bitmap, int iteractions, bool removeSource);

// apply sharpenm, factor < 1 blurs, > 1 sharpens
FIBITMAP *fiSharpen(FIBITMAP *bitmap, float factor, int iteractions, bool removeSource);

// fix transparent pixels for alpha blending
FIBITMAP *fiFixTransparentPixels(FIBITMAP *bitmap);