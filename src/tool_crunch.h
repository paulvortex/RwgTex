// tool_crunch.h
#ifndef H_TOOL_CRUNCH_H
#define H_TOOL_CRUNCH_H

#include "crunch/inc/crnlib.h"

void Crunch_Init(void);
void Crunch_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void Crunch_Load(void);
bool Crunch_Compress(TexEncodeTask *task);
const char *Crunch_Version(void);

extern TexTool TOOL_CRUNCH;

#endif