////////////////////////////////////////////////////////////////
//
// DpOmniTool / Sprite Stuff
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "dpomnilibinternal.h"

namespace omnilib
{

// function pointers
void *(*_omnilib_ptr_malloc)(size_t) = malloc;
void *(*_omnilib_ptr_realloc)(void *,size_t) = realloc;
void  (*_omnilib_ptr_free)(void*) = free;
void  (*_omnilib_ptr_print)(int, char *) = NULL;
void  (*_omnilib_ptr_error)(char *) = NULL;

/*
==========================================================================================

  PRINT/ERROR FUNCTIONS

==========================================================================================
*/

// _omnilib_print
// prints out message
void _omnilib_print(char *str, ...)
{
	va_list argptr;
	char msg[2048];
	// sanity check
	if (_omnilib_ptr_print == NULL)
		return;
	// print
	va_start(argptr, str);
	vsnprintf(msg, sizeof(msg), str, argptr);
	va_end(argptr);
	_omnilib_ptr_print(0, msg);
}

// _omnilib_verbose
// prints out verbose message
void _omnilib_verbose(char *str, ...)
{
	va_list argptr;
	char msg[2048];
	// sanity check
	if (_omnilib_ptr_print == NULL)
		return;
	// print
	va_start(argptr, str);
	vsnprintf(msg, sizeof(msg), str, argptr);
	va_end(argptr);
	_omnilib_ptr_print(1, msg);
}

// _omnilib_warning
// prints out warning
void _omnilib_warning(char *str, ...)
{
	va_list argptr;
	char msg[2048];
	// sanity check
	if (_omnilib_ptr_print == NULL)
		return;
	// print
	va_start(argptr, str);
	vsnprintf(msg, sizeof(msg), str, argptr);
	va_end(argptr);
	_omnilib_ptr_print(2, msg);
}

// _omnilib_error
// prints out error (and stops execution)
void _omnilib_error(char *str, ...)
{
	va_list argptr;
	char msg[2048];
	// sanity check
	if (_omnilib_ptr_error == NULL)
		return;
	// print
	va_start(argptr, str);
	vsnprintf(msg, sizeof(msg), str, argptr);
	va_end(argptr);
	_omnilib_ptr_error(msg);
}

/*
==========================================================================================

  MEMORY FUNCTIONS

==========================================================================================
*/

// _omnilib_malloc
// allocate new memory chunk
void *_omnilib_malloc(size_t size)
{
	void *ptr;

	// sanity check
	if (size == 0 || _omnilib_ptr_malloc == NULL)
		return NULL;
	// allocate
	ptr = _omnilib_ptr_malloc(size);
	if (ptr == NULL)
		return NULL;
	// zero out and return
	memset(ptr, 0, size);
	return ptr;
}

// _omnilib_realloc
// resize current memory chunk (keeps data)
void *_omnilib_realloc(void *buf, size_t size)
{
	void *ptr;

	// sanity check
	if (size == 0 || _omnilib_ptr_realloc == NULL)
		return NULL;
	// allocate
	ptr = _omnilib_ptr_realloc(buf, size);
	if (ptr == NULL)
		return NULL;
	// return
	return ptr;
}

// _omnilib_free
// release memory chunk
void _omnilib_free(void *ptr)
{
	// sanity check
	if (ptr == NULL || _omnilib_ptr_free == NULL)
		return;
	// free
	_omnilib_ptr_free(ptr);
}

/*
==========================================================================================

  MEMORY STREAM

==========================================================================================
*/

// _omnilib_bcreate()
// creates new buffer
_omnilib_membuf_t *_omnilib_bcreate(int size)
{
	_omnilib_membuf_t *b;

	b = (_omnilib_membuf_t *)_omnilib_malloc(sizeof(_omnilib_membuf_t));
	memset(b, 0, sizeof(_omnilib_membuf_t));
	if (size)
	{
		b->ptr = b->buffer = (unsigned char *)_omnilib_malloc(size);
		b->size = size;
	}
	return b;
}

// _omnilib_bgrow()
// checks if buffer can take requested amount of data, if not - expands
void _omnilib_bgrow(_omnilib_membuf_t *b, int morebytes)
{
	int newused;

	newused = b->used + morebytes;
	if (newused > b->size)
	{
		b->size = newused + (1024 * 16); // grow size
		if (b->buffer)
			b->buffer = (unsigned char *)_omnilib_realloc(b->buffer, b->size);
		else
			b->buffer = (unsigned char *)_omnilib_malloc(b->size);
		b->ptr = b->buffer + b->used;
	}
}

// bputlitleint()
// puts a little int to buffer
void _omnilib_bputlittleint(_omnilib_membuf_t *b, int n)
{
	_omnilib_bgrow(b, 4);
	b->ptr[0] = n & 0xFF;
	b->ptr[1] = (n >> 8) & 0xFF;
	b->ptr[2] = (n >> 16) & 0xFF;
	b->ptr[3] = (n >> 24) & 0xFF;
	b->used += 4;
	b->ptr += 4;
}

// _omnilib_bputlittlefloat()
// puts a little float to buffer
void _omnilib_bputlittlefloat(_omnilib_membuf_t *b, float n)
{
	union {int i; float f;} in;
	in.f = n;
	_omnilib_bputlittleint(b, in.i);
}

// _omnilib_bwrite()
// writes arbitrary data grow buffer
void _omnilib_bwrite(_omnilib_membuf_t *b, void *data, int size)
{
	_omnilib_bgrow(b, size);
	memcpy(b->ptr, data, size);
	b->used += size;
	b->ptr += size;
}

// _omnilib_brelease()
// returns size of used data of buffer, fills bufferptr and frees temp data
int _omnilib_brelease(_omnilib_membuf_t *b, void **bufferptr)
{
	int bufsize;
	
	*bufferptr = b->buffer;
	bufsize = b->used;
	_omnilib_free(b);
	return bufsize;
}

// _omnilib_bfree()
// free buffer and stored data
void _omnilib_bfree(_omnilib_membuf_t *b, unsigned char **bufferptr)
{
	if (b->buffer)
		_omnilib_free(b->buffer);
	_omnilib_free(b);
}

/*
==========================================================================================

  FILE OPERATIONS

==========================================================================================
*/

// _omnilib_safeopen
// opens a file handle, casts error on fail
FILE *_omnilib_safeopen(char *filename, char mode[])
{
	FILE *f;
	if (mode[0] == 'w')
		_omnilib_createpath(filename);
	f = fopen(filename, mode);
	if(!f)
		_omnilib_error("_omnilib_safeopen %s: %s", filename, strerror(errno));
	return f;
}


// _omnilib_createpath
// creates a path for file to be saved
void _omnilib_createpath(char *createpath)
{
	char *ofs, save, *opath;
	char path[MAX_FPATH];

	strncpy(path, createpath, sizeof(path)); 
	opath = path;
	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{
			// create the directory
			save = *ofs;
			*ofs = 0;
			if (path[0] && path[strlen(path)-1] != ':' && strcmp(path, "..") && strcmp(path, "."))
			{
				#if defined(WIN32) || defined(_WIN64)
				  if (_mkdir (path) != -1)
				#else
				  if (mkdir (path, 0777) != -1)
				#endif
				if (errno != 0 && errno != EEXIST)
					_omnilib_error("_omnilib_createpath '%s': %s %i", opath, strerror(errno), errno);
			}
			*ofs = save;
		}
	}
}

/*
==========================================================================================

  LIBRARY EXTERNAL

==========================================================================================
*/

// OmnilibSetMemFunc
// set the memory functions
void OmnilibSetMemFunc(void *(*allocfunc)(size_t), void *(*reallocfunc)(void *,size_t), void (*freefunc)(void *))
{
	_omnilib_ptr_malloc = allocfunc;
	_omnilib_ptr_realloc = reallocfunc;
	_omnilib_ptr_free = freefunc;
}

// OmnilibSetPrintFunc
// set the print function
void OmnilibSetMessageFunc(void (*messagefunc)(int, char *), void (*errorfunc)(char *))
{
	_omnilib_ptr_print = messagefunc;
	_omnilib_ptr_error = errorfunc;
}

}