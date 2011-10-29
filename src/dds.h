#include "cmd.h"
#include "dll.h"

#ifndef RWGDDS_VERSION
#define RWGDDS_VERSION "v1.0"
static char *RWGDDS_WELCOME =
"----------------------------------------\n"
" RwgDDS %s by Pavel P. [VorteX] Timofeyev\n"
"----------------------------------------\n"
;
#define MAX_DDSPATH 1024
#endif

extern bool waitforkey;
extern bool error_waitforkey;
extern bool memstats;
extern bool verbose;
extern bool noprint;
extern bool solidpacifier;
extern bool errorlog;

extern char progname[MAX_DDSPATH];
extern char progpath[MAX_DDSPATH];

// printing
void Print (char *str, ...);
void Verbose (char *str, ...);
void Warning (char *str, ...);
void Pacifier(char *str, ...);
void PercentPacifier(char *str, ...);
void PacifierEnd();
