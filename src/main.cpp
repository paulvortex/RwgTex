////////////////////////////////////////////////////////////////
//
// RWGTEX - main
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

#include "main.h"
#include "dds.h"

bool waitforkey;
bool memstats;
bool errorlog;
int  numthreads = 0;
char progname[MAX_FPATH];
char progpath[MAX_FPATH];

FCLIST opt_include;
FCLIST opt_nomip;
FCLIST opt_forceDXT1;
FCLIST opt_forceDXT2;
FCLIST opt_forceDXT3;
FCLIST opt_forceDXT4;
FCLIST opt_forceDXT5;
FCLIST opt_forceBGRA;
FCLIST opt_forceRXGB;
FCLIST opt_forceYCG1;
FCLIST opt_forceYCG2;
FCLIST opt_forceNvCompressor;
FCLIST opt_forceATICompressor;
FCLIST opt_forceGimpDDSCompressor;
FCLIST opt_isNormal;
FCLIST opt_isHeight;
string opt_basedir;
string opt_ddsdir;
bool   opt_detectBinaryAlpha;
byte   opt_binaryAlphaMin;
byte   opt_binaryAlphaMax;
byte   opt_binaryAlphaCenter;
FCLIST opt_archiveFiles;
string opt_archivePath;
FCLIST opt_scale2x;
FCLIST opt_scale4x;
bool   opt_useFileCache;
bool   opt_forceNoMipmaps;
bool   opt_allowNonPowerOfTwoDDS;
bool   opt_forceScale2x;
bool   opt_forceScale4x;
bool   opt_normalmapRXGB;
SCALER opt_scaler;
SCALER opt_scaler2;
int    opt_zipMemory;
DWORD  opt_forceAllFormat;
bool   opt_forceAllNormalmap;
TOOL   opt_compressor;
bool   opt_useDDSMagic;
DWORD  opt_ddsMagic1;
DWORD  opt_ddsMagic2;
DWORD  opt_ddsVersion;

bool OptionBoolean(char *val)
{
	if (!stricmp(val, "true"))
		return true;
	if (!stricmp(val, "enabled"))
		return true;
	if (!stricmp(val, "enable"))
		return true;
	if (!stricmp(val, "on"))
		return true;
	if (!stricmp(val, "yes"))
		return true;
	if (!stricmp(val, "1"))
		return true;
	return false;
}

void LoadOptions(char *filename)
{
	FILE *f;
	char line[1024], group[64], key[64],  *val;
	int linenum, l;

	// set default values
	opt_detectBinaryAlpha = false;
	opt_binaryAlphaMin = 0;
	opt_binaryAlphaMax = 255;
	opt_binaryAlphaCenter = 180;
	opt_include.clear();
	opt_nomip.clear();
	opt_forceDXT1.clear();
	opt_forceDXT2.clear();
	opt_forceDXT3.clear();
	opt_forceDXT4.clear();
	opt_forceDXT5.clear();
	opt_forceBGRA.clear();
	opt_forceRXGB.clear();
	opt_forceYCG1.clear();
	opt_forceYCG2.clear();
	opt_forceNvCompressor.clear();
	opt_forceATICompressor.clear();
	opt_forceGimpDDSCompressor.clear();
	opt_isNormal.clear();
	opt_isHeight.clear();
	opt_basedir = "id1";
	opt_ddsdir = "dds";
	opt_archiveFiles.clear();
	opt_archivePath = "";
	opt_scale2x.clear();
	opt_scale4x.clear();
	opt_useFileCache = true;
	opt_forceNoMipmaps = false;
	opt_allowNonPowerOfTwoDDS = false;
	opt_forceScale2x = false;
	opt_forceScale4x = false;
	opt_normalmapRXGB = false;
	opt_useDDSMagic = true;
	opt_ddsMagic1 = MAKEFOURCC('R', 'W', 'G', 'T');
	opt_ddsMagic2 = MAKEFOURCC('E', 'X', 0, 0);
	opt_ddsVersion = 0;
	opt_forceAllFormat = 0;
	opt_forceAllNormalmap = false;
	opt_compressor = COMPRESSOR_AUTOSELECT;
	opt_scaler = IMAGE_SCALER_SUPER2X;
	opt_scaler2 = opt_scaler2;
	opt_zipMemory = 0;
	
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
				opt_basedir = val;
			else if (!stricmp(key, "ddspath"))
				opt_ddsdir = val;
			else if (!stricmp(key, "binaryalpha"))
				opt_detectBinaryAlpha = OptionBoolean(val);
			else if (!stricmp(key, "filecache"))
				opt_useFileCache = OptionBoolean(val);
			else if (!stricmp(key, "archivepath"))
				opt_archivePath = val;
			else if (!stricmp(key, "compressor"))
			{
				if (!stricmp(val, "ati"))
					opt_compressor = COMPRESSOR_ATI;
				else if (!stricmp(val, "nv"))
					opt_compressor = COMPRESSOR_NVIDIA; 
				else if (!stricmp(val, "nvtt"))
					opt_compressor = COMPRESSOR_NVIDIA_TT; 
				else if (!stricmp(val, "hybrid"))
					opt_compressor = COMPRESSOR_AUTOSELECT; 
			}
			else if (!stricmp(key, "nonpoweroftwotextures"))
				opt_allowNonPowerOfTwoDDS = OptionBoolean(val);
			else if (!stricmp(key, "generatemipmaps"))
				opt_forceNoMipmaps = OptionBoolean(val) ? false : true;
			else if (!stricmp(key, "waitforkey"))
				waitforkey = OptionBoolean(val);
			else if (!stricmp(key, "binaryalpha_0"))
				opt_binaryAlphaMin = (byte)(min(max(0, atoi(val)), 255));
			else if (!stricmp(key, "binaryalpha_1"))
				opt_binaryAlphaMax = (byte)(min(max(0, atoi(val)), 255));
			else if (!stricmp(key, "binaryalpha_center"))
				opt_binaryAlphaCenter = (byte)(min(max(0, atoi(val)), 255));
			else if (!stricmp(key, "normalmapcompression"))
			{
				if (!stricmp(val, "default"))
					opt_normalmapRXGB = false;
				else if (!stricmp(val, "rxgb"))
					opt_normalmapRXGB = true;
			}
			else if (!stricmp(key, "scaler"))
			{
				if (!stricmp(val, "nearest"))
					opt_scaler = IMAGE_SCALER_BOX;
				else if (!stricmp(val, "bilinear"))
					opt_scaler = IMAGE_SCALER_BILINEAR;
				else if (!stricmp(val, "bicubic"))
					opt_scaler = IMAGE_SCALER_BICUBIC;
				else if (!stricmp(val, "bspline"))
					opt_scaler = IMAGE_SCALER_BSPLINE;
				else if (!stricmp(val, "catmullrom"))
					opt_scaler = IMAGE_SCALER_CATMULLROM;
				else if (!stricmp(val, "lanczos"))
					opt_scaler = IMAGE_SCALER_LANCZOS;
				else if (!stricmp(val, "scale2x"))
					opt_scaler = IMAGE_SCALER_SCALE2X;
				else if (!stricmp(val, "super2x"))
					opt_scaler = IMAGE_SCALER_SUPER2X;
				opt_scaler2 = opt_scaler;
			}
			else if (!stricmp(key, "scaler2"))
			{
				if (!stricmp(val, "nearest"))
					opt_scaler2 = IMAGE_SCALER_BOX;
				else if (!stricmp(val, "bilinear"))
					opt_scaler2 = IMAGE_SCALER_BILINEAR;
				else if (!stricmp(val, "bicubic"))
					opt_scaler2 = IMAGE_SCALER_BICUBIC;
				else if (!stricmp(val, "bspline"))
					opt_scaler2 = IMAGE_SCALER_BSPLINE;
				else if (!stricmp(val, "catmullrom"))
					opt_scaler2 = IMAGE_SCALER_CATMULLROM;
				else if (!stricmp(val, "lanczos"))
					opt_scaler2 = IMAGE_SCALER_LANCZOS;
				else if (!stricmp(val, "scale2x"))
					opt_scaler2 = IMAGE_SCALER_SCALE2X;
				else if (!stricmp(val, "super2x"))
					opt_scaler2 = IMAGE_SCALER_SUPER2X;
			}
			else if (!stricmp(key, "sign"))
				opt_useDDSMagic = OptionBoolean(val);
			else if (!stricmp(key, "signword"))
			{
				opt_ddsMagic1 = MAKEFOURCC( strlen(val) < 1 ? 0 : val[0], strlen(val) < 2 ? 0 : val[1], strlen(val) < 3 ? 0 : val[2], strlen(val) < 4 ? 0 : val[3] );
				opt_ddsMagic2 = MAKEFOURCC( strlen(val) < 5 ? 0 : val[4], strlen(val) < 6 ? 0 : val[5], strlen(val) < 7 ? 0 : val[6], strlen(val) < 8 ? 0 : val[7] );
			}
			else if (!stricmp(key, "signversion"))
				opt_ddsVersion = MAKEFOURCC( strlen(val) < 1 ? 0 : val[0], strlen(val) < 2 ? 0 : val[1], strlen(val) < 3 ? 0 : val[2], strlen(val) < 4 ? 0 : val[3] );
			else
				Warning("%s:%i: unknown key '%s'", filename, linenum, key);
			continue;
		}
		if (!stricmp(group, "input")      || !stricmp(group, "nomip")      || !stricmp(group, "is_normalmap") || !stricmp(group, "is_heightmap") ||
			!stricmp(group, "force_dxt1") || !stricmp(group, "force_dxt2") || !stricmp(group, "force_dxt3")   || !stricmp(group, "force_dxt4")   || !stricmp(group, "force_dxt5") || !stricmp(group, "force_bgra") ||
			!stricmp(group, "force_rxgb") || !stricmp(group, "force_ycg1") || !stricmp(group, "force_ycg2")   ||    
			!stricmp(group, "force_nv")   || !stricmp(group, "force_ati")  || !stricmp(group, "force_gimp")   || !stricmp(group, "archives") ||    
		    !stricmp(group, "scale")      || !stricmp(group, "scale_2x")   || !stricmp(group, "scale_4x"))
		{
			CompareOption O;
			if (stricmp(key, "path") && stricmp(key, "suffix") && stricmp(key, "ext") && stricmp(key, "name") && stricmp(key, "match") && stricmp(key, "bpp") && stricmp(key, "alpha") &&
				stricmp(key, "path!") && stricmp(key, "suffix!") && stricmp(key, "ext!") && stricmp(key, "name!") && stricmp(key, "match!") && stricmp(key, "bpp!") && stricmp(key, "alpha!"))
				Warning("%s:%i: unknown key '%s'", filename, linenum, key);
			else
			{
				O.parm = key;
				O.pattern = val;
				     if (!stricmp(group, "input"))        opt_include.push_back(O);
				else if (!stricmp(group, "nomip"))        opt_nomip.push_back(O);
				else if (!stricmp(group, "is_normalmap")) opt_isNormal.push_back(O);
				else if (!stricmp(group, "is_heightmap")) opt_isHeight.push_back(O);
				else if (!stricmp(group, "force_dxt1"))   opt_forceDXT1.push_back(O);
				else if (!stricmp(group, "force_dxt2"))   opt_forceDXT2.push_back(O);
				else if (!stricmp(group, "force_dxt3"))   opt_forceDXT3.push_back(O);
				else if (!stricmp(group, "force_dxt4"))   opt_forceDXT4.push_back(O);
				else if (!stricmp(group, "force_dxt5"))   opt_forceDXT5.push_back(O);
				else if (!stricmp(group, "force_bgra"))   opt_forceBGRA.push_back(O);
				else if (!stricmp(group, "force_rxgb"))   opt_forceRXGB.push_back(O);
				else if (!stricmp(group, "force_ycg1"))   opt_forceYCG1.push_back(O);
				else if (!stricmp(group, "force_ycg2"))   opt_forceYCG2.push_back(O);
				else if (!stricmp(group, "force_nv"))     opt_forceNvCompressor.push_back(O);
				else if (!stricmp(group, "force_ati"))    opt_forceATICompressor.push_back(O);
				else if (!stricmp(group, "force_gimp"))   opt_forceGimpDDSCompressor.push_back(O);
				else if (!stricmp(group, "archives"))     opt_archiveFiles.push_back(O);
				else if (!stricmp(group, "scale") || !stricmp(group, "scale_2x")) opt_scale2x.push_back(O);
				else if (!stricmp(group, "scale_4x"))     opt_scale2x.push_back(O);
			}
			continue;
		}
		Warning("%s:%i: unknown group '%s'", filename, linenum, group);
	}
	fclose(f);

	// override with commandline
	if (CheckParm("-nocache"))    opt_useFileCache = false;
	if (CheckParm("-nv"))         opt_compressor = COMPRESSOR_NVIDIA;
	if (CheckParm("-nvtt"))       opt_compressor = COMPRESSOR_NVIDIA_TT;
	if (CheckParm("-ati"))        opt_compressor = COMPRESSOR_ATI;
	if (CheckParm("-gimp"))       opt_compressor = COMPRESSOR_GIMPDDS;
	if (CheckParm("-dxt1"))       opt_forceAllFormat = FORMAT_DXT1;
	if (CheckParm("-dxt2"))       opt_forceAllFormat = FORMAT_DXT2;
	if (CheckParm("-dxt3"))       opt_forceAllFormat = FORMAT_DXT3;
	if (CheckParm("-dxt4"))       opt_forceAllFormat = FORMAT_DXT4;
	if (CheckParm("-dxt5"))       opt_forceAllFormat = FORMAT_DXT5;
	if (CheckParm("-bgra"))       opt_forceAllFormat = FORMAT_BGRA;
	if (CheckParm("-rxgb"))       opt_forceAllFormat = FORMAT_RXGB;
	if (CheckParm("-ycg1"))       opt_forceAllFormat = FORMAT_YCG1;
	if (CheckParm("-ycg2"))       opt_forceAllFormat = FORMAT_YCG2;
	if (CheckParm("-nm"))         opt_forceAllNormalmap = true;	
	if (CheckParm("-2x"))         opt_forceScale2x = true; 
	if (CheckParm("-4x"))         opt_forceScale4x = true; 
	if (CheckParm("-npot"))       opt_allowNonPowerOfTwoDDS = true;
	if (CheckParm("-nomip"))      opt_forceNoMipmaps = true;
	if (CheckParm("-nearest"))    opt_scaler = opt_scaler2 = IMAGE_SCALER_BOX;
	if (CheckParm("-bilinear"))   opt_scaler = opt_scaler2 = IMAGE_SCALER_BILINEAR;
	if (CheckParm("-bicubic"))    opt_scaler = opt_scaler2 = IMAGE_SCALER_BICUBIC;
	if (CheckParm("-bspline"))    opt_scaler = opt_scaler2 = IMAGE_SCALER_BSPLINE;
	if (CheckParm("-catmullrom")) opt_scaler = opt_scaler2 = IMAGE_SCALER_CATMULLROM;
	if (CheckParm("-lanczos"))    opt_scaler = opt_scaler2 = IMAGE_SCALER_LANCZOS;
	if (CheckParm("-scale2x"))    opt_scaler = opt_scaler2 = IMAGE_SCALER_SCALE2X;				
	if (CheckParm("-super2x"))    opt_scaler = opt_scaler2 = IMAGE_SCALER_SUPER2X;	
	if (CheckParm("-nearest2"))   opt_scaler2 = IMAGE_SCALER_BOX;
	if (CheckParm("-2bilinear"))  opt_scaler2 = IMAGE_SCALER_BILINEAR;
	if (CheckParm("-2bicubic"))   opt_scaler2 = IMAGE_SCALER_BICUBIC;
	if (CheckParm("-2bspline"))   opt_scaler2 = IMAGE_SCALER_BSPLINE;
	if (CheckParm("-2catmullrom"))opt_scaler2 = IMAGE_SCALER_CATMULLROM;
	if (CheckParm("-2lanczos"))   opt_scaler2 = IMAGE_SCALER_LANCZOS;
	if (CheckParm("-2scale2x"))   opt_scaler2 = IMAGE_SCALER_SCALE2X;				
	if (CheckParm("-2super2x"))   opt_scaler2 = IMAGE_SCALER_SUPER2X;	
	if (CheckParm("-nosign"))     opt_useDDSMagic = false;
	if (CheckParm("-gimpsign")) { opt_useDDSMagic = true; opt_ddsMagic1 = MAKEFOURCC('G', 'I', 'M', 'P'); opt_ddsMagic2 = MAKEFOURCC('-', 'D', 'D', 'S');  opt_ddsVersion = 131585; }
	// string parameters
	for (int i = 1; i < myargc; i++) 
	{
		if (!stricmp(myargv[i], "-ap"))
		{
			i++;
			if (i < myargc)
			{
				opt_archivePath = myargv[i];
				AddSlash(opt_archivePath);
			}
			continue;
		}
		if (!stricmp(myargv[i], "-zipmem"))
		{
			i++;
			if (i < myargc)
				opt_zipMemory = atoi(myargv[i]);
			continue;
		}
		if (!stricmp(myargv[i], "-scaler"))
		{
			i++;
			if (i < myargc)
			{
				    if (!stricmp(myargv[i], "nearest"))
					opt_scaler = IMAGE_SCALER_BOX;
				else if (!stricmp(myargv[i], "bilinear"))
					opt_scaler = IMAGE_SCALER_BILINEAR;
				else if (!stricmp(myargv[i], "bicubic"))
					opt_scaler = IMAGE_SCALER_BICUBIC;
				else if (!stricmp(myargv[i], "bspline"))
					opt_scaler = IMAGE_SCALER_BSPLINE;
				else if (!stricmp(myargv[i], "catmullrom"))
					opt_scaler = IMAGE_SCALER_CATMULLROM;
				else if (!stricmp(myargv[i], "lanczos"))
					opt_scaler = IMAGE_SCALER_LANCZOS;
				else if (!stricmp(myargv[i], "scale2x"))
					opt_scaler = IMAGE_SCALER_SCALE2X;
				else if (!stricmp(myargv[i], "super2x"))
					opt_scaler = IMAGE_SCALER_SUPER2X;
				opt_scaler2 = opt_scaler;
			}
			continue;
		}
		else if (!stricmp(myargv[i], "-scaler2"))
		{
			i++;
			if (i < myargc)
			{
				    if (!stricmp(myargv[i], "nearest"))
					opt_scaler2 = IMAGE_SCALER_BOX;
				else if (!stricmp(myargv[i], "bilinear"))
					opt_scaler2 = IMAGE_SCALER_BILINEAR;
				else if (!stricmp(myargv[i], "bicubic"))
					opt_scaler2 = IMAGE_SCALER_BICUBIC;
				else if (!stricmp(myargv[i], "bspline"))
					opt_scaler2 = IMAGE_SCALER_BSPLINE;
				else if (!stricmp(myargv[i], "catmullrom"))
					opt_scaler2 = IMAGE_SCALER_CATMULLROM;
				else if (!stricmp(myargv[i], "lanczos"))
					opt_scaler2 = IMAGE_SCALER_LANCZOS;
				else if (!stricmp(myargv[i], "scale2x"))
					opt_scaler2 = IMAGE_SCALER_SCALE2X;
				else if (!stricmp(myargv[i], "super2x"))
					opt_scaler2 = IMAGE_SCALER_SUPER2X;
			}
			continue;
		}
	}
}

void Help(void)
{
	waitforkey = true;
	Print(
	"usage: rwgtex [global options] [phase]\n"
	"\n"
	"phases:\n"
	"  -dds : generate DDS files for input textures\n"
	"\n"
	"global options:\n"
	"       -nc: print no caption\n"
	"        -w: wait for key when finished\n"
	"      -mem: show memstats\n"
	"       -sp: solid pacifier prints\n"
	"        -c: compact mode (disable verbose prints)\n"
	"        -f: function mode (disable all prints except pacifier)\n"
	"     -cd S: set a different working directory\n"
	"-threads X: set explicit number of threads\n"
	"    -opt X: load custom option file\n"
	"   -errlog: write errlog.txt on error\n"
	"\n");
}

int main(int argc, char **argv)
{
	int i, returncode = 0;
	char optionfile[MAX_FPATH];
	bool printcap;

	// get program name
	myargv = argv;
	myargc = argc;
	memset(progname, 0, MAX_FPATH);
	memset(progpath, 0, MAX_FPATH);
	ExtractFileBase(argv[0], progname);
	ExtractFilePath(argv[0], progpath);

	verbose = true;
	noprint = false;
	printcap = CheckParm("-nc") ? false : true;
	waitforkey = (CheckParm("-w") || CheckParm("-"));
	memstats = CheckParm("-mem");
	solidpacifier = CheckParm("-sp");
	errorlog = CheckParm("-errlog");
	if (CheckParm("-c")) 
	{
		// disable partial printings
		verbose = false;
		printcap = false;
	}
	if (CheckParm("-f")) 
	{
		// disable all printings
		verbose = false;
		printcap = false;
		noprint = true;
	}
	strcpy(optionfile, "rwgtex.opt");
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
		if (!strnicmp(argv[i], "-opt", 8))
		{
			i++;
			if (i < argc)
			{
				strcpy(optionfile, argv[i]);
				Print("Custom option file %s\n", optionfile);
			}
		}
		if (argv[i][0] != '-')
			break;
	}

	Mem_Init();
	crc32_init();
	FS_Init();
	Image_Init();
	Thread_Init();
	if (!numthreads)
		numthreads = num_cpu_cores;

	if (printcap)
	{
		Print("-----------------------------------------------------------\n");
		Print(" RwgTex v%s.%s by Pavel [VorteX] Timofeyev\n", RWGTEX_VERSION_MAJOR, RWGTEX_VERSION_MINOR);
		DDS_PrintModules();
		Image_PrintModules();
		FS_PrintModules();
		Print("-----------------------------------------------------------\n");
		Print("%i threads\n", numthreads);
		if (memstats)
			Print("showing memstats\n");
		if (waitforkey)
			Print("waiting for key\n");
		Print("\n");
	}

	if (CheckParm("-dds"))
	{
		LoadOptions(optionfile);
		returncode = DDS_Main(argc-i, argv+i);
	}
	else
		Help();
	Print("\n");

	Thread_Shutdown();
	Image_Shutdown();
	FS_Shutdown();
	Mem_Shutdown();

#if _MSC_VER
	if (waitforkey && !noprint)
	{
		printf("press any key\n");
		getchar();
	}
#endif

	return returncode;
}