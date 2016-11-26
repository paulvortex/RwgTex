// tex.h
#ifndef H_TEX_H
#define H_TEX_H

#include "main.h"
#include "options.h"
#include "math.h"

// disable certain tools
// #define NO_ATITC

// fourCC macro
#ifndef FOURCC
#define FOURCC(c0,c1,c2,c3) ((ulong)(byte)(c0) | ((ulong)(byte)(c1) << 8) | ((DWORD)(byte)(c2) << 16) | ((ulong)(byte)(c3) << 24 ))
#endif

// structures
// compressed format features
#define FF_ALPHA                   1   // alpha
#define FF_BINARYALPHA             2   // 'punch-through' alpha
#define FF_NOMIP                   4   // does not support mipmaps
#define FF_POT                     8   // should have power-of-two dimensions
#define FF_SQUARE                  16  // should have width = height
#define FF_SRGB                    32  // support hardware sRGB decoding
#define FF_SWIZZLE_RESERVED_ALPHA  64  // alpha is used to store part of color data
#define FF_SWIZZLE_INTERNAL_SRGB   128 // internal swizzled color supports sRGB while the swizzled texture is linear, shader decoding is required

// encoding profiles
enum texprofile
{
	PROFILE_FAST,		   // very fast encoding, quality is not a concert
	PROFILE_REGULAR,       // regular usage, where both quality and speed is important
	PROFILE_BEST,          // final buildquality, use most exhaustive and high quality methods
	NUM_PROFILES,
}; 
#ifdef F_TEX_C
	OptionList tex_profiles[] = 
	{ 
		{ "fast", PROFILE_FAST }, 
		{ "regular", PROFILE_REGULAR }, 
		{ "best", PROFILE_BEST },
		{ 0 }
	};
#else
	extern OptionList tex_profiles[];
#endif

// compression block
typedef struct TexBlock_s
{
	DWORD fourCC;
	char  *name;
	size_t width;     // width of block element in pixels
	size_t height;    // height of block element in pixels
	size_t bitlength; // size of block element in bits
	size_t blocksize; // texture cannot be lesses than this size in bytes
} TexBlock;

// texture compression format
typedef struct TexFormat_s
{
	DWORD                fourCC;
	char                *name;
	char                *fullName;
	char                *parmName;
	TexBlock            *block;
	struct TexCodec_s   *codec;
	uint                 glInternalFormat;
	uint                 glInternalFormat_SRGB;
	uint                 glFormat;
	uint                 glType;
	ulong                features;
	void               (*colorSwizzle)(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode);
	TexFormat_s        **swizzledFormats;
	// system part
	char                *cmdParm;
	char                *cmdParmDisabled;
	char                *forceGroup;
	FCLIST               forceFileList;
	char                *suffix;
	TexFormat_s         *baseFormat; // points to base format for swizzled formats (RXGB, YCoGg etc)
	TexFormat_s         *next;
} TexFormat;

// texture tool input flags
#define TEXINPUT_BGR         1
#define TEXINPUT_BGRA        2
#define TEXINPUT_RGB         4
#define TEXINPUT_RGBA        8

// texture compression/decompression tool
typedef struct TexTool_s
{
	char              *name;
	char              *fullName;
	char              *parmName;
	ulong              inputflags;
	void             (*fInit)(void);
	void             (*fOption)(const char *group, const char *key, const char *val, const char *filename, int linenum);
	void             (*fLoad)(void);
	bool             (*fCompress)(struct TexEncodeTask_s *task);
	const char      *(*fGetVersion)(void);
	// system fields
	vector<TexFormat*> formats;
	char              *cmdParm;
	char              *cmdParmDisabled;
	char              *forceGroup;
	FCLIST             forceFileList;
	char              *suffix;
	char              *featuredCodecs;  // filled by Tex_LinkTools()
	char              *featuredFormats; // filled by Tex_LinkTools()
	TexTool_s         *next;
} TexTool;

// texture compression/decompression codec
typedef struct TexCodec_s
{
	char              *name;
	char              *fullName;
	char              *parmName;
	void             (*fInit)(void);
	void             (*fOption)(const char *group, const char *key, const char *val, const char *filename, int linenum);
	void             (*fLoad)(void);
	bool             (*fAccept)(struct TexEncodeTask_s *task);
	void             (*fEncode)(struct TexEncodeTask_s *task);
	void             (*fDecode)(struct TexDecodeTask_s *task);
	// system fields
	vector<TexTool*>   tools;
	vector<TexFormat*> formats;
	char              *cmdParm;
	char              *cmdParmDisabled;
	TexTool           *forceTool;
	TexFormat         *forceFormat;
	struct TexCodec_s *fallback;
	bool               disabled;
	FCLIST             discardList;
	char               destDir[MAX_FPATH];
	double             stat_inputDiskMB;
	double             stat_inputRamMB;
	double             stat_inputPOTRamMB;
	double             stat_outputDiskMB;
	double             stat_outputRamMB;
	size_t             stat_numTextures;
	size_t             stat_numImages;
	TexCodec_s        *next;
	TexCodec_s        *nextActive;
} TexCodec;

// conteiner file format
typedef struct TexContainer_s
{
	char              *name;
	char              *fullName;
	char              *extensionName;
	size_t             scanBytes;
	bool             (*fScan)(byte *data);
	size_t             headerSize;
	size_t             mipHeaderSize;
	size_t             mipDataPadding;
	void             (*fPrintHeader)(byte *data);
	byte            *(*fCreateHeader)(LoadedImage *image, TexFormat *format, size_t *headersize);
	size_t           (*fWriteMipHeader)(byte *stream, size_t width, size_t height, size_t pixeldatasize);
	bool             (*fReadHeader)(struct TexDecodeTask_s *task);
	// system fields
	char              *cmdParm;
	TexContainer_s    *next;
} TexContainer;

//
// modes
//
#include "tex_compress.h"
#include "tex_decompress.h"

//
// compression codecs
//

#include "tex_glformats.h"

#include "codec_dxt.h"
#include "codec_etc1.h"
#include "codec_etc2.h"
#include "codec_pvrtc.h"
#include "codec_pvrtc2.h"
#include "codec_unc.h"

//
// compression tools
//

#include "tool_nvdxtlib.h"
#include "tool_nvtt.h"
#include "tool_atitc.h"
#include "tool_rwgtt.h"
#include "tool_gimp.h"
#include "tool_etcpack.h"
#include "tool_crunch.h"
#include "tool_rgetc1.h"
#include "tool_pvrtex.h"

//
// Texture container files
//
#include "file_dds.h"
#include "file_ktx.h"

//
// Generic
//

// codec/tool/format/container architecture
extern TexCodec     *tex_codecs;
extern TexCodec     *tex_active_codecs;
extern TexTool      *tex_tools;
extern TexFormat    *tex_formats;
extern TexContainer *tex_containers;
extern size_t        tex_containers_scanbytes;

// architecture functions
TexCodec     *findCodec(const char *name, bool quiet);
void          RegisterCodec(TexCodec *codec);
void          UseCodec(TexCodec *codec);
void          FreeCodecs(void);
void          RegisterTool(TexTool *tool, TexCodec *codec);
TexTool      *findTool(const char *name, bool quiet);
void          FreeTools(void);
void          RegisterFormat(TexFormat *format, TexTool *tool);
TexFormat    *findFormat(const char *name, bool quiet);
bool          findFormatByFourCCAndAlpha(DWORD fourCC, bool alpha, TexCodec **codec, TexFormat **format);
bool          findFormatByGLType(uint glFormat, uint glInternalFormat, uint glType, TexCodec **codec, TexFormat **format);
void          FreeFormats(void);
TexContainer *findContainer(const char *name, bool quiet);
void          RegisterContainer(TexContainer *container);
TexContainer *findContainerForFile(char *filename, byte *data, size_t datasize);
void          FreeContainers(void);

// main
size_t        compressedTextureSize(LoadedImage *image, TexFormat *format, TexContainer *container, bool baseTex, bool mipLevels);
size_t        compressedTextureBPP(LoadedImage *image, TexFormat *format, TexContainer *container);
void          Tex_PrintCodecs(void);
void          Tex_PrintTools(void);
void          Tex_PrintContainers(void);
void          Tex_PrintModules(void);
void          Tex_Init(void);
void          Tex_Shutdown(void);
int           TexMain(int argc, char **argv);

// options
// util launch mode
enum texmode
{
	TEXMODE_NORMAL,
	TEXMODE_DROP_FILE,
	TEXMODE_DROP_DIRECTORY,
	NUM_TEXMODES
};

// suffixes (used with tex_useSuffix)
#define TEXSUFF_TOOL    1
#define TEXSUFF_FORMAT  2
#define TEXSUFF_PROFILE 4

extern texmode       tex_mode;
extern char          tex_srcDir[MAX_FPATH];
extern char          tex_srcFile[MAX_FPATH];
extern char          tex_destPath[MAX_FPATH];
extern bool          tex_destPathUseCodecDir;
extern bool          tex_generateArchive;
extern string        tex_gameDir;
extern bool          tex_allowNPOT;
extern bool          tex_noMipmaps;
extern bool          tex_noAvgColor;
extern bool          tex_forceScale2x;
extern bool          tex_forceScale4x;
extern bool          tex_sRGB_allow;
extern bool          tex_sRGB_autoconvert;
extern bool          tex_sRGB_forceconvert;
extern bool          tex_useSign;
extern char          tex_sign[1024];
extern DWORD         tex_signVersion;
extern FCLIST        tex_includeFiles;
extern FCLIST        tex_noMipFiles;
extern FCLIST        tex_normalMapFiles;
extern FCLIST        tex_grayScaleFiles;
extern FCLIST        tex_sRGBcolorspace;
extern bool          tex_forceBestPSNR;
extern bool          tex_detectBinaryAlpha;
extern byte          tex_binaryAlphaMin;
extern byte          tex_binaryAlphaMax;
extern byte          tex_binaryAlphaCenter;
extern float         tex_binaryAlphaThreshold;
extern FCLIST        tex_archiveFiles;
extern string        tex_addPath;
extern int           tex_zipInMemory;
extern int           tex_zipCompression;
extern FCLIST        tex_zipAddFiles;
extern FCLIST        tex_scale2xFiles;
extern FCLIST        tex_scale4xFiles;
extern ImageScaler   tex_firstScaler;
extern ImageScaler   tex_secondScaler;
extern int           tex_useSuffix;
extern bool          tex_testCompresion;
extern bool          tex_testCompresion_keepSize;
extern bool          tex_testCompresionError;
extern bool          tex_testCompresionAllErrors;

extern TexErrorMetric tex_errorMetric;
extern TexContainer *tex_container;
extern texprofile    tex_profile;

#endif