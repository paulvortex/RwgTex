// codec_pvrtc.h
#ifndef H_CODECPVRTC2_H
#define H_CODECPVRTC2_H

#include "tex.h"

extern TexCodec CODEC_PVRTC2;

void CodecPVRTC2_Init(void);
void CodecPVRTC2_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void CodecPVRTC2_Load(void);
bool CodecPVRTC2_Accept(TexEncodeTask *task);
void CodecPVRTC2_Encode(TexEncodeTask *task);
void CodecPVRTC2_Decode(TexDecodeTask *task);

// associated compression block format and texture format
extern TexBlock  B_PVRTC2_2BPP;
extern TexBlock  B_PVRTC2_4BPP;

extern TexFormat F_PVRTC2_2BPP;
extern TexFormat F_PVRTC2_4BPP;

#endif