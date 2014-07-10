////////////////////////////////////////////////////////////////
//
// RwgTex / memory management
// (c) Pavel [VorteX] Timofeyev
// based on functions picked from Darkplaces engine
// See LICENSE text file for a license agreement
//
////////////////////////////////

#include "main.h"
#include "mem.h"
#include <string>
#include <vector>
using namespace std;

typedef struct
{
	char  *name;
	char  *file;
	int    line;
	size_t size;
	void  *ptr;
	bool   free;
}memsentinel;

bool                initialized = false;
size_t              total_allocated;
size_t              total_active;
size_t              total_active_peak;
vector<memsentinel> sentinels;
HANDLE              sentinelMutex = NULL;
HANDLE              sentinelMutex2 = NULL;

void Mem_Error(char *message_format, ...) 
{
	char msg[16384];
	va_list argptr;

	va_start(argptr, message_format);
	vsprintf(msg, message_format, argptr);
	va_end(argptr);
	Error(msg);
}

void Mem_Init(void)
{
	if (initialized)
		return;

	initialized = true;
	total_allocated = 0;
	total_active = 0;
	total_active_peak = 0;

	sentinels.clear();
	sentinelMutex = CreateMutex(NULL, FALSE, NULL);
	sentinelMutex2 = CreateMutex(NULL, FALSE, NULL);
}

void Mem_Shutdown(void)
{
	if (!initialized)
		return;
	if (memstats)
	{
		size_t leaks = 0;
		size_t leaked = 0;
		for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
		{
			if (!s->free)
			{
				leaked += s->size;
				leaks;
			}
		}

		Print("----------------------------------------\n");
		Print(" Dynamic memory usage stats\n");
		Print("----------------------------------------\n");
		Print("        Peak allocated: %.3f Mb\n", (double)total_active_peak / 1048576.0 );
		Print("       total allocated: %.3f Mb\n", (double)total_allocated / 1048576.0 );
		Print("                 leaks: %.3f Mb\n", (double)leaked / 1048576.0 );
		if (leaks)
		{
			Print("            leak spots: %i\n", leaks );
			Print("\n");
			for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
				if (!s->free)
					Print("%s:%i (%s) %i bytes (%.3f Mb)\n", s->file, s->line, s->name, s->size, (double)s->size / 1048576.0);
		}
		Print("\n");
	}
	initialized = false;
	sentinels.clear();
}

void _mem_sentinel(char *name, void *ptr, size_t size, char *file, int line)
{
	if (!memstats)
		return;

	WaitForSingleObject(sentinelMutex, INFINITE);

	// create sentinel
	// pick free one or allocate
	bool picked = false;
	for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
	{
		if (s->free)
		{
			s->name = name;
			s->file = file;
			s->line = line;
			s->size = size;
			s->ptr = ptr;
			s->free = false;
			picked = true;
			break;
		}
	}
	if (!picked)
	{
		memsentinel s;
		s.name = name;
		s.file = file;
		s.line = line;
		s.size = size;
		s.ptr  = ptr;
		s.free = false;
		sentinels.push_back(s);
	}

	// pop global stats
	total_allocated += size;
	total_active += size;
	if (total_active > total_active_peak)
		total_active_peak = total_active;

	ReleaseMutex(sentinelMutex);
}

bool _mem_sentinel_free(char *name, void *ptr, char *file, int line)
{
	if (!memstats)
		return true;

	WaitForSingleObject(sentinelMutex2, INFINITE);
	// find sentinel for pointer
	int found = -1;
	for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
	{
		if (s->ptr == ptr)
		{
			if (!s->free)
			{
				// throw sentinel
				total_active -= s->size;
				s->size = 0;
				s->file = 0;
				s->line = 0;
				s->name = 0;
				s->ptr = 0;
				s->free = true;
				found = 1;
			}
			else
			{
				// already freed
				found = -2;
			}
			break;
		}
	}
	ReleaseMutex(sentinelMutex2);
	// oops, this pointer was not allocated
	if (found == 1)
		return true;
	if (found == -1)
		Mem_Error("%s:%i (%s) - trying to free non-allocated page %i (sentinel not found)\n", file, line, name, ptr);
	if (found == -2)
		Mem_Error("%s:%i (%s) - trying to free non-allocated page %i (sentinel already freed)\n", file, line, name, ptr);
	return false;
}

void *_mem_realloc(void *data, size_t size, char *file, int line)
{
	if (size <= 0)
		return NULL;
	if (data == NULL) // no data, just alloc
		return _mem_alloc(size, file, line);
	if (!_mem_sentinel_free("mem_realloc", data, file, line))
		return NULL;
	data = realloc(data, size);
	if (!data)
	{
		if (memstats)
		{
			Print("memory allocations: %i\n", sentinels.size());
			for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
				if (!s->free)
					Print("%s:%i (%s) %i bytes (%.3f Mb)\n", s->file, s->line, s->name, s->size, (double)s->size / 1048576.0);
		}
		Mem_Error("%s:%i - error reallocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	}
	if (!initialized)
		return data;
	_mem_sentinel("mem_realloc", data, size, file, line);
	return data;
}

void *_mem_alloc(size_t size, char *file, int line)
{
	void *data;

	//printf("_mem_alloc: %i - %s:%i\n", size, file, line);
	if (size <= 0)
		return NULL;
	data = malloc(size);
	if (!data)
	{
		if (memstats)
		{
			Print("memory allocations: %i\n", sentinels.size());
			for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
				if (!s->free)
					Print("%s:%i (%s) %i bytes (%.3f Mb)\n", s->file, s->line, s->name, s->size, (double)s->size / 1048576.0);
		}
		Mem_Error("%s:%i - error allocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	}
	if (!initialized)
		return data;
	_mem_sentinel("mem_alloc", data, size, file, line);
	return data;
}

void _mem_calloc(void **bufferptr, size_t size, char *file, int line)
{
	void *data;

	if (size <= 0)
		return;
	data = malloc(size);
	memset(data, 0, size);
	if (!data)
	{
		if (memstats)
		{
			Print("memory allocations: %i\n", sentinels.size());
			for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
				if (!s->free)
					Print("%s:%i (%s) %i bytes (%.3f Mb)\n", s->file, s->line, s->name, s->size, (double)s->size / 1048576.0);
		}
		Mem_Error("%s:%i - error allocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	}
	if (!initialized)
		return;
	_mem_sentinel("mem_calloc", data, size, file, line);
	*bufferptr = data;
}

void _mem_free(void **data, char *file, int line)
{
	void *ptr;

	ptr = *data;
	if (!ptr)
		return;
	if (!initialized)
	{
		free(ptr);
		return;
	}
	if (!_mem_sentinel_free("mem_free", ptr, file, line))
		return;
	free(ptr);
	*data = NULL;
}
