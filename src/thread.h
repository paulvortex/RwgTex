// thread.h
#pragma once

#ifdef WIN32
#include <windows.h>
#endif

#define	MAX_THREADS 32
#define THREAD_STACK_SIZE (4 * 1024 * 1024)

extern int num_cpu_cores;

typedef struct 
{
	int    work_num;     // total work count
	int    work_pending; // work counter
	HANDLE work_mutex;  // mutex to access work
	int    threads_num;  // number of threads in this pool
	void  *threads;      // pointer to threads data 
	void  *data;         // shared user data

	// critical section
#ifdef WIN32
	CRITICAL_SECTION crit;
	bool crit_entered;
#endif
}ThreadPool;

typedef struct
{
	int         id;		// thread system id
	int         num;    // thread num (0 - number of threads)
	HANDLE      handle; // thread handle
	ThreadPool *pool;   // pointer to shared thread pool
	void       *data;   // user data
} ThreadData;

// get a new work for thread
int	GetWorkForThread(ThreadData *thread);

// thread synchronization
void _ThreadLock(ThreadData *thread, char *file, int line);
#define ThreadLock(thread) _ThreadLock(thread, __FILE__, __LINE__)
void _ThreadUnlock(ThreadData *thread, char *file, int line);
#define ThreadUnlock(thread) _ThreadUnlock(thread, __FILE__, __LINE__)

// run thread in parallel
double RunThreads(int num_threads, int work_count, void *common_data, void(*thread_func)(ThreadData *thread), void(*thread_start)(ThreadData *thread) = NULL, void(*thread_finish)(ThreadData *thread) = NULL);

// init threading system
void Thread_Init(void);
void Thread_Shutdown(void);