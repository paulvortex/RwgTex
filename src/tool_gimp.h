// tool_gimp.h
#ifndef H_TOOL_GIMPDDS_H
#define H_TOOL_GIMPDDS_H

#include "gimpdds/ddsplugin_lib.h"

void GimpDDS_Init(void);
void GimpDDS_Option(const char *group, const char *key, const char *val, const char *filename, int linenum);
void GimpDDS_Load(void);
bool GimpDDS_Compress(TexEncodeTask *task);
const char *GimpDDS_Version(void);

extern TexTool TOOL_GIMPDDS;

#endif