////////////////////////////////////////////////////////////////
//
// RwgTex / texture main
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_TEX_C
#include "main.h"
#include "tex.h"

// codec/tool/format/container architecture
TexCodec     *tex_codecs         = NULL;
TexCodec     *tex_active_codecs  = NULL;
TexTool      *tex_tools          = NULL;
TexFormat    *tex_formats        = NULL;
TexContainer *tex_containers     = NULL;
size_t        tex_containers_scanbytes = 0;

// options
texmode       tex_mode = TEXMODE_NORMAL;
char          tex_srcDir[MAX_FPATH];
char          tex_srcFile[MAX_FPATH];
char          tex_destPath[MAX_FPATH];
bool          tex_destPathUseCodecDir;
bool          tex_generateArchive;
string        tex_gameDir;
bool          tex_allowNPOT;
bool          tex_noMipmaps;
bool          tex_noAvgColor;
bool          tex_forceScale2x;
bool          tex_forceScale4x;
bool          tex_sRGB_allow;
bool          tex_sRGB_autoconvert;
bool          tex_sRGB_forceconvert;
bool          tex_useSign;
char          tex_sign[1024];
DWORD         tex_signVersion;
FCLIST        tex_includeFiles;
FCLIST        tex_noMipFiles;
FCLIST        tex_normalMapFiles;
FCLIST        tex_grayScaleFiles;
FCLIST        tex_glossMapFiles;
FCLIST        tex_glowMapFiles;
FCLIST        tex_sRGBcolorspace;
bool          tex_forceBestPSNR;
bool          tex_detectBinaryAlpha;
byte          tex_binaryAlphaMin;
byte          tex_binaryAlphaMax;
byte          tex_binaryAlphaCenter;
float         tex_binaryAlphaThreshold;
FCLIST        tex_archiveFiles;
string        tex_addPath;
int           tex_zipInMemory;
int           tex_zipCompression;
FCLIST        tex_zipAddFiles;
FCLIST        tex_scale2xFiles;
FCLIST        tex_scale4xFiles;
ImageScaler   tex_firstScaler;
ImageScaler   tex_secondScaler;
int           tex_useSuffix;
bool          tex_testCompresion = false;
bool          tex_testCompresion_keepSize = false;
bool          tex_testCompresionError = false;
bool          tex_testCompresionAllErrors = false;
TexErrorMetric tex_errorMetric = ERRORMETRIC_AUTO;
TexContainer *tex_container = NULL;
texprofile    tex_profile;

/*
==========================================================================================

  Texture codecs

==========================================================================================
*/

TexCodec *findCodec(const char *name, bool quiet)
{
	TexCodec *c;

	for (c = tex_codecs; c; c = c->next)
		if (!stricmp(c->name, name))
			return c;
	if (!quiet)
		Error("Unsupported texture codec: %s\n", name);
	return NULL;
}

void RegisterCodec(TexCodec *codec)
{
	TexCodec *c, *last;

	if (!codec)
		return;
	// check if codec already registered
	for (c = tex_codecs; c; c = c->next)
		if (!strcmp(c->name, codec->name))
			Warning("Texture codec '%s' already registered\n", c->fullName);	
	// register
	if (!tex_codecs)
		tex_codecs = codec;
	else
	{
		for (c = tex_codecs; c; c = c->next) last = c;
		last->next = codec;
	}
	// initialize
	uint len = strlen(codec->parmName);
	codec->cmdParm = (char *)mem_alloc(len + 2);
	sprintf(codec->cmdParm, "-%s", codec->parmName);
	codec->cmdParmDisabled = (char *)mem_alloc(len + 10);
	sprintf(codec->cmdParmDisabled, "-disable-%s", codec->parmName);
	if (codec->fInit)
	{
		codec->fInit();
		// register supported formats
		vector<TexTool*>::iterator tl;
		vector<TexFormat*>::iterator fmt, fmt2;
		for (tl = codec->tools.begin(); tl < codec->tools.end(); tl++)
		{
			for (fmt = (*tl)->formats.begin(); fmt < (*tl)->formats.end(); fmt++)
			{
				if ((*fmt)->codec != codec)
					continue;
				for (fmt2 = codec->formats.begin(); fmt2 < codec->formats.end(); fmt2++)
					if (!strcmp((*fmt)->name, (*fmt2)->name))
						break;
				if (fmt2 >= codec->formats.end())
					codec->formats.push_back(*fmt);
			}
		}
	}
}

void UseCodec(TexCodec *codec)
{
	TexCodec *c;

	if (!codec)
		return;
	// check if codec is registered
	for (c = tex_codecs; c; c = c->next)
		if (!strcmp(c->name, codec->name))
			break;
	if (!c)
		Error("UseCodec: codec '%s' not registered", codec->name);
	// check if codec already used
	for (c = tex_active_codecs; c; c = c->nextActive)
		if (!strcmp(c->name, codec->name))
			return;
	// register and use
	codec->nextActive = tex_active_codecs;
	tex_active_codecs = codec;
}

void FreeCodecs(void)
{
	TexCodec *c;

	for (c = tex_codecs; c; c = c->next)
	{
		if (c->cmdParm != NULL)
		{
			mem_free(c->cmdParm);
			c->cmdParm = NULL;
		}
		if (c->cmdParmDisabled != NULL)
		{
			mem_free(c->cmdParmDisabled);
			c->cmdParmDisabled = NULL;
		}
	}
}

/*
==========================================================================================

  Texture tools

==========================================================================================
*/

TexTool *findTool(const char *name, bool quiet)
{
	TexTool *t;

	for (t = tex_tools; t; t = t->next)
		if (!stricmp(t->name, name))
			return t;
	if (!quiet)
		Error("Unsupported texture tool: %s\n", name);
	return NULL;
}

void RegisterTool(TexTool *tool, TexCodec *codec)
{
	TexTool *t, *last;

	if (!tool)
		return;
	
	// register in codec scope
	vector<TexTool*>::iterator tl;
	for (tl = codec->tools.begin(); tl < codec->tools.end(); tl++)
		if (!strcmp((*tl)->name, tool->name))
			break;
	if (tl >= codec->tools.end())
		codec->tools.push_back(tool);
	// check if tool already registered
	for (t = tex_tools; t; t = t->next)
		if (!strcmp(t->name, tool->name))
			return;
	// register
	if (!tex_tools)
		tex_tools = tool;
	else
	{
		for (t = tex_tools; t; t = t->next) last = t;
		last->next = tool;
	}
	// initialize
	uint len = strlen(tool->parmName);
	tool->cmdParm = (char *)mem_alloc(len + 2);
	sprintf(tool->cmdParm, "-%s", tool->parmName);
	tool->cmdParmDisabled = (char *)mem_alloc(len + 10);
	sprintf(tool->cmdParmDisabled, "-disable-%s", tool->parmName);
	tool->forceGroup = (char *)mem_alloc(len + 7);
	sprintf(tool->forceGroup, "force_%s", tool->parmName);
	tool->forceFileList.clear();
	tool->suffix = (char *)mem_alloc(len + 2);
	sprintf(tool->suffix, "_%s", tool->parmName);
	if (tool->fInit)
		tool->fInit();
}

void FreeTools(void)
{
	TexTool *t;

	for (t = tex_tools; t; t = t->next)
	{
		if (t->cmdParm != NULL)
		{
			mem_free(t->cmdParm);
			t->cmdParm = NULL;
		}
		if (t->cmdParmDisabled != NULL)
		{
			mem_free(t->cmdParmDisabled);
			t->cmdParmDisabled = NULL;
		}
		if (t->forceGroup != NULL)
		{
			mem_free(t->forceGroup);
			t->forceGroup = NULL;
		}
		if (t->suffix != NULL)
		{
			mem_free(t->suffix);
			t->suffix = NULL;
		}
		if (t->featuredCodecs != NULL)
		{
			mem_free(t->featuredCodecs);
			t->featuredCodecs = NULL;
		}
		if (t->featuredFormats != NULL)
		{
			mem_free(t->featuredFormats);
			t->featuredFormats = NULL;
		}
	}
}

/*
==========================================================================================

  Texture compression formats

==========================================================================================
*/

TexFormat *findFormat(const char *name, bool quiet)
{
	TexFormat *f;

	for (f = tex_formats; f; f = f->next)
		if (!stricmp(f->name, name))
			return f;
	if (!quiet)
		Error("Unsupported texture format: %s", name);
	return NULL;
}

void _RegisterFormat(TexFormat *format, TexTool *tool, TexFormat *baseFormat)
{
	TexFormat *f, *last;
	int i;

	if (!format || !tool)
		return;

	// register in tool scope
	vector<TexFormat*>::iterator fmt;
	for (fmt = tool->formats.begin(); fmt < tool->formats.end(); fmt++)
		if (!strcmp((*fmt)->name, format->name))
			break;
	if (fmt >= tool->formats.end())
		tool->formats.push_back(format);
	// register format in global scope
	for (f = tex_formats; f; f = f->next)
		if (!strcmp(f->name, format->name))
			break;
	if (!f)
	{
		if (!tex_formats)
			tex_formats = format;
		else
		{
			for (f = tex_formats; f; f = f->next) last = f;
			last->next = format;
		}
	}
	// initialize
	format->baseFormat = baseFormat;
	uint len = strlen(format->parmName);
	if( format->cmdParm == NULL )
		format->cmdParm = (char *)mem_alloc(len + 2);
	sprintf(format->cmdParm, "-%s", format->parmName);
	if( format->cmdParmDisabled == NULL )
		format->cmdParmDisabled = (char *)mem_alloc(len + 10);
	sprintf(format->cmdParmDisabled, "-disable-%s", format->parmName);
	if( format->forceGroup == NULL )
		format->forceGroup = (char *)mem_alloc(len + 7);
	sprintf(format->forceGroup, "force_%s", format->parmName);
	format->forceFileList.clear();
	if (format->baseFormat != NULL)
	{
		if( format->suffix == NULL )
			format->suffix = (char *)mem_alloc(strlen(baseFormat->parmName) + len + 3);
		sprintf(format->suffix, "_%s%s", baseFormat->parmName, format->parmName);
	}
	else
	{
		if( format->suffix == NULL )
			format->suffix = (char *)mem_alloc(len + 2);
		sprintf(format->suffix, "_%s", format->parmName);
	}
	// register swizzled versions
	if (format->swizzledFormats != NULL)
		for(i = 0; format->swizzledFormats[i] != NULL; i++)
			_RegisterFormat(format->swizzledFormats[i], tool, format);
}

void RegisterFormat(TexFormat *format, TexTool *tool)
{
	_RegisterFormat(format, tool, NULL);
}

size_t compressedTextureSize(LoadedImage *image, TexFormat *format, TexContainer *container, bool baseTex, bool mipLevels)
{
	size_t size = 0, s;
	TexBlock *b;
	int x, y;

	b = format->block;
	// base layer
	if (baseTex)
	{
		size += container->mipHeaderSize;
		x = (int)ceil((float)image->width / (float)b->width);
		y = (int)ceil((float)image->height / (float)b->height);
		s = max(b->blocksize, x*y*b->bitlength/8);
		if (container->mipDataPadding)
			size += s + ((s/container->mipDataPadding)*container->mipDataPadding - s);
		else
			size += s;
	}
	// maps
	if (mipLevels)
	{
		for (ImageMap *map = image->maps->next; map; map = map->next)
		{
			size += container->mipHeaderSize;
			x = (int)ceil((float)map->width / (float)b->width);
			y = (int)ceil((float)map->height / (float)b->height);
			s = max(b->blocksize, x*y*b->bitlength/8);
			if (container->mipDataPadding)
				size += s + ((s/container->mipDataPadding)*container->mipDataPadding - s);
			else
				size += s;
		}
	}
	return size;
}

size_t compressedTextureBPP(LoadedImage *image, TexFormat *format, TexContainer *container)
{
	uint fmt;

	fmt = format->glFormat;
	if (fmt == GL_RGB || fmt == GL_BGR || fmt == GL_RG)
		return 3;
	if (fmt == GL_RGBA || fmt == GL_BGRA)
		return 4;
	return 1;
}

bool findFormatByFourCCAndAlpha(DWORD fourCC, bool alpha, TexCodec **codec, TexFormat **format)
{
	TexFormat *f;
	int features = 0;

	// find with features support (support DXT1a that have same fourCC as DXT1)
	if (alpha)
		features = FF_ALPHA;
	for (TexCodec *cdc = tex_codecs; cdc; cdc = cdc->next)
	{
		for (vector<TexFormat*>::iterator fmt = cdc->formats.begin(); fmt < cdc->formats.end(); fmt++)
		{
			f = *fmt;
			if (f->fourCC == fourCC && (!features || (f->features & features)))
			{
				*codec = cdc;
				*format = f;
				return true;
			}
		}
	}
	// find with no features support (fallback as some ETC1 DDS can have DDPF_ALPHAPIXELS)
	for (TexCodec *cdc = tex_codecs; cdc; cdc = cdc->next)
	{
		for (vector<TexFormat*>::iterator fmt = cdc->formats.begin(); fmt < cdc->formats.end(); fmt++)
		{
			f = *fmt;
			if (f->fourCC == fourCC)
			{
				*codec = cdc;
				*format = f;
				return true;
			}
		}
	}
	return false;
}

bool findFormatByGLType(uint glFormat, uint glInternalFormat, uint glType, TexCodec **codec, TexFormat **format)
{
	TexFormat *f;

	for (TexCodec *cdc = tex_codecs; cdc; cdc = cdc->next)
	{
		for (vector<TexFormat*>::iterator fmt = cdc->formats.begin(); fmt < cdc->formats.end(); fmt++)
		{
			f = *fmt;
			if (f->glFormat == glFormat && f->glInternalFormat == glInternalFormat && f->glType == glType)
			{
				*codec = cdc;
				*format = f;
				return true;
			}
		}
	}
	return false;
}

void FreeFormats(void)
{
	TexFormat *f;

	for (f = tex_formats; f; f = f->next)
	{
		if (f->cmdParm != NULL)
		{
			mem_free(f->cmdParm);
			f->cmdParm = NULL;
		}
		if (f->cmdParmDisabled != NULL)
		{
			mem_free(f->cmdParmDisabled);
			f->cmdParmDisabled = NULL;
		}
		if (f->forceGroup != NULL)
		{
			mem_free(f->forceGroup);
			f->forceGroup = NULL;
		}
		if (f->suffix != NULL)
		{
			mem_free(f->suffix);
			f->suffix = NULL;
		}
	}
}

/*
==========================================================================================

  Texture file containers

==========================================================================================
*/

TexContainer *findContainer(const char *name, bool quiet)
{
	TexContainer *c;

	for (c = tex_containers; c; c = c->next)
		if (!stricmp(c->name, name))
			return c;
	if (!quiet)
		Error("Unsupported container: %s", name);
	return NULL;
}

void RegisterContainer(TexContainer *container)
{
	TexContainer *c, *last;

	if (!container)
		return;
	// check if codec already registered
	for (c = tex_containers; c; c = c->next)
		if (!strcmp(c->name, container->name))
			Warning("Texture container '%s' already registered\n", c->fullName);	
	// register
	if (!tex_containers)
		tex_containers = container;
	else
	{
		for (c = tex_containers; c; c = c->next) last = c;
		last->next = container;
	}
	// initialize
	uint len = strlen(container->extensionName);
	container->cmdParm = (char *)mem_alloc(len + 2);
	sprintf(container->cmdParm, "-%s", container->extensionName);
	tex_containers_scanbytes = max(tex_containers_scanbytes, container->scanBytes);
}

TexContainer *findContainerForFile(char *filename, byte *data, size_t datasize)
{
	byte *scandata;
	TexContainer *c;
	char ext[MAX_FPATH];
	bool scanned;
	FILE *f;

	// find by file header
	if (tex_containers_scanbytes)
	{
		scanned = true;
		if (data)
		{
			scandata = data;
			if (datasize < tex_containers_scanbytes)
				scanned = false;
		}
		else
		{
			f = fopen(filename, "rb");
			if (f)
			{
				scandata = (byte *)mem_alloc(tex_containers_scanbytes);
				if (!fread(scandata, tex_containers_scanbytes, 1, f))
					scanned = false;
				fclose(f);
			}
		}
		if (scanned)
			for (c = tex_containers; c; c = c->next)
				if (c->fScan(scandata))
					break;
		if (!data)
			mem_free(scandata);
		if (c)
			return c;
	}

	// find by extension
	ExtractFileExtension(filename, ext);
	for (c = tex_containers; c; c = c->next)
		if (!stricmp(c->extensionName, ext))
			break;
	if (c)
		return c;
	return NULL;
}

void FreeContainers(void)
{
	TexContainer *c;

	for (c = tex_containers; c; c = c->next)
	{
		if (c->cmdParm != NULL)
		{
			mem_free(c->cmdParm);
			c->cmdParm = NULL;
		}
	}
}

/*
==========================================================================================

  CommandLine Options

==========================================================================================
*/


void CommandLineOptions(void)
{
	// COMMANDLINEPARM: -nm: for all files to compress with best Peak-Signal-To-Noise (really it is using normalmap path for them)
	if (CheckParm("-nm"))         tex_forceBestPSNR = true;	
	// COMMANDLINEPARM: -2x: apply 2x scale (Scale2X)
	if (CheckParm("-2x"))         tex_forceScale2x = true; 
	// COMMANDLINEPARM: -4x: apply 4x scale (2 pass scale)
	if (CheckParm("-4x"))         tex_forceScale4x = true; 
	// COMMANDLINEPARM: -npot: allow non-power-of-two textures
	if (CheckParm("-npot"))       tex_allowNPOT = true;
	// COMMANDLINEPARM: -nomip: do not generate maps
	if (CheckParm("-nomip"))      tex_noMipmaps = true;
	// COMMANDLINEPARM: -noavgcolor: do not generate average color informatiom
	if (CheckParm("-noavgcolor")) tex_noAvgColor = true;
	// COMMANDLINEPARM: -nosrgb: disable sRGB texture support (all sRGB images will be converted to linear space)
	if (CheckParm("-nosrgb"))     tex_sRGB_allow = false;
	// COMMANDLINEPARM: -srgb: forces sRGB on all textures which format supports hardware sRGB
	if (CheckParm("-srgb"))       tex_sRGB_allow = tex_sRGB_forceconvert = true;
	// COMMANDLINEPARM: -nearest: select nearest filter for scaler (2x and 4x)
	if (CheckParm("-nearest"))    tex_firstScaler = tex_secondScaler = IMAGE_SCALER_BOX;
	// COMMANDLINEPARM: -bilinear: select bilinear filter for scaler (2x and 4x)
	if (CheckParm("-bilinear"))   tex_firstScaler = tex_secondScaler = IMAGE_SCALER_BILINEAR;
	// COMMANDLINEPARM: -bicubic: select bicubic filter for scaler (2x and 4x)
	if (CheckParm("-bicubic"))    tex_firstScaler = tex_secondScaler = IMAGE_SCALER_BICUBIC;
	// COMMANDLINEPARM: -bspline: select bspline filter for scaler (2x and 4x)
	if (CheckParm("-bspline"))    tex_firstScaler = tex_secondScaler = IMAGE_SCALER_BSPLINE;
	// COMMANDLINEPARM: -catmullrom: select catmullrom filter for scaler (2x and 4x)
	if (CheckParm("-catmullrom")) tex_firstScaler = tex_secondScaler = IMAGE_SCALER_CATMULLROM;
	// COMMANDLINEPARM: -lanczos: select lanczos filter for scaler (2x and 4x)
	if (CheckParm("-lanczos"))    tex_firstScaler = tex_secondScaler = IMAGE_SCALER_LANCZOS;
	// COMMANDLINEPARM: -scale2x: select scale2x filter for scaler (2x and 4x)
	if (CheckParm("-scale2x"))    tex_firstScaler = tex_secondScaler = IMAGE_SCALER_SCALE2X;	
	// COMMANDLINEPARM: -super2x: select super2x filter for scaler (2x and 4x)
	if (CheckParm("-super2x"))    tex_firstScaler = tex_secondScaler = IMAGE_SCALER_SUPER2X;
	// COMMANDLINEPARM: -xbrz:    select xbrz filter for scaler (2x and 4x)
	if (CheckParm("-xbrz"))       tex_firstScaler = tex_secondScaler = IMAGE_SCALER_XBRZ;
	// COMMANDLINEPARM: -sbrz:    select sbrz filter for scaler (2x and 4x)
	if (CheckParm("-sbrz"))       tex_firstScaler = tex_secondScaler = IMAGE_SCALER_SBRZ;
	// COMMANDLINEPARM: -2nearest: select nearest filter for second scaler (4x)
	if (CheckParm("-2nearest"))   tex_secondScaler = IMAGE_SCALER_BOX;
	// COMMANDLINEPARM: -2bilinear: select bilinear filter for second scaler (4x)
	if (CheckParm("-2bilinear"))  tex_secondScaler = IMAGE_SCALER_BILINEAR;
	// COMMANDLINEPARM: -2bicubic: select bicubic filter for second scaler (4x)
	if (CheckParm("-2bicubic"))   tex_secondScaler = IMAGE_SCALER_BICUBIC;
	// COMMANDLINEPARM: -2bspline: select bspline filter for second scaler (4x)
	if (CheckParm("-2bspline"))   tex_secondScaler = IMAGE_SCALER_BSPLINE;
	// COMMANDLINEPARM: -2catmullrom: select catmullrom filter for second scaler (4x)
	if (CheckParm("-2catmullrom"))tex_secondScaler = IMAGE_SCALER_CATMULLROM;
	// COMMANDLINEPARM: -2lanczos: select lanczos filter for second scaler (4x)
	if (CheckParm("-2lanczos"))   tex_secondScaler = IMAGE_SCALER_LANCZOS;
	// COMMANDLINEPARM: -2scale2x: select scale2x filter for second scaler (4x)
	if (CheckParm("-2scale2x"))   tex_secondScaler = IMAGE_SCALER_SCALE2X;	
	// COMMANDLINEPARM: -2super2x: select super2x filter for second scaler (4x)
	if (CheckParm("-2super2x"))   tex_secondScaler = IMAGE_SCALER_SUPER2X;	
	// COMMANDLINEPARM: -2xbrz: select xBRz filter for second scaler (4x)
	if (CheckParm("-2xbrz"))      tex_secondScaler = IMAGE_SCALER_XBRZ;	
	// COMMANDLINEPARM: -2sbrz: select sBRz filter for second scaler (4x)
	if (CheckParm("-2sbrz"))      tex_secondScaler = IMAGE_SCALER_SBRZ;	
	// COMMANDLINEPARM: -nosign: do not add comment to generated texture files
	if (CheckParm("-nosign"))     tex_useSign = false;
	// COMMANDLINEPARM: -nosign: use GIMP comment for generated texture files
	if (CheckParm("-gimpsign")) { tex_useSign = true; strcpy(tex_sign, "GIMP-DDS"); tex_signVersion = 131585; }
	// COMMANDLINEPARM: -stfp/-stf/-stp/-sfp/-st/-sf/-sp: generate compressor/format file suffix (useful for file comparison)
	if (CheckParm("-stfp"))   { tex_useSuffix = TEXSUFF_TOOL|TEXSUFF_FORMAT|TEXSUFF_PROFILE; }
	if (CheckParm("-stf"))    { tex_useSuffix = TEXSUFF_TOOL|TEXSUFF_FORMAT; }
	if (CheckParm("-stp"))    { tex_useSuffix = TEXSUFF_TOOL|TEXSUFF_PROFILE; }
	if (CheckParm("-sfp"))    { tex_useSuffix = TEXSUFF_FORMAT|TEXSUFF_PROFILE; }
	if (CheckParm("-st"))     { tex_useSuffix = TEXSUFF_TOOL; }
	if (CheckParm("-sf"))     { tex_useSuffix = TEXSUFF_FORMAT; }
	if (CheckParm("-sp"))     { tex_useSuffix = TEXSUFF_PROFILE; }
	if (CheckParm("-t"))      { tex_testCompresion = true; tex_testCompresion_keepSize = false; if (!tex_useSuffix) tex_useSuffix = TEXSUFF_TOOL | TEXSUFF_FORMAT; }
	if (CheckParm("-te"))     { tex_testCompresion = true; tex_testCompresion_keepSize = false; if (!tex_useSuffix) tex_useSuffix = TEXSUFF_TOOL | TEXSUFF_FORMAT; tex_testCompresionError = true; }
	if (CheckParm("-ta"))     { tex_testCompresion = true; tex_testCompresion_keepSize = false; if (!tex_useSuffix) tex_useSuffix = TEXSUFF_TOOL | TEXSUFF_FORMAT; tex_testCompresionAllErrors = true; }
	if (CheckParm("-tk"))     { tex_testCompresion = true; tex_testCompresion_keepSize = true; if (!tex_useSuffix) tex_useSuffix = TEXSUFF_TOOL | TEXSUFF_FORMAT; }
	if (CheckParm("-tek"))    { tex_testCompresion = true; tex_testCompresion_keepSize = true; if (!tex_useSuffix) tex_useSuffix = TEXSUFF_TOOL | TEXSUFF_FORMAT; tex_testCompresionError = true; }
	if (CheckParm("-tak"))    { tex_testCompresion = true; tex_testCompresion_keepSize = true; if (!tex_useSuffix) tex_useSuffix = TEXSUFF_TOOL | TEXSUFF_FORMAT; tex_testCompresionAllErrors = true; }

	// string parameters
	for (int i = 1; i < myargc; i++) 
	{
		// COMMANDLINEPARM: -ap: additional path
		if (!stricmp(myargv[i], "-ap"))
		{
			i++;
			if (i < myargc)
			{
				tex_addPath = myargv[i];
				AddSlash(tex_addPath);
			}
			continue;
		}
		// COMMANDLINEPARM: -zipmem: keep generated zip file in memory until (avoids many file writes)
		if (!stricmp(myargv[i], "-zipmem"))
		{
			i++;
			if (i < myargc)
				tex_zipInMemory = atoi(myargv[i]);
			continue;
		}
		// COMMANDLINEPARM: -zipmem: keep generated zip file in memory until (avoids many file writes)
		if (!stricmp(myargv[i], "-zipcompression"))
		{
			i++;
			if (i < myargc)
				tex_zipCompression = atoi(myargv[i]);
			if (tex_zipCompression < 0)
				tex_zipCompression = 0;
			if (tex_zipCompression > 9)
				tex_zipCompression = 9;
			continue;
		}
		// COMMANDLINEPARM: -zipadd path internal_path: add external file to ZIP archive
		if (!stricmp(myargv[i], "-zipadd"))
		{
			if (i+2 < myargc)
			{
				CompareOption O;
				O.parm = myargv[i+1];
				O.pattern =  myargv[i+2];
				tex_zipAddFiles.push_back(O);
			}
			i += 2;
			continue;
		}
		// COMMANDLINEPARM: -scaler: set a filter to be used for scaling (2x and 4x)
		if (!stricmp(myargv[i], "-scaler"))
		{
			i++;
			if (i < myargc)
				tex_firstScaler = tex_secondScaler = (ImageScaler)OptionEnum(myargv[i], ImageScalers, IMAGE_SCALER_SUPER2X);
			continue;
		}
		// COMMANDLINEPARM: -scaler2: set a filter to be used for second scale pass (4x)
		if (!stricmp(myargv[i], "-scaler2"))
		{
			i++;
			if (i < myargc)
				tex_secondScaler = (ImageScaler)OptionEnum(myargv[i], ImageScalers, IMAGE_SCALER_SUPER2X);
			continue;
		}
		// COMMANDLINEPARM: -errormetric: set a metric to be used for compression error calculation
		if (!stricmp(myargv[i], "-errormetric"))
		{
			i++;
			if (i < myargc)
				tex_errorMetric = (TexErrorMetric)OptionEnum(myargv[i], tex_error_metrics, ERRORMETRIC_AUTO);
			continue;
		}
	}
	// auto-enable codecs 
	for (TexCodec *c = tex_active_codecs; c; c = c->next)
		c->disabled = false;
}


/*
==========================================================================================

  Init

==========================================================================================
*/

void Tex_LinkTools(void)
{
	vector<TexFormat*>::iterator fmt;
	size_t maxlen;

	// fill features format and codecs string for tools
	maxlen = 32768;
	for (TexTool *t = tex_tools; t; t = t->next)
	{
		t->featuredCodecs = (char *)mem_alloc(maxlen + 1);
		memset(t->featuredCodecs, 0, maxlen + 1);
		t->featuredFormats = (char *)mem_alloc(maxlen + 1);
		memset(t->featuredFormats, 0, maxlen + 1);
		for (fmt = t->formats.begin(); fmt < t->formats.end(); fmt++)
		{
			// add to featured codec list
			if (strstr(t->featuredCodecs, (*fmt)->codec->name) == NULL)
			{
				if (t->featuredCodecs[0])
					strlcat(t->featuredCodecs , ", ", maxlen);
				strlcat(t->featuredCodecs , (*fmt)->codec->name, maxlen);
			}
			// add to featured format list
			if (t->featuredFormats[0])
				strlcat(t->featuredFormats, ", ", maxlen);
			strlcat(t->featuredFormats, (*fmt)->name, maxlen);
		}
	}
}

void Tex_Init(void)
{
	RegisterCodec(&CODEC_DXT);
	RegisterCodec(&CODEC_ETC1);
	RegisterCodec(&CODEC_ETC2);
	RegisterCodec(&CODEC_PVRTC);
	RegisterCodec(&CODEC_PVRTC2);
	RegisterCodec(&CODEC_BGRA);
	RegisterContainer(&CONTAINER_DDS);
	RegisterContainer(&CONTAINER_KTX);
	Tex_LinkTools();

	// determine active codecs
	for (TexCodec *c = tex_codecs; c; c = c->next)
	{
		c->fallback = &CODEC_BGRA;
		if (CheckParm(c->cmdParm))
			UseCodec(c);
	}
	if (!tex_active_codecs)
	{
		// no codecs given, check for particular format
		for (TexFormat *f = tex_formats; f; f = f->next)
			if (CheckParm(f->cmdParm))
				if (f->codec)
					UseCodec(f->codec);
	}

	// set default options
	tex_detectBinaryAlpha = false;
	tex_binaryAlphaMin = 0;
	tex_binaryAlphaMax = 255;
	tex_binaryAlphaCenter = 180;
	tex_binaryAlphaThreshold = 99.0f;
	tex_includeFiles.clear();
	tex_noMipFiles.clear();
	tex_sRGBcolorspace.clear();
	tex_normalMapFiles.clear();
	tex_grayScaleFiles.clear();
	tex_glossMapFiles.clear();
	tex_glowMapFiles.clear();
	tex_gameDir = "id1";
	tex_archiveFiles.clear();
	tex_addPath = "";
	tex_scale2xFiles.clear();
	tex_scale4xFiles.clear();
	tex_noMipmaps = false;
	tex_noAvgColor = false;
	tex_allowNPOT = false;
	tex_forceScale2x = false;
	tex_forceScale4x = false;
	tex_sRGB_allow = true;
	tex_sRGB_autoconvert = false;
	tex_sRGB_forceconvert = false;
	tex_useSign = true;
	strcpy(tex_sign, "RWGTEX");
	tex_signVersion = 0;
	tex_forceBestPSNR = false;
	tex_firstScaler = tex_secondScaler = IMAGE_SCALER_SUPER2X;
	tex_zipInMemory = 0;
	tex_zipCompression = 8;
	tex_zipAddFiles.clear();
	tex_useSuffix = 0;
	tex_testCompresion = false;
	tex_testCompresion_keepSize = false;
	tex_container = findContainer("DDS", false);
}

void Tex_Shutdown(void)
{
	FreeCodecs();
	FreeTools();
	FreeFormats();
	FreeContainers();
}

/*
==========================================================================================

  Interface

==========================================================================================
*/

void Tex_PrintCodecs(void)
{
	Print("Codecs:\n");
	for (TexCodec *c = tex_codecs; c; c = c->next)
		Print("%10s: %s\n", c->cmdParm, c->fullName);
	Print("\n");
}

void Tex_PrintTools(void)
{
	Print("Tools:\n");
	for (TexTool *t = tex_tools; t; t = t->next)
		Print("%10s: %s v%s (%s)\n", t->cmdParm, t->fullName, t->fGetVersion(), t->featuredCodecs);
	Print("\n");
}

void Tex_PrintContainers(void)
{
	Print("Export file formats:\n");
	for (TexContainer *c = tex_containers; c; c = c->next)
		Print("%10s: %s\n", c->cmdParm, c->fullName);
	Print("\n");
}

void Tex_Help(void)
{
	waitforkey = true;
	Print(
	"Usage: rwgtex [options] \"input path\" [-o \"output path\"] [codec] [codec options]\n"
	"\n"
	"Input path:\n"
	"  - folder\n"
	"  - a single file or file mask\n"
	"  - archive file (extension should be listed in option file)\n"
	"\n"
	"Output path:\n"
	"  - folder\n"
	"  - archive (will be created from scratch)\n"
	"\n");
	Tex_PrintCodecs();
	Print(
	"Codec general options:\n"
	"      -npot: allow non-power-of-two textures\n"
	"     -nomip: dont generate maps\n"
	"        -2x: apply 2x scale (Scale2X)\n"
	"        -4x: apply 4x scale (2 pass scale)\n"
	"  -scaler X: set a filter to be used for scaling\n"
	" -scaler2 X: set a filter to be used for second scale pass\n"
	"        -ap: additional archive path\n"
	"  -zipmem X: speeds up compression by generating ZIP in memory\n"
	"         -t: compress and decompress to a new file to inspect compression\n"
	"       -stf: add Compressor tool/Format suffix to generated files\n"
	"        -st: add Compressor tool suffix to generated files\n"
	"        -sf: add Compression format suffix to generated files\n"
	"      -psnr: show artifacts on the destination pic (use with -t)\n"
	" -disable-x: disable 'X' codec (see codec list)\n"
	"\n"
	"Codec profiles:\n"
	"      -fast: sacrifice quality for fast encoding\n"
	"   -regular: high quality and average quality, for regular usage (default)\n"
	"        -hq: most exhaustive methods for best quality (take a lot of time)\n"
	"\n"
	"Scalers:\n"
	"        box: nearest\n"
	"   bilinear: bilinear filter\n"
	"    bicubic: Mitchell & Netravali's two-param cubic filter\n"
	"    bspline: th order (cubic) b-spline\n"
	" catmullrom: Catmull-Rom spline, Overhauser spline\n"
	"    lanczos: Lanczos3 filter\n"
	"    scale2x: Scale2x\n"
	"    super2x: Scale4x with backscale to 2x (default)\n"
	"\n");
}

int TexMain(int argc, char **argv)
{
	double timeelapsed;
	vector<string> drop_files;
	vector<string> add_files;
	char f[MAX_FPATH], path[MAX_FPATH], file[MAX_FPATH];
	int i;

	// parse commandline options
	CommandLineOptions();

	// launched without parms, try to find kain.exe
	i = 0;
	tex_mode = TEXMODE_NORMAL;
	strcpy(tex_srcDir, "");
	strcpy(tex_srcFile, "");
	strcpy(tex_destPath, "");
	tex_destPathUseCodecDir = false;
	drop_files.clear();
	if (argc < 1)
	{
		// launched without parms, try to find engine
		char find[MAX_FPATH];
		bool found_dir = false;
		if (tex_gameDir.c_str()[0])
		{
			for (int i = 0; i < 10; i++)
			{
				strcpy(find, "../");
				for (int j = 0; j < i; j++)
					strcat(find, "../");
				strcat(find, tex_gameDir.c_str());
				if (FS_FindDir(find))
				{
					found_dir = true;
					break;
				}
			}
		}
		if (found_dir)
		{
			Print("Base and output directory detected\n");
			sprintf(tex_srcDir, "%s/", find);
			strcpy(tex_destPath, tex_srcDir);
			tex_destPathUseCodecDir = true;
		}
		else
		{
			Tex_Help();
			Error("no commands specified", progname);
		}
	}
	else 
	{
		bool foundNoParms = true;
		for (i = 0; i < argc; i++)
		{
			if (argv[i][0] == '-')
			{
				foundNoParms = false;
				break;
			}
			if (i > 0)
				drop_files.push_back(argv[i]);
		}
		if (foundNoParms || drop_files.size())
		{
			// dragged files to exe, there is no output path
			strncpy(tex_srcDir, argv[0], sizeof(tex_srcDir));
			if (FS_FindDir(tex_srcDir)) 
			{
				// dragged a directory
				AddSlash(tex_srcDir);
				strcpy(tex_destPath, tex_srcDir);
				tex_destPathUseCodecDir = true;
				tex_mode = TEXMODE_DROP_DIRECTORY;
			}
			else
			{
				// dragged a file
				ExtractFileName(tex_srcDir, tex_srcFile);
				ExtractFilePath(tex_srcDir, tex_destPath);
				strncpy(tex_srcDir, tex_destPath, sizeof(tex_srcDir));
				if (FS_FileMatchList(tex_srcFile, tex_archiveFiles))
				{
					tex_destPathUseCodecDir = true;
					tex_mode = TEXMODE_DROP_DIRECTORY;
				}
				else
					tex_mode = TEXMODE_DROP_FILE;
			}
		}
		else
		{
			// commandline launch
			drop_files.clear();
			strncpy(tex_srcDir, argv[0], sizeof(tex_srcDir));

			// check if input is folder
			// set default output directory
			if (FS_FindDir(tex_srcDir))
				AddSlash(tex_srcDir);
			else
			{
				ExtractFileName(tex_srcDir, tex_srcFile);
				ExtractFilePath(tex_srcDir, tex_srcDir);
			}

			// default output directory
			if (!strcmp(tex_destPath, ""))
			{
				AddSlash(tex_srcDir);
				strcpy(tex_destPath, tex_srcDir);
				tex_destPathUseCodecDir = true;
			}
		}
	}
	AddSlash(tex_srcDir);

	// options
	char *destPath = NULL;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-o"))
		{
			i++;
			if (i < argc)
			{
				destPath = argv[i];
				break;
			}
		}
	}
	if (destPath)
		strncpy(tex_destPath, destPath, sizeof(tex_destPath));

	// find files
	Print("Entering \"%s%s\"\n", tex_srcDir, tex_srcFile);
	textures.clear();
	texturesSkipped = 0;
	FS_ScanPath(tex_srcDir, tex_srcFile, NULL);
	if (drop_files.size())
	{
		for (vector<string>::iterator i = drop_files.begin(); i < drop_files.end(); i++)
		{
			strcpy(f, i->c_str());
			ExtractFilePath(f, path);
			ExtractFileName(f, file);
			Print("Entering \"%s%s\"\n", path, file);
			FS_ScanPath(path, file, NULL);
		}
		drop_files.clear();
	}
	if (texturesSkipped)
		Print("Skipping %i unchanged files\n", texturesSkipped);
	if (!textures.size())
	{
		Print("No files to convert\n");
		return 0;
	}

	// decompress
	if (tex_srcFile[0] && textures.size() == 1)
	{
		char decfile[MAX_FPATH];
		sprintf(decfile, "%s%s", tex_srcDir, tex_srcFile);
		if (TexDecompress(decfile))
			return 0;
	}

	// run conversion
	Print("%i files to encode\n", textures.size());
	TexCompress_Load();
	TexCompressData SharedData;
	memset(&SharedData, 0, sizeof(TexCompressData));
	timeelapsed = ParallelThreads(numthreads, textures.size(), &SharedData, TexCompress_WorkerThread, TexCompress_MainThread);

	// show stats
	Print("Conversion finished!\n");
	Print("--------\n");
	Print("  files exported: %i\n", SharedData.num_exported_files);
	Print("    time elapsed: %i:%02.1f\n", (int)(timeelapsed / 60), (double)(timeelapsed - ((int)(timeelapsed / 60)*60)));
	Print("     input files: %.2f mb\n", SharedData.size_original_files);
	for (TexCodec *codec = tex_codecs; codec; codec = codec->next)
	{
		if (!codec->stat_numTextures)
			continue;
		Print("%s:\n", codec->name);
		Print("  input textures: %.2f mb (%.2f VRAM, %.2f PoT VRAM)\n", codec->stat_inputDiskMB, codec->stat_inputRamMB, codec->stat_inputPOTRamMB);
		Print(" output textures: %.2f mb (%.2f VRAM)\n", codec->stat_outputDiskMB, codec->stat_outputRamMB);
	}
	if (SharedData.zip_len)
		Print("    archive size: %.2f mb\n", SharedData.zip_len / 1048576.0f);
	return 0; 
}