// freeimage.h
#ifndef H_TEX_FREEIMAGE_H
#define H_TEX_FREEIMAGE_H

// freeimage lib
#include "freeimage/freeimage.h"
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

// get image pixel data and pitch (width*bpp, pitch is needed because image lines is aligned to 4)
byte *fiGetData(FIBITMAP *bitmap, int *pitch);

// copy out image pixel data, solves alignment
byte *fiGetUnalignedData(FIBITMAP *bitmap, bool *data_allocated, bool force_allocate);
void fiStoreUnalignedData(FIBITMAP *bitmap, byte *dataptr, int width, int height, int bpp);
void fiFreeUnalignedData(byte *dataptr, bool data_allocated);

// remove bitmap and set pointer
FIBITMAP *_fiFree(FIBITMAP *bitmap, char *file, int line);
#define fiFree(bitmap) _fiFree(bitmap, __FILE__, __LINE__)

// create empty bitmap
FIBITMAP *fiCreate(int width, int height, int bpp);

// clone bitmap
FIBITMAP *fiClone(FIBITMAP *bitmap);

// rescale bitmap
FIBITMAP *fiRescale(FIBITMAP *bitmap, int width, int height, FREE_IMAGE_FILTER filter, bool removeSource);

// rescale bitmap usign nearest neighbor scaling
FIBITMAP *fiRescaleNearestNeighbor(FIBITMAP* bitmap, int new_width, int new_height, bool removeSource);

// bind bitmap to image
bool fiBindToImage(FIBITMAP *bitmap, LoadedImage *image, FREE_IMAGE_FORMAT format = FIF_UNKNOWN, bool keep_color_profile = true);

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
FIBITMAP *fiConvertBPP(FIBITMAP *bitmap, int want_bpp, int want_palette_size, byte *want_external_palette);

// get image palette
bool fiGetPalette(FIBITMAP *bitmap, byte *palette, int palettesize);

// converts image to requested type
FIBITMAP *fiConvertType(FIBITMAP *bitmap, FREE_IMAGE_TYPE want_type);

// scale bitmap with scale2x
FIBITMAP *fiScale2x(byte *data, int pitch, int width, int height, int bpp, int scaler, bool freeData);
FIBITMAP *fiScale2x(FIBITMAP *bitmap, int scaler, bool freeSource);

// apply a custom filter matrix to bitmap
FIBITMAP *fiFilter(FIBITMAP *bitmap, double *m, double scale, double bias, int iteractions, bool removeSource);

// apply gaussian blur to bitmap
FIBITMAP *fiBlur(FIBITMAP *bitmap, int iteractions, bool removeSource);

// apply median filter (source image is deleted)
void fiApplyMedianFilter(FIBITMAP **bitmap, int kernel_x_radius, int kernel_y_radius, int iteractions);

// apply unsharp mask (source image is deleted)
void fiApplyUnsharpMask(FIBITMAP **bitmap, double radius, double amount, double threshold, int iteractions);

// apply sharpen, factor < 1 blurs, > 1 sharpens
FIBITMAP *fiSharpen(FIBITMAP *bitmap, float factor, int iteractions, bool removeSource);

// fix transparent pixels for alpha blending
FIBITMAP *fiFixTransparentPixels(FIBITMAP *bitmap);

#endif