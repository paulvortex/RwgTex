// codec_etc1.h
#ifndef H_CODECETC1_H
#define H_CODECETC1_H

#include "tex.h"

extern TexCodec CODEC_ETC1;

void CodecETC1_Init(void);
void CodecETC1_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void CodecETC1_Load(void);
bool CodecETC1_Accept(TexEncodeTask *task);
void CodecETC1_Encode(TexEncodeTask *task);
void CodecETC1_Decode(TexDecodeTask *task);

// associated compression block format and texture format
extern TexBlock B_ETC1;
extern TexFormat F_ETC1;

// functions
// extract 4x4 block from source image
void ETC1_ExtractBlock(const unsigned char *src, int x, int y, int w, int h, unsigned char *block);

#endif