// dll.h
#ifndef H_TEX_DLL_H
#define H_TEX_DLL_H

#include "cmd.h"

//
// DLL management
//

// Win32 specific
#ifdef WIN32
#include <windows.h>
typedef HMODULE dllhandle_t;
// Other platforms
#else
  typedef void* dllhandle_t;
#endif

typedef struct dllfunction_s
{
	const char *name;
	void **funcvariable;
}
dllfunction_t;

/*! Loads a library. 
 * \param dllnames a NULL terminated array of possible names for the DLL you want to load.
 * \param handle
 * \param fcts
 */
bool LoadDll (const char** dllnames, dllhandle_t* handle, const dllfunction_t *fcts, bool verbose);
void UnloadDll (dllhandle_t* handle);
void* DllGetProcAddress (dllhandle_t handle, const char* name);

#endif