////////////////////////////////////////////////////////////////
//
// Blood Pill - zlib support for PK3 writing
// coded by Pavel [VorteX] Timofeyev and placed to public domain
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

#include "zlib.h"
#include "mem.h"

#pragma comment(lib, "zlib.lib")

// buffering
#define Zlib_MAX_FILESIZE 67108864/2 // 32 megs
unsigned char zipbuf[Zlib_MAX_FILESIZE];

char *zlibErrorString(int ret)
{
	if (ret == Z_STREAM_END || ret == Z_OK)
		return "no error";
	if (ret == Z_STREAM_ERROR)
		return "stream error";
	if (ret == Z_DATA_ERROR)
		return "data error";
	if (ret == Z_MEM_ERROR)
		return "mem error";
	if (ret == Z_BUF_ERROR)
		return "buf error!";
	return "unknown error";
}

/*
====================
Zlib_CreateFile
====================
*/

zlib_file_t *Zlib_Create(char *filepath)
{
	zlib_file_t *zip;

	zip = (zlib_file_t *)qmalloc(sizeof(zlib_file_t));
	CreatePath(filepath);
	zip->file = SafeOpen(filepath, "wb");
	zip->numfiles = 0;

	return zip;
}

// creates a new zip file handle
zlib_entry_t *Zlib_CreateFile(zlib_file_t *zip, char *filename)
{
	zlib_entry_t *entry;

	// allocate file
	if (zip->numfiles >= MAX_ZLIB_FILES)
		Error("Zlib_CreateFile: MAX_Zlib_FILES exceeded!");
	entry = (zlib_entry_t *)qmalloc(sizeof(zlib_entry_t));
	memset(entry, 0, sizeof(zlib_entry_t));
	zip->files[zip->numfiles] = entry;
	zip->numfiles++;

	// set filename under central directory
	entry->filenamelen = strlen(filename)+1;
	strcpy(entry->filename, filename);

	return entry;
}

// compress file contents into PK3 file
void Zlib_CompressFileData(zlib_file_t *zip, zlib_entry_t *entry, unsigned char *filedata, unsigned int datasize)
{
	unsigned int compressed_size;
	int ret;

	// allocate file compressor
	memset(&entry->stream, 0, sizeof(z_stream));
	entry->stream.zalloc = Z_NULL;
    entry->stream.zfree = Z_NULL;
    entry->stream.opaque = Z_NULL;
	if (deflateInit(&entry->stream, Z_DEFAULT_COMPRESSION) != Z_OK)
		Error("Zlib_CompressFile: failed to allocate compressor");

	// compress
	entry->stream.avail_in = datasize;
	entry->stream.next_in = filedata;
	entry->stream.avail_out = Zlib_MAX_FILESIZE;
    entry->stream.next_out = zipbuf;
	if (deflate(&entry->stream, Z_FINISH) == Z_STREAM_ERROR)
		Error("Zlib_CompressFile: error during compression");
	compressed_size = Zlib_MAX_FILESIZE - entry->stream.avail_out;
	if (compressed_size >= Zlib_MAX_FILESIZE)
		Error("Zlib_CompressFile: Zlib_MAX_FILESIZE exceeded!");

	// register in central directory
	entry->xver = 20; // File is a folder (directory), is compressed using Deflate compression
	entry->bitflag = 2; // maximal compression
	entry->crc32 = crc32(filedata, datasize);
	entry->compression = 8; // Deflate
	entry->csize = compressed_size;
	entry->usize = datasize;
	entry->offset = ftell(zip->file);

	// write local file header & file contents
	ret = FOURCC_ZIP_ENTRY;
	fwrite(&ret, 4, 1, zip->file);
	fwrite(&entry->xver, 2, 1, zip->file);
	fwrite(&entry->bitflag, 2, 1, zip->file);
	fwrite(&entry->compression, 2, 1, zip->file);
	ret = 0;
	fwrite(&ret, 2, 1, zip->file);
	fwrite(&ret, 2, 1, zip->file);
	fwrite(&entry->crc32, 4, 1, zip->file);
	fwrite(&entry->csize, 4, 1, zip->file);
	fwrite(&entry->usize, 4, 1, zip->file);
	fwrite(&entry->filenamelen, 2, 1, zip->file);
    fwrite(&ret, 2, 1, zip->file);
	fwrite(entry->filename, entry->filenamelen, 1, zip->file);
	fwrite(zipbuf, entry->csize, 1, zip->file);
	if (ferror(zip->file))
		Error("Zlib_CompressFile: failed write zip");
	deflateEnd(&entry->stream);
}

// write out a whole file to the zip
void Zlib_AddFile(zlib_file_t *zip, char *filename, unsigned char *filedata, unsigned int datasize)
{
	zlib_entry_t *entry;
	
	entry = Zlib_CreateFile(zip, filename);
	Zlib_CompressFileData(zip, entry, filedata, datasize);
}

// add file from disk to zip
void Zlib_AddExternalFile(zlib_file_t *zip, char *filename, char *externalfile)
{
	byte *filedata;
	zlib_entry_t *entry;
	unsigned int datasize;

	datasize = LoadFile(externalfile, &filedata);
	entry = Zlib_CreateFile(zip, filename);
	Zlib_CompressFileData(zip, entry, filedata, datasize);
	qfree(filedata);
}

#ifdef __CMDLIB_WRAPFILES__
// add wrapped files to PK3
void Zlib_AddWrappedFiles(zlib_file_t *zip)
{
	int i, numfiles;
	zlib_entry_t *entry;
	byte *filedata;
	char *filename;
	unsigned int datasize;

	numfiles = CountWrappedFiles();
	for (i = 0; i < numfiles; i++)
	{
		datasize = LoadWrappedFile(i, &filedata, &filename);
		entry = Zlib_CreateFile(zip, filename);
		Zlib_CompressFileData(zip, entry, filedata, datasize);
		qfree(filedata);
	}
	FreeWrappedFiles();
}
#endif

// write PK3 foot and close it
void Zlib_Close(zlib_file_t *zip)
{
	unsigned short i;
	unsigned int ret, cdofs, cdsize;
	zlib_entry_t *entry;

	// write central directory and file tail
	cdofs = ftell(zip->file);
	for (i = 0; i < zip->numfiles; i++)
	{
		entry = zip->files[i];
		ret = FOURCC_ZIP_CENTRALDIR;
		fwrite(&ret, 4, 1, zip->file);
		ret = 0;
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&entry->xver, 2, 1, zip->file);
		fwrite(&entry->bitflag, 2, 1, zip->file);
		fwrite(&entry->compression, 2, 1, zip->file);
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&entry->crc32, 4, 1, zip->file);
		fwrite(&entry->csize, 4, 1, zip->file);
		fwrite(&entry->usize, 4, 1, zip->file);
		fwrite(&entry->filenamelen, 2, 1, zip->file);
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&ret, 2, 1, zip->file);
		fwrite(&ret, 4, 1, zip->file);
		fwrite(&entry->offset, 4, 1, zip->file);
		fwrite(entry->filename, entry->filenamelen, 1, zip->file);
		if (ferror(zip->file))
			Error("Zlib_Close: failed write zip");
		qfree(entry);
	}
	cdsize = ftell(zip->file) - cdofs;

	ret = FOURCC_ZIP_TAIL;
	fwrite(&ret, 4, 1, zip->file);
	ret = 0;
	fwrite(&ret, 2, 1, zip->file);
	fwrite(&ret, 2, 1, zip->file);
	fwrite(&zip->numfiles, 2, 1, zip->file);
	fwrite(&zip->numfiles, 2, 1, zip->file);
	fwrite(&cdsize, 4, 1, zip->file);
	fwrite(&cdofs, 4, 1, zip->file);
	fwrite(&ret, 2, 1, zip->file);
	if (ferror(zip->file))
		Error("Zlib_Close: failed write zip");

	fclose(zip->file);
	qfree(zip);
}