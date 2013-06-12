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

// associated compression block format and texture format
extern TexBlock  B_ETC2;
extern TexBlock  B_ETC2A;

extern TexFormat F_ETC2_RGB;
extern TexFormat F_ETC2_RGBA;
extern TexFormat F_ETC2_RGBA1;
extern TexFormat F_EAC1;
extern TexFormat F_EAC2;

#endif