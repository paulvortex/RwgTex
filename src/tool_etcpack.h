// tool_etcpack.h
#ifndef H_TOOL_ETCPACK_H
#define H_TOOL_ETCPACK_H

void ETCPack_Init(void);
void ETCPack_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void ETCPack_Load(void);
bool ETCPack_Compress(TexEncodeTask *task);
const char *ETCPack_Version(void);

extern TexTool TOOL_ETCPACK;

#endif