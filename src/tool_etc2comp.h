// tool_etc2comp.h
#ifndef H_TOOL_ETC2COMP_H
#define H_TOOL_ETC2COMP_H

void Etc2Comp_Init(void);
void Etc2Comp_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void Etc2Comp_Load(void);
bool Etc2Comp_Compress(TexEncodeTask *task);
const char *Etc2Comp_Version(void);

extern TexTool TOOL_ETC2COMP;

#endif