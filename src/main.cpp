////////////////////////////////////////////////////////////////
//
// RwgTex / main
// (c) Pavel [VorteX] Timofeyev
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define F_MAIN_C
#include "main.h"

int Help(void)
{
	waitforkey = true;
	Print(
	"Usage:\n"
	" rwgtex [options] [file_or_path] [-o output_file_or_path] [codec] [codec options]\n"
	"\n"
	"Options:\n"
	"       -nc: print no caption\n"
	"        -w: wait for key when finished\n"
	"       -mw: do not wait for key\n"
	"      -mem: show memstats\n"
	"        -v: show verbose messages\n"
	"     -spac: solid pacifier prints\n"
	"        -c: compact mode (only generic prints)\n"
	"        -f: function mode (only pacifier prints)\n"
	"     -cd S: set a different working directory\n"
	"-threads X: set number of threads\n"
	"    -opt X: load custom option file\n"
	"   -errlog: write errlog.txt on error\n"
	"  -version: show external modules and their version\n"
	"\n");
	Tex_PrintCodecs();
	Tex_PrintTools();
	Tex_PrintContainers();
	Print("Check out rwgtex -codec for more help\n");
	return 0;
}

int PrintModules(void)
{
	Print("RwgTex modules:\n");
	FS_PrintModules();
	Image_PrintModules();
	for (TexTool *tool = tex_tools; tool; tool = tool->next)
		Print(" %s %s\n", tool->fullName, tool->fGetVersion());
	Print("\n");
	return 0;
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

	noprint = false;
	// COMMANDLINEPARM: -nc: print no caption
	printcap = CheckParm("-nc") ? false : true;
	// COMMANDLINEPARM: -w: wait for key when finished
	waitforkey = (CheckParm("-w") || CheckParm("-"));
	// COMMANDLINEPARM: -mem: show memstats
	memstats = CheckParm("-mem");
	// COMMANDLINEPARM: -v: show verbose messages
	verbose = CheckParm("-v");
	// COMMANDLINEPARM: -spac: solid pacifier prints
	solidpacifier = CheckParm("-spac");
	// COMMANDLINEPARM: -errlog: write errlog.txt on error
	errorlog = CheckParm("-errlog");
	// COMMANDLINEPARM: -c: compact mode (only generic prints)
	if (CheckParm("-c")) 
	{
		verbose = false;
		printcap = false;
	}
	// COMMANDLINEPARM: -f: function mode (only pacifier prints)
	if (CheckParm("-f")) 
	{
		verbose = false;
		printcap = false;
		noprint = true;
	}
	strcpy(optionfile, "rwgtex.ini");
	for (i = 1; i < argc; i++) 
	{
		// COMMANDLINEPARM: -cd: set a different working directory
		if (!strnicmp(argv[i], "-cd", 3))
		{
			i++;
			if (i < argc)
				ChangeDirectory(argv[i]);
			continue;
		}
		// COMMANDLINEPARM: -threads: set number of threads
		if (!strnicmp(argv[i], "-threads", 8))
		{
			i++;
			if (i < argc)
				numthreads = atoi(argv[i]);
			continue;
		}
		// COMMANDLINEPARM: -opt: load custom option file
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
	Tex_Init();

	if (printcap)
	{
		Print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		Print(" RwgTex v%s.%s by Pavel [VorteX] Timofeyev\n", RWGTEX_VERSION_MAJOR, RWGTEX_VERSION_MINOR);
		Print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		Print("%i threads", numthreads);
		if (memstats)
			Print(", showing memstats");
		// COMMANDLINEPARM: -nw: don't wait for key
		if (waitforkey && !CheckParm("-nw"))
			Print(", waiting for key");
		Print("\n\n");
	}

	//COMMANDLINEPARM: -version: show modules versions
	if (tex_active_codecs)
	{
		LoadOptions(optionfile);
		returncode = TexMain(argc-i, argv+i);
	}
	else if (CheckParm("-version"))
		returncode = PrintModules();
	else
		returncode = Help();
	Print("\n");

	Thread_Shutdown();
	Tex_Shutdown();
	Image_Shutdown();
	FS_Shutdown();
	Mem_Shutdown();

#if _MSC_VER
	if (waitforkey && !noprint && !CheckParm("-nw"))
	{
		printf("press any key\n");
		getchar();
	}
#endif

	return returncode;
}