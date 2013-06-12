// tool_nvdxtlib.h
#ifndef H_TOOL_NVDXTLIB_H
#define H_TOOL_NVDXTLIB_H

void NvDXTLib_Init(void);
void NvDXTLib_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void NvDXTLib_Load(void);
bool NvDXTLib_Compress(TexEncodeTask *task);
const char *NvDXTLib_Version(void);

extern TexTool TOOL_NVDXTLIB;

#endif