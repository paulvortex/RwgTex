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

#ifndef __DLL__
#define __DLL__

#include "cmd.h"

//
// DLL management
//

// Win32 specific
#ifdef WIN32
# include <windows.h>
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

