// tool_bgra.h
#ifndef H_TOOL_BGRA_H
#define H_TOOL_BGRA_H

void ToolRWGTP_Init(void);
void ToolRWGTP_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void ToolRWGTP_Load(void);
bool ToolRWGTP_Compress(TexEncodeTask *task);
const char *ToolRWGTP_Version(void);

extern TexTool TOOL_RWGTP;

#endif