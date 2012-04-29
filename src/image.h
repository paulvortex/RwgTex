// image.h

#pragma once

#include "fs.h"
#include "image.h"
#include <freeimage.h>

typedef struct MipMap_s
{
	int    width;
	int    height;
	int    level;
	byte  *data;
	size_t datasize;

	MipMap_s *nextmip;
}MipMap;

typedef struct LoadedImage_s
{
	// load-time parameters
	size_t	  filesize;

	// FreeImage parameters
	FIBITMAP *bitmap;
	int       width;
	int       height;
	bool      colorSwap; // BGR instead of RGB
	bool      colorPremodulate;
	int       scale;

	// FinishLoad parameters
	int       bpp;              // bits per pixel
	bool      hasAlpha;         // have alpha channel
	bool      hasGradientAlpha; // alpha channel is a gradient type (not binary)

	// special
	MipMap   *mipMaps;       // generated mipmaps
	char      texname[128];  // null if there is no custom texture name
	bool      useTexname;

	// set by DDS exporter
	DWORD    formatCC;

	// next image's frame
	LoadedImage_s *next;
}
LoadedImage;

// scalers
typedef enum
{
	IMAGE_SCALER_BOX,
	IMAGE_SCALER_BILINEAR,
	IMAGE_SCALER_BICUBIC,
	IMAGE_SCALER_BSPLINE,
	IMAGE_SCALER_CATMULLROM,
	IMAGE_SCALER_LANCZOS,
	IMAGE_SCALER_SCALE2X,
	IMAGE_SCALER_SUPER2X
}
SCALER;

LoadedImage *Image_Create(void);
void Image_Load(FS_File *file, LoadedImage *image);
void Image_GenerateMipmaps(LoadedImage *image);
void Image_Scale2x(LoadedImage *image, SCALER scaler, bool makePowerOfTwo);
void Image_MakePowerOfTwo(LoadedImage *image);
void Image_MakeAlphaBinary(LoadedImage *image, int thresh);
byte *Image_GetData(LoadedImage *image);
size_t Image_GetDataSize(LoadedImage *image);
void Image_Unload(LoadedImage *image);
void Image_Delete(LoadedImage *image);

void Image_ConvertColors(LoadedImage *image, bool swappedColor, bool premodulatedColor);

bool Image_WriteTarga(char *filename, int width, int height, int bpp, byte *data, bool flip);
bool Image_Save(LoadedImage *image, char *filename);
bool Image_Test(LoadedImage *image, char *filename);

void Image_Init(void);
void Image_Shutdown(void);
void Image_PrintModules(void);

int NextPowerOfTwo(int n);