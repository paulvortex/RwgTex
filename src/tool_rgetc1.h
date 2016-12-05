// tool_rg-etc1.h
#ifndef H_TOOL_RGETC1_H
#define H_TOOL_RGETC1_H

void RgEtc1_Init(void);
void RgEtc1_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void RgEtc1_Load(void);
bool RgEtc1_Compress(TexEncodeTask *task);
const char *RgEtc1_Version(void);

extern TexTool TOOL_RGETC1;

#endif