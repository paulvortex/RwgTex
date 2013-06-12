// tool_crunch.h
#ifndef H_TOOL_PVRTEX_H
#define H_TOOL_PVRTEX_H

void PVRTex_Init(void);
void PVRTex_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void PVRTex_Load(void);
bool PVRTex_Compress(TexEncodeTask *task);
const char *PVRTex_Version(void);

extern TexTool TOOL_PVRTEX;

#endif