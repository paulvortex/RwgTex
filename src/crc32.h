// crc32.h
#ifndef H_TEX_CRC32_H
#define H_TEX_CRC32_H

#include "main.h"

#ifndef _QWORD_DEFINED
#define _QWORD_DEFINED
typedef __int64 QWORD, *LPQWORD;
#endif

#define MAKEQWORD(a, b)	((QWORD)( ((QWORD) ((DWORD) (a))) << 32 | ((DWORD) (b))))
#define LODWORD(l) ((DWORD)(l))
#define HIDWORD(l) ((DWORD)(((QWORD)(l) >> 32) & 0xFFFFFFFF))

// Read 4K of data at a time (used in the C++ streams, Win32 I/O, and assembly functions)
#define MAX_BUFFER_SIZE	4096

// Map a "view" size of 10MB (used in the filemap function)
#define MAX_VIEW_SIZE	10485760

DWORD FileCrc32Win32(LPCTSTR szFilename, DWORD &dwCrc32);
DWORD FileCrc32Filemap(LPCTSTR szFilename, DWORD &dwCrc32);
DWORD FileCrc32Assembly(LPCTSTR szFilename, DWORD &dwCrc32);

#endif