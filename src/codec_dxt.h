// codec_dxt.h
#ifndef H_CODECDXT_H
#define H_CODECDXT_H

#include "tex.h"

extern TexCodec CODEC_DXT;

void CodecDXT_Init(void);
void CodecDXT_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void CodecDXT_Load(void);
bool CodecDXT_Accept(TexEncodeTask *task);
void CodecDXT_Encode(TexEncodeTask *task);
void CodecDXT_Decode(TexDecodeTask *task);

// associated compression block format and texture format
extern TexBlock  B_DXT1;
extern TexBlock  B_DXT2;
extern TexBlock  B_DXT3;
extern TexBlock  B_DXT4;
extern TexBlock  B_DXT5;
extern TexFormat F_DXT1;
extern TexFormat F_DXT1A;
extern TexFormat F_DXT2;
extern TexFormat F_DXT3;
extern TexFormat F_DXT4;
extern TexFormat F_DXT5;
extern TexFormat F_RXGB;
extern TexFormat F_YCG1;
extern TexFormat F_YCG2;
extern TexFormat F_YCG3;
extern TexFormat F_YCG4;

// swizzle functions
void Swizzle_Premult(LoadedImage *image, bool decode);
void Swizzle_XGBR(LoadedImage *image, bool decode);
void Swizzle_AGBR(LoadedImage *image, bool decode);
void Swizzle_YCoCg(LoadedImage *image, bool decode);
void Swizzle_YCoCg_Gamma2(LoadedImage *image, bool decode);
void Swizzle_YCoCgScaled(LoadedImage *image, bool decode);
void Swizzle_YCoCgScaled_Gamma2(LoadedImage *image, bool decode);

#endif