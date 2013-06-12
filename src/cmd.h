// cmdlib.h
#ifndef H_TEX_CMDLIB_H
#define H_TEX_CMDLIB_H

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string>
#include <vector>

using namespace std;

typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef unsigned int   uint;

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)

// LordHavoc: increased maximum token length from 128 to 16384
#define	MAXTOKEN	16384

// VorteX: avoid 'deprecated' messages
#if _MSC_VER >= 1400
#define stricmp _stricmp
#define strnicmp _strnicmp
#define mkdir _mkdir
#endif

// lists, used for several switches
#define MAX_LIST_ITEMS	256
typedef struct list_s
{
	int			items;
	char		*item[MAX_LIST_ITEMS];
	unsigned char x[MAX_LIST_ITEMS];
}list_t;
list_t *NewList();
void FreeList(list_t *list);
void ListAdd(list_t *list, const char *str, unsigned char x);

// set these before calling CheckParm
extern int myargc;
extern char **myargv;

// consol stuff
extern bool verbose;
extern bool noprint;
extern bool solidpacifier;
void Print (char *str, ...);
void Verbose (char *str, ...);
void Warning (char *str, ...);
void Pacifier(char *str, ...);
void PercentPacifier(char *str, ...);
void SimplePacifier();
void PacifierEnd();

extern char *Q_strupr (char *in);
extern char *Q_strlower (char *in);
extern int Q_strncasecmp (char *s1, char *s2, int n);
extern int Q_strcasecmp (char *s1, char *s2);
extern char *ConvSlashU2W (char *start);
extern char *ConvSlashW2U (char *start);
char *ConvDot(char *start);
//extern void Q_getwd (char *out);
size_t strlcat(char *dst, const char *src, size_t siz);
size_t strlcpy(char *dst, const char *src, size_t siz);

extern unsigned int Q_filelength (FILE *f);
size_t FileSize(const char *filename);
extern int	FileTime (char *path);
void TempFileName(char *out);

extern void	Q_mkdir (char *path);

void AddSlash(char *path);
void AddSlash(string &path);

void CreatePath (char *path);
void ChangeDirectory (char *path);
void GetDirectory(char *path, int size_bytes);
void GetRealPath(char *outpath, char *inpath);

extern double I_DoubleTime (void);
#define I_FloatTime (float)I_DoubleTime()

extern void Error (char *error, ...);

extern bool CheckParm (char *check);

extern FILE *SafeOpen (char *filename, char mode[]);
extern void SafeRead (FILE *f, void *buffer, int count);
extern void SafeWrite (FILE *f, void *buffer, int count);

extern int LoadFile (char *filename, byte **bufferptr);
extern int LoadFileUnsafe (char *filename, byte **bufferptr);

extern void DefaultPath (char *path, char *basepath);
extern void ReplaceExtension (char *path, char *oldextension, char *replacementextension, char *missingextension);
void DefaultExtension (char *path, const char *extension, size_t size_path);

extern void ExtractFilePath (char *path, char *dest);
extern void ExtractFileBase (char *path, char *dest);
extern void ExtractFileName (char *path, char *dest);
extern void StripFileExtension (char *path, char *dest);
extern void ExtractFileExtension (char *path, char *dest);
extern void ExtractFileSuffix(char *path, char *dest, char suffix_sym);
void AddSuffix(char *outpath, char *inpath, char *suffix);
extern bool FileExists(char *filename);

extern int ParseNum (char *str);
int ParseHex(char *hex);

extern char *COM_Parse (char *data);

extern char com_token[MAXTOKEN];
extern bool com_eof;

extern char *copystring(char *s);

extern char token[MAXTOKEN];
extern int	scriptline;

int ReadInt(byte *buffer);
unsigned int ReadUInt(byte *buffer);
unsigned int ReadUShort(byte *buffer);
int ReadShort(byte *buffer);
int ReadSignedByte(byte *buffer);

void StartTokenParsing (char *data);
bool GetToken (bool crossline);
void UngetToken (void);

extern void CRC_Init(unsigned short *crcvalue);
extern void CRC_ProcessByte(unsigned short *crcvalue, byte data);
extern unsigned short CRC_Value(unsigned short crcvalue);

unsigned int crc32(unsigned char *block, unsigned int length);

extern void COM_CreatePath (char *path);
extern void crc32_init();

// file writing and wrapping
#define __CMDLIB_WRAPFILES__
void FreeWrappedFiles();
int CountWrappedFiles();
int LoadWrappedFile(int wrapnum, byte **bufferptr, char **realfilename);
void WrapFileWritesToMemory();
FILE *SafeOpenWrite (char *filename);
FILE *OpenReadWrite(char *filename);
void WriteClose(FILE *f);

#endif