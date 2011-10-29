// thread.h
// parts of code picked from q3map2 sourcecode

#define	MAX_THREADS	64

extern int numthreads;
extern int numthreadwork;

int	GetWorkForThread(void);
void ThreadInit(void);
void ThreadLock(void);
void ThreadUnlock(void);
double RunThreads(int workcnt, void(*func)(int));