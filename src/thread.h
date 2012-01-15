// thread.h
// parts of code picked from q3map2 sourcecode

#ifndef MAX_THREADS

#define	MAX_THREADS	64

extern int numthreads;
extern int numthreadwork;

int	GetWorkForThread(void);
void ThreadInit(void);
#define ThreadLock() ThreadLock2(__FILE__, __LINE__)
void ThreadLock2(char *file, int line);
#define ThreadUnlock() ThreadUnlock2(__FILE__, __LINE__)
void ThreadUnlock2(char *file, int line);
double RunThreads(int workcnt, void(*func)(int));

#endif