////////////////////////////////////////////////////////////////
//
// RwgTex / BGRA encoder
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "tex.h"

TexTool TOOL_BGRA =
{
	"BGRA", "RwgTex BGRA exporter", "bgra",
	"BGRA, BGRA6666, BGRA7773, BGRA8871",
	TEXINPUT_BGR | TEXINPUT_BGRA,
	&ToolBGRA_Init,
	&ToolBGRA_Option,
	&ToolBGRA_Load,
	&ToolBGRA_Compress,
	&ToolBGRA_Version,
};

/*
==========================================================================================

  Init

==========================================================================================
*/

void ToolBGRA_Init(void)
{
	RegisterFormat(&F_BGRA, &TOOL_BGRA);
	RegisterFormat(&F_BGR6, &TOOL_BGRA);
	RegisterFormat(&F_BGR3, &TOOL_BGRA);
	RegisterFormat(&F_BGR1, &TOOL_BGRA);
}

void ToolBGRA_Option(const char *group, const char *key, const char *val, const char *filename, int linenum)
{
}

void ToolBGRA_Load(void)
{
}

const char *ToolBGRA_Version(void)
{
	static char versionstring[200];
	sprintf(versionstring, "1.0");
	return versionstring;
}

/*
==========================================================================================

  Compression

==========================================================================================
*/

size_t PackBGRAData(TexEncodeTask *t, byte *stream, byte *data, int width, int height)
{
	if (t->format->block == &B_BGRA || t->format->block == &B_BGR)
	{
		byte *in = data;
		byte *end = in + width * height * t->image->bpp;
		byte *out = stream;
		int bpp = t->format->block->bitlength / 8;
		if (t->image->hasAlpha)
		{
			while(in < end)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				out[3] = in[3];
				out += bpp;		
				in  += t->image->bpp;
			}
		}
		else
		{
			while(in < end)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				out[3] = 255;
				out += bpp;
				in  += t->image->bpp;
			}
		}
		return width * height * ( t->format->block->bitlength / 8);
	}
	Warning("PackBGRA : %s%s.dds - unsupported compression %s/%s", t->file->path.c_str(), t->file->name.c_str(), t->format->name, t->format->block->name);
	return false;
}

bool ToolBGRA_Compress(TexEncodeTask *t)
{
	byte *stream = t->stream;
	stream += PackBGRAData(t, stream, Image_GetData(t->image, NULL), t->image->width, t->image->height);
	for (MipMap *mipmap = t->image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		stream += PackBGRAData(t, stream, mipmap->data, mipmap->width, mipmap->height);
	return true;
}