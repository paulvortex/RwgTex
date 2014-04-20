////////////////////////////////////////////////////////////////
//
// RwgTex / Gimp DDS Plugin compressor support
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

TexTool TOOL_GIMPDDS =
{
	"GimpDDS", "Gimp DDS Plugin", "gimp",
	"YCoGg, DXT",
	TEXINPUT_BGRA,
	&GimpDDS_Init,
	&GimpDDS_Option,
	&GimpDDS_Load,
	&GimpDDS_Compress,
	&GimpDDS_Version,
};

// tool options
int            gimpdds_dithering;
DDS_COLOR_TYPE gimpdds_colorBlockMethod;

/*
==========================================================================================

  Init

==========================================================================================
*/

void GimpDDS_Init(void)
{
	RegisterFormat(&F_DXT1, &TOOL_GIMPDDS);
	RegisterFormat(&F_DXT1A, &TOOL_GIMPDDS);
	RegisterFormat(&F_DXT2, &TOOL_GIMPDDS);
	RegisterFormat(&F_DXT3, &TOOL_GIMPDDS);
	RegisterFormat(&F_DXT4, &TOOL_GIMPDDS);
	RegisterFormat(&F_DXT5, &TOOL_GIMPDDS);
	RegisterFormat(&F_RXGB, &TOOL_GIMPDDS);
	RegisterFormat(&F_YCG1, &TOOL_GIMPDDS);
	RegisterFormat(&F_YCG2, &TOOL_GIMPDDS);
	RegisterFormat(&F_YCG3, &TOOL_GIMPDDS);
	RegisterFormat(&F_YCG4, &TOOL_GIMPDDS);
	// options
	gimpdds_dithering = false;
	gimpdds_colorBlockMethod = DDS_COLOR_MAX;
}

void GimpDDS_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
	if (!stricmp(group, "options"))
	{
		if (!stricmp(key, "dither"))
			gimpdds_dithering = OptionBoolean(val) ? 1 : 0;
		else if (!stricmp(key, "color_method"))
		{
			if (!stricmp(val, "distance"))
				gimpdds_colorBlockMethod = DDS_COLOR_DISTANCE;
			else if (!stricmp(val, "luminance"))
				gimpdds_colorBlockMethod = DDS_COLOR_LUMINANCE; 
			else if (!stricmp(val, "insetbox"))
				gimpdds_colorBlockMethod = DDS_COLOR_INSET_BBOX; 
			else if (!stricmp(val, "colormax"))
				gimpdds_colorBlockMethod = DDS_COLOR_MAX; 
			else
				gimpdds_colorBlockMethod = DDS_COLOR_MAX; 
		}
		else
			Warning("%s:%i: unknown key '%s'", filename, linenum, key);
		return;
	}
	Warning("%s:%i: unknown group '%s'", filename, linenum, group);
}

void GimpDDS_Load(void)
{
	if (CheckParm("-dither"))    gimpdds_dithering = 1;
	if (CheckParm("-distance"))  gimpdds_colorBlockMethod = DDS_COLOR_DISTANCE;
	if (CheckParm("-luminance")) gimpdds_colorBlockMethod = DDS_COLOR_LUMINANCE;
	if (CheckParm("-insetbox"))  gimpdds_colorBlockMethod = DDS_COLOR_INSET_BBOX;
	if (CheckParm("-colormax"))  gimpdds_colorBlockMethod = DDS_COLOR_MAX;

	// note
	if (gimpdds_dithering)
		Print("%s tool: enabled dithering\n", TOOL_GIMPDDS.name);
	if (gimpdds_colorBlockMethod != DDS_COLOR_MAX)
	{
		if (gimpdds_colorBlockMethod == DDS_COLOR_DISTANCE)
			Print("%s: COLOR_DISTANCE method for  block compression\n", TOOL_GIMPDDS.name);
		else if (gimpdds_colorBlockMethod == DDS_COLOR_LUMINANCE)
			Print("%s: COLOR_LUMINANCE method for block compression\n", TOOL_GIMPDDS.name);
		else if (gimpdds_colorBlockMethod == DDS_COLOR_INSET_BBOX)
			Print("%s: INSET_BBOX method for block compression\n", TOOL_GIMPDDS.name);
	}
}

const char *GimpDDS_Version(void)
{
	static char versionstring[200];
	sprintf(versionstring, "%i.%i.%i", DDS_PLUGIN_VERSION_MAJOR, DDS_PLUGIN_VERSION_MINOR, DDS_PLUGIN_VERSION_REVISION);
	return versionstring;
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

typedef struct gimpdds_options_s
{
	DDS_COMPRESSION_TYPE compressionType;
	DDS_COLOR_TYPE colorBlockMethod;
	int dithering;
} gimpdds_options_t;

unsigned int GimpGetCompressedSize(int width, int height, int bpp, int level, int num, int format)
{
	int w, h, n = 0;
	unsigned int size = 0;

	w = width >> level;
	h = height >> level;
	w = max(1, w);
	h = max(1, h);
	w <<= 1;
	h <<= 1;

	while(n < num && (w != 1 || h != 1))
	{
		if(w > 1) w >>= 1;
		if(h > 1) h >>= 1;
		if(format == DDS_COMPRESS_NONE)
			size += (w * h);
		else
			size += ((w + 3) >> 2) * ((h + 3) >> 2);
		++n;
	}
	if(format == DDS_COMPRESS_NONE)
		size *= bpp;
	else
	{
		if(format == DDS_COMPRESS_BC1 || format == DDS_COMPRESS_BC4)
			size *= 8;
		else
			size *= 16;
	}
	return(size);
}

int GimpDDS_CompressSingleImage(byte *dst, byte *src, int w, int h, gimpdds_options_t *options)
{
	switch(options->compressionType)
    {
		case DDS_COMPRESS_BC1:
			compress_DXT1(dst, src, w, h, options->colorBlockMethod, options->dithering, 1);
			break;
		case DDS_COMPRESS_BC2:
			compress_DXT3(dst, src, w, h, options->colorBlockMethod, options->dithering);
			break;
		case DDS_COMPRESS_BC3:
		case DDS_COMPRESS_BC3N:
		case DDS_COMPRESS_RXGB:
		case DDS_COMPRESS_AEXP:
		case DDS_COMPRESS_YCOCG:
			compress_DXT5(dst, src, w, h, options->colorBlockMethod, options->dithering);
			break;
		case DDS_COMPRESS_BC4:
			compress_BC4(dst, src, w, h);
            break;
		case DDS_COMPRESS_BC5:
			compress_BC5(dst, src, w, h);
			break;
		case DDS_COMPRESS_YCOCGS:
			compress_YCoCg(dst, src, w, h);
			break;
		default:
            compress_DXT5(dst, src, w, h, options->colorBlockMethod, options->dithering);
            break;
	}
	return GimpGetCompressedSize(w, h, 0, 0, 1, options->compressionType);
}

bool GimpDDS_Compress(TexEncodeTask *t)
{
	gimpdds_options_t options;
	byte *data;
	size_t res;

	// get options
	if (t->format->block == &B_DXT1)
		options.compressionType = DDS_COMPRESS_BC1;
	else if (t->format->block == &B_DXT3)
		options.compressionType = DDS_COMPRESS_BC2;
	else if (t->format->block == &B_DXT5)
	{
		if (t->format == &F_YCG1 || t->format == &F_YCG3)
			options.compressionType = DDS_COMPRESS_YCOCG;
		else if (t->format == &F_YCG2 || t->format == &F_YCG4)
			options.compressionType = DDS_COMPRESS_YCOCGS;
		else
			options.compressionType = DDS_COMPRESS_BC3;
	}
	else
	{
		Warning("GimpDDS : %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
		return false;
	}
	options.colorBlockMethod = gimpdds_colorBlockMethod;
	options.dithering = gimpdds_dithering;

	// compress
	data = t->stream;
	res = GimpDDS_CompressSingleImage(data, Image_GetData(t->image, NULL), t->image->width, t->image->height, &options);
	if (res >= 0)
	{
		data += res;
		// mipmaps
		for (MipMap *mipmap = t->image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			res = GimpDDS_CompressSingleImage(data, mipmap->data, mipmap->width, mipmap->height, &options);
			if (res < 0)
				break;
			data += res;
		}
	}

	// end, advance stats
	if (res < 0)
	{
		Warning("GimpDDS : %s%s.dds - compressor fail (error code %i)", t->file->path.c_str(), t->file->name.c_str(), res);
		return false;
	}
	return true;
}