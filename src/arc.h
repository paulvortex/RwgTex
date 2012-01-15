////////////////////////////////////////////////////////////////
//
// Blood Pill - zlib support for PK3 writing
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////

#include "cmd.h"

#ifndef __ZLIB_H__
#define __ZLIB_H__

#include <zlib.h>

// zlib errors
char *zlibErrorString(int ret);

// exporting zipfile structure
#define MAX_ZLIB_FILES 65536
typedef struct zlib_entry_s
{
	z_stream stream;
	unsigned short	xver; // version needed to extract
	unsigned short	bitflag; // general purpose bit flag
	unsigned short	compression; // compression method
	unsigned short	mtime; // last mod file time
	unsigned short	mdate; // last mod file date 
	unsigned int	crc32; // crc-32
	unsigned int	csize; // compressed size
	unsigned int	usize; // uncompressed size 
	unsigned int	offset; // relative offset of local header 4 bytes
	unsigned short	filenamelen;
	char			filename[1024];
}zlib_entry_t;

typedef struct zlib_file_s
{
	FILE *file;
	zlib_entry_t *files[MAX_ZLIB_FILES];
	unsigned short numfiles;
	
}zlib_file_t;

// zip writing
zlib_file_t  *Zlib_Create(char *filepath);
zlib_entry_t *Zlib_CreateFile(zlib_file_t *zip, char *filename);
void          Zlib_CompressFileData(zlib_file_t *zip, zlib_entry_t *entry, unsigned char *filedata, unsigned int datasize);
void          Zlib_AddFile(zlib_file_t *zip, char *filename, unsigned char *filedata, unsigned int datasize);
void          Zlib_AddExternalFile(zlib_file_t *zip, char *filename, char *externalfile);
void          Zlib_Close(zlib_file_t *zip);

#ifdef __CMDLIB_WRAPFILES__
void Zlib_AddWrappedFiles(zlib_file_t *zip);
#endif

//
// zip file structure
//

#define FOURCC_ZIP_ENTRY          0x04034b50
#define FOURCC_ZIP_CENTRALDIR     0x02014b50
#define FOURCC_ZIP_TAIL           0x06054b50

typedef struct
{
	unsigned long   fourCC;
	unsigned short	xver;        // version needed to extract
	unsigned short	bitflag;     // general purpose bit flag
	unsigned short	compression; // compression method
	unsigned short	mtime;       // last mod file time
	unsigned short	mdate;       // last mod file date 
	unsigned int	crc32;       // crc-32
	unsigned int	csize;       // compressed size
	unsigned int	usize;       // uncompressed size 
	unsigned short	filenamelen; // length of filename
	unsigned short  xfieldlen;   // length of extra field
	// then goes filename, filedata, extra field...
}
ZIPLOCALHEADER;
#define ZIPLOCALHEADER_SIZE 30

typedef struct
{
	unsigned long   fourCC;
	unsigned short  mver;        // version made by
	unsigned short	xver;        // version needed to extract
	unsigned short	bitflag;     // general purpose bit flag
	unsigned short	compression; // compression method
	unsigned short	mtime;       // last mod file time
	unsigned short	mdate;       // last mod file date 
	unsigned int	crc32;       // crc-32
	unsigned int	csize;       // compressed size
	unsigned int	usize;       // uncompressed size 
	unsigned short	filenamelen; // length of filename
	unsigned short  xfieldlen;   // length of extra field
	unsigned short  commentlen;  // length of comment
	unsigned short  disknum;     // disk number start
	unsigned short  iattr;       // internal file attributes
	unsigned int    ettr;        // external file attributes
	unsigned int	offset;      // relative offset of local header 4 bytes
	// then goes filename, extra field, file comment...
}
ZIPCDHEADER;
#define ZIPCDHEADER_SIZE 46

typedef struct
{
	unsigned long  fourCC;
	unsigned short u0;
	unsigned short u1;
	unsigned short numfiles;
	unsigned short numfiles2;
	unsigned int   cdsize;
	unsigned int   cdofs;
	unsigned short u3;
}
ZIPCDTAIL;
#define ZIPCDTAIL_SIZE 22 // not matching sizeof(ZIPCDTAIL) because of structure aligning

#endif