// codec_etc2.h
#ifndef H_CODECETC2_H
#define H_CODECETC2_H

#include "tex.h"

extern TexCodec CODEC_ETC2;

void CodecETC2_Init(void);
void CodecETC2_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void CodecETC2_Load(void);
bool CodecETC2_Accept(TexEncodeTask *task);
void CodecETC2_Encode(TexEncodeTask *task);
void CodecETC2_Decode(TexDecodeTask *task);

// associated compression block format and texture format
extern TexBlock  B_ETC2;
extern TexBlock  B_ETC2A;
extern TexBlock  B_ETC2A1;
extern TexBlock  B_EAC1;
extern TexBlock  B_EAC2;

extern TexFormat F_ETC2;
extern TexFormat F_ETC2A;
extern TexFormat F_ETC2A1;
extern TexFormat F_EAC1;
extern TexFormat F_EAC2;

// functions
// extract 4x4 block from source BGRA image
void CodecETC2_ExtractBlockRGBA(const unsigned char *src, int x, int y, int w, int h, unsigned char *block);
void CodecETC2_ExtractBlockAlpha(const unsigned char *src, int x, int y, int w, int h, unsigned char *block);

#endif