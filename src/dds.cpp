////////////////////////////////////////////////////////////////
//
// RWGDDS - utility main
// coded by Pavel [VorteX] Timofeyev and placed to public domain
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

#include "dds.h"
#include "cmd.h"
#include "mem.h"
#include "thread.h"
#include "arc.h"

#include "windows.h"
#include <string>
#include <vector>
using namespace std;

// DevID image library
#include <IL/il.h>
#pragma comment(lib, "DevIL.lib")
#include <IL/ilu.h>
#pragma comment(lib, "ILU.lib")

// NVidia DXT compressor
#include <dxtlib/dxtlib.h>
#pragma comment(lib, "nvDXTlibMT.vc8.lib")
using namespace nvDDS;

// ATI Compress
#include <ATI_Compress.h>
#include <ATI_Compress_Test_Helpers.cpp>
#pragma comment(lib, "ATI_Compress_MT.lib")

bool waitforkey;
bool memstats;
bool errorlog;

char progname[MAX_DDSPATH];
char progpath[MAX_DDSPATH];

// options
char   dir[MAX_DDSPATH];
char   filemask[MAX_DDSPATH];
char   outdir[MAX_DDSPATH];
double inputsize;
double outputsize;
double compressionweight;
double exportedfiles;
bool   nomips;
bool   useaticompressor;
bool   usenvidiacompressor;
bool   usecache;
bool   usenopoweroftwo;
bool   usescaler;

// supporter compressors
typedef enum
{
	COMPRESSOR_NVIDIA_DXTC,
	COMPRESSOR_ATI
}
COMPRESSOR_TOOL;
char *getCompessionToolStr(COMPRESSOR_TOOL i)
{
	if (i == COMPRESSOR_NVIDIA_DXTC) return "COMPRESSOR_NVIDIA_DXTC";
	if (i == COMPRESSOR_ATI)         return "COMPRESSOR_ATI";
	                                 return "UNKNOWN_COMPRESSOR_TOOL";
}

// supported compressions
typedef enum
{
	COMPRESSION_DXT1,
	COMPRESSION_DXT1a,
	COMPRESSION_DXT3,
	COMPRESSION_DXT5
}
COMPRESSION_TYPE;
char *getCompessionTypeStr(COMPRESSION_TYPE i)
{
	if (i == COMPRESSION_DXT1)  return "COMPRESSION_DXT1";
	if (i == COMPRESSION_DXT1a) return "COMPRESSION_DXT1a";
	if (i == COMPRESSION_DXT3)  return "COMPRESSION_DXT3";
	if (i == COMPRESSION_DXT5)  return "COMPRESSION_DXT5";
	                            return "UNKNOWN_COMPRESSION_TYPE";
}

// scale2x
#include "scale2x.h"

/*
==========================================================================================

  DEVIL RAW IMAGE HELPER

==========================================================================================
*/

#define IL_RAWHS (sizeof(ILuint)+sizeof(ILuint)+sizeof(ILuint)+sizeof(ILubyte)+sizeof(ILubyte))

byte *ilRaw(unsigned int width, unsigned int height, unsigned char bpp)
{
	byte *data;

	data = (byte *)qmalloc(width * height * bpp + IL_RAWHS);
	*(ILuint  *)(data) = width;
	*(ILuint  *)(data + sizeof(ILuint)) = height;
	*(ILuint  *)(data + sizeof(ILuint)+sizeof(ILuint)) = 1;
	*(ILubyte *)(data + sizeof(ILuint)+sizeof(ILuint)+sizeof(ILuint)) = bpp;
	*(ILubyte *)(data + sizeof(ILuint)+sizeof(ILuint)+sizeof(ILuint)+sizeof(ILubyte)) = 1;
	return data + IL_RAWHS;
}

byte *ilRawGet()
{
	if (ilGetInteger(IL_IMAGE_TYPE) != IL_UNSIGNED_BYTE)
		ilConvertImage(ilGetInteger(IL_IMAGE_FORMAT), IL_UNSIGNED_BYTE);

	unsigned int width  = ilGetInteger(IL_IMAGE_WIDTH);
	unsigned int height = ilGetInteger(IL_IMAGE_HEIGHT);
	unsigned char bpp   = ilGetInteger(IL_IMAGE_BPP);

	byte *data = ilRaw(width, height, bpp);
	memcpy(data, ilGetData(), width * height * bpp);
	return data;
}

void ilRawFree(byte *data)
{
	data = (byte *)((byte *)data - IL_RAWHS);
	qfree(data);
}

int ilRawLoad(byte *data)
{
	data = (byte *)((byte *)data - IL_RAWHS);
	ILuint size = *(ILuint  *)(data) *
		          *(ILuint  *)(data + sizeof(ILuint)) *
				  *(ILubyte *)(data + sizeof(ILuint)+sizeof(ILuint)+sizeof(ILuint));
	int ret = ilLoadL(IL_RAW, data, size + IL_RAWHS);
	if (!ret)
		Error("ilLoadRawImage: %s\n", iluErrorString(ilGetError()));
	qfree(data);
	return 0;
}

/*
==========================================================================================

  CONFIGURATION FILE

==========================================================================================
*/

typedef struct
{
	string parm;
	string pattern;
}CompareOption;

vector<CompareOption> opt_include;
vector<CompareOption> opt_nomip;
vector<CompareOption> opt_forceDXT1;
vector<CompareOption> opt_forceDXT1a;
vector<CompareOption> opt_forceDXT3;
vector<CompareOption> opt_forceDXT5;
vector<CompareOption> opt_forceNvDXTlib;
vector<CompareOption> opt_forceATICompressor;
vector<CompareOption> opt_isNormal;
vector<CompareOption> opt_isHeight;
vector<CompareOption> opt_premodulateColor;
char                  opt_basedir[MAX_DDSPATH];
char                  opt_ddsdir[MAX_DDSPATH];
bool                  opt_detectBinaryAlpha;
byte                  opt_binaryAlphaMin;
byte                  opt_binaryAlphaMax;
vector<CompareOption> opt_archiveFiles;
vector<CompareOption> opt_scale2x;

void LoadOptions(char *filename)
{
	FILE *f;
	char line[1024], group[64], key[64],  *val;
	int linenum, l;

	// flush options
	opt_detectBinaryAlpha = false;
	opt_binaryAlphaMin = 0;
	opt_binaryAlphaMax = 255;
	opt_include.clear();
	opt_nomip.clear();
	opt_forceDXT1.clear();
	opt_forceDXT1a.clear();
	opt_forceDXT3.clear();
	opt_forceDXT5.clear();
	opt_forceNvDXTlib.clear();
	opt_forceATICompressor.clear();
	opt_isNormal.clear();
	opt_isHeight.clear();
	strcpy(opt_basedir, "id1");
	strcpy(opt_ddsdir, "dds");
	opt_premodulateColor.clear();
	opt_archiveFiles.clear();
	opt_scale2x.clear();

	// parse file
	sprintf(line, "%s%s", progpath, filename);
	f = fopen(line, "r");
	if (!f)
	{
		Warning("LoadOptions: failed to open '%s' - %s!", filename, strerror(errno));
		return;
	}
	linenum = 0;
	while (fgets(line, sizeof(line), f) != NULL)
	{
		linenum++;

		// parse comment
		if (line[0] == '#' || line[0] == '\n')
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
		if (!stricmp(group, "options"))
		{
			if (!stricmp(key, "basepath"))
				strncpy(opt_basedir, val, sizeof(opt_ddsdir));
			else if (!stricmp(key, "ddspath"))
				strncpy(opt_ddsdir, val, sizeof(opt_ddsdir));
			else if (!stricmp(key, "binaryalpha"))
			{
				if (!stricmp(val, "auto"))
					opt_detectBinaryAlpha = true;
				else
					opt_detectBinaryAlpha = false;
			}
			else if (!stricmp(key, "waitforkey"))
			{
				if (!stricmp(val, "yes"))
					waitforkey = true;
				else
					waitforkey = false;
			}
			else if (!stricmp(key, "binaryalpha_0"))
				opt_binaryAlphaMin = (byte)(min(max(0, atoi(val)), 255));
			else if (!stricmp(key, "binaryalpha_1"))
				opt_binaryAlphaMax = (byte)(min(max(0, atoi(val)), 255));
			else
				Warning("%s:%i: unknown key '%s'", filename, linenum, key);
			continue;
		}
		if (!stricmp(group, "input")      || 
			!stricmp(group, "nomip")      || !stricmp(group, "is_normalmap")  || !stricmp(group, "is_heightmap") ||
			!stricmp(group, "force_dxt1") || !stricmp(group, "force_dxt1a")   || !stricmp(group, "force_dxt3") || !stricmp(group, "force_dxt5") ||
			!stricmp(group, "force_nv")   || !stricmp(group, "force_ati")     || !stricmp(group, "premodulate_color") ||
			!stricmp(group, "archives")   || !stricmp(group, "scale_2x") )
		{
			CompareOption O;
			if (stricmp(key, "path") && stricmp(key, "suffix") && stricmp(key, "ext") && stricmp(key, "name") &&
				stricmp(key, "path!") && stricmp(key, "suffix!") && stricmp(key, "ext!") && stricmp(key, "name!"))
				Warning("%s:%i: unknown key '%s'", filename, linenum, key);
			else
			{
				O.parm = key;
				O.pattern = val;
				     if (!stricmp(group, "input"))            opt_include.push_back(O);
				else if (!stricmp(group, "nomip"))            opt_nomip.push_back(O);
				else if (!stricmp(group, "is_normalmap"))     opt_isNormal.push_back(O);
				else if (!stricmp(group, "is_heightmap"))     opt_isHeight.push_back(O);
				else if (!stricmp(group, "force_dxt1"))       opt_forceDXT1.push_back(O);
				else if (!stricmp(group, "force_dxt1a"))      opt_forceDXT1a.push_back(O);
				else if (!stricmp(group, "force_dxt3"))       opt_forceDXT3.push_back(O);
				else if (!stricmp(group, "force_dxt5"))       opt_forceDXT5.push_back(O);
				else if (!stricmp(group, "force_nv"))         opt_forceNvDXTlib.push_back(O);
				else if (!stricmp(group, "force_ati"))        opt_forceATICompressor.push_back(O);
				else if (!stricmp(group, "premodulate_color"))opt_premodulateColor.push_back(O);
				else if (!stricmp(group, "archives"))         opt_archiveFiles.push_back(O);
				else if (!stricmp(group, "scale_2x"))         opt_scale2x.push_back(O);
			}
			continue;
		}
		Warning("%s:%i: unknown group '%s'", filename, linenum, group);
	}
	fclose(f);
}

/*
==========================================================================================

  FILE CACHE

==========================================================================================
*/

typedef struct
{
	char            filename[MAX_DDSPATH];
	unsigned int    crc;
	bool            used;
}
FileCacheS;
vector<FileCacheS> FileCache;

bool LoadFileCache(char *filename)
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
			Error("LoadFileCache: damaged line '%s'", line); 
		memset(NewFC.filename, 0, MAX_DDSPATH);
		strncpy(NewFC.filename, line, crcstr - line);
		NewFC.crc = ParseNum(crcstr + 1);
		NewFC.used = false;
		FileCache.push_back(NewFC);
	}
	fclose(f);
	return true;
}

void SaveFileCache(char *filename)
{
	FILE *f;

	f = SafeOpen(filename, "w");
	fprintf(f, "# Crc32 table for source files used to generate DDS\n");
	fprintf(f, "# generated automatically, do not modify\n");
	for (std::vector<FileCacheS>::iterator file = FileCache.begin(); file < FileCache.end(); file++)
		if (file->used)
			fprintf(f, "%s 0x%08X\n", file->filename, file->crc);
	fclose(f);
}

unsigned int FileCRC32(char *filename)
{
	unsigned int crc;
	int filesize;
	byte *filedata;

	filesize = LoadFile(filename, &filedata);
	crc = crc32(filedata, filesize);
	qfree(filedata);
	return crc;
}

// check if file was modified and updates cache
bool FileWasModified(const char *filepath, unsigned int *fileCRC)
{
	char filename[MAX_DDSPATH];
	unsigned int crc;

	// get crc
	sprintf(filename, "%s%s", dir, filepath);
	if (!fileCRC)
		crc = FileCRC32(filename);
	else
		crc = *fileCRC;

	// find in cache
	for (std::vector<FileCacheS>::iterator file = FileCache.begin(); file < FileCache.end(); file++)
	{
		if (!strnicmp(file->filename, filepath, MAX_DDSPATH))
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
	strncpy(NewFC.filename, filepath, MAX_DDSPATH);
	NewFC.crc = crc;
	NewFC.used = true;
	FileCache.push_back(NewFC);
	return true; 
}

/*
==========================================================================================

  SCAN DIRECTORY

==========================================================================================
*/

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

	// used for archive files
	unsigned int        arc_offset;
	ScanFileArchiveType arc_type;
	FILE               *arc_file;
}
ScanFile;

vector<ScanFile> textures;
int texturesSkipped;

bool FindDir(char *pattern)
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

	#error "FindFile not implemented!"

#endif
}

bool FindFile(char *pattern)
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

	#error "FindFile not implemented!"

#endif
}

bool FileMatchList(ScanFile *file, vector<CompareOption> &list)
{
	// check rules
	for (vector<CompareOption>::iterator option = list.begin(); option < list.end(); option++)
	{
		// exclude rules
		if (!stricmp(option->parm.c_str(), "path!"))
		{
			if (!strnicmp(file->fullpath.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return false;
			continue;
		}
		if (!stricmp(option->parm.c_str(), "suffix!"))
		{
			if (!strnicmp(file->suf.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return false;
			continue;
		}
		if (!stricmp(option->parm.c_str(), "ext!"))
		{
			if (!strnicmp(file->ext.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return false;
			continue;
		}
		if (!stricmp(option->parm.c_str(), "name!"))
		{
			if (!strnicmp(file->name.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return false;
			continue;
		}
		// include rules
		if (!stricmp(option->parm.c_str(), "path"))
		{
			if (!strnicmp(file->fullpath.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return true;
			continue;
		}
		if (!stricmp(option->parm.c_str(), "suffix"))
		{
			if (!strnicmp(file->suf.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return true;
			continue;
		}
		if (!stricmp(option->parm.c_str(), "ext"))
		{
			if (!strnicmp(file->ext.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return true;
			continue;
		}
		if (!stricmp(option->parm.c_str(), "name"))
		{
			if (!strnicmp(file->name.c_str(), option->pattern.c_str(), strlen(option->pattern.c_str())))
				return true;
			continue;
		}
	}
	return false;
}

bool FileMatchList(char *filename, vector<CompareOption> &list)
{
	char path[MAX_DDSPATH], name[512], ext[512], suf[64];

	ExtractFilePath(filename, path);
	ExtractFileName(filename, name);
	ExtractFileExtension(filename, ext);
	StripFileExtension(filename, name);
	ExtractFileSuffix(name, suf, '_');
	ScanFile NF;
		NF.path = path;
		NF.name = name;
		NF.ext = ext;
		NF.suf = suf;
		NF.fullpath = NF.path.c_str();
		NF.fullpath.append(name);
		NF.fullpath.append(".");
		NF.fullpath.append(ext);
	return FileMatchList(&NF, list);
}

bool AddFileToConversion(ScanFile &file, bool checkinclude, unsigned int *fileCRC)
{
	if (checkinclude)
	{
		if (!FileMatchList(&file, opt_include))
			return false;

		if (usecache)
		if (!FileWasModified(file.fullpath.c_str(), fileCRC))
		{
			texturesSkipped++;
			return false;
		}
	}
	// passed
	textures.push_back(file);
	return true;
}

int ScanZIP(FILE *f, byte *ziptail, char *basepath, char *arcfilename)
{
	unsigned short numfiles, filesadded;
	char filename[MAX_DDSPATH], path[MAX_DDSPATH], name[512], ext[512], suf[64];
	ZIPCDHEADER z;

	numfiles = *(unsigned short *)(ziptail + 8);
	fseek(f, *(unsigned int *)(ziptail + 16), SEEK_SET);
	filesadded = 0;
	size_t offset = 0;
	while(numfiles > 0)
	{
		memset(&z, 0, sizeof(z));
		fread(&z, ZIPCDHEADER_SIZE, 1, f);
		if (z.fourCC != FOURCC_ZIP_CENTRALDIR)
		{
			Warning("%s : broken ZIP file (entry fourCC mismatch)", arcfilename);
			return filesadded;
		}
		
		memset(filename, 0, sizeof(filename));
		fread(filename, z.filenamelen, 1, f);

		// check if file matches
		ExtractFilePath(filename, path);
		ExtractFileName(filename, name);
		ExtractFileExtension(filename, ext);
		StripFileExtension(filename, name);
		ExtractFileSuffix(name, suf, '_');
		ScanFile NF;
		         NF.path = basepath;
				 NF.path.append(basepath);
		         NF.name = name;
		         NF.ext = ext;
		         NF.suf = suf;
				 NF.fullpath = NF.path.c_str();
				 NF.fullpath.append(name);
				 NF.fullpath.append(".");
				 NF.fullpath.append(ext);
				 NF.arc_type = ARCHIVE_ZIP;
				 NF.arc_offset = offset;
				 NF.arc_file = f;
		if (AddFileToConversion(NF, true, &z.crc32))
			filesadded++;
		// seek next file
		offset += ZIPLOCALHEADER_SIZE + z.filenamelen + z.csize + z.xfieldlen + z.commentlen;
		if (z.xfieldlen) fseek(f, z.xfieldlen, SEEK_CUR);
		if (z.commentlen) fseek(f, z.commentlen, SEEK_CUR);
		numfiles--;
	}
	return filesadded;
}

void ScanFiles(char *basepath, char *singlefile, char *addpath)
{
	char pattern[MAX_DDSPATH], path[MAX_DDSPATH], scanpath[MAX_DDSPATH], name[512], ext[512], suf[64];

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
		if (!strnicmp(n_file.cFileName, ".", 1))
			continue;
		// directory
		if (n_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strlcpy(scanpath, path, sizeof(scanpath));
			strlcat(scanpath, n_file.cFileName, sizeof(scanpath));
			ScanFiles(basepath, NULL, scanpath);
			continue;
		}
		ExtractFileExtension(n_file.cFileName, ext);
		StripFileExtension(n_file.cFileName, name);
		ExtractFileSuffix(name, suf, '_');
		ScanFile NF;
		         NF.path = path;
		         NF.name = name;
		         NF.ext = ext;
		         NF.suf = suf;
				 NF.fullpath = path;
				 NF.fullpath.append(name);
				 NF.fullpath.append(".");
				 NF.fullpath.append(ext);
				 NF.arc_type = ARCHIVE_NOT;
		// scan archives
		if (FileMatchList(&NF, opt_archiveFiles))
		{
			sprintf(scanpath, "%s%s", dir, NF.fullpath.c_str());
			FILE *f = fopen(scanpath, "rb");
			if (!f)
				Warning("ScanFiles: failed to open archive %s", scanpath);
			else
			{
				int filesadded = 0;
				byte cdtail[ZIPCDTAIL_SIZE];
				fseek(f, 0, SEEK_END);
				fseek(f, 0-sizeof(cdtail), SEEK_CUR);
				fread(&cdtail, sizeof(cdtail), 1, f);
				if (ferror(f))
					Warning("ScanFiles: failed to read archive %s (%s)", scanpath, strerror(errno));
				else if (*(unsigned int *)(cdtail) == FOURCC_ZIP_TAIL)
					filesadded = ScanZIP(f, cdtail, path, scanpath);
				else
					Warning("ScanFiles: %s is not a ZIP archive (tail is %08x)", scanpath, cdtail);
				if (!filesadded)
					fclose(f);
			}
			continue;
		}
		// add simple file
		AddFileToConversion(NF, singlefile ? false : true, NULL);
	}
	while(FindNextFile(hFile, &n_file) != 0);
	FindClose(hFile);
#else

#error "ScanFiles not implemented!"

#endif
}	

/*
==========================================================================================

  IMAGE LOADING

==========================================================================================
*/

typedef struct MipMapImage_s
{
	int width;
	int height;
	int level;
	size_t size;
	byte *data;
	MipMapImage_s *nextmip;
}MipMapImage;

typedef struct LoadedImage_s
{
	int width;
	int height;
	int format;      // IL_RGB, IL_RGBA, IL_BGR, IL_BGRA
	int bpp;         // bits per pixel, 3 or 4
	int type;        // should be IL_UNSIGNED_BYTE
	size_t size;     // size of data
	size_t filesize; // file size
	byte *data;      // data
	bool error;      // dont convert

	// tricks
	bool generateMipmaps;
	MipMapImage *mipMaps;
	bool isNormalmap;
	bool isHeightmap;
	bool premodulateColor;
	
	// compressor/compression used for this image
	COMPRESSOR_TOOL compressorTool;
	COMPRESSION_TYPE compressionType;

	// frames/subimages
	int groupnum;     // -1 if ita not a groupframe
	int framenum;     // -1 if its not multiframe image
	char texname[128];// null if there is no custom texture name
	LoadedImage_s *next;
}
LoadedImage;

void FlushImage(LoadedImage *image)
{
	image->width = 0;
	image->height = 0;
	image->format = IL_RGB;
	image->bpp = 0;
	image->type = IL_UNSIGNED_BYTE;
	image->size = 0;
	image->filesize = 0;
	image->data = NULL;
	image->generateMipmaps = true;
	image->mipMaps = NULL;
	image->isNormalmap = false;
	image->isHeightmap = false;
	image->premodulateColor = false;
	image->error = false;
	image->framenum = -1;
	image->groupnum = -1;
	strcpy(image->texname, "");
	image->next = NULL;
}

LoadedImage *NewImage()
{
	LoadedImage *img;

	img = (LoadedImage *)qmalloc(sizeof(LoadedImage));
	FlushImage(img);
	return img;
}

int NextPowerOfTwo(int n) 
{ 
    if ( n <= 1 ) return n;
    double d = n-1; 
    return 1 << ((((int*)&d)[1]>>20)-1022); 
}

void ImageLoadFromIL(ScanFile *file, LoadedImage *image)
{
	// convert image to suitable format
	// we are only accepting RGB or RGBA
	image->format = ilGetInteger(IL_IMAGE_FORMAT);
	image->type = ilGetInteger(IL_IMAGE_TYPE);
	if (image->format == IL_BGR)
		ilConvertImage(IL_RGB, image->type);
	else if (image->format == IL_BGRA)
		ilConvertImage(IL_RGBA, image->type);
	else if (image->format != IL_RGB && image->format != IL_RGBA)
		ilConvertImage(IL_RGB, image->type);

	// check alpha
	bool hasAlpha = false;
	bool hasGradientAlpha = false;
	image->width = ilGetInteger(IL_IMAGE_WIDTH);
	image->height = ilGetInteger(IL_IMAGE_HEIGHT);
	byte *data = (byte *)ilGetData();
	if (image->format == IL_RGBA || image->format == IL_BGRA)
	{
		int  num_grad = 0;
		int  need_grad = (image->width + image->height) / 4;
		long pixels = image->width*image->height*4;
		hasAlpha = true;
		if (!opt_detectBinaryAlpha)
			hasGradientAlpha = true;
		else
		{
			for (long i = 0; i < pixels; i+= 4)
			{
				if (data[i+3] < opt_binaryAlphaMax && data[i+3] > opt_binaryAlphaMin)
					num_grad++;
				if (num_grad > need_grad)
				{
					hasGradientAlpha = true;
					break;
				}
			}
		}
	}

	// apply scale
	// we do scale 4x and then backscale 1/2 to nearest power of two for best quality
	if (FileMatchList(file, opt_scale2x) || usescaler)
	{
		// scale2x does not allows BPP = 3, convert to 4 (and convert back after finish)
		ilConvertImage(IL_RGBA, image->type);
		data = (byte *)ilGetData();

		// check if can scale
		int ret = sxCheck(4, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
		int scaler = 4;
		if (ret == SCALEX_OK)
		{
			int width  = ilGetInteger(IL_IMAGE_WIDTH);
			int height = ilGetInteger(IL_IMAGE_HEIGHT);
			int bpp    = ilGetInteger(IL_IMAGE_BPP);
			if (hasAlpha)
			{
				// copy out alpha to scale on deffered pass
				byte *alpha = (byte *)qmalloc(width * height);
				byte *in = data;
				byte *end = data + width * height * bpp;
				byte *out = alpha;
				while(in < end)
				{
					out[0] = in[3];
					in[3] = 255; // all alpha is white
					out++;
					in+=4;
				}

				// scale rgb
				byte *scaled = ilRaw(width*scaler, height*scaler, bpp);
				sxScale(scaler, scaled, width*scaler*bpp, data, width*bpp, bpp, width, height);
				ilRawLoad(scaled);

				// scale back to 0.5 and add some blur
				iluBlurGaussian(1);
				iluSharpen(1.2f, 1);
				iluScale(usenopoweroftwo?ilGetInteger(IL_IMAGE_WIDTH)/2:NextPowerOfTwo(ilGetInteger(IL_IMAGE_WIDTH)/2), usenopoweroftwo?ilGetInteger(IL_IMAGE_HEIGHT)/2:NextPowerOfTwo(ilGetInteger(IL_IMAGE_HEIGHT)/2), ilGetInteger(IL_IMAGE_DEPTH));
				scaled = ilRawGet(); // save out

				// now scale alpha
				byte *scaled_alpha = ilRaw(width*scaler, height*scaler, 1);
				sxScale(scaler, scaled_alpha, width*scaler, alpha, width, 1, width, height);
				qfree(alpha);
				ilRawLoad(scaled_alpha);
				iluSharpen(0.7f, 1);
				iluScale(usenopoweroftwo?ilGetInteger(IL_IMAGE_WIDTH)/2:NextPowerOfTwo(ilGetInteger(IL_IMAGE_WIDTH)/2), usenopoweroftwo?ilGetInteger(IL_IMAGE_HEIGHT)/2:NextPowerOfTwo(ilGetInteger(IL_IMAGE_HEIGHT)/2), ilGetInteger(IL_IMAGE_DEPTH));
				iluSharpen(1.2f, 1);

				// combine alpha and rgb
				data = ilGetData();
				in = data;
				end = data + ilGetInteger(IL_IMAGE_WIDTH) * ilGetInteger(IL_IMAGE_HEIGHT);
				out = scaled;
				while(in < end)
				{
					out[3] = in[0];
					out+=4;
					in++;
				}
				ilRawLoad(scaled);
			}
			else
			{
				// rgb scale2x
				byte *scaled = ilRaw(width*scaler, height*scaler, bpp);
				sxScale(scaler, scaled, width*scaler*bpp, data, width*bpp, bpp, width, height);
				ilRawLoad(scaled);

				// scale back to 0.5 and add some blur
				iluSharpen(0.6f, 5);
				iluScale(usenopoweroftwo?ilGetInteger(IL_IMAGE_WIDTH)/2:NextPowerOfTwo(ilGetInteger(IL_IMAGE_WIDTH)/2), usenopoweroftwo?ilGetInteger(IL_IMAGE_HEIGHT)/2:NextPowerOfTwo(ilGetInteger(IL_IMAGE_HEIGHT)/2), ilGetInteger(IL_IMAGE_DEPTH));
			}
			
		}

		// convert back
		if (!hasAlpha)
			ilConvertImage(IL_RGB, image->type);
		data = (byte *)ilGetData();

	}

	// check if need to generate mipmaps for texture
	if (image->generateMipmaps)
		if (nomips || FileMatchList(file, opt_nomip))
			image->generateMipmaps = false;

	// check for special texture type
	if (FileMatchList(file, opt_isNormal))
		image->isNormalmap = true;
	else if (FileMatchList(file, opt_isHeight))
		image->isHeightmap = true;

	// select compression type
	if (image->isHeightmap)
		image->compressionType = COMPRESSION_DXT1;
	else if (hasAlpha)
		image->compressionType = hasGradientAlpha ? COMPRESSION_DXT5 : COMPRESSION_DXT1a;
	else
		image->compressionType = COMPRESSION_DXT1;

	// premultiply RGB by alpha
	if (FileMatchList(file, opt_premodulateColor))
		image->premodulateColor = true;

	// check if forcing compression
	if (FileMatchList(file, opt_forceDXT1))
		image->compressionType = COMPRESSION_DXT1;
	else if (FileMatchList(file, opt_forceDXT1a))
		image->compressionType = COMPRESSION_DXT1a;
	else if (FileMatchList(file, opt_forceDXT3))
		image->compressionType = COMPRESSION_DXT3;
	else if (FileMatchList(file, opt_forceDXT5))
		image->compressionType = COMPRESSION_DXT5;

	// check if selected DXT3/DXT5 and image have no alpha information
	if ((image->compressionType == COMPRESSION_DXT3 || image->compressionType == COMPRESSION_DXT5) && !hasAlpha)
		image->compressionType = COMPRESSION_DXT1;

	// select compressor tool
	if (useaticompressor)
		image->compressorTool = COMPRESSOR_ATI;
	else if (usenvidiacompressor)
		image->compressorTool = COMPRESSOR_NVIDIA_DXTC;
	else
	{
		// hybrid mode, pick best
		// NVidia tool compresses normalmaps better than ATI
		// while ATI wins over general DXT1 and DXT5
		if (image->isNormalmap)
			image->compressorTool = COMPRESSOR_NVIDIA_DXTC;
		else
			image->compressorTool = COMPRESSOR_ATI;

		// check if forcing
		if (FileMatchList(file, opt_forceNvDXTlib))
			image->compressorTool = COMPRESSOR_NVIDIA_DXTC;
		else if (FileMatchList(file, opt_forceATICompressor))
			image->compressorTool = COMPRESSOR_ATI;
	}

	if (!usenopoweroftwo)
	{
		// scale image to nearest power of two
		int w2 = NextPowerOfTwo(ilGetInteger(IL_IMAGE_WIDTH));
		int h2 = NextPowerOfTwo(ilGetInteger(IL_IMAGE_HEIGHT));
		if (w2 != ilGetInteger(IL_IMAGE_WIDTH) || h2 != ilGetInteger(IL_IMAGE_HEIGHT))
			iluScale(w2, h2, ilGetInteger(IL_IMAGE_DEPTH));
	}

	// read image
	image->format = ilGetInteger(IL_IMAGE_FORMAT);
	image->type = ilGetInteger(IL_IMAGE_TYPE);
	image->width = ilGetInteger(IL_IMAGE_WIDTH);
	image->height = ilGetInteger(IL_IMAGE_HEIGHT);
	image->bpp = ilGetInteger(IL_IMAGE_BPP);
	image->format = ilGetInteger(IL_IMAGE_FORMAT);
	image->type = ilGetInteger(IL_IMAGE_TYPE);
	image->size = image->width * image->height * image->bpp;
	image->data = (byte *)qmalloc(image->size);
	memcpy(image->data, (byte *)ilGetData(), image->size);

	// convert to binary alpha if no gradient alpha is found, pick best value
	if (image->compressionType == COMPRESSION_DXT1a)
	{
		long pixels = image->width*image->height*4;
		for (long i = 0; i < pixels; i+= 4)
		{
			if (image->data[i+3] < 180)
				image->data[i+3] = 0;
			else
				image->data[i+3] = 255;
		}
		ilSetPixels(0, 0, 0, image->width, image->height, ilGetInteger(IL_IMAGE_DEPTH), ilGetInteger(IL_IMAGE_FORMAT), ilGetInteger(IL_IMAGE_TYPE), image->data);
	}

	// manually generate mipmaps for ATI Compressor
	if (image->generateMipmaps && image->compressorTool == COMPRESSOR_ATI)
	{
		MipMapImage *mipmap;
		int s = min(image->width, image->height);
		int w = image->width;
		int h = image->height;
		int l = 0;
		while(s > 1)
		{
			l++;
			w = w / 2;
			h = h / 2;
			s = s / 2;
			if (image->mipMaps)
			{
				mipmap->nextmip = (MipMapImage *)qmalloc(sizeof(MipMapImage));
				mipmap = mipmap->nextmip;
			}
			else
			{
				image->mipMaps = (MipMapImage *)qmalloc(sizeof(MipMapImage));
				mipmap = image->mipMaps;
			}
			mipmap->nextmip = NULL;
			iluSharpen(0.4f, 1);
			iluScale(w, h, ilGetInteger(IL_IMAGE_DEPTH));
			mipmap->width = w;
			mipmap->height = h;
			mipmap->level = l;
			mipmap->size = mipmap->width*mipmap->height*image->bpp;
			mipmap->data = (byte *)qmalloc(mipmap->size);
			memcpy(mipmap->data, (byte *)ilGetData(), mipmap->size);
		}
	}
}

// picked from Darkplaces engine source
// John Carmack said the quake palette.lmp can be considered public domain because it is not an important asset to id,
// so I include it here as a fallback if no external palette file is found.
byte quake_palette [768] = { 0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,139,107,107,151,115,115,163,123,123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,55,0,75,59,7,87,67,7,95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,95,183,135,107,195,147,123,211,163,139,227,179,151,171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,107,87,71,95,75,59,83,63,51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,43,175,47,47,159,47,47,143,47,47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,243,147,255,247,199,255,255,255,159,91,83 };

// a quake sprite loader
void LoadImage_QuakeSprite(ScanFile *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	byte *buf;

	if (filesize < 36 || *(unsigned int *)(filedata + 4) != 32)
	{
		image->error = true;
		Warning("%s%s.%s : not a Darkplaces SPR32 file", file->path.c_str(), file->name.c_str(), file->ext.c_str());
	}
	else
	{
		// load frames
		int framenum = 0;
		int numframes = *(int *)(filedata + 24);
		LoadedImage *frame = image;
		filesize -= 36;
		      buf = filedata + 36;
		while(1)
		{
			// group sprite?
			if (*(unsigned int *)(buf) == 1) 
				buf += *(unsigned int *)(buf + 4) * 4 + 4;

			// fill data
			unsigned int framewidth  = *(unsigned int *)(buf + 12);
			unsigned int frameheight = *(unsigned int *)(buf + 16);
			frame->filesize = framewidth*frameheight*4;
			filesize -= 20;
			     buf += 20;
			byte *temp = ilRaw(framewidth, frameheight, 4);
			memcpy(temp, buf, framewidth*frameheight*4);
			ilRawLoad(temp);
			filesize -= frame->filesize;
			     buf += frame->filesize;

			// generic image setup
			frame->generateMipmaps = false;
			ImageLoadFromIL(file, frame);

			// set next frame
			frame->framenum = framenum;
			framenum++;
			if (framenum >= numframes)
				break;
			frame->next = NewImage();
			frame = frame->next;
		}
	}
	qfree(filedata);
}

// a quake bsp stored textures loader
void LoadImage_QuakeBSP(ScanFile *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	unsigned int lump_ofs = *(unsigned int *)(filedata + 20);
	unsigned int lump_size = *(unsigned int *)(filedata + 24);

	image->error = true;
	if ((size_t)(lump_ofs + lump_size) <= filesize)
	{
		byte *buf = filedata + lump_ofs;
		int   numtextures = *(int *)(buf);
		int  *textureoffsets = (int *)(buf + 4); 
		int   texnum = 0;
		LoadedImage *tex = image;
		while(texnum < numtextures)
		{
			// there could be null textures
			if (textureoffsets[texnum] > 0)
			{
				char *texname = (char *)(buf + textureoffsets[texnum]);
				int  texwidth = *(int *)(buf + textureoffsets[texnum] + 16); 
				int texheight = *(int *)(buf + textureoffsets[texnum] + 20); 
				int texmipofs = *(int *)(buf + textureoffsets[texnum] + 24); 

				// fill data
				tex->error = false;
				byte *temp = ilRaw(texwidth, texheight, 3);
				byte *in = (byte *)(buf + textureoffsets[texnum] + texmipofs);
				byte *end = in + texwidth*texheight;
				byte *out = temp;
				while(in < end)
				{
					out[0] = quake_palette[in[0]*3];
					out[1] = quake_palette[in[0]*3 + 1];
					out[2] = quake_palette[in[0]*3 + 2];
					out+=3;
					in++;
				}
				ilRawLoad(temp);

				// generic image setup
				if (texname[0] == '*')
					texname[0] = '#';
				sprintf(tex->texname, "%s/%s", file->name.c_str(), texname);
				ImageLoadFromIL(file, tex);

				// set next texture
				tex->next = NewImage();
				tex->next->error = true;
				tex = tex->next;
			}
			texnum++;
		}
	}
	qfree(filedata);
}

void LoadImage(ScanFile *file, byte *filedata, size_t filesize, LoadedImage *image)
{
	unsigned int fourCC;

	ThreadLock();
	FlushImage(image);
	fourCC =  *(unsigned int *)filedata;
	if (fourCC == MAKEFOURCC('I','D','S','P'))
		LoadImage_QuakeSprite(file, filedata, filesize, image);
	else if (fourCC == 29)
		LoadImage_QuakeBSP(file, filedata, filesize, image);
	else
	{
		// load standart image
		int loaded = ilLoadL(IL_TYPE_UNKNOWN, filedata, filesize);
		qfree(filedata);
		if (!loaded)
		{
			image->error = true;
			Warning("%s%s.%s : failed to load - %s", file->path.c_str(), file->name.c_str(), file->ext.c_str(), iluErrorString(ilGetError()));
		}
		else
		{
			image->filesize = filesize;
			ImageLoadFromIL(file, image);
		}
	}
	ThreadUnlock();
}

void FreeImage_d(LoadedImage *image)
{
	if (image->data)
		qfree(image->data);
	if (image->mipMaps)
	{
		MipMapImage *mipmap, *nextmip;
		for (mipmap = image->mipMaps; mipmap; mipmap = nextmip)
		{
			nextmip = mipmap->nextmip;
			qfree(mipmap->data);
			qfree(mipmap);
		}
		image->mipMaps = NULL;
	}
}

void FreeImage(LoadedImage *image)
{
	LoadedImage *frame, *next;

	FreeImage_d(image);
	for (frame = image->next; frame != NULL; frame = next)
	{
		next = frame->next;
		FreeImage_d(frame);
		qfree(frame);
	}
	image->data = NULL;
}



/*
==========================================================================================

  DDS compression - DDS Writing routines

==========================================================================================
*/

bool IsDXT5SwizzledFormat(DWORD FourCC)
{
   if (FourCC == FOURCC_DXT5_xGBR || FourCC == FOURCC_DXT5_RxBG || FourCC == FOURCC_DXT5_RBxG ||
       FourCC == FOURCC_DXT5_xRBG || FourCC == FOURCC_DXT5_RGxB || FourCC == FOURCC_DXT5_xGxR ||
       FourCC == FOURCC_ATI2N_DXT5)
       return true;
   return false;
}

bool WriteDDSHeader(FILE *f, LoadedImage *image, DWORD FourCC, bool headerFourCC)
{
	DDSD2 dds;

	memset(&dds, 0, sizeof(dds));
	if (headerFourCC)
		if (!fwrite(&DDS_HEADER, sizeof(DWORD), 1, f))
			return false;

	// write header
	dds.dwSize = sizeof(DDSD2);
	dds.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
	dds.dwWidth = image->width;
	dds.dwHeight = image->height;
	dds.dwMipMapCount = 1;
	for (MipMapImage *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip) dds.dwMipMapCount++;
	dds.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	dds.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	dds.ddpfPixelFormat.dwFourCC = FourCC;
	dds.ddpfPixelFormat.dwFlags = DDPF_FOURCC;

	// fill alphapixels information, ensure that our texture have actual alpha channel
	// also texture may truncate it's alpha by forcing DXT1 compression no it
	if (image->format == IL_RGBA && (image->compressionType != COMPRESSION_DXT1 || image->compressionType != COMPRESSION_DXT3 || image->compressionType != COMPRESSION_DXT5))
		dds.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;

	// fill alphapremult
	if (FourCC == FOURCC_DXT2 || FourCC == FOURCC_DXT4)
		dds.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPREMULT;
		
	// is we writing swizzled DXT5 format
	if (IsDXT5SwizzledFormat(FourCC))
	{
		dds.ddpfPixelFormat.dwPrivateFormatBitCount = dds.ddpfPixelFormat.dwFourCC;
		dds.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;
	}
	if (!fwrite(&dds, sizeof(DDSD2), 1, f))
		return false;
	return true;
}

/*
==========================================================================================

  DDS compression - nvDXTlib path

==========================================================================================
*/

typedef struct
{
	FILE *f;
	LoadedImage *image;
	DWORD fourCC;
	int numwrites;
}nvWriteInfo;

NV_ERROR_CODE NvDXTLib_WriteDDS(const void *buffer, size_t count, const MIPMapData *mipMapData, void *userData)
{
	size_t res;

	nvWriteInfo *wparm = (nvWriteInfo *)userData;
	if (wparm->numwrites == 1 && count == sizeof(DDSD2))
	{
		// VorteX: hack nvidia DDS writer
		// write out own DDS header
		WriteDDSHeader(wparm->f, wparm->image, wparm->fourCC, false);
	}
	else
		res = fwrite(buffer, 1, count, wparm->f);
	wparm->numwrites++;
	return (res == count) ? NV_OK : NV_WRITE_FAILED;
}

bool GenerateDDS_NvDXTLib(ScanFile *file, LoadedImage *image, char *outfile, bool dostats)
{
	nvPixelOrder pixelOrder;
	nvCompressionOptions options;
	nvTextureFormats compression;
	nvWriteInfo wparm;
	size_t filesize;

	// open file
	wparm.f = fopen(outfile, "wb");
	wparm.numwrites = 0;
	if (!wparm.f)
	{
		Warning("NvDXTlib : %s - cannot write file (%s)", outfile, strerror(errno));
		return false;
	}

	// start options
	options.SetDefaultOptions();
	options.DoNotGenerateMIPMaps();
	if (image->generateMipmaps)
	{
		options.mipFilterType = kMipFilterSinc;
		options.GenerateMIPMaps(0);
	}

	// get compression
	if (image->compressionType == COMPRESSION_DXT1)
	{
		options.bForceDXT1FourColors = true;
		compression = kDXT1;
	}
	else if (image->compressionType == COMPRESSION_DXT1a)
	{
		options.bForceDXT1FourColors = false;
		compression = kDXT1a;
	}
	else if (image->compressionType == COMPRESSION_DXT3)
	{
		compression = kDXT3;
		if (image->premodulateColor)
			options.bPreModulateColorWithAlpha = true;
	}
	else if (image->compressionType == COMPRESSION_DXT5)
	{
		compression = kDXT5;
		if (image->premodulateColor)
			options.bPreModulateColorWithAlpha = true;
	}
	else
	{
		fclose(wparm.f);
		Warning("NvDXTlib : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompessionTypeStr(image->compressionType));
		return false;
	}
	wparm.fourCC = (compression == kDXT5) ? (image->premodulateColor ? FOURCC_DXT4 : FOURCC_DXT5) : (compression == kDXT3 ? (image->premodulateColor ? FOURCC_DXT2 : FOURCC_DXT3) : FOURCC_DXT1);
	wparm.image = image;

	// other options
	options.user_data = &wparm;
	options.SetQuality(kQualityHighest, 400);
	options.SetTextureFormat(kTextureTypeTexture2D, compression);
         if (image->format == IL_RGB)  pixelOrder = nvRGB;
	else if (image->format == IL_RGBA) pixelOrder = nvRGBA;
	else 
	{
		fclose(wparm.f);
		Warning("NvDXTlib : %s%s.dds - unsupported format 0x%04X", file->path.c_str(), file->name.c_str(), image->format);
		return false;
	}

	// compress
	NV_ERROR_CODE res = nvDXTcompress((unsigned char *)image->data, image->width, image->height, image->width*image->bpp, pixelOrder, &options, NvDXTLib_WriteDDS, NULL);
	if (res != NV_OK)
	{
		fclose(wparm.f);
		Warning("NvDXTlib : %s%s.dds - compressor fail (%s)", file->path.c_str(), file->name.c_str(), getErrorString(res));
		return false;
	}
	filesize = Q_filelength(wparm.f);
	fclose(wparm.f);

	// advance stats
	if (dostats)
	{
		exportedfiles++;
		compressionweight += (compression == kDXT1 || compression == kDXT1a) ? (1.0f/8.0f) : (1.0f/4.0f);
		outputsize += (double)(filesize / 1024.0f / 1024.0f);
		inputsize += (double)(image->filesize / 1024.0f / 1024.0f);
	}
	return true;
}

/*
==========================================================================================

  DDS compression - ATI Compressor path

==========================================================================================
*/

// compress single image
ATI_TC_ERROR atiCompress(ATI_TC_Texture *src, ATI_TC_Texture *dst, byte *data, int width, int height, ATI_TC_CompressOptions *options, ATI_TC_FORMAT compressFormat, int bpp, bool premodulateColor)
{
	// fill source pixels, swap rgb->bgr
	// premodulate color if requested
	src->dwWidth = width;
	src->dwHeight = height;
	src->dwPitch = width * bpp;
	src->dwDataSize = width * height * bpp;
	byte *in = data;
	byte *out = src->pData;
	byte *end = src->pData + src->dwDataSize;
	if (bpp == 3)
	{
		while (out < end)
		{
			out[0] = in[2];
			out[1] = in[1];
			out[2] = in[0];
			in += 3;
			out += 3;
		}
	}
	else
	{
		if (premodulateColor)
		{
			while (out < end)
			{
				out[0] = (byte)(in[2] * ((float)in[3] / 255.0f));
				out[1] = (byte)(in[1] * ((float)in[3] / 255.0f));
				out[2] = (byte)(in[0] * ((float)in[3] / 255.0f));
				out[3] = in[3];
				in += 4;
				out += 4;
			}
		}
		else
		{
			while (out < end)
			{
				out[0] = in[2];
				out[1] = in[1];
				out[2] = in[0];
				out[3] = in[3];
				in += 4;
				out += 4;
			}
		}
	}
	
	// fill dst texture
	dst->dwWidth = width;
	dst->dwHeight = height;
	dst->dwPitch = 0;
	dst->format = compressFormat;
	dst->dwDataSize = ATI_TC_CalculateBufferSize(dst);

	// convert
	return ATI_TC_ConvertTexture(src, dst, options, NULL, NULL, NULL);
}

bool GenerateDDS_AtiCompress(ScanFile *file, LoadedImage *image, char *outfile, bool dostats)
{
	ATI_TC_Texture src;
	ATI_TC_Texture dst;
	ATI_TC_CompressOptions options;
	ATI_TC_FORMAT compress;
	DWORD fourCC;
	size_t filesize;
	FILE *f;

	memset(&src, 0, sizeof(src));
	memset(&options, 0, sizeof(options));

	// open file
	f = fopen(outfile, "wb");
	if (!f)
	{
		Warning("AtiCompress : %s - cannot write file (%s)", outfile, strerror(errno));
		return false;
	}

	// get options
	options.nAlphaThreshold = 127;
	options.nCompressionSpeed = ATI_TC_Speed_Normal;
	options.bUseAdaptiveWeighting = true;
	options.bUseChannelWeighting = false;
	options.dwSize = sizeof(options);
	// get compression
	if (image->compressionType == COMPRESSION_DXT1)
	{
		options.bDXT1UseAlpha = false;
		compress = ATI_TC_FORMAT_DXT1;
	}
	else if (image->compressionType == COMPRESSION_DXT1a)
	{
		options.bDXT1UseAlpha = true;
		compress = ATI_TC_FORMAT_DXT1;
	}
	else if (image->compressionType == COMPRESSION_DXT3)
		compress = ATI_TC_FORMAT_DXT3;
	else if (image->compressionType == COMPRESSION_DXT5)
		compress = ATI_TC_FORMAT_DXT5;
	else
	{
		fclose(f);
		Warning("AtiCompress : %s%s.dds - unsupported compression %s", file->path.c_str(), file->name.c_str(), getCompessionTypeStr(image->compressionType));
		return false;
	}
	fourCC = (compress == ATI_TC_FORMAT_DXT3) ? (image->premodulateColor ? FOURCC_DXT2 : FOURCC_DXT3) : (compress == ATI_TC_FORMAT_DXT5) ? (image->premodulateColor ? FOURCC_DXT4 : FOURCC_DXT5) : GetFourCC(compress);

	// init source texture
	src.dwSize = sizeof(src);
	src.dwWidth = image->width;
	src.dwHeight = image->height;
	src.dwPitch = image->width*image->bpp;
	     if (image->format == IL_RGB)  src.format = ATI_TC_FORMAT_RGB_888;
	else if (image->format == IL_RGBA) src.format = ATI_TC_FORMAT_ARGB_8888;
	else 
	{
		fclose(f);
		Warning("AtiCompress : %s%s.dds - unsupported format 0x%04X", file->path.c_str(), file->name.c_str(), image->format);
		return false;
	}
	src.dwDataSize = image->size;
	
	// init dest texture
	memset(&dst, 0, sizeof(dst));
	dst.dwSize = sizeof(dst);
	dst.dwWidth = image->width;
	dst.dwHeight = image->height;
	dst.dwPitch = 0;
	dst.format = compress;
	dst.dwDataSize = ATI_TC_CalculateBufferSize(&dst);
	
	// allocate source & dest data
	src.pData = (ATI_TC_BYTE*)qmalloc(src.dwDataSize);
	dst.pData = (ATI_TC_BYTE*)qmalloc(dst.dwDataSize);

	// compress
	ATI_TC_ERROR res = ATI_TC_OK;
	if (!WriteDDSHeader(f, image, fourCC, true))
		res = ATI_TC_ERR_GENERIC;
	else
	{
		// write base texture and mipmaps
		res = atiCompress(&src, &dst, image->data, image->width, image->height, &options, compress, image->bpp, image->premodulateColor);
		if (res == ATI_TC_OK)
		{
			if (!fwrite(dst.pData, dst.dwDataSize, 1, f))
				res = ATI_TC_ERR_GENERIC;
			else
			{
				for (MipMapImage *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip)
				{
					res = atiCompress(&src, &dst, mipmap->data, mipmap->width, mipmap->height, &options, compress, image->bpp, image->premodulateColor);
					if (res != ATI_TC_OK)
						break;
					if (!fwrite(dst.pData, dst.dwDataSize, 1, f))
					{
						res = ATI_TC_ERR_GENERIC;
						break;
					}
				}
			}
		}
	}

	// free source & dest data 
	qfree(src.pData);
	qfree(dst.pData);

	// end advance stats
	filesize = Q_filelength(f);
	fclose(f);
	if (res != ATI_TC_OK)
	{
		Warning("AtiCompress : %s%s.dds - compressor fail (error code %i)", file->path.c_str(), file->name.c_str(), res);
		return false;
	}
	if (dostats)
	{
		exportedfiles++;
		compressionweight += (compress == ATI_TC_FORMAT_DXT1) ? (1.0f/8.0f) : (1.0f/4.0f);
		outputsize += (double)(filesize / 1024.0f / 1024.0f);
		inputsize += (double)(image->filesize / 1024.0f / 1024.0f);
	}
	return true;
}

/*
==========================================================================================

  DDS compression - Generic

==========================================================================================
*/

bool GenerateDDS(ScanFile *file, LoadedImage *image)
{
	char outfile[MAX_DDSPATH];
	bool res;

	// error image
	if (image->error)
		return false;

	// only support unsigned byte image type at the moment
	if (image->type != IL_UNSIGNED_BYTE)
	{
		Warning("%s - sorry, only unsigned byte color images are supported", outfile);
		return false;
	}

	// open file
	const char *texname = image->texname[0] ? image->texname : file->name.c_str();
	if (image->framenum < 0)
		sprintf(outfile, "%s%s%s.dds", outdir, file->path.c_str(), texname);
	else if (image->groupnum >= 0)
		sprintf(outfile, "%s%s%s.%s_%i_%i.dds", outdir, file->path.c_str(), texname, file->ext.c_str(), image->groupnum, image->framenum);
	else
		sprintf(outfile, "%s%s%s.%s_%i.dds", outdir, file->path.c_str(), texname, file->ext.c_str(), image->framenum);
	CreatePath(outfile);

	// compress
	if (image->compressorTool == COMPRESSOR_ATI)
		res = GenerateDDS_AtiCompress(file, image, outfile, true);
	else
		res = GenerateDDS_NvDXTLib(file, image, outfile, true);
	return res;
}

void GenerateDDS_ThreadWork(int threadnum)
{
	int	work;
	ILuint image_unit;
	char filename[MAX_DDSPATH];
	LoadedImage image, *frame;

	ilGenImages(1, &image_unit);
	ilBindImage(image_unit);

	while(1)
	{
		work = GetWorkForThread();
		if (work == -1)
			break;
		
		Pacifier(" file %i of %i", work + 1, numthreadwork);

		// load image
		ScanFile *file = &textures[work];
		sprintf(filename, "%s%s%s.%s", dir, file->path.c_str(), file->name.c_str(), file->ext.c_str());
		image.error = true;
		if (file->arc_type == ARCHIVE_ZIP)
		{
			image.error = true;

			// load image from ZIP archive
			byte localheader[ZIPLOCALHEADER_SIZE];
			fseek(file->arc_file, file->arc_offset, SEEK_SET);
			fread(&localheader, sizeof(localheader), 1, file->arc_file);
			if (ferror(file->arc_file))
				Warning("%s%s.%s : cannot read archive (%s)", file->path.c_str(), file->name.c_str(), file->ext.c_str(), strerror(errno));
			else if (*(unsigned int *)(localheader) != FOURCC_ZIP_ENTRY)
				Warning("%s%s.%s : broken ZIP", file->path.c_str(), file->name.c_str(), file->ext.c_str());
			else
			{
				unsigned int csize = *(unsigned int *)(localheader + 18);
				unsigned int usize = *(unsigned int *)(localheader + 22);
				unsigned int filenamelen = *(unsigned int *)(localheader + 26);
				unsigned short compression = *(unsigned short *)(localheader + 8);
				fseek(file->arc_file, filenamelen, SEEK_CUR);

				// extract & load
				byte *compressed = (byte *)qmalloc(csize);
				fread(compressed, csize, 1, file->arc_file);
				if (ferror(file->arc_file))
					Warning("%s%s.%s : cannot read ZIP (%s)", file->path.c_str(), file->name.c_str(), file->ext.c_str(), strerror(errno));
				else
				{
					byte *decompressed = (byte *)qmalloc(usize);
					if (compression == 8) // deflate
					{
						// unpack deflated
						z_stream z;
						int ret;
						memset(&z, 0, sizeof(z));
						z.zalloc = Z_NULL;
						z.zfree = Z_NULL;
						z.opaque = Z_NULL;
						if (inflateInit2(&z, -MAX_WBITS) != Z_OK)
							Warning("%s%s.%s : cannot read ZIP (inflateInit %s)", file->path.c_str(), file->name.c_str(), file->ext.c_str(), zlibErrorString(ret));
						else
						{
							byte tmp[2048];
							byte *out = decompressed;
							z.next_in = compressed;
							z.avail_in = csize;
							do
							{
								z.next_out = tmp;
								z.avail_out = sizeof(tmp);
								ret = inflate(&z, Z_SYNC_FLUSH);
								if (ret != Z_OK && ret != Z_STREAM_END)
								{
									Warning("%s%s.%s : cannot read ZIP (inflate %s)", file->path.c_str(), file->name.c_str(), file->ext.c_str(), zlibErrorString(ret));
									break;
								}
								int have = sizeof(tmp) - z.avail_out;
								memcpy(out, tmp, have);
								out += have;
							} while(ret != Z_STREAM_END);
							inflateEnd(&z);

							// load file
							qfree(compressed);
							LoadImage(file, decompressed, usize, &image);
						}
					}
					else
					{
						qfree(compressed);
						Warning("%s : unsupported ZIP compression %i", file->fullpath.c_str(), compression);
					}
				}
			}
			// check if there are more files referring to this archive
			for (work++; work < numthreadwork; work++)
				if (textures[work].arc_file == file->arc_file)
					break;
			if (work >= numthreadwork)
				fclose(file->arc_file);
		}
		else
		{
			image.error = true;
			// load standart image
			byte *filedata;
			size_t filesize = LoadFileUnsafe(filename, &filedata);
			if (filesize < 0)
			{
				Warning("%s%s.%s : cannot open file (%s)", file->path.c_str(), file->name.c_str(), file->ext.c_str(), strerror(errno));
				return;
			}
			LoadImage(file, filedata, filesize, &image);
		}

		// make DDS for all frames
		for (frame = &image; frame != NULL; frame = frame->next)
			GenerateDDS(file, frame);
		FreeImage(&image);
	}
}

int Help_Main();
int DDS_Main(int argc, char **argv)
{
	double timeelapsed;
	char cachefile[MAX_DDSPATH];
	int i;

	// launched without parms, try to find kain.exe
	i = 0;
	strcpy(dir, "");
	strcpy(filemask, "");
	strcpy(outdir, "");
	usecache = CheckParm("-nocache") ? false : true;
	if (argc < 1)
	{
		char find[MAX_PATH];
		bool found_dir = false;
		for (int i = 0; i < 10; i++)
		{
			strcpy(find, "../");
			for (int j = 0; j < i; j++)
				strcat(find, "../");
			strcat(find, opt_basedir);
			if (FindDir(find))
			{
				found_dir = true;
				break;
			}
		}
		if (found_dir)
		{
			Print("Base and output dir detected\n");
			sprintf(dir, "%s/", find);
			sprintf(outdir, "%s/%s/", find, opt_ddsdir);
		}
		else
		{
			waitforkey = true;
			Help_Main();
			Error("bad commandline", progname);
		}
	}
	else
	{
		strcpy(dir, argv[0]);
		i++;
		// get output path
		// if dragged file to exe, there is no output path
		if (argc < 2)
		{
			usecache = false;
			if (FindDir(dir)) // a directory
			{
				AddSlash(dir);
				sprintf(outdir, "%s%s", dir, opt_ddsdir);
			}
			else
			{
				ExtractFileName(dir, filemask);
				ExtractFilePath(dir, outdir);
				strcpy(dir, outdir);
				// for archive files make a dds directory (just like folder)
				if (FileMatchList(filemask, opt_archiveFiles))
					strcat(outdir, opt_ddsdir);
			}
		}
		else
		{
			strcpy(outdir, argv[1]);
			waitforkey = true;
			i++;
		}
	}
	AddSlash(dir);
	AddSlash(outdir);

	// parameters
	nomips = CheckParm("-nomip");
	useaticompressor = CheckParm("-ati");
	usenvidiacompressor = CheckParm("-nv");
	usenopoweroftwo = CheckParm("-npot");
	usescaler = CheckParm("-2x");
	if (nomips)
		Print("Not generating mipmaps\n");
	if (useaticompressor)
		Print("Using ATI compressor\n");
	else if (usenvidiacompressor)
		Print("Using NVidia DXTlib compressor\n");
	else
		Print("Using hybrid ATI/NVidia compressor\n");
	if (usenopoweroftwo)
		Print("Allowing non-power-of-two texture dimensions\n");
	if (usescaler)
		Print("Upscaling texture using scale2x effect\n");
	if (usecache)
	{
		Print("Converting only files that was changed\n");
		sprintf(cachefile, "%sfilescrc.txt", outdir);
		LoadFileCache(cachefile);
	}

	// find files
	Print("Entering \"%s%s\"\n", dir, filemask);
	if (usecache)
		Print("Calculating crc32 for files\n");
	textures.clear();
	texturesSkipped = 0;
	ScanFiles(dir, filemask, NULL);
	if (texturesSkipped)
		Print("Skipping %i unchanged files\n", texturesSkipped);
	if (!textures.size())
	{
		Print("No files to convert\n");
		return 0;
	}

	// decompress DDS
	if (filemask[0] && textures.size() == 1 && !strnicmp(textures[0].suf.c_str(), "dds", 3))
	{
		Print("Decompressing DDS\n");
		Print("Not implemented\n");
		return 0;
	}

	// convert to DDS
	inputsize = 0;
	outputsize = 0;
	compressionweight = 0;
	exportedfiles = 0;
	Print("Generating to \"%s\"\n", outdir);
	timeelapsed = RunThreads(textures.size(), GenerateDDS_ThreadWork);

	// save cache file
	if (usecache)
		SaveFileCache(cachefile);

	// show some stats
	Print("Conversion stats:\n");
	Print("       time elapsed: %i minutes\n", (int)(timeelapsed / 60));
	Print("         files size: %.2f mb\n", inputsize);
	Print("     DDS files size: %.2f mb\n", outputsize);
	Print("  compression ratio: %.0f%%\n", (outputsize / (inputsize + 0.01f)) * 100.0f);
	Print("    video RAM saved: %.0f%%\n", (1.0f - (float)(compressionweight / (float)exportedfiles)) * 100.0f);
	return 0;
}

/*
==========================================================================================

  Help section

==========================================================================================
*/

int Help_Main()
{
	Print(
	"usage: rwgdds <path> [<ddspath>]\n"
	"\n");
	return 0;
}

/*
==========================================================================================

  Program main

==========================================================================================
*/

int main(int argc, char **argv)
{
	int i, returncode = 0;
	bool printcap;

	// get program name
	myargv = argv;
	myargc = argc;
	memset(progname, 0, MAX_DDSPATH);
	memset(progpath, 0, MAX_DDSPATH);
	ExtractFileBase(argv[0], progname);
	ExtractFilePath(argv[0], progpath);

	// check command line flags
	verbose = true;
	noprint = false;
	printcap      = CheckParm("-nc") ? false : true;
	waitforkey    = (CheckParm("-w") || CheckParm("-"));
	memstats      = CheckParm("-mem");
	solidpacifier = CheckParm("-sp");
	errorlog      = CheckParm("-errlog");
	if (CheckParm("-c")) // disable partial printings
	{
		verbose = false;
		printcap = false;
	}
	if (CheckParm("-f")) // disable all printings
	{
		verbose = false;
		printcap = false;
		noprint = true;
	}
	for (i = 1; i < argc; i++) 
	{
		if (!strnicmp(argv[i], "-cd", 3))
		{
			i++;
			if (i < argc)
				ChangeDirectory(argv[i]);
			continue;
		}
		if (!strnicmp(argv[i], "-threads", 8))
		{
			i++;
			if (i < argc)
				numthreads = atoi(argv[i]);
			continue;
		}
		if (argv[i][0] != '-')
			break;
	}

	// init stuff
	crc32_init();
	ilInit();
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	iluInit();
	iluImageParameter(ILU_FILTER, ILU_BILINEAR);
	ThreadInit();
	LoadOptions("rwgdds.opt");

	// print caption
	if (printcap)
	{
		Print("-----------------------------------------------------------\n");
		Print(" RwgDDS %s by Pavel [VorteX] Timofeyev\n", RWGDDS_VERSION);
		Print(" NVidia DXTC %s\n", GetDXTCVersion());
		Print(" ATI Compress %i.%i\n", ATI_COMPRESS_VERSION_MAJOR, ATI_COMPRESS_VERSION_MINOR);
		Print(" ZLib %s (linked against %s)\n", zlibVersion(), ZLIB_VERSION);
		Print(" DevIL %i.%i\n", (int)(ilGetInteger(IL_VERSION_NUM) / 100), ilGetInteger(IL_VERSION_NUM) - ((int)(ilGetInteger(IL_VERSION_NUM) / 100))*100);
		Print("-----------------------------------------------------------\n");
		Print("%i threads\n", numthreads);
		if (memstats)
			Print("showing memstats\n");
		if (waitforkey)
			Print("waiting for key\n");
		Print("\n");
	}

	// no args check
	//if (i >= argc)
	//{
	//	waitforkey = true;
	//	Help_Main ();
	//	Error  ("bad commandline" , progname );
	//}

	// init memory
	Q_InitMem();

	// do the action
	returncode = DDS_Main(argc-i, argv+i);
	Print("\n");

	// free allocated memory
	Q_ShutdownMem(memstats);

#if _MSC_VER
	if (waitforkey)
	{
		Print("press any key\n");
		getchar();
	}
#endif

	return returncode;
}