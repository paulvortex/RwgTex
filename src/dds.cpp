////////////////////////////////////////////////////////////////
//
// RWGTEX - DDS exporting
// coded by Pavel [VorteX] Timofeyev and placed to public domain
// DDS compression routines are picked from DDS GIMP plugin
//  Copyright (C) 2004 Shawn Kirst <skirst@gmail.com>, with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
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

#include "main.h"
#include "dds.h"

char opt_srcDir[MAX_FPATH];
char opt_srcFile[MAX_FPATH];
char opt_destPath[MAX_FPATH];

#include <ATI_Compress_Test_Helpers.cpp>

char *getCompressionFormatString(DWORD formatCC)
{
	if (formatCC == FORMAT_DXT1) return "DXT1";
	if (formatCC == FORMAT_DXT2) return "DXT2";
	if (formatCC == FORMAT_DXT3) return "DXT3";
	if (formatCC == FORMAT_DXT4) return "DXT4";
	if (formatCC == FORMAT_DXT5) return "DXT5";
	if (formatCC == FORMAT_BGRA) return "BGRA";
	if (formatCC == FORMAT_RXGB) return "RXGB";
	if (formatCC == FORMAT_YCG1) return "YCG1";
	if (formatCC == FORMAT_YCG2) return "YCG2";
	return "UNKNOWN";
}

double getCompressionRatio(DWORD formatCC)
{
	if (formatCC == FORMAT_BGRA)
		return 1.0f;
	if (formatCC == FORMAT_DXT1)
		return 1.0f/8.0f;
	return 1.0f/4.0f;
}

bool IsDXT5SwizzledFormat(DWORD formatCC)
{
   if (formatCC == FOURCC_DXT5_xGBR || formatCC == FOURCC_DXT5_RxBG || formatCC == FOURCC_DXT5_RBxG ||
       formatCC == FOURCC_DXT5_xRBG || formatCC == FOURCC_DXT5_RGxB || formatCC == FOURCC_DXT5_xGxR ||
       formatCC == FOURCC_ATI2N_DXT5)
       return true;
   return false;
}

size_t getCompressedImageSize(LoadedImage *image, DWORD formatCC, bool ddsHeader, bool baseTex, bool mipMaps)
{
	size_t size, block, blockBytes;
	int x, y;

	if (formatCC == FORMAT_BGRA)
	{
		block = 1;
		blockBytes = 4;
	}
	else
	{
		block = 4;
		blockBytes = (size_t)(64*getCompressionRatio(formatCC));
	}
	// header
	size = ddsHeader ? (sizeof(DWORD) + sizeof(DDSD2)) : 0;
	// base layer
	if (baseTex)
	{
		x = (int)ceil((float)image->width / block);
		y = (int)ceil((float)image->height / block);
		size += x*y*blockBytes;
	}
	// mipmaps
	if (mipMaps)
	{
		for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip)
		{
			x = (int)ceil((float)mipmap->width / block);
			y = (int)ceil((float)mipmap->height / block);
			size += x*y*blockBytes;
		}
	}
	return size;
}

byte *DDSHeader(byte *in, LoadedImage *image, DWORD formatCC)
{
	DDSD2 dds;
	
	memcpy(in, &DDS_HEADER, sizeof(DWORD));
	in += sizeof(DWORD);

	// write header
	memset(&dds, 0, sizeof(dds));
	dds.dwSize = sizeof(DDSD2);
	dds.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
	dds.dwWidth = image->width;
	dds.dwHeight = image->height;
	dds.dwMipMapCount = 1;
	for (MipMap *mipmap = image->mipMaps; mipmap; mipmap = mipmap->nextmip) dds.dwMipMapCount++;
	dds.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	dds.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	if (formatCC == FORMAT_BGRA)
	{
		dds.lPitch = dds.dwWidth * 4;
		dds.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds.ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds.ddpfPixelFormat.dwRGBBitCount = 32;
        dds.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
		dds.ddpfPixelFormat.dwRGBAlphaBitMask = 0xff000000;
	}
	else if (formatCC == FORMAT_YCG1 || formatCC == FORMAT_YCG2)
	{
		dds.ddpfPixelFormat.dwFourCC = FORMAT_DXT5;
		dds.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds.ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		dds.dwEmptyFaceColor = formatCC;
	}
	else
	{
		dds.ddpfPixelFormat.dwFourCC = formatCC;
		dds.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
		dds.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
		dds.ddpfPixelFormat.dwBBitMask = 0x000000ff;
		dds.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
	}

	// set DDS magic works (GIMP uses it as special info)
	if (opt_useDDSMagic)
	{
		dds.dwAlphaBitDepth = opt_ddsMagic1;
		dds.dwReserved = opt_ddsMagic2;
	}

	// fill alphapixels information, ensure that our texture have actual alpha channel
	// also texture may truncate it's alpha by forcing DXT1 compression no it
	if (image->hasAlpha)
		dds.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPIXELS;

	// fill alphapremult
	if (formatCC == FORMAT_DXT2 || formatCC == FORMAT_DXT4)
		dds.ddpfPixelFormat.dwFlags |= DDPF_ALPHAPREMULT;
		
	// is we writing swizzled DXT5 format
	if (IsDXT5SwizzledFormat(formatCC))
	{
		dds.ddpfPixelFormat.dwPrivateFormatBitCount = dds.ddpfPixelFormat.dwFourCC;
		dds.ddpfPixelFormat.dwFourCC = FORMAT_DXT5;
	}
	
	memcpy(in, &dds, sizeof(DDSD2));
	in += sizeof(DDSD2);
	return in;
}

/*
==========================================================================================

  DDS compression - Generic

==========================================================================================
*/

void DDS_PrintModules(void)
{
	Print(" NVidia DDS Utilities %s\n", GetDXTCVersion());
	//Print(" NVidia Texture Tools  %i.%i\n", (int)(nvtt::version() / 100), nvtt::version() - ((int)(nvtt::version()) / 100)*100);
	Print(" ATI Compress %i.%i\n", ATI_COMPRESS_VERSION_MAJOR, ATI_COMPRESS_VERSION_MINOR);
	Print(" GIMP DDS Plugin %i.%i.%i\n", DDS_PLUGIN_VERSION_MAJOR, DDS_PLUGIN_VERSION_MINOR, DDS_PLUGIN_VERSION_REVISION);
}

byte *GenerateDDS(FS_File *file, LoadedImage *image, size_t *outdatasize)
{
	bool isNormalmap = false;
	bool isHeightmap = false;
	bool forceBGRA = false;
	COLORSWIZZLE colorSwizzle;
	DWORD formatCC;
	TOOL tool;
	bool res;

	if (!image->bitmap)
		return NULL;

	// detect special texture types
	isNormalmap = FS_FileMatchList(file, image, opt_isNormal);
	isHeightmap = FS_FileMatchList(file, image, opt_isHeight);

	// select compression type
	colorSwizzle = IMAGE_COLORSWIZZLE_NONE;
	if (isHeightmap)
		formatCC = FORMAT_DXT1;
	if (isNormalmap && opt_normalmapRXGB)
		formatCC = FORMAT_RXGB;
	else if (image->hasAlpha)
		formatCC = image->hasGradientAlpha ? FORMAT_DXT5 : FORMAT_DXT1;
	else
		formatCC = FORMAT_DXT1;

	if (opt_forceAllFormat)
		formatCC = opt_forceAllFormat;
	else if (FS_FileMatchList(file, image, opt_forceDXT1))
		formatCC = FORMAT_DXT1;
	else if (FS_FileMatchList(file, image, opt_forceDXT2))
		formatCC = FORMAT_DXT2;
	else if (FS_FileMatchList(file, image, opt_forceDXT3))
		formatCC = FORMAT_DXT3;
	else if (FS_FileMatchList(file, image, opt_forceDXT4))
		formatCC = FORMAT_DXT4;
	else if (FS_FileMatchList(file, image, opt_forceDXT5))
		formatCC = FORMAT_DXT5;
	else if (FS_FileMatchList(file, image, opt_forceBGRA))
		formatCC = FORMAT_BGRA;
	else if (FS_FileMatchList(file, image, opt_forceRXGB))
		formatCC = FORMAT_RXGB;
	else if (FS_FileMatchList(file, image, opt_forceYCG1))
		formatCC = FORMAT_YCG1;
	else if (FS_FileMatchList(file, image, opt_forceYCG2))
		formatCC = FORMAT_YCG2;
	if (formatCC == FORMAT_DXT4 || formatCC == FORMAT_DXT2)
		colorSwizzle = IMAGE_COLORSWIZZLE_PREMODULATE;

	// check if selected DXT3/DXT5 and image have no alpha information
	if (!image->hasAlpha)
		if (formatCC == FORMAT_DXT3 || formatCC == FORMAT_DXT5)
			formatCC = FORMAT_DXT1;

	// check for swizzled format
	if (formatCC == FORMAT_RXGB)
		colorSwizzle = IMAGE_COLORSWIZZLE_XGBR;
	if (formatCC == FORMAT_YCG1 || formatCC == FORMAT_YCG2)
	{
		colorSwizzle = IMAGE_COLORSWIZZLE_YCOCG;
		forceBGRA = true;
	}

	// select compressor tool
	tool = opt_compressor;
	if (tool == COMPRESSOR_AUTOSELECT)
	{
		// hybrid mode, pick best
		// NVidia tool compresses normalmaps better than ATI
		// while ATI wins over general DXT1 and DXT5
		if (isNormalmap)
			tool = COMPRESSOR_NVIDIA;
		else
			tool = COMPRESSOR_ATI;
		// check if forcing
		if (FS_FileMatchList(file, image, opt_forceNvCompressor))
			tool = COMPRESSOR_NVIDIA;
		else if (FS_FileMatchList(file, image, opt_forceGimpDDSCompressor))
			tool = COMPRESSOR_GIMPDDS;
		else if (FS_FileMatchList(file, image, opt_forceATICompressor))
			tool = COMPRESSOR_ATI;
	}

	// specific formats only supported by a few compressors
	if (formatCC == FORMAT_BGRA)
		tool = COMPRESSOR_BGRA;
	if (formatCC == FORMAT_YCG1 || formatCC == FORMAT_YCG2)
		tool = COMPRESSOR_GIMPDDS;

	// for AI compressor we set color weiting (NVidia compressor has nice automatic weighting)
	if (tool == COMPRESSOR_ATI)
	{
		if (formatCC == FORMAT_RXGB)
		{
			image->useChannelWeighting = true;
			image->weightRed = 0.0;
			image->weightGreen = 0.75;
			image->weightBlue = 0.25;
		}
		else if (isNormalmap)
		{
			image->useChannelWeighting = true;
			image->weightRed = 0.5f;
			image->weightGreen = 0.5f;
			image->weightBlue = 0.0f;
		}
	}

	// postprocess
	Image_ConvertColorsForCompression(image, true, colorSwizzle, forceBGRA); // DDS compressors expects BGR
	if (FS_FileMatchList(file, image, opt_scale4x) || opt_forceScale4x)
	{
		Image_Scale2x(image, opt_scaler, true);
		Image_Scale2x(image, opt_scaler2, opt_allowNonPowerOfTwoDDS ? false : true);
	}
	else if (FS_FileMatchList(file, image, opt_scale2x) || opt_forceScale2x)
		Image_Scale2x(image, opt_scaler, opt_allowNonPowerOfTwoDDS ? false : true);
	if (!opt_allowNonPowerOfTwoDDS)
		Image_MakePowerOfTwo(image);
	if (formatCC == FORMAT_DXT1 && image->hasAlpha)
		Image_MakeAlphaBinary(image, 180);
	if (!opt_forceNoMipmaps && !FS_FileMatchList(file, image, opt_nomip))
		Image_GenerateMipmaps(image);

	// allocate memory for destination DDS
	image->formatCC = formatCC;
	size_t datasize = getCompressedImageSize(image, formatCC, true, true, true);
	byte  *data = (byte *)mem_alloc(datasize);
	memset(data, 0, datasize);
	byte  *in = DDSHeader(data, image, formatCC);

	// compress
	if (tool == COMPRESSOR_ATI)
		res = AtiCompress(in, file, image, formatCC);
	else if (tool == COMPRESSOR_NVIDIA)
		res = NvDXTlib(in, file, image, formatCC);
	else if (tool == COMPRESSOR_NVIDIA_TT)
		res = NvTTlib(in, file, image, formatCC);
	else if (tool == COMPRESSOR_BGRA)
		res = BGRA(in, file, image, formatCC);
	else if (tool == COMPRESSOR_GIMPDDS)
		res = GimpDDS(in, file, image, formatCC);
	else
		Error("bad compression tool!\n");

	*outdatasize = datasize;
	return data;
}

typedef struct writeDDS_s
{
	char        outfile[MAX_FPATH];
	byte       *data;
	size_t      datasize;
	writeDDS_s *next;
} writeDDS_t;

typedef struct
{
	// stats
	size_t     num_dds_files;
	double     size_dds_headerdata;
	double     size_dds_texturedata;
	double     size_dds_mipdata;
	size_t     num_original_files;
	double     size_original_textures;
	double     size_original_files;

	// zip file in memory
	void *zip_data;
	size_t zip_len;
	size_t zip_maxlen;
	bool zip_warning;

	// write chain
	writeDDS_t *writeDDS;
	int num_write_files;
	int num_writen_files;
} GenerateDDS_t;


void GenerateDDS_Thread(ThreadData *thread)
{
	LoadedImage *image, *frame;
	GenerateDDS_t *SharedData;
	writeDDS_t *WriteData;
	FS_File *file;
	byte *data;
	size_t datasize;
	int work;

	SharedData = (GenerateDDS_t *)thread->data;

	image = Image_Create();
	while(1)
	{
		work = GetWorkForThread(thread);
		if (work == -1)
			break;

		file = &textures[work];

		// load image
		Image_Load(file, image);
		if (!image->bitmap)
			continue;

		// postprocess and export all frames
		double dds_texsize  = 0;
		double dds_mipsize  = 0;
		double dds_headsize = 0;
		double org_filesize = 0;
		double org_texsize  = 0;
		size_t numdds    = 0;
		for (frame = image; frame != NULL; frame = frame->next)
		{
			data = GenerateDDS(file, frame, &datasize);

			// save for saving thread
			WriteData = (writeDDS_t *)mem_alloc(sizeof(writeDDS_t));
			memset(WriteData, 0, sizeof(writeDDS_t));
			sprintf(WriteData->outfile, "%s%s.dds", file->path.c_str(), frame->useTexname ? frame->texname : file->name.c_str());
			WriteData->data = data;
			WriteData->datasize = datasize;
			WriteData->next = SharedData->writeDDS;
			SharedData->writeDDS = WriteData;
			SharedData->num_write_files++;
			
			// stats
			dds_headsize += (double)getCompressedImageSize(frame, frame->formatCC,  true, false, false) / (1024.0f * 1024.0f);
			dds_texsize  += (double)getCompressedImageSize(frame, frame->formatCC, false,  true, false) / (1024.0f * 1024.0f);
			dds_mipsize  += (double)getCompressedImageSize(frame, frame->formatCC, false, false,  true) / (1024.0f * 1024.0f); 
			org_filesize += (double)frame->filesize / (1024.0f * 1024.0f);
			org_texsize  += (double)((frame->width*frame->height*frame->bpp)/frame->scale) / (1024.0f * 1024.0f);
			numdds++;
		}
		
		// add to global stats
		SharedData->num_original_files++;
		SharedData->size_original_files    += org_filesize;
		SharedData->size_original_textures += org_texsize;
		SharedData->num_dds_files          += numdds;
		SharedData->size_dds_headerdata    += dds_headsize;
		SharedData->size_dds_texturedata   += dds_texsize;
		SharedData->size_dds_mipdata       += dds_mipsize;

		// delete image
		Image_Unload(image);
	}
	Image_Delete(image);
}

void GenerateDDS_MainThread(ThreadData *thread)
{
	HZIP outzip = NULL;
	GenerateDDS_t *SharedData;
	writeDDS_t *WriteData, *PrevData;
	void *zipdata;
	unsigned long zipdatalen;
	char outfile[MAX_FPATH];

	SharedData = (GenerateDDS_t *)thread->data;

	// check if dest path is an archive
	if (!FS_FileMatchList(opt_destPath, opt_archiveFiles))
	{
		Print("Generating to \"%s\"\n", opt_destPath);
		AddSlash(opt_destPath);
	}
	else
	{
		if (opt_zipMemory <= 0)
			outzip = CreateZip(opt_destPath, "");
		else
		{
			SharedData->zip_maxlen = 1024 * 1024 * opt_zipMemory;
			SharedData->zip_data = mem_alloc(SharedData->zip_maxlen);
			outzip = CreateZip(SharedData->zip_data, SharedData->zip_maxlen, "");
		}
		if (!outzip)
		{
			thread->pool->stop = true;
			Print("Failed to create output archive file %s\n", opt_destPath);
			return;
		}
		Print("Generating to \"%s\" (ZIP archive)\n", opt_destPath);
		if (opt_zipMemory > 0)
			Print("Generating ZIP in memory (max size %i MBytes)\n", opt_zipMemory);
		if (opt_archivePath.c_str()[0])
			Print("Archive inner path \"%s\"\n", opt_archivePath.c_str());
	}
	thread->pool->started = true;

	// write files
	while(1)
	{
		// print pacifier
		if (!noprint)
			Pacifier(" file %i of %i", SharedData->num_original_files, thread->pool->work_num /*, SharedData->num_write_files, SharedData->num_writen_files*/);
		else
		{
			int p = (int)(((float)(SharedData->num_original_files) / (float)thread->pool->work_num)*100);
			PercentPacifier("%i", p);
		}

		// check if files to write, thats because they havent been added it, or we are finished
		if (!SharedData->writeDDS)
		{
			if (thread->pool->finished == true)
				break;
			// todo: do some help?
			Sleep(1);
		}
		else
		{
			// get a last file from chain
			PrevData = NULL;
			for (WriteData = SharedData->writeDDS; WriteData->next; WriteData = WriteData->next)
				PrevData = WriteData;
			if (!PrevData)
			{
				WriteData = SharedData->writeDDS;
				SharedData->writeDDS = NULL;
			}
			else
			{
				WriteData = PrevData->next;
				PrevData->next = NULL;
			}
			SharedData->num_write_files--;
			SharedData->num_writen_files++;

			// write
			if (outzip)
			{
				sprintf(outfile, "%s%s", opt_archivePath.c_str(), WriteData->outfile);
				zipdatalen = ZipGetMemoryWritten(outzip);
				if (!SharedData->zip_warning && (float)((float)zipdatalen / (float)SharedData->zip_maxlen) > 0.9)
				{
					Warning("GenerateDDS: there is less than 10%% of free ZIP memory, program may crash!\n");
					SharedData->zip_warning = true;
				}
				ZRESULT zr = ZipAdd(outzip, outfile, WriteData->data, WriteData->datasize);
				if (zr != ZR_OK)
				{
					if (zr == ZR_MEMSIZE)
						Error("GenerateDDS(%s): failed to pack DDS into ZIP - out of memory (consider increasing -zipmem). Process stopped. ", outfile);
					else if (zr == ZR_FAILED)
						Error("GenerateDDS(%s): failed to pack DDS into ZIP - previous file failed. Process stopped. ", outfile);
					else
						Warning("GenerateDDS(%s): failed to pack DDS into ZIP - error code 0x%08X", outfile, zr);
				}
			}
			else
			{
				sprintf(outfile, "%s%s", opt_destPath, WriteData->outfile);
				// write file
				CreatePath(outfile);
				FILE *f = fopen(outfile, "wb");
				if (!f)
					Warning("GenerateDDS(%s): cannot open file (%s) for writing", outfile, strerror(errno));
				else
				{
					if (!fwrite(WriteData->data, WriteData->datasize, 1, f))
						Warning("GenerateDDS(%s): cannot write file (%s)", outfile, strerror(errno));
					fclose(f);
				}
			}
			mem_free(WriteData->data);
			mem_free(WriteData);
		}
	}

	// close zip
	if (outzip)
	{
		if (SharedData->zip_data)
		{
			ZRESULT zr = ZipGetMemory(outzip, &zipdata, &zipdatalen);
			if (zr != ZR_OK)
				Warning("GenerateDDS(%s): ZipGetMemory failed - error code 0x%08X", outfile, zr);
			else
			{
				FILE *f = fopen(opt_destPath, "wb");
				if (!f)
					Warning("GenerateDDS(%s): cannot open ZIP file (%s) for writing", opt_destPath, strerror(errno));
				else
				{
					if (!fwrite(zipdata, zipdatalen, 1, f))
						Warning("GenerateDDS(%s): cannot write ZIP file (%s)", opt_destPath, strerror(errno));
					fclose(f);
				}
				mem_free(SharedData->zip_data);
			}
		}
		CloseZip(outzip);
	}
}

void DDS_Help(void)
{
	waitforkey = true;
	Print(
	"usage: rwgtex -dds [input path] [output path] [options]\n"
	"\n"
	"input path could be:\n"
	"  - folder\n"
	"  - a single file or file mask\n"
	"  - archive file (extension should be listed in option file)\n"
	"\n"
	"output path:\n"
	"  - folder\n"
	"  - archive (will be created from scratch)\n"
	"\n"
	"options:\n"
	"        -ap: additional archive path\n"
	"   -nocache: disable file caching\n"
	"      -npot: allow non-power-of-two textures\n"
	"     -nomip: dont generate mipmaps\n"
	"        -nv: force NVidia DDS Utilities compressor\n"
	"       -ati: force ATI compressor\n"
	"      -gimp: force Gimp DDS Plugin compressor\n"
	"      -dxt1: force DXT1 compression\n"
	"      -dxt2: force DXT2 compression\n"
	"      -dxt3: force DXT3 compression\n"
	"      -dxt4: force DXT4 compression\n"
	"      -dxt5: force DXT5 compression\n"
	"      -bgra: force BGRA format (no compression)\n"
	"        -2x: apply 2x scale (Scale2X)\n"
	"        -4x: apply 4x scale (2 pass scale)\n"
	"  -scaler X: set a filter to be used for scaling\n"
	" -scaler2 X: set a filter to be used for second scale pass\n"
	"  -zipmem X: keep zip file in memory until end\n"
	"             this avoids many file writes\n"
	"\n"
	"scalers:\n"
	"        box: nearest\n"
	"   bilinear: bilinear filter\n"
	"    bicubic: Mitchell & Netravali's two-param cubic filter\n"
	"    bspline: th order (cubic) b-spline\n"
	" catmullrom: Catmull-Rom spline, Overhauser spline\n"
	"    lanczos: Lanczos3 filter\n"
	"    scale2x: Scale2x\n"
	"    super2x: Scale4x with backscale to 2x (default)\n"
	"\n");
}

int DDS_Main(int argc, char **argv)
{
	double timeelapsed;
	char cachefile[MAX_FPATH], comment[9];
	int i;

	// launched without parms, try to find kain.exe
	i = 0;
	strcpy(opt_srcDir, "");
	strcpy(opt_srcFile, "");
	strcpy(opt_destPath, "");
	if (argc < 1)
	{
		// launched without parms, try to find engine
		char find[MAX_FPATH];
		bool found_dir = false;
		if (opt_basedir.c_str()[0])
		{
			for (int i = 0; i < 10; i++)
			{
				strcpy(find, "../");
				for (int j = 0; j < i; j++)
					strcat(find, "../");
				strcat(find, opt_basedir.c_str());

				{
					found_dir = true;
					break;
				}
			}
		}
		if (found_dir)
		{
			Print("Base and output directory detected\n");
			sprintf(opt_srcDir, "%s/", find);
			sprintf(opt_destPath, "%s/%s/", find, opt_ddsdir.c_str());
		}
		else
		{
			DDS_Help();
			Error("no commands specified", progname);
		}
	}
	else if (argc < 2)
	{
		// dragged file to exe, there is no output path
		strncpy(opt_srcDir, argv[0], sizeof(opt_srcDir));
		opt_useFileCache = false;
		if (FS_FindDir(opt_srcDir)) 
		{
			// dragged a directory
			AddSlash(opt_srcDir);
			sprintf(opt_destPath, "%s%s", opt_srcDir, opt_ddsdir.c_str());
		}
		else
		{
			// dragged a file
			ExtractFileName(opt_srcDir, opt_srcFile);
			ExtractFilePath(opt_srcDir, opt_destPath);
			strncpy(opt_srcDir, opt_destPath, sizeof(opt_srcDir));
			// if file is archive, add "dds/" folder
			if (FS_FileMatchList(opt_srcFile, opt_archiveFiles))
				strcat(opt_destPath, opt_ddsdir.c_str());
		}
	}
	else
	{
		// commandline launch
		strncpy(opt_srcDir, argv[0], sizeof(opt_srcDir));

		// optional output directory
		if (argc > 1 && strncmp(argv[1], "-", 1))
			strncpy(opt_destPath, argv[1], sizeof(opt_destPath));

		// check if input is folder
		// set default output directory
		if (FS_FindDir(opt_srcDir))
			AddSlash(opt_srcDir);
		else
		{
			ExtractFileName(opt_srcDir, opt_srcFile);
			ExtractFilePath(opt_srcDir, opt_srcDir);
		}

		// default output directory
		if (!strcmp(opt_destPath, ""))
		{
			AddSlash(opt_srcDir);
			sprintf(opt_destPath, "%s%s", opt_srcDir, opt_ddsdir.c_str());
		}
	}
	AddSlash(opt_srcDir);

	// print options
	if (opt_compressor == COMPRESSOR_NVIDIA)
		Print("Using NVidia DDS Utilities compressor\n");
	else if (opt_compressor == COMPRESSOR_NVIDIA_TT)
		Print("Using NVidia Texture Tools compressor\n");
	else if (opt_compressor == COMPRESSOR_ATI)
		Print("Using ATI compressor\n");
	else if (opt_compressor == COMPRESSOR_GIMPDDS)
		Print("Using GIMP DDS Plugin compressor\n");
	else
		Print("Using hybrid (autoselect) compressor\n");
	if (opt_forceAllFormat)
	{
		if (opt_forceAllFormat == FORMAT_DXT1)
			Print("Forcing DDS format to DXT1\n");
		else if (opt_forceAllFormat == FORMAT_DXT3)
			Print("Forcing DDS format to DXT3\n");
		else if (opt_forceAllFormat == FORMAT_DXT5)
			Print("Forcing DDS format to DXT5\n");
		else if (opt_forceAllFormat == FORMAT_BGRA)
			Print("Forcing DDS format to BGRA\n");
		else if (opt_forceAllFormat == FORMAT_RXGB)
			Print("Forcing DDS format to RXGB (Doom 3 DXT5 swizzled normalmap format)\n");
		else if (opt_forceAllFormat == FORMAT_YCG1)
			Print("Forcing DDS format to YCoCg Unscaled\n");
		else if (opt_forceAllFormat == FORMAT_YCG2)
			Print("Forcing DDS format to YCoCg Scaled\n");
	}	
	if (opt_forceAllNormalmap)
		Print("Forcing normalmap compression tricks\n");
	if (opt_forceScale2x || opt_forceScale4x)
	{
		if (opt_forceScale2x)
			Print("Upscale images to 200%% ");
		else
			Print("Upscale images to 400%% ");
		if (opt_scaler == IMAGE_SCALER_BOX)
			Print("using box filter\n");
		else if (opt_scaler == IMAGE_SCALER_BILINEAR)
			Print("using bilinear filter\n");
		else if (opt_scaler == IMAGE_SCALER_BICUBIC)
			Print("using cubic Mitchell filter\n");
		else if (opt_scaler == IMAGE_SCALER_BSPLINE)
			Print("using cubic b-spline\n");
		else if (opt_scaler == IMAGE_SCALER_CATMULLROM)
			Print("using Catmull-Rom spline\n");
		else if (opt_scaler == IMAGE_SCALER_LANCZOS)
			Print("using Lanczos filter\n");
		else if (opt_scaler == IMAGE_SCALER_SCALE2X)
			Print("using Scale2x filter\n");
		else
			Print("using Super2x filter\n");
		if (opt_forceScale4x && (opt_scaler2 != opt_scaler))
		{
			if (opt_scaler2 == IMAGE_SCALER_BOX)
				Print("using box filter for second scaler\n");
			else if (opt_scaler2 == IMAGE_SCALER_BILINEAR)
				Print("using bilinear filter for second scaler\n");
			else if (opt_scaler2 == IMAGE_SCALER_BICUBIC)
				Print("using cubic Mitchell filter for second scaler\n");
			else if (opt_scaler2 == IMAGE_SCALER_BSPLINE)
				Print("using cubic b-spline for second scaler\n");
			else if (opt_scaler2 == IMAGE_SCALER_CATMULLROM)
				Print("using Catmull-Rom spline for second scaler\n");
			else if (opt_scaler2 == IMAGE_SCALER_LANCZOS)
				Print("using Lanczos filter for second scaler\n");
			else if (opt_scaler2 == IMAGE_SCALER_SCALE2X)
				Print("using Scale2x filter for second scaler\n");
			else
				Print("using Super2x filter for second scaler\n");
		}
	}
	if (opt_allowNonPowerOfTwoDDS)
		Print("Allow non-power-of-two texture dimensions\n");
	if (opt_forceNoMipmaps)
		 Print("Not generating mipmaps\n");
	if (opt_useDDSMagic)
	{
		memset(comment, 0, sizeof(comment));
		memcpy(comment, &opt_ddsMagic1, 4);
		memcpy(comment + 4, &opt_ddsMagic2, 4);
		Print("Inserting signword '%s' to DDS file\n", comment);
	}

	// load cache
	if (opt_useFileCache)
	{
		// check if dest path is an archive
		if (FS_FileMatchList(opt_destPath, opt_archiveFiles))
		{
			opt_useFileCache = false;
			Print("Archive output does not support file cache at the moment\n");
		}
		else
		{
			Print("Converting only files that was changed\n");
			sprintf(cachefile, "%s_filescrc.txt", opt_destPath);
			FS_LoadCache(cachefile);
		}
	}

	// find files
	Print("Entering \"%s%s\"\n", opt_srcDir, opt_srcFile);
	if (opt_useFileCache)
		Print("Calculating crc32 for files\n");
	textures.clear();
	texturesSkipped = 0;
	FS_ScanPath(opt_srcDir, opt_srcFile, NULL);
	if (texturesSkipped)
		Print("Skipping %i unchanged files\n", texturesSkipped);
	if (!textures.size())
	{
		Print("No files to convert\n");
		return 0;
	}

	// decompress DDS
	if (opt_srcFile[0] && textures.size() == 1 && !strnicmp(textures[0].suf.c_str(), "dds", 3))
	{
		Print("Decompressing DDS\n");
		Print("Not implemented\n");
		return 0;
	}

	// run DDS conversion
	GenerateDDS_t SharedData;
	memset(&SharedData, 0, sizeof(GenerateDDS_t));
	timeelapsed = ParallelThreads(numthreads, textures.size(), &SharedData, GenerateDDS_Thread, GenerateDDS_MainThread);

	// save cache file
	if (opt_useFileCache)
		FS_SaveCache(cachefile);

	// show stats
	double outputsize = SharedData.size_dds_headerdata + SharedData.size_dds_mipdata + SharedData.size_dds_texturedata;
	Print("Conversion finished!\n");
	Print("--------\n");
	Print("  files exported: %i\n", SharedData.num_dds_files);
	Print("    time elapsed: %i:%02.0f\n", (int)(timeelapsed / 60), (timeelapsed - ((int)(timeelapsed / 60)*60)));
	Print("     input files: %.2f mb\n", SharedData.size_original_files);
	Print("  input textures: %.2f mb\n", SharedData.size_original_textures);
	Print("       DDS files: %.2f mb\n", SharedData.size_dds_headerdata + SharedData.size_dds_mipdata + SharedData.size_dds_texturedata);
	Print("     inc mipmaps: %.2f mb\n", SharedData.size_dds_mipdata);
	Print("   space economy: %.0f%%\n", (1.0 - (outputsize / SharedData.size_original_files)) * 100.0f);
	Print("    VRAM economy: %.0f%%\n", (1.0 - (SharedData.size_dds_texturedata / SharedData.size_original_textures)) * 100.0f);
	return 0;
}