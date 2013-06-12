// thread.h
#ifndef H_TEX_THREAD_H
#define H_TEX_THREAD_H

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
	HANDLE work_mutex;   // mutex to access work
	int    threads_num;  // number of threads in this pool
	void  *threads;      // pointer to threads data 

	// start & finish marks
	bool   stop;         // stop all threads, this only can be set before started mark
	bool   started;      // central thread is started
	bool   finished;     // work threads are finished
}ThreadPool;

typedef struct
{
	int         id;		// thread system id
	int         num;    // thread num (0 - number of threads)
	HANDLE      handle; // thread handle
	ThreadPool *pool;   // pointer to shared thread pool

	// shared data
	void       *data;   
} ThreadData;

// get a new work for thread
int	GetWorkForThread(ThreadData *thread);

// run thread in parallel
double ParallelThreads(int num_threads, int work_count, void *common_data, void(*thread_func)(ThreadData *thread), void(*central_thread)(ThreadData *thread) = NULL);

// init threading system
void Thread_Init(void);
void Thread_Shutdown(void);

#endif