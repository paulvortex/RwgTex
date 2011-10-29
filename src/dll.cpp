////////////////////////////////////////////////////////////////
//
// RWGDDS - dll management
// scrapped from Darkplaces Engine sourcecode
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

#define SUPPORTDLL
#ifdef WIN32
# ifdef _WIN64
#  ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0502
#  endif
   // for SetDllDirectory
# endif
# include <windows.h>
# include <mmsystem.h> // timeGetTime
# include <time.h> // localtime
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#else
# include <unistd.h>
# include <fcntl.h>
# include <sys/time.h>
# include <time.h>
# ifdef SUPPORTDLL
#  include <dlfcn.h>
# endif
#endif

static bool LoadDllFunctions(dllhandle_t dllhandle, const dllfunction_t *fcts, bool complain, bool has_next)
{
	const dllfunction_t *func;
	if(dllhandle)
	{
		for (func = fcts; func && func->name != NULL; func++)
			if (!(*func->funcvariable = (void *) DllGetProcAddress (dllhandle, func->name)))
			{
				if(complain)
				{
					Verbose (" - missing function \"%s\" - broken library!", func->name);
					if (has_next)
						Verbose("\nContinuing with");
				}
				goto notfound;
			}
		return true;

	notfound:
		for (func = fcts; func && func->name != NULL; func++)
			*func->funcvariable = NULL;
	}
	return false;
}

bool LoadDll (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts, bool verbose)
{
#ifdef SUPPORTDLL
	const dllfunction_t *func;
	dllhandle_t dllhandle = 0;
	char dllpath[MAX_DDSPATH];
	unsigned int i;

	if (handle == NULL)
		return false;

#ifndef WIN32
#ifdef PREFER_PRELOAD
	dllhandle = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);
	if(LoadDllFunctions(dllhandle, fcts, false, false))
	{
		Verbose ("All of %s's functions were already linked in! Not loading dynamically...\n", dllnames[0]);
		*handle = dllhandle;
		return true;
	}
	else
		UnloadDll(&dllhandle);
notfound:
#endif
#endif

	// Initializations
	for (func = fcts; func && func->name != NULL; func++)
		*func->funcvariable = NULL;

	// Try every possible name
	if (verbose)
		Verbose ("Trying to load library...");
	for (i = 0; dllnames[i] != NULL; i++)
	{
		if (verbose)
			Verbose (" \"%s\"", dllnames[i]);
		sprintf(dllpath, "%s%s", progpath, dllnames[i]);
#ifdef WIN32
# ifdef _WIN64
		SetDllDirectory("bin64");
# endif
		dllhandle = LoadLibrary(dllpath);
# ifdef _WIN64
		SetDllDirectory(NULL);
# endif
#else
		dllhandle = dlopen (dllpath, RTLD_LAZY | RTLD_GLOBAL);
#endif
		if (LoadDllFunctions(dllhandle, fcts, verbose, (dllnames[i+1] != NULL) /* vortex: broke || (strrchr(com_argv[0], '/'))*/))
			break;
		else
			UnloadDll (&dllhandle);
	}

	// No DLL found
	if (!dllhandle)
	{
		if (verbose)
			Verbose(" - failed.\n");
		return false;
	}

	if (verbose)
		Verbose(" - loaded.\n");

	*handle = dllhandle;
	return true;
#else
	return false;
#endif
}

void UnloadDll(dllhandle_t* handle)
{
#ifdef SUPPORTDLL
	if (handle == NULL || *handle == NULL)
		return;

#ifdef WIN32
	FreeLibrary (*handle);
#else
	dlclose (*handle);
#endif

	*handle = NULL;
#endif
}

void* DllGetProcAddress (dllhandle_t handle, const char* name)
{
#ifdef SUPPORTDLL
#ifdef WIN32
	return (void *)GetProcAddress (handle, name);
#else
	return (void *)dlsym (handle, name);
#endif
#else
	return NULL;
#endif
}
