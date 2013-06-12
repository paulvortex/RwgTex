////////////////////////////////////////////////////////////////
//
// RwgTex / ini file loader
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"

// enumeration
int OptionEnum(const char *name, OptionList *num, int def_value, const char *warningname)
{
	while(1)
	{
		if (!num->name)
		{
			if (warningname)
				Warning("%s: unknown enum option '%s'", warningname, name);
			return def_value;
		}
		if (!stricmp(num->name, name))
			return num->val;
		OptionNextEnum(num);
	}
}
int OptionEnum(const char *name, OptionList *num, int def_value = 0)
{
	return OptionEnum(name, num, def_value, NULL);
}
int OptionEnum(const char *name, OptionList *num)
{
	return OptionEnum(name, num, 0, NULL);
}

// reverse enumeration
char *OptionEnumName(const int val, OptionList *num, char *def_name, const char *warningname)
{
	while(1)
	{
		if (!num->name)
		{
			if (warningname)
				Warning("%s: unknown enum value '%i'", warningname, val);
			return def_name;
		}
		if (num->val == val)
			return num->name;
		OptionNextEnum(num);
	}
}
char *OptionEnumName(const int val, OptionList *num, char *def_name)
{
	return OptionEnumName(val, num, def_name, (const char *)NULL);
}
char *OptionEnumName(const int val, OptionList *num)
{
	return OptionEnumName(val, num, 0, NULL);
}

// boolean option
bool OptionBoolean(const char *val)
{
	if (!stricmp(val, "true"))    return true;
	if (!stricmp(val, "enabled")) return true;
	if (!stricmp(val, "enable"))  return true;
	if (!stricmp(val, "on"))      return true;
	if (!stricmp(val, "yes"))     return true;
	if (!stricmp(val, "1"))       return true;
	return false;
}

// file include/exclude option
bool OptionFCList(FCLIST *list, const char *key, const char *val)
{
	CompareOption O;

	if (stricmp(key, "path") && stricmp(key, "suffix") && stricmp(key, "ext") && stricmp(key, "name") && stricmp(key, "match") && stricmp(key, "bpp") && stricmp(key, "alpha") && stricmp(key, "type") &&
		stricmp(key, "path!") && stricmp(key, "suffix!") && stricmp(key, "ext!") && stricmp(key, "name!") && stricmp(key, "match!") && stricmp(key, "bpp!") && stricmp(key, "alpha!") && stricmp(key, "type!"))
	{
		Warning("Unknown include/exclude list key '%s'", key);
		return false;
	}
	O.parm = key;
	O.pattern = val;
	list->push_back(O);
	return true;
}

// load options file
void LoadOptions(char *filename)
{
	FILE *f;
	char section[64], line[1024], group[64], key[64],  *val;
	TexCodec *codec;
	TexTool *tool;
	int linenum, l;

	// parse file
	sprintf(line, "%s%s", progpath, filename);
	f = fopen(line, "r");
	if (!f)
	{
		Warning("LoadOptions: failed to open '%s' - %s!", filename, strerror(errno));
		return;
	}
	linenum = 0;
	strcpy(section, "GENERAL");
	while (fgets(line, sizeof(line), f) != NULL)
	{
		linenum++;

		// parse comment
		if (line[0] == ';' || line[0] == '#' || line[0] == '\n')
			continue;

		// parse group
		if (line[0] == '[')
		{
			val = strstr(line, "]");
			if (!val)
				Warning("%s:%i: bad group %s", filename, linenum, line);
			else
			{	
				l = min(val - line - 1, sizeof(group) - 1);
				strncpy(group, line + 1, l); group[l] = 0;
				if (group[0] == '!')
				{
					strncpy(section, group + 1, sizeof(section));
					if (!strnicmp(section, "CODEC:", 6))
						codec = findCodec(section + 6, true);
					if (!strnicmp(section, "TOOL:", 5))
						tool = findTool(section + 5, true);
					strcpy(group, "options");
				}
			}
			continue;
		}
			
		// key=value pair
		while(val = strstr(line, "\n")) val[0] = 0;
		val = strstr(line, "=");
		if (!val)
		{
			Warning("%s:%i: bad key pair '%s'", filename, linenum, line);
			continue;
		}
		l = min(val - line, sizeof(key) - 1);
		strncpy(key, line, l); key[l] = 0;
		val++;

		// parse
		if (!strcmp(section, "GENERAL"))
		{
			if (!stricmp(key, "waitforkey"))
				waitforkey = OptionBoolean(val);
			continue;
		}
		if (!strcmp(section, "TEXCOMPRESS"))
		{
			TexCompress_Option(section, group, key, val, filename, linenum);
			continue;
		}
		if (!strnicmp(section, "CODEC:", 6))
		{
			TexCompress_CodecOption(codec, group, key, val, filename, linenum);
			
			continue;
		}
		if (!strnicmp(section, "TOOL:", 5))
		{
			if (tool)
				TexCompress_ToolOption(tool, group, key, val, filename, linenum);
			continue;
		}
		Warning("%s:%i: unknown section '%s'", filename, linenum, section);
	}
	fclose(f);
}