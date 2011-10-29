// thread.h
// parts of code picked from q3map2 sourcecode

#include "thread.h"
#include "dds.h"
#include "cmd.h"


// parts of code picked from q3map2 sourcecode

int	dispatch;
int	numthreadwork;
bool threaded;

// get a free work num for thread
int	GetWorkForThread(void)
{
	int	r;

	ThreadLock();
	if (dispatch == numthreadwork)
	{
		ThreadUnlock();
		return -1;
	}
	r = dispatch;
	dispatch++;
	ThreadUnlock();
	return r;
}

/*
===================================================================

WIN32

===================================================================
*/

#ifdef WIN32

#include <windows.h>

int	numthreads = -1;
CRITICAL_SECTION crit;
static int enter;

void ThreadInit(void)
{
	SYSTEM_INFO info;

	if (numthreads == -1) // not set manually
	{
		GetSystemInfo(&info);
		numthreads = info.dwNumberOfProcessors;
		if (numthreads < 1 || numthreads > 32)
			numthreads = 1;
	}
}

void ThreadLock(void)
{
	if (!threaded)
		return;
	EnterCriticalSection(&crit);
	if (enter)
		Error("Recursive ThreadLock");
	enter = 1;
}

void ThreadUnlock(void)
{
	if (!threaded)
		return;
	if (!enter)
		Error ("ThreadUnlock without lock");
	enter = 0;
	LeaveCriticalSection(&crit);
}

double RunThreads(int workcnt, void(*func)(int))
{
	int	threadid[MAX_THREADS];
	HANDLE threadhandle[MAX_THREADS];
	double start;
	int	i;

	start = I_DoubleTime();
	dispatch = 0;
	numthreadwork = workcnt;
	threaded = true;

	if (numthreads == -1)
		ThreadInit();

	// run threads in parallel
	InitializeCriticalSection(&crit);
	if (numthreads == 1)
		func(0);
	else
	{
		for (i = 0; i < numthreads; i++)
			threadhandle[i] = CreateThread(NULL, (4096 * 1024), (LPTHREAD_START_ROUTINE)func, (LPVOID)i, 0, (LPDWORD)&threadid[i]);
		for (i = 0; i < numthreads; i++)
			WaitForSingleObject(threadhandle[i], INFINITE);
	}
	DeleteCriticalSection(&crit);
	threaded = false;

	return I_DoubleTime() - start;
}

#else

#error "Threads not implemented!"

#endif