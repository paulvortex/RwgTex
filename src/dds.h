#include "cmd.h"
#include "dll.h"

#ifndef RWGDDS_VERSION
#define RWGDDS_VERSION "v1.1"
#define MAX_DDSPATH 1024
#endif

extern bool waitforkey;
extern bool memstats;
extern bool errorlog;

extern char progname[MAX_DDSPATH];
extern char progpath[MAX_DDSPATH];
