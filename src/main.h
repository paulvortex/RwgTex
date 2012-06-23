// main.h

#pragma once

#include "windows.h"
#include "process.h"
#include <assert.h>
#include <math.h>
#include <string>
#include <vector>
using namespace std;

#include "cmd.h"
#include "mem.h"
#include "dll.h"
#include "thread.h"
#include "image.h"
#include "dds.h"
#include "fs.h"

#define RWGTEX_VERSION "1.1"
#define MAX_FPATH 1024

// general
extern bool waitforkey;
extern bool memstats;
extern bool errorlog;
extern int  numthreads;
extern char progname[MAX_FPATH];
extern char progpath[MAX_FPATH];

// options
extern char   opt_srcDir[MAX_FPATH];
extern char   opt_srcFile[MAX_FPATH];
extern char   opt_destPath[MAX_FPATH];
extern bool   opt_useFileCache;
extern bool   opt_allowNonPowerOfTwoDDS;
extern bool   opt_forceNoMipmaps;
extern bool   opt_forceScale2x;
extern FCLIST opt_include;
extern FCLIST opt_nomip;
extern FCLIST opt_forceDXT1;
extern FCLIST opt_forceDXT2;
extern FCLIST opt_forceDXT3;
extern FCLIST opt_forceDXT4;
extern FCLIST opt_forceDXT5;
extern FCLIST opt_forceBGRA;
extern DWORD  opt_forceFormat;
extern FCLIST opt_forceNvCompressor;
extern FCLIST opt_forceATICompressor;
extern FCLIST opt_isNormal;
extern FCLIST opt_isHeight;
extern string opt_basedir;
extern string opt_ddsdir;
extern bool   opt_detectBinaryAlpha;
extern byte   opt_binaryAlphaMin;
extern byte   opt_binaryAlphaMax;
extern byte   opt_binaryAlphaCenter;
extern FCLIST opt_archiveFiles;
extern string opt_archivePath;
extern FCLIST opt_scale;
extern SCALER opt_scaler;
extern TOOL   opt_compressor;
extern int    opt_zipMemory;

void Help(void);
