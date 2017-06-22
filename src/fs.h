// fs.h
#ifndef RWGTEX_FS_H
#define RWGTEX_FS_H

#include "main.h"
#include "zip.h"
#include "unzip.h"

typedef enum
{
	ARCHIVE_NOT,
	ARCHIVE_ZIP
}
ScanFileArchiveType;

typedef struct
{
	// file info
	string fullpath;
	string path;
	string name;
	string ext;
	string suf;

	// zip info
	string zipfile;
	size_t zipindex;
}
FS_File;

bool         FS_LoadCache(char *filename);
void         FS_SaveCache(char *filename);
unsigned int FS_CRC32(char *filename);
bool         FS_CheckCache(const char *filepath, unsigned int *fileCRC);
void         FS_ScanPath(char *basepath, const char *singlefile, char *addpath);
byte        *FS_LoadFile(FS_File *file, size_t *filesize);

typedef enum
{
	OPERATOR_EQUAL,
	OPERATOR_LESSER,
	OPERATOR_GREATER,
	OPERATOR_NOTEQUAL,
	OPERATOR_NOTLESSER,
	OPERATOR_NOTGREATER
} CompareOperator;

typedef struct CompareOption_s
{
	string parm;
	string pattern;
	CompareOperator op;
	vector<CompareOption_s> and;
}CompareOption;

typedef struct
{
	bool           imageControl; // have rules for image properties, so it require image to be loaded
	bool           errorControl; // have rules for compression errors
	CompareOption *last;         // for &&-options
	vector<CompareOption> items;
}CompareList;

#ifdef F_FS_C
	OptionList CompareOperators[] =
	{
		{ "=", OPERATOR_EQUAL },
		{ "<=", OPERATOR_LESSER },
		{ ">=", OPERATOR_GREATER },
		{ "!=", OPERATOR_NOTEQUAL },
		{ "!<=", OPERATOR_NOTLESSER },
		{ "!>=", OPERATOR_NOTGREATER },
		{ 0 },
	};
#else
extern OptionList CompareOperators[];
#endif

#define NegateOp(op) (op == OPERATOR_EQUAL ? OPERATOR_NOTEQUAL : (op == OPERATOR_LESSER ? OPERATOR_NOTLESSER : (op == OPERATOR_GREATER ? OPERATOR_NOTGREATER : \
	(op == OPERATOR_NOTEQUAL ? OPERATOR_EQUAL : (op == OPERATOR_NOTLESSER ? OPERATOR_LESSER : (op == OPERATOR_NOTGREATER ? OPERATOR_GREATER : OPERATOR_EQUAL))))))

#define IsNegativeOp(op) (op == OPERATOR_NOTEQUAL || op == OPERATOR_NOTLESSER || op == OPERATOR_NOTGREATER)

extern vector<FS_File> textures;
extern int texturesSkipped;

bool FS_FindDir(char *pattern);
bool FS_FindFile(char *pattern);
bool FS_FileMatchList(FS_File *file, void *image, void *errorcalc, vector<CompareOption> &list);
bool FS_FileMatchList(FS_File *file, void *image, vector<CompareOption> &list);
bool FS_FileMatchList(FS_File *file, vector<CompareOption> &list);
bool FS_FileMatchList(char *filename, vector<CompareOption> &list);
void FS_Init(void);
void FS_Shutdown(void);
void FS_PrintModules(void);

FS_File *FS_NewFile(char *filepath);
FS_File *FS_NewFile(char *path, char *name);
void     FS_SetFile(FS_File *file, char *path, char *name);
void     FS_SetFile(FS_File *file, char *fullpath);
void     FS_FreeFile(FS_File *file);

#endif