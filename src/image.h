// image.h
#ifndef H_IMAGE_H
#define H_IMAGE_H

#include "freeimage/freeimage.h"
#include "fs.h"
#include "options.h"

typedef struct MipMap_s
{
	int       width;
	int       height;
	int       level;
	byte     *data;
	size_t    datasize;
	MipMap_s *nextmip;
}MipMap;

// data type
typedef enum
{
	IMAGE_COLOR,
	IMAGE_NORMALMAP,
	IMAGE_GRAYSCALE,
}DATATYPE;

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
ImageScaler;
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
	bool         scaled;           // set to true if image was scaled
	bool         converted;        // BPP was converted        
	bool         unused2; 
	bool         unused3; 

	// saved loaded state
	ImageState   loadedState;

	// special
	MipMap      *mipMaps;       // generated mipmaps
	char         texname[128];  // null if there is no custom texture name
	bool         useTexname;

	// texture average color (used by exporter)
	byte         averagecolor[4];
	bool         hasAverageColor;

	// set by texture tool
	DATATYPE     datatype;

	// next image's frame
	LoadedImage_s *next;
}
LoadedImage;

LoadedImage *Image_Create(void);
void  Image_Generate(LoadedImage *image, int width, int height, int bpp);
void  Image_Load(FS_File *file, LoadedImage *image);
void  Image_LoadFinish(LoadedImage *image);
bool  Image_Changed(LoadedImage *image);
void  Image_GenerateMipmaps(LoadedImage *image, bool overwrite);
void  Image_FreeMipmaps(LoadedImage *image);
void  Image_Scale2x(LoadedImage *image, ImageScaler scaler, bool makePowerOfTwo);
void  Image_MakeDimensions(LoadedImage *image, bool powerOfTwo, bool square);
void  Image_MakeAlphaBinary(LoadedImage *image, int thresh);
void  Image_SetAlpha(LoadedImage *image, byte value);
void  Image_Swizzle(LoadedImage *image, void (*swizzleFunction)(LoadedImage *image, bool decode), bool decode);
byte *Image_GetData(LoadedImage *image, size_t *datasize);
void  Image_Unload(LoadedImage *image);
void  Image_Delete(LoadedImage *image);

// internal color conversion
byte* Image_ConvertBPP(LoadedImage *image, int bpp);
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