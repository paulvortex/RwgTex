// tool_atitc.h
#ifndef H_TOOL_ATITC_H
#define H_TOOL_ATITC_H
#ifndef NO_ATITC

#include "aticompress/inc/ATI_Compress.h"

void ATITC_Init(void);
void ATITC_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void ATITC_Load(void);
bool ATITC_Compress(TexEncodeTask *task);
const char *ATITC_Version(void);

extern TexTool TOOL_ATITC;

#endif
#endif