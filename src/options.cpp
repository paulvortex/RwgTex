////////////////////////////////////////////////////////////////
//
// RwgTex / ini file loader
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"

// OptionEnum
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

// OptionEnumName
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

// OptionBoolean
// boolean option
bool OptionBoolean(const char *val, bool default_value)
{
	if (!stricmp(val, "true"))    return true;
	if (!stricmp(val, "enabled")) return true;
	if (!stricmp(val, "enable"))  return true;
	if (!stricmp(val, "on"))      return true;
	if (!stricmp(val, "yes"))     return true;
	if (!stricmp(val, "1"))       return true;

	if (!stricmp(val, "false"))    return false;
	if (!stricmp(val, "disabled")) return false;
	if (!stricmp(val, "disable"))  return false;
	if (!stricmp(val, "off"))      return false;
	if (!stricmp(val, "no"))       return false;
	if (!stricmp(val, "0"))        return false;

	if (!stricmp(val, "default"))  return false;

	return default_value;
}

// OptionFCList
// file include/exclude option
void OptionFCList(CompareList *list, const char *key, const char *val)
{
	CompareOperator op;
	bool appendLast;
	int len;

	len = strlen(key);
	op = OPERATOR_EQUAL;

	// check if appending to previous (as AND statement)
	appendLast = false;
	if (len > 2)
	{
		if (key[0] == '&' && key[1] == '&')
		{
			len -= 2;
			key += 2;
			appendLast = true;
		}
	}

	// check special operators
	if (len > 1)
	{
		if (key[len - 1] == '!') // !=
		{
			op = OPERATOR_NOTEQUAL;
			len--;
		}
		else if (key[len - 1] == '>') // >=, !>=
		{
			if (len > 2 && key[len - 2] == '!')
			{
				op = OPERATOR_NOTGREATER;
				len -= 2;
			}
			else
			{
				op = OPERATOR_GREATER;
				len--;
			}
		}
		else if (key[len - 1] == '<') // <=, !<=
		{
			if (len > 2 && key[len - 2] == '!')
			{
				op = OPERATOR_NOTLESSER;
				len -= 2;
			}
			else
			{
				op = OPERATOR_LESSER;
				len--;
			}
		}
	}

	// check key
	if (strncmp(key, "path", len) != 0 && strncmp(key, "suffix", len) != 0 && strncmp(key, "ext", len) != 0 && strncmp(key, "name", len) != 0 && strncmp(key, "match", len) != 0)
	{
		if (strncmp(key, "bpp", len) == 0 || strncmp(key, "alpha", len) == 0 || strncmp(key, "srgb", len) == 0 || strncmp(key, "type", len) == 0 || strncmp(key, "width", len) == 0 || strncmp(key, "height", len) == 0)
			list->imageControl = true;
		else if (strncmp(key, "error", len) == 0 || strncmp(key, "dispersion", len) == 0 || strncmp(key, "rms", len) == 0)
			list->errorControl = true;
		else
		{
			Warning("Unknown include/exclude list key '%s' (len %i)", key, len);
			return;
		}
	}

	// add key
	CompareOption O;
	O.op = op;
	O.parm = key;
	O.pattern = val;
	if (op != OPERATOR_EQUAL)
		O.parm.pop_back();
	if (appendLast)
	{
		// append AND
		if (list->last == NULL)
		{
			// new expression
			Warning("Cannot add AND statement ( %s %s %s ) - no start statement defined, addind as new expression", O.parm.c_str(), OptionEnumName((int)op, CompareOperators), val);
			list->items.push_back(O);
			list->last = &list->items[list->items.size() - 1];
		}
		else
		{
			list->last->and.push_back(O);
		}
	}
	else
	{
		// new expression
		list->items.push_back(O);
		list->last = &list->items[list->items.size() - 1];
	}
}

// OptionInt
// int option
int OptionInt(const char *val)
{
	if (val == NULL)
	{
		Warning("Value NULL cannot be parsed as integer");
		return 0;
	}
	if (*val == '\0')
	{
		Warning("Value '%s' cannot be parsed as integer", val);
		return 0;
	}
	const char *s = val;
	bool negate = (s[0] == '-');
	if (*s == '+' || *s == '-')
		++s;
	if (*s == '\0')
	{
		Warning("Value '%s' cannot be parsed as integer", val);
		return 0;
	}
	int result = 0;
	while (*s)
	{
		if (*s >= '0' && *s <= '9')
			result = result * 10 - (*s - '0'); // assume negative number
		else
		{
			Warning("Value '%s' cannot be parsed as integer", val);
			return 0;
		}
		++s;
	}
	return negate ? result : -result; // -result is positive!
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
					{
						codec = findCodec(section + 6, true);
						if (!codec)
							Error("LoadOptions: unknown codec section [%s], please fix your config file\n", section);
					}
					if (!strnicmp(section, "TOOL:", 5))
					{
						tool = findTool(section + 5, true);
						if (!tool)
							Error("LoadOptions: unknown tool section [%s], please fix your config file\n", tool);
					}
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
				waitforkey = OptionBoolean(val, waitforkey);
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