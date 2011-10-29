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

#include "windows.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// DevID image library
#include <IL/il.h>
#pragma comment(lib, "DevIL.lib")
#include <IL/ilu.h>
#pragma comment(lib, "ILU.lib")

// DXT compressor
#include <dxtlib/dxtlib.h>
#pragma comment(lib, "nvDXTlibMT.vc8.lib")
using namespace nvDDS;

bool waitforkey;
bool error_waitforkey;
bool memstats;
bool verbose;
bool noprint;
bool solidpacifier;
bool errorlog;

char progname[MAX_DDSPATH];
char progpath[MAX_DDSPATH];

// DDS converting 
char dir[MAX_DDSPATH];
char filemask[MAX_DDSPATH];
char outdir[MAX_DDSPATH];
double inputsize;
double outputsize;
double compressionweight;

// console stuff
void Print(char *str, ...)
{
	va_list argptr;

	if (noprint)
		return;

	ThreadLock();
	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
	ThreadUnlock();
}

void Verbose(char *str, ...)
{
	va_list argptr;

	if (!verbose || noprint)
		return;

	ThreadLock();
	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
	ThreadUnlock();
}

// flush out after that string 
void PercentPacifier(char *str, ...)
{
	va_list argptr;

	ThreadLock();
	va_start(argptr, str);
	if (solidpacifier)
	{
		vprintf(str, argptr);
		printf("\n");
		va_end(argptr);
		Sleep(20);
	}
	else
	{
		printf("\r");
		vprintf(str, argptr);
		printf("%\r");
		va_end(argptr);
	}
	fflush(stdout);
	ThreadUnlock();
}

void Pacifier(char *str, ...)
{
	va_list argptr;

	if (noprint)
		return;

	ThreadLock();
	va_start(argptr, str);
	printf("\r");
	vprintf(str, argptr);
	printf("                                        ");
	printf("\r");
	va_end(argptr);
	fflush(stdout);
	ThreadUnlock();
}

void PacifierEnd() 
{
	if (noprint)
		return;

	ThreadLock();
	printf("\n");
	ThreadUnlock();
}

void Warning(char *str, ...)
{
	va_list argptr;

	ThreadLock();
	va_start(argptr, str);
	printf("Warning: ");
	vprintf(str, argptr);
	va_end(argptr);
	printf("\n");
	ThreadUnlock();
}

void ExtractFileSuffix(char *path, char *dest)
{
	char *src;

	src = path + strlen(path) - 1;
	// back up until a _ or the start
	while (src != path && *(src-1) != '_')
		src--;
	if (src == path)
	{
		*dest = 0;
		return;
	}
	strcpy(dest, src);
}

/*
==========================================================================================

  SCAN DIRECTORY

==========================================================================================
*/

typedef struct
{
	string path;
	string name;
	string ext;
	string suf;
}
ScanFile;

vector<ScanFile> files;

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
		Error("ScanFiles: failed to open %s\n", pattern);
		return;
	}
	do
	{
		if (!strncmp(n_file.cFileName, ".", 1))
			continue;
		if (n_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			strlcpy(scanpath, path, sizeof(scanpath));
			strlcat(scanpath, n_file.cFileName, sizeof(scanpath));
			ScanFiles(basepath, NULL, scanpath);
			continue;
		}
		ExtractFileExtension(n_file.cFileName, ext);
		Q_strlower(ext);
		StripFileExtension(n_file.cFileName, name);
		Q_strlower(name);
		ExtractFileSuffix(name, suf);
		// dont compress radiant-only images
		if (!strncmp(suf, "radiant", 7))
			continue;
		if (!strncmp(path, "cubemaps/", 9) || !strncmp(path, "screenshots/", 12) || !strncmp(path, "video/", 6) || !strncmp(path, "dds/", 4) || !strncmp(path, "data/", 5))
			continue;
		// dont compress cubemaps at the moment
		// only supply tga or jpeg (unless custom file are given)
		if (!singlefile)
			if (strncmp(ext, "tga", 3) && strncmp(ext, "jpg", 3))
				continue;
		// add file
		ScanFile NF;
		NF.path = path;
		NF.name = name;
		NF.ext = ext;
		NF.suf = suf;
		files.push_back(NF);
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

	// frames
	int framenum; // -1 if its not multiframe image
	LoadedImage_s *nextframe;
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
	image->error = false;
	image->framenum = -1;
	image->nextframe = NULL;
}

int NextPowerOfTwo(int n) 
{ 
    if ( n <= 1 ) return n;
    double d = n-1; 
    return 1 << ((((int*)&d)[1]>>20)-1022); 
}

void LoadImage_Texture(char *filename, ScanFile *file, LoadedImage *image)
{
	ThreadLock();

	// load
	FlushImage(image);
	if (!ilLoadImage(filename))
	{
		image->error = true;
		ThreadUnlock();
		Warning("%s%s.%s : failed to load", file->path.c_str(), file->name.c_str(), file->ext.c_str());
		return;
	}

	image->filesize = FileSize(filename);

	// convert image to suitable format
	image->format = ilGetInteger(IL_IMAGE_FORMAT);
	image->type = ilGetInteger(IL_IMAGE_TYPE);
	if (image->format == IL_BGR)
		ilConvertImage(IL_RGB, image->type);
	else if (image->format == IL_BGRA)
		ilConvertImage(IL_RGBA, image->type);
	else if (image->format != IL_RGB && image->format != IL_RGBA)
		ilConvertImage(IL_RGB, image->type);

	// scale image to nearest power of two
	// VorteX: we scale manually because we do compare compressed image with original
	iluImageParameter(ILU_FILTER, ILU_SCALE_LANCZOS3);
	iluScale(NextPowerOfTwo(ilGetInteger(IL_IMAGE_WIDTH)), NextPowerOfTwo(ilGetInteger(IL_IMAGE_HEIGHT)), ilGetInteger(IL_IMAGE_BPP));

	// read
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
	image->nextframe = NULL;
	ThreadUnlock();
}

void LoadImage_SPR32(char *filename, ScanFile *file, LoadedImage *image)
{
	ThreadLock();
	FlushImage(image);
	ThreadUnlock();
	/*
	int bufsize, i, framehead[5], y;
	char scriptfile[MAX_BLOODPATH], framepic[MAX_BLOODPATH], outfolder[MAX_BLOODPATH];
	byte *b, *buf, *buffer, *out, *in, *end;
	spr_t sprheader;
	FILE *f, *f2;

	// open scriptfile
	ExtractFilePath(file, outfolder);
	if (outfolder[0])
		sprintf(scriptfile, "%s/sprinfo.txt", outfolder);
	else
		sprintf(scriptfile, "sprinfo.txt", outfolder);
	printf("open %s...\n", scriptfile);
	f = SafeOpenWrite(scriptfile);

	// load spr header
	bufsize = LoadFile(file, &b);
	buf = b;
	if (!SPR_ReadHeader(buf, bufsize, &sprheader))
		Error("%s: not IDSP file\n", file);
	if (sprheader.ver1 != SPR_DARKPLACES)
		Error("%s: not Darkplaces SPR32 sprite\n", file);
	fprintf(f, "TYPE %i\n", sprheader.type);
	fprintf(f, "SYNCTYPE %i\n", sprheader.synchtype);
	// write frames
	buf+=sizeof(spr_t);
	bufsize-=sizeof(spr_t);
	for (i = 0; i < sprheader.nframes; i++)
	{
		if (bufsize < 20)
			Error("unexpected EOF");
		// read frame header
		framehead[0] = LittleInt(buf);
		framehead[1] = LittleInt(buf + 4);
		framehead[2] = LittleInt(buf + 8);
		framehead[3] = LittleInt(buf + 12);
		framehead[4] = LittleInt(buf + 16);
		buf+=20;
		bufsize-=20;
		// write frame
		fprintf(f, "FRAME %i %i %i\n", framehead[0], framehead[1], framehead[2]);
		if (bufsize < framehead[3]*framehead[4]*4)
			Error("unexpected EOF");
		if (framehead[3]*framehead[4] <= 0)
			Error("bad frame width/height");
		// write tga image
		sprintf(framepic, "frame%04i.tga", i);
		Verbose("writing frame %s\n", framepic);
		f2 = SafeOpenWrite(framepic);
		buffer = qmalloc(framehead[3]*framehead[4]*4 + 18);
		memset(buffer, 0, 18);
		buffer[2] = 2; // uncompressed
		buffer[12] = (framehead[3] >> 0) & 0xFF;
		buffer[13] = (framehead[3] >> 8) & 0xFF;
		buffer[14] = (framehead[4] >> 0) & 0xFF;
		buffer[15] = (framehead[4] >> 8) & 0xFF;
		buffer[16] = 32;
		out = buffer + 18;
		for (y = framehead[4] - 1;y >= 0;y--)
		{
			in = buf + y * framehead[3] * 4;
			end = in + framehead[3] * 4;
			for (;in < end;in += 4)
			{
				*out++ = in[2];
				*out++ = in[1];
				*out++ = in[0];
				*out++ = in[3];
			}
		}
		fwrite(buffer, framehead[3]*framehead[4]*4 + 18, 1, f2);
		qfree(buffer);
		WriteClose(f2);
		// advance
		buf+=framehead[3]*framehead[4]*4;
		bufsize-=framehead[3]*framehead[4]*4;
	}
	WriteClose(f);
	qfree(b);
	*/
}

void FreeImage(LoadedImage *image)
{
	LoadedImage *frame, *nextframe;

	ThreadLock();
	if (!image->error)
		qfree(image->data);
	for (frame = image->nextframe; frame != NULL; frame = nextframe)
	{
		nextframe = frame->nextframe;
		qfree(frame);
	}
	image->data = NULL;
	ThreadUnlock();
}

/*
==========================================================================================

  DDS converting

==========================================================================================
*/

NV_ERROR_CODE WriteDDS(const void *buffer, size_t count, const MIPMapData *mipMapData, void * userData)
{
	FILE *f = (FILE *)userData;
	size_t res = fwrite(buffer, 1, count, f);
	return (res == count) ? NV_OK : NV_WRITE_FAILED;
}

bool GenerateDDS(ScanFile *file, LoadedImage *image)
{
	char outfile[MAX_DDSPATH];
	size_t filesize;

	// only support unsigned byte image type at the moment
	if (image->type != IL_UNSIGNED_BYTE)
	{
		Warning("%s - sorry, only unsigned byte color images are supported", outfile);
		return false;
	}

	// open file
	if (image->framenum >= 0)
		sprintf(outfile, "%s%s%s_%i.dds", outdir, file->path.c_str(), file->name.c_str(), image->framenum);
	else
		sprintf(outfile, "%s%s%s.dds", outdir, file->path.c_str(), file->name.c_str());
	CreatePath(outfile);
	ThreadLock();
	FILE *f = fopen(outfile, "wb");
	ThreadUnlock();
	if (!f)
	{
		Warning("%s - cannot write file (%s)", outfile, strerror(errno));
		return false;
	}

	// check alpha
	bool has_alpha = false;
	bool gradient_alpha = false;
	if (image->format == IL_RGBA || image->format == IL_BGRA)
	{
		has_alpha = true;
		int num_grad = 0;
		int need_grad = max(image->width, image->height) / 4;
		for (long i = 0; i < image->width*image->height*4; i+= 4)
		{
			if (*(image->data + i) < 215 && *(image->data + i) > 40)
				num_grad++;
			if (num_grad > need_grad)
			{
				gradient_alpha = true;
				break;
			}
		}
	}

	// get default format
	nvPixelOrder pixelOrder;
	nvCompressionOptions options;
	nvTextureFormats compression;
	options.SetDefaultOptions();
	if (!strncmp(file->path.c_str(), "gfx/", 4) || !strncmp(file->path.c_str(), "particles/", 10) || !strncmp(file->path.c_str(), "script/", 7))
	{
		compression = has_alpha ? kDXT5 : kDXT3;
		options.DoNotGenerateMIPMaps();
	}
	else if (!strcmp(file->suf.c_str(), "norm"))
	{
		// todo: 3dc
		compression = kDXT3;
		options.mipFilterType = kMipFilterSinc;
		options.GenerateMIPMaps(0);
	}
	else
	{
		compression = has_alpha ? ( gradient_alpha ? kDXT3 : kDXT1a ) : kDXT1a; 
		options.mipFilterType = kMipFilterSinc;
		options.GenerateMIPMaps(0);
	}

	// make other options
	options.user_data = f;
	options.SetQuality(kQualityHighest, 400);
	options.SetTextureFormat(kTextureTypeTexture2D, compression);
         if (image->format == IL_RGB)  pixelOrder = nvRGB;
	else if (image->format == IL_RGBA) pixelOrder = nvRGBA;
	else if (image->format == IL_BGR)  pixelOrder = nvBGR;
	else if (image->format == IL_BGRA) pixelOrder = nvBGRA;
	else 
	{
		fclose(f);
		Warning("%s%s.dds - unsupported format 0x%04X", file->path.c_str(), file->name.c_str(), image->format);
		return false;
	}

	// compress
	NV_ERROR_CODE res = nvDXTcompress((unsigned char *)image->data, image->width, image->height, image->width*image->bpp, pixelOrder, &options, WriteDDS, NULL);
	if (res != NV_OK)
	{
		Warning("%s%s.dds - DXT compressor fail (%s)", file->path.c_str(), file->name.c_str(), getErrorString(res));
		return false;
	}
	filesize = Q_filelength(f);
	fclose(f);

	compressionweight += (compression == kDXT1 || compression == kDXT1a) ? (1.0f/6.0f) : (1.0f/1.4f);
	outputsize += (double)(filesize / 1024.0f / 1024.0f);
	inputsize += (double)(image->filesize / 1024.0f / 1024.0f);
	return true;
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
		ScanFile *file = &files[work];
		sprintf(filename, "%s%s%s.%s", dir, file->path.c_str(), file->name.c_str(), file->ext.c_str());
		if (!strcmp(file->ext.c_str(), "spr32"))
			LoadImage_SPR32(filename, file, &image);
		else
			LoadImage_Texture(filename, file, &image);
		if (image.error)
			continue;

		// make DDS for all frames
		for (frame = &image; frame != NULL; frame = frame->nextframe)
			GenerateDDS(file, frame);
		FreeImage(&image);
	}
}

int Help_Main();
int DDS_Main(int argc, char **argv)
{
	double timeelapsed;

	// launched without parms, try to find kain.exe
	strcpy(dir, "");
	strcpy(filemask, "");
	strcpy(outdir, "");
	if (argc < 1)
	{
		char find[MAX_PATH];
		bool found_dir = false;
		for (int i = 0; i < 10; i++)
		{
			strcpy(find, "../");
			for (int j = 0; j < i; j++)
				strcat(find, "../");
			strcat(find, "kain");
			if (FindDir(find))
			{
				found_dir = true;
				break;
			}
		}
		if (found_dir)
		{
			Print("Base and output dir detected\n");
			strcpy(dir, find);
			strcat(dir, "/");
			strcpy(outdir, find);
			strcat(outdir, "/dds/");
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
		// get output path
		// if dragged file to exe, there is no output path
		if (argc < 2)
		{
			ExtractFileName(dir, filemask);
			ExtractFilePath(dir, outdir);
			strcpy(dir, outdir);
		}
		else
		{
			strcpy(outdir, argv[1]);
			waitforkey = true;
		}
	}
	AddSlash(dir);
	AddSlash(outdir);

	// find files
	Print("entering \"%s%s\"\n", dir, filemask);
	files.clear();
	ScanFiles(dir, filemask, NULL);
	if (!files.size())
		Error("No files to convert!");

	// process
	inputsize = 0;
	outputsize = 0;
	compressionweight = 0;
	Print("generating to \"%s\"\n", outdir);
	timeelapsed = RunThreads(files.size(), GenerateDDS_ThreadWork);
	Print("Conversion finished:\n");
	Print("       time elapsed: %i minutes\n", (int)(timeelapsed / 60));
	Print("         files size: %.2f mb\n", inputsize);
	Print("     DDS files size: %.2f mb\n", outputsize);
	Print("  compression ratio: %.0f%%\n", (outputsize / (inputsize + 0.01f)) * 100.0f);
	Print("    video RAM saved: %.0f%%\n", (1.0f - (float)(compressionweight / (float)files.size())) * 100.0f);
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
	memset(progname, 0, MAX_DDSPATH);
	memset(progpath, 0, MAX_DDSPATH);
	ExtractFileBase(argv[0], progname);
	ExtractFilePath(argv[0], progpath);

	// check command line flags
	verbose = true;
	printcap = true;
	waitforkey = true;
	memstats = false;
	noprint = false;
	solidpacifier = false;
	errorlog = false;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i],"-nc")) // disable caption
		{
			printcap = false;
			continue;
		}
		if (!strcmp(argv[i], "-c")) // disable partial printings
		{
			verbose = false;
			printcap = false;
			continue;
		}
		if (!strcmp(argv[i],"-f"))  // disable all printings
		{
			verbose = false;
			printcap = false;
			noprint = true;
			continue;
		}
		if (!strcmp(argv[i],"-mem"))
		{
			memstats = true;
			continue;
		}
		if (!strcmp(argv[i], "-w"))
		{
			waitforkey = true;
			continue;
		}
		if (!strcmp(argv[i], "-ew"))
		{
			error_waitforkey = true;
			continue;
		}
		if (!strcmp(argv[i], "-cd"))
		{
			i++;
			if (i < argc)
				ChangeDirectory(argv[i]);
			continue;
		}
		if (!strcmp(argv[i], "-sp"))
		{
			solidpacifier = true;
			continue;
		}
		if (!strcmp(argv[i], "-errlog"))
		{
			errorlog = true;
			continue;
		}
		if (!strcmp(argv[i], "-threads"))
		{
			i++;
			if (i < argc)
				numthreads = atoi(argv[i]);
			continue;
		}
		break;
	}

	// init DevIL and thread worker
	ilInit();
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	iluInit();
	ThreadInit();

	// print caption
	if (printcap)
	{
		Print(RWGDDS_WELCOME, RWGDDS_VERSION);
		if (memstats)
			Print("memstats = true\n");
		if (waitforkey)
			Print("waitforkey = true\n");
		Print("NVidia DXTC %s\n", GetDXTCVersion());
		Print("DevIL Version %i\n", ilGetInteger(IL_VERSION_NUM));
		Print("%i threads\n", numthreads);
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