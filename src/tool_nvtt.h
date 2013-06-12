// tool_nvtt.h
#ifndef H_TOOL_NVTTLIB_H
#define H_TOOL_NVTTLIB_H
#ifndef NO_NVTT

#include <nvtt.h>

void NvTT_Init(void);
void NvTT_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void NvTT_Load(void);
bool NvTT_Compress(TexEncodeTask *task);
const char *NvTT_Version(void);

extern TexTool TOOL_NVTT;

#endif
#endif