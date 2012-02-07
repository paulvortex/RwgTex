// thread.h
// based on q3map2's thread manager

#include "thread.h"
#include "main.h"
#include "cmd.h"

// get a new work for thread
int	GetWorkForThread(ThreadData *thread)
{
	int	r;

	WaitForSingleObject(thread->pool->work_mutex, INFINITE);
	if (thread->pool->work_pending >= thread->pool->work_num)
		r = -1;
	else
	{
		r = thread->pool->work_pending;
		thread->pool->work_pending++;
	}
	ReleaseMutex(thread->pool->work_mutex);
	return r;
}

/*
===================================================================

WIN32

===================================================================
*/

#ifdef WIN32

#include <windows.h>

int	num_cpu_cores = -1;

void Thread_Init(void)
{
	SYSTEM_INFO info;

	GetSystemInfo(&info);
	num_cpu_cores = info.dwNumberOfProcessors;
	if (num_cpu_cores < 1 || num_cpu_cores > 32)
		num_cpu_cores = 1;
}

void Thread_Shutdown(void)
{
}

// thread synchronization
void _ThreadLock(ThreadData *thread, char *file, int line)
{
	EnterCriticalSection(&thread->pool->crit);
	if (thread->pool->crit_entered)
		Error("Recursive ThreadLock on %s:%i", file, line);
	thread->pool->crit_entered = true;
}

void _ThreadUnlock(ThreadData *thread, char *file, int line)
{
	if (!thread->pool->crit_entered)
		Error ("ThreadUnlock without lock on %s:%i", file, line);
	thread->pool->crit_entered = false;
	LeaveCriticalSection(&thread->pool->crit);
}

// run thread in parallel
double RunThreads(int num_threads, int work_count, void *common_data, void(*thread_func)(ThreadData *thread), void(*thread_start)(ThreadData *thread), void(*thread_finish)(ThreadData *thread))
{
	double start;
	ThreadPool pool;
	ThreadData *threads;
	int	i;

	start = I_DoubleTime();

	// init threading system
	if (num_cpu_cores == -1)
		Thread_Init();

	// create thread pool
	pool.work_num = work_count;
	pool.work_pending = 0;
	pool.work_mutex = CreateMutex(NULL, FALSE, NULL);
	pool.threads_num = min(num_threads, work_count);
	pool.threads = mem_alloc(sizeof(ThreadData) * pool.threads_num);
	memset(pool.threads, 0, sizeof(ThreadData) * pool.threads_num);
#ifdef WIN32
	InitializeCriticalSection(&pool.crit);
	pool.crit_entered = false;
#endif
	pool.data = common_data;

	//  start threads
	threads = (ThreadData *)pool.threads;
	for (i = 0; i < pool.threads_num; i++)
	{
		threads[i].num = i;
		threads[i].pool = &pool;
		if (thread_start)
			thread_start(&threads[i]);
	}

	// run works
	for (i = 0; i < pool.threads_num; i++)
		threads[i].handle = CreateThread(NULL, THREAD_STACK_SIZE, (LPTHREAD_START_ROUTINE)thread_func, (LPVOID)&threads[i], 0, (LPDWORD)&threads[i].id);
	for (i = 0; i < pool.threads_num; i++)
		WaitForSingleObject(threads[i].handle, INFINITE);

	// finish threads
	if (thread_finish)
		for (i = 0; i < pool.threads_num; i++)
			thread_finish(&threads[i]);

	// delete thread pool
	DeleteCriticalSection(&pool.crit);
	mem_free(pool.threads);

	// return whole time
	return I_DoubleTime() - start;
}

#else

#error "Threads not implemented!"

#endif