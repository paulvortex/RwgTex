////////////////////////////////////////////////////////////////
//
// RwgTex / file system
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_FS_C

#include "main.h"
#include "zip.h"
#include "unzip.h"
#include "crc32.h"
#include "tex.h"

vector<FS_File> textures;
int texturesSkipped;

void FS_SetFile(FS_File *file, char *fullpath)
{
	char c[MAX_FPATH];

	memset(file, 0, sizeof(FS_File));
	if (!fullpath[0])
		return;
	file->fullpath = fullpath;
	ExtractFilePath(fullpath, c);
	file->path = c;
	ExtractFileBase(fullpath, c);
	file->name = c;
	ExtractFileExtension(fullpath, c);
	file->ext = c;
	ExtractFileSuffix((char *)(file->name.c_str()), c, '_');
	file->suf = c;
}

void FS_SetFile(FS_File *file, char *path, char *name)
{
	char c[MAX_FPATH];

	memset(file, 0, sizeof(FS_File));
	//if (!path[0])
	//	path = "./";
	if (!name[0])
		name = "?";
	file->path = path;
	ExtractFileBase(name, c);
	file->name = c;
	ExtractFileExtension(name, c);
	file->ext = c;
	ExtractFileSuffix((char *)(file->name.c_str()), c, '_');
	file->suf = c;
	// make fullpath
	file->fullpath = file->path.c_str();
	file->fullpath.append(name);
}

FS_File *FS_NewFile(char *filepath)
{
	FS_File *file;

	file = (FS_File *)mem_alloc(sizeof(FS_File));
	memset(file, 0, sizeof(FS_File));
	FS_SetFile(file, filepath);

	return file;
}

FS_File *FS_NewFile(char *path, char *name)
{
	FS_File *file;

	file = (FS_File *)mem_alloc(sizeof(FS_File));
	memset(file, 0, sizeof(FS_File));
	FS_SetFile(file, path, name);

	return file;
}

void FS_FreeFile(FS_File *file)
{
	mem_free(file);
}

/*
==========================================================================================

  FILE CACHE

==========================================================================================
*/

typedef struct
{
	char            filename[MAX_FPATH];
	unsigned int    crc;
	bool            used;
}
FileCacheS;
vector<FileCacheS> FileCache;

bool FS_LoadCache(char *filename)
{
	char line[2048], *crcstr;
	FileCacheS NewFC;
	FILE *f;

	FileCache.clear();
	f = fopen(filename, "r");
	if (!f)
		return false;
	while(fgets(line, 100, f) != NULL)
	{
		if (line[0] == '#' || (line[0] == '/' && line[1] == '/'))
			continue;
		// load line
		crcstr = strstr(line, " 0x");
		if (!crcstr)
			Error("FS_LoadCache: damaged line '%s'", line); 
		memset(NewFC.filename, 0, MAX_FPATH);
		strncpy(NewFC.filename, line, crcstr - line);
		NewFC.crc = ParseNum(crcstr + 1);
		NewFC.used = false;
		FileCache.push_back(NewFC);
	}
	fclose(f);
	return true;
}

void FS_SaveCache(char *filename)
{
	FILE *f;

	f = SafeOpen(filename, "w");
	fprintf(f, "# Crc32 table for source files\n");
	fprintf(f, "# generated automatically, do not modify\n");
	for (std::vector<FileCacheS>::iterator file = FileCache.begin(); file < FileCache.end(); file++)
		if (file->used)
			fprintf(f, "%s 0x%08X\n", file->filename, file->crc);
	fclose(f);
}

unsigned int FS_CRC32(char *filename)
{
	DWORD crc;
	FileCrc32Assembly(filename, crc);
	return (unsigned int)crc;
}

// check if file was modified and updates cache
bool FS_CheckCache(const char *filepath, unsigned int *fileCRC)
{
	char filename[MAX_FPATH];
	unsigned int crc;

	// get crc
	sprintf(filename, "%s%s", tex_srcDir, filepath);
	if (!fileCRC)
		crc = FS_CRC32(filename);
	else
		crc = *fileCRC;

	// find in cache
	for (std::vector<FileCacheS>::iterator file = FileCache.begin(); file < FileCache.end(); file++)
	{
		if (!strnicmp(file->filename, filepath, MAX_FPATH))
		{
			file->used = true;
			if (crc == file->crc)
				return false;
			file->crc = crc;
			return true;
		}
	}

	// not found in cache, add
	FileCacheS NewFC;
	strncpy(NewFC.filename, filepath, MAX_FPATH);
	NewFC.crc = crc;
	NewFC.used = true;
	FileCache.push_back(NewFC);
	return true; 
}

/*
==========================================================================================

  PATTERN MATCH

==========================================================================================
*/

#define MATCH_AUTO     0
#define MATCH_WHOLE    1
#define MATCH_LEFT     2
#define MATCH_RIGHT    3
#define MATCH_MIDDLE   4
#define MATCH_PATTERN  5


// wildcard_least_one: if true * matches 1 or more characters
//                     if false * matches 0 or more characters
int matchpattern_with_separator(const char *in, const char *pattern, int caseinsensitive, const char *separators, bool wildcard_least_one)
{
	int c1, c2;
	while (*pattern)
	{
		switch (*pattern)
		{
		case 0:
			return 1; // end of pattern
		case '?': // match any single character
			if (*in == 0 || strchr(separators, *in))
				return 0; // no match
			in++;
			pattern++;
			break;
		case '*': // match anything until following string
			if(wildcard_least_one)
			{
				if (*in == 0 || strchr(separators, *in))
					return 0; // no match
				in++;
			}
			pattern++;
			while (*in)
			{
				if (strchr(separators, *in))
					break;
				// see if pattern matches at this offset
				if (matchpattern_with_separator(in, pattern, caseinsensitive, separators, wildcard_least_one))
					return 1;
				// nope, advance to next offset
				in++;
			}
			break;
		default:
			if (*in != *pattern)
			{
				if (!caseinsensitive)
					return 0; // no match
				c1 = *in;
				if (c1 >= 'A' && c1 <= 'Z')
					c1 += 'a' - 'A';
				c2 = *pattern;
				if (c2 >= 'A' && c2 <= 'Z')
					c2 += 'a' - 'A';
				if (c1 != c2)
					return 0; // no match
			}
			in++;
			pattern++;
			break;
		}
	}
	if (*in)
		return 0; // reached end of pattern but not end of input
	return 1; // success
}


int matchpattern(const char *in, const char *pattern, bool caseinsensitive)
{
	return matchpattern_with_separator(in, pattern, caseinsensitive, "", false);
}

/*
==========================================================================================

  TOOLS

==========================================================================================
*/

bool FS_CheckOption(FS_File *file, LoadedImage *loadedimage, TexCalcErrors *errorcalc, CompareOption *option, CompareOperator op)
{
	const char *key;
	double floatVal;
	int intVal;
	bool r, got;

	r = false;
	got = false;

	#define StringOp(v) { got = true; \
						 if (op == OPERATOR_NOTEQUAL) r = strnicmp(v, option->pattern.c_str(), strlen(option->pattern.c_str())) != 0; \
						 else if (op == OPERATOR_EQUAL) r = strnicmp(v, option->pattern.c_str(), strlen(option->pattern.c_str())) == 0; \
						 else Error("Bad expression for string: %s %s %s\n", option->parm.c_str(), OptionEnumName((int)op, CompareOperators), option->pattern.c_str()); }
	#define MatchOp(v) { got = true; \
						 if (op == OPERATOR_NOTEQUAL) r = matchpattern(v, option->pattern.c_str(), true) == 0; \
						 else if (op == OPERATOR_EQUAL) r = matchpattern(v, option->pattern.c_str(), true) == 1; \
						 else Error("Bad expression for match: %s %s %s\n", option->parm.c_str(), OptionEnumName((int)op, CompareOperators), option->pattern.c_str()); }
	#define IntOp(v) { got = true; \
						 intVal = atoi(option->pattern.c_str()); \
						 if (op == OPERATOR_NOTEQUAL) r = v != intVal; \
						 else if (op == OPERATOR_EQUAL) r = v == intVal; \
						 else if (op == OPERATOR_GREATER) r = v >= intVal; \
						 else if (op == OPERATOR_NOTGREATER) r = v < intVal; \
						 else if (op == OPERATOR_LESSER) r = v <= intVal; \
						 else if (op == OPERATOR_NOTLESSER) r = v > intVal; \
						 else Error("Bad expression for int: %s %s %s\n", option->parm.c_str(), OptionEnumName((int)op, CompareOperators), option->pattern.c_str()); }
	#define FloatOp(v) { got = true; \
						 floatVal = atof(option->pattern.c_str()); \
						 if (op == OPERATOR_NOTEQUAL) r = v != floatVal; \
						 else if (op == OPERATOR_EQUAL) r = v == floatVal; \
						 else if (op == OPERATOR_GREATER) r = v >= floatVal; \
						 else if (op == OPERATOR_NOTGREATER) r = v < floatVal; \
						 else if (op == OPERATOR_LESSER) r = v <= floatVal; \
						 else if (op == OPERATOR_NOTLESSER) r = v > floatVal; \
						 else Error("Bad expression for float: %s %s %s\n", option->parm.c_str(), OptionEnumName((int)op, CompareOperators), option->pattern.c_str()); }
	#define EnumOp(v,t,tv) { got = true; \
                         t need = (t)OptionEnum(option->pattern.c_str(), tv); \
						 if (op == OPERATOR_NOTEQUAL) r = v != need; \
						 else if (op == OPERATOR_EQUAL) r =  v == need; \
						 else Error("Bad expression for enum: %s %s %s\n", option->parm.c_str(), OptionEnumName((int)op, CompareOperators), option->pattern.c_str()); }

	key = option->parm.c_str();

	// file rules
	if (file != NULL)
	{
		if (!stricmp(key, "path"))
			StringOp(file->fullpath.c_str())
		else if (!stricmp(key, "suffix"))
			StringOp(file->suf.c_str())
		else if (!stricmp(key, "ext"))
			StringOp(file->ext.c_str())
		else if (!stricmp(key, "name"))
			StringOp(file->name.c_str())
		else if (!stricmp(key, "name"))
			MatchOp(file->fullpath.c_str())
	}
	// image rules
	if (!got && loadedimage != NULL)
	{
		if (!stricmp(key, "bpp"))
			IntOp(loadedimage->bpp)
		else if (!stricmp(key, "width"))
			IntOp(loadedimage->bpp)
		else if (!stricmp(key, "height"))
			IntOp(loadedimage->bpp)
		else if (!stricmp(key, "alpha"))
			IntOp(loadedimage->hasAlpha ? 1 : 0)
		else if (!stricmp(key, "srgb"))
			IntOp(loadedimage->sRGB ? 1 : 0)
		else if (!stricmp(key, "type"))
			EnumOp(loadedimage->datatype, ImageType, ImageTypes)
	}
	// error calc
	if (!got && errorcalc != NULL)
	{
		if (!stricmp(key, "error"))
			FloatOp(errorcalc->average)
		else if (!stricmp(key, "dispersion"))
			FloatOp(errorcalc->dispersion)
		else if (!stricmp(key, "rms"))
			FloatOp(errorcalc->rms)
	}

	if (r && option->and.size() > 0)
	{
		// check all "AND"
		for (unsigned int i = 0; i < option->and.size(); i++)
			if (!FS_CheckOption(file, loadedimage, errorcalc, &option->and[i], option->and[i].op))
				return false;
	}

	#undef StringOp
	#undef IntOp
	#undef FloatOp
	#undef EnumOp

	return r;
}

bool FS_FileMatchList(FS_File *file, void *image, void *errorcalc, vector<CompareOption> &list)
{
	LoadedImage *loadedimage = (LoadedImage *)image;
	TexCalcErrors *calcerrors = (TexCalcErrors *)errorcalc;

	// check rules
	for (unsigned int i = 0; i < list.size(); i++)
	{
		CompareOption *o = &list[i];
		if (IsNegativeOp(o->op))
		{
			// exclude rules
			if (FS_CheckOption(file, loadedimage, calcerrors, o, NegateOp(o->op)))
			{
				//Print("Excluded: %s %s %s\n", o->parm.c_str(), OptionEnumName((int)o->op, CompareOperators), o->pattern.c_str());
				return false;
			}
			continue;
		}
		// include rules
		if (FS_CheckOption(file, loadedimage, calcerrors, o, o->op))
		{
			//Print("Included: %s %s %s\n", o->parm.c_str(), OptionEnumName((int)o->op, CompareOperators), o->pattern.c_str());
			return true;
		}
	}
	//Print("Skipped: %s\n", file->fullpath.c_str());
	return false;
}

bool FS_FileMatchList(FS_File *file, void *image, vector<CompareOption> &list)
{
	return FS_FileMatchList(file, image, NULL, list);
}

bool FS_FileMatchList(FS_File *file, vector<CompareOption> &list)
{
	return FS_FileMatchList(file, NULL, NULL, list);
}

bool FS_FileMatchList(char *filename, vector<CompareOption> &list)
{
	FS_File file;

	FS_SetFile(&file, filename);
	return FS_FileMatchList(&file, NULL, NULL, list);
}

/*
==========================================================================================

  SCAN DIRECTORY

==========================================================================================
*/

bool FS_FindDir(char *pattern)
{
#ifdef WIN32
	WIN32_FIND_DATA n_file;
	HANDLE hFile = FindFirstFile(pattern, &n_file);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	bool res = false;
	if (n_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		res = true;
	FindClose(hFile);
	return res;
#else
	#error "FS_FindDir not implemented!"
#endif
}

bool FS_FindFile(char *pattern)
{
#ifdef WIN32
	WIN32_FIND_DATA n_file;
	HANDLE hFile = FindFirstFile(pattern, &n_file);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	bool res = true;
	if (n_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		res = false;
	FindClose(hFile);
	return true;
#else
	#error "FS_FindFile not implemented!"
#endif
}

bool AllowFile(FS_File *file)
{
	if (!FS_FileMatchList(file, tex_includeFiles.items))
		return false;
	return true;
}

bool AddFile(FS_File &file, bool checkinclude, unsigned int *fileCRC)
{
	if (checkinclude)
		if (!AllowFile(&file))
			return false;
	// passed
	textures.push_back(file);
	return true;
}

bool AddArchive(FS_File &archive_file, bool checkinclude)
{
	char filepath[MAX_FPATH];
	FS_File file;

	if (checkinclude)
		if (!AllowFile(&archive_file))
			return false;
	sprintf(filepath, "%s%s", tex_srcDir, archive_file.fullpath.c_str());

	// open zip
	HZIP zh = OpenZip(filepath, "");
	if (!zh)
	{
		Warning("AddArchive: failed to open ZIP archive '%s'", filepath);
		return false;
	}

	// scan zip archive
	for (int i = 0; ; i++)
	{
		ZIPENTRY ze;
		ZRESULT zr = GetZipItem(zh, i, &ze);
		if (zr != ZR_OK)
		{
			if (zr != ZR_ARGS)
				Warning("AddArchive(%s): failed to open entry %i- error code 0x%08X", filepath, i, zr);
			break;
		}
		if (ze.attr & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		FS_SetFile(&file, ze.name);
		            file.zipfile = filepath;
		            file.zipindex = i;
		AddFile(file, true, &ze.crc32);
	}
	CloseZip(zh);
	return true;
}

void FS_ScanPath(char *basepath, const char *singlefile, char *addpath)
{
	char pattern[MAX_FPATH], path[MAX_FPATH], scanpath[MAX_FPATH];
	FS_File file;

	// start path
	strlcpy(pattern, basepath, sizeof(pattern));
	strlcpy(path, "", sizeof(path));
	if (addpath)
	{
		strlcpy(path, addpath, sizeof(path));
		strlcat(path, "/", sizeof(path));
	}
	strlcat(pattern, path, sizeof(pattern));
	if (singlefile && singlefile[0])
		strlcat(pattern, singlefile, sizeof(pattern));
	else
	{
		strlcat(pattern, "*", sizeof(pattern));
		singlefile = NULL;
	}

	// begin search
#ifdef WIN32
	WIN32_FIND_DATA n_file;
	HANDLE hFile = FindFirstFile(pattern, &n_file);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Warning("ScanFiles: failed to open %s", pattern);
		return;
	}
	do
	{
		SimplePacifier();

		// get file
		if (!strnicmp(n_file.cFileName, ".", 1))
			continue;
		if (n_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strlcpy(scanpath, path, sizeof(scanpath));
			strlcat(scanpath, n_file.cFileName, sizeof(scanpath));
			FS_ScanPath(basepath, NULL, scanpath);
			continue;
		}
		FS_SetFile(&file, path, n_file.cFileName);

		// add
		if (FS_FileMatchList(&file, tex_archiveFiles.items))
		{
			AddArchive(file, singlefile ? false : true);
			continue;
		}
		AddFile(file, singlefile ? false : true, NULL);
	}
	while(FindNextFile(hFile, &n_file) != 0);
	FindClose(hFile);
#else

#error "FS_ScanPath not implemented!"

#endif
}	

/*
==========================================================================================

  FILE ACCESSING

==========================================================================================
*/

byte *FS_LoadFile(FS_File *file, size_t *filesize)
{
	char filename[MAX_FPATH], filepath[MAX_FPATH];
	byte *filedata;

	sprintf(filename, "%s%s.%s", file->path.c_str(), file->name.c_str(), file->ext.c_str());
	sprintf(filepath, "%s%s", tex_srcDir, filename);

	// unpack ZIP
	if (!file->zipfile.empty())
	{
		HZIP zh = OpenZip(file->zipfile.c_str(), "");
		if (!zh)
		{
			Warning("FS_LoadFile(%s:%s): failed to open archive", file->zipfile.c_str(), file->fullpath.c_str());
			return NULL;
		}
		ZIPENTRY ze;
		ZRESULT zr = GetZipItem(zh, file->zipindex, &ze);
		if (zr != ZR_OK)
		{
			Warning("FS_LoadFile(%s:%s): failed to seek ZIP entry - error code 0x%08X", file->zipfile.c_str(), file->fullpath.c_str(), zr);
			return NULL;
		}
		filedata = (byte *)mem_alloc(ze.unc_size);
		zr = UnzipItem(zh, file->zipindex, filedata, ze.unc_size);
		if (zr != ZR_OK)
		{
			mem_free(filedata);
			Warning("FS_LoadFile(%s:%s): failed to unpack ZIP entry - error code 0x%08X", file->zipfile.c_str(), file->fullpath.c_str(), zr);
			return NULL;
		}
		CloseZip(zh);
		*filesize = ze.unc_size;
		return filedata;
	}
		
	// read real file
	*filesize = LoadFileUnsafe(filepath, &filedata);
	if (!filedata)
	{
		Warning("%s : cannot open file (%s)", filename, strerror(errno));
		return NULL;
	}
	return filedata;
}

/*
==========================================================================================

  COMMON

==========================================================================================
*/

void FS_Init(void)
{
}

void FS_Shutdown(void)
{
}

void FS_PrintModules(void)
{
}