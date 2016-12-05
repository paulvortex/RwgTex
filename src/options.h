// options.h
#ifndef H_OPTIONS_H
#define H_OPTIONS_H

#include "fs.h"

// enum
typedef struct 
{
	char *name;
	int   val;
}OptionList;

#define OptionNextEnum(num) num = (OptionList *)((byte *)num + sizeof(OptionList))

// functions
int    OptionEnum(const char *name, OptionList *num);
int    OptionEnum(const char *name, OptionList *num, int def_value);
int    OptionEnum(const char *name, OptionList *num, int def_value, const char *warningname);
char  *OptionEnumName(const int val, OptionList *num);
char  *OptionEnumName(const int val, OptionList *num, char *def_name);
char  *OptionEnumName(const int val, OptionList *num, char *def_name, const char *warningname);
bool   OptionBoolean(const char *val, bool default_value = false);
int    OptionInt(const char *val);
bool   OptionFCList(FCLIST *list, const char *key, const char *val);
void   LoadOptions(char *filename);

#endif