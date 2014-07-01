// codec_none.h
#ifndef H_CODECNONE_H
#define H_CODECNONE_H

#include "tex.h"

extern TexCodec CODEC_BGRA;

void CodecBGRA_Init(void);
void CodecBGRA_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void CodecBGRA_Load(void);
bool CodecBGRA_Accept(TexEncodeTask *task);
void CodecBGRA_Encode(TexEncodeTask *task);
void CodecBGRA_Decode(TexDecodeTask *task);

// associated compression block format and texture format
extern TexBlock B_BGRA;
extern TexBlock B_BGR6;
extern TexBlock B_BGR3;
extern TexBlock B_BGR1;

extern TexFormat F_BGRA;
extern TexFormat F_BGR6;
extern TexFormat F_BGR3;
extern TexFormat F_BGR1;

// swizzle functions
void Swizzle_AlphaInRGB6(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode);
void Swizzle_AlphaInRGB3(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode);
void Swizzle_AlphaInRGB1(byte *data, int width, int height, int pitch, int bpp, bool rgbSwap, bool sRGB, bool decode);

#endif