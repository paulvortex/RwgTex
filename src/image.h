// image.h
#ifndef H_IMAGE_H
#define H_IMAGE_H

#include "freeimage/freeimage.h"
#include "fs.h"
#include "options.h"

typedef struct ImageMap_s
{
	int         level;
	int         width;
	int         height;
	byte       *data;
	size_t      datasize; // size of data
	bool        external; // using external data, do not free
	bool        sRGB;     // using sRGB colorspace
	ImageMap_s *next;
}ImageMap;

// data type
typedef enum
{
	IMAGE_COLOR,
	IMAGE_NORMALMAP,
	IMAGE_GRAYSCALE,
	IMAGE_GLOSSMAP,
	IMAGE_GLOWMAP
}ImageType;

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
	IMAGE_SCALER_SUPER2X,
	IMAGE_SCALER_XBRZ,
	IMAGE_SCALER_SBRZ, // scale by 5x then back to 2x
}ImageScaler;
#ifdef F_IMAGE_C
	OptionList ImageScalers[] =
	{
		{ "nearest",    IMAGE_SCALER_BOX },
		{ "bilinear",   IMAGE_SCALER_BILINEAR },
		{ "bicubic",    IMAGE_SCALER_BICUBIC },
		{ "bspline",    IMAGE_SCALER_BSPLINE },
		{ "catmullrom", IMAGE_SCALER_CATMULLROM },
		{ "lanczos",    IMAGE_SCALER_LANCZOS },
		{ "scale2x",    IMAGE_SCALER_SCALE2X },
		{ "super2x",    IMAGE_SCALER_SUPER2X },
		{ "xbrz",       IMAGE_SCALER_XBRZ },
		{ "sbrz",       IMAGE_SCALER_SBRZ },
		{ 0 },
	};
#else
	extern OptionList ImageScalers[];
#endif

typedef struct ImageState_s
{
	int          width;
	int          height;
	int          bpp;              // bits per pixel
	bool         colorSwap;        // BGR instead of RGB
	bool         swizzled;         // colors was explicitly altered
	bool         hasAlpha;         // have alpha channel (this indicates if SOURCE file actually have alpha and may differ from bpp)
	bool         hasGradientAlpha; // alpha channel is a gradient type (not binary
	bool         sRGB;             // use sRGB for color
	bool         scaled;           // set to true if image was scaled
	bool         converted;        // BPP was converted
	bool         unused2; 
	bool         unused3;  
} ImageState;

typedef struct LoadedImage_s
{
	// load-time parameters
	size_t	     filesize;

	// FreeImage parameters
	FIBITMAP    *bitmap;
	int          scale;

	// current image state (should match contents and order of ImageState!)
	int          width;
	int          height;
	int          bpp;              // bits per pixel
	bool         colorSwap;        // BGR instead of RGB
	bool         swizzled;         // colors was explicitly altered
	bool         hasAlpha;         // have alpha channel (this indicates if SOURCE file actually have alpha and may differ from bpp)
	bool         hasGradientAlpha; // alpha channel is a gradient type (not binary
	bool         sRGB;             // use sRGB for color
	bool         scaled;           // set to true if image was scaled
	bool         converted;        // BPP was converted        
	bool         unused2; 
	bool         unused3; 

	// saved loaded state
	ImageState   loadedState;

	// special
	ImageMap    *maps;          // generated maps
	char         texname[128];  // null if there is no custom texture name
	bool         useTexname;

	// texture average color (used by exporter)
	byte         averagecolor[3];
	bool         hasAverageColor;

	// set by texture tool
	ImageType    datatype;

	// next image's frame
	LoadedImage_s *next;
}
LoadedImage;

LoadedImage *Image_Create(void);
void  Image_Generate(LoadedImage *image, int width, int height, int bpp);
void  Image_Load(FS_File *file, LoadedImage *image);
void  Image_LoadFinish(LoadedImage *image);
bool  Image_Changed(LoadedImage *image);
void  Image_GenerateMaps(LoadedImage *image, bool overwrite, bool miplevels, bool binaryalpha, bool srgb);
void  Image_FreeMaps(LoadedImage *image);
void  Image_ScaleBy2(LoadedImage *image, ImageScaler scaler, bool makePowerOfTwo);
void  Image_ScaleBy4(LoadedImage *image, ImageScaler scaler, ImageScaler scaler2, bool makePowerOfTwo);
void  Image_MakeDimensions(LoadedImage *image, bool powerOfTwo, bool square);
//void  Image_MakeAlphaBinary(LoadedImage *image, int thresh);
void  Image_SetAlpha(LoadedImage *image, byte value);
byte *Image_GetData(LoadedImage *image, size_t *datasize, int *pitch);
byte *Image_GetUnalignedData(LoadedImage *image, size_t *datasize, bool *data_allocated, bool force_allocate);
void  Image_StoreUnalignedData(LoadedImage *image, byte *dataptr, size_t datasize);
void  Image_FreeUnalignedData(byte *dataptr, bool data_allocated);
void  Image_Unload(LoadedImage *image);
void  Image_Delete(LoadedImage *image);

// raw data functions
void ImageData_SwapRB(byte *data, int width, int height, int pitch, int bpp);
void ImageData_ConvertSRGB(byte *data, int width, int height, int pitch, int bpp, bool srcSRGB, bool dstSRGB);
bool ImageData_ProbeLinearToSRGB_16bit(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap);

// internal color conversion
void  Image_ConvertBPP(LoadedImage *image, int bpp);
void  Image_ConvertSRGB(LoadedImage *image, bool useSRGB);
void  Image_SwapColors(LoadedImage *image, bool swappedColor);
void  Image_CalcAverageColor(LoadedImage *image);
byte *Image_GenerateTarga(size_t *outsize, int width, int height, int bpp, byte *data, bool flip, bool rgb, bool grayscale);
bool  Image_Save(LoadedImage *image, char *filename);
byte *Image_ExportTarga(LoadedImage *image, size_t *tgasize);
bool  Image_ExportTarga(LoadedImage *image, char *filename);

void  Image_Init(void);
void  Image_Shutdown(void);
void  Image_PrintModules(void);

int NextPowerOfTwo(int n);

#endif