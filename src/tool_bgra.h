// tool_bgra.h
#ifndef H_TOOL_BGRA_H
#define H_TOOL_BGRA_H

void ToolBGRA_Init(void);
void ToolBGRA_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void ToolBGRA_Load(void);
bool ToolBGRA_Compress(TexEncodeTask *task);
const char *ToolBGRA_Version(void);

extern TexTool TOOL_BGRA;

#endif