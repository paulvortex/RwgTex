// codec_pvrtc.h
#ifndef H_CODECPVRTC_H
#define H_CODECPVRTC_H

#include "tex.h"

extern TexCodec CODEC_PVRTC;

void CodecPVRTC_Init(void);
void CodecPVRTC_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void CodecPVRTC_Load(void);
bool CodecPVRTC_Accept(TexEncodeTask *task);
void CodecPVRTC_Encode(TexEncodeTask *task);
void CodecPVRTC_Decode(TexDecodeTask *task);

// associated compression block format and texture format
extern TexBlock  B_PVRTC_2BPP;
extern TexBlock  B_PVRTC_4BPP;
extern TexFormat F_PVRTC_2BPP_RGB;
extern TexFormat F_PVRTC_2BPP_RGBA;
extern TexFormat F_PVRTC_4BPP_RGB;
extern TexFormat F_PVRTC_4BPP_RGBA;

#endif