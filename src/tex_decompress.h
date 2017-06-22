// tex_decompress.h
#ifndef H_TEX_DECOMPRESS_H
#define H_TEX_DECOMPRESS_H

#include "tex.h"

// file decode task
typedef struct TexDecodeTask_s
{
	// initialized right before shipping task to the tool
	char             *filename;
	byte             *data;
	size_t            datasize;
	TexContainer     *container;
	// initialized by container loader
	TexCodec         *codec;
	TexFormat        *format;
	int               width;
	int               height;
	int               numMipmaps;
	byte             *pixeldata;
	size_t            pixeldatasize;
	char             *comment;
	// image parameters (initialized by container loader)
	struct
	{
		int               hasAlpha;
		bool              colorSwap; // true - BGR, false - RGB
		bool              hasAverageColor;
		byte              averagecolor[3];
		bool              isNormalmap;
		bool              sRGB;
	}ImageParms;
	// initialized by exporter code
	LoadedImage      *image;
	// error message
	char              errorMessage[4000];
} TexDecodeTask;

// util
void DecodeFromEncode(TexDecodeTask *out, TexEncodeTask *in, char *filename);
size_t DecompressImage(TexDecodeTask *task);
void UnswizzleImage(TexDecodeTask *task);

// generic
bool  TexDecompress(char *filename);
byte *TexDecompress(char *filename, TexEncodeTask *encodetask, size_t *outdatasize);

#endif