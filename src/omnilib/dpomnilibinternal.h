// dpomnilibinternal.h
#ifndef F_DPOMNILIBINTERNAL_H
#define F_DPOMNILIBINTERNAL_H

// dependencies
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <errno.h>
#if defined(WIN32) || defined(_WIN64)
	#include <limits.h>
	#include <direct.h>
	#include <windows.h>
#endif

// max path
#ifndef MAX_FPATH
#define MAX_FPATH 8192
#endif

namespace omnilib
{

// print/error functions
void _omnilib_print(char *str, ...);
void _omnilib_verbose(char *str, ...);
void _omnilib_warning(char *str, ...);
void _omnilib_error(char *str, ...);

// memory functions
void *_omnilib_malloc(size_t size);
void *_omnilib_realloc(void *buf, size_t size);
void  _omnilib_free(void *ptr);

// memory stream
typedef struct
{
	unsigned char *buffer;
	unsigned char *ptr;
	int            used;
	int            size;
} _omnilib_membuf_t;
_omnilib_membuf_t *_omnilib_bcreate(int size);
void  _omnilib_bputlittleint(_omnilib_membuf_t *b, int n);
void  _omnilib_bputlittlefloat(_omnilib_membuf_t *b, float n);
void  _omnilib_bwrite(_omnilib_membuf_t *b, void *data, int size);
int   _omnilib_brelease(_omnilib_membuf_t *b, void **bufferptr);
void  _omnilib_bfree(_omnilib_membuf_t *b, unsigned char **bufferptr);

// file operations
FILE *_omnilib_safeopen(char *filename, char mode[]);
void  _omnilib_createpath(char *createpath);

}

#endif
