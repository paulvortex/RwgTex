// memory management

#include "main.h"

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

void Mem_Shutdown()
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

		printf("----------------------------------------\n");
		printf(" Memory stats\n");
		printf("----------------------------------------\n");
		printf("     peak active memory: %.2f Mbytes\n", (double)total_active_peak / 1048576.0 );
		printf(" total memory allocated: %.2f Mbytes\n", (double)total_allocated / 1048576.0 );
		printf("          leaked memory: %.2f Mbytes\n", (double)leaked / 1048576.0 );
		if (leaks)
		{
			printf("     leaked allocations: %i\n", leaks );
			printf("\n");
			printf("----------------------------------------\n");
			printf(" Leaked memory\n");
			printf("----------------------------------------\n");
			for (std::vector<memsentinel>::iterator s = sentinels.begin(); s < sentinels.end(); s++)
				if (!s->free)
					printf("%s:%i (%s) %i bytes (%.2f Mb)\n", s->file, s->line, s->name, s->size, (double)s->size / 1048576.0);
		}

		printf("\n");
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
		Error("%s:%i (%s) - tried to free a non-allocated page %i (sentinel not found)\n", name, file, line, ptr);
	if (found == -2)
		Error("%s:%i (%s) - tried to free a non-allocated page %i (sentinel already freed)\n", name, file, line, ptr);
	return false;
}

void *_mem_alloc(size_t size, char *file, int line)
{
	void *data;

	if (size <= 0)
		return NULL;
	data = malloc(size);
	if (!data)
		Error("%s:%i - failed on allocating %d bytes (%.2f Mb)\n", file, line, size, (double)size / 1048576.0);
	if (!initialized)
		return data;
	_mem_sentinel("mem_alloc", data, size, file, line);
	return data;
}

void _mem_free(void *data, char *file, int line)
{
	if (!data)
		return;
	if (!initialized)
	{
		free(data);
		return;
	}
	if (!_mem_sentinel_free("mem_free", data, file, line))
		return;
	free(data);
}
