// main.h
#ifndef H_TEX_MAIN_H
#define H_TEX_MAIN_H

#define MAX_FPATH 4096
#define RWGTEX_VERSION_MAJOR "1"
#define RWGTEX_VERSION_MINOR "65"

#include "cmd.h"
#include "mem.h"
#include "dll.h"
#include "options.h"
#include "thread.h"
#include "image.h"
#include "tex.h"
#include "fs.h"

// parameters
#ifdef F_MAIN_C
	#define MAIN_EXTERN 
#else
	#define MAIN_EXTERN extern
#endif

// general
MAIN_EXTERN bool waitforkey;
MAIN_EXTERN bool memstats;
MAIN_EXTERN bool errorlog;
MAIN_EXTERN int  numthreads;
MAIN_EXTERN char progname[MAX_FPATH];
MAIN_EXTERN char progpath[MAX_FPATH];

#endif