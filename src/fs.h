// fs.h

#ifndef RWGTEX_FS_H
#define RWGTEX_FS_H

#include "main.h"
#include "zip.h"
#include "unzip.h"
#include "image.h"

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
void         FS_ScanPath(char *basepath, char *singlefile, char *addpath);
byte        *FS_LoadFile(FS_File *file, size_t *filesize);

typedef struct
{
	string parm;
	string pattern;
}CompareOption;

#define FCLIST vector<CompareOption>

extern vector<FS_File> textures;
extern int texturesSkipped;

bool FS_FindDir(char *pattern);
bool FS_FindFile(char *pattern);
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