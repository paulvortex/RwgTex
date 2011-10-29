// mem.h
// parts of code picked from Darkplaces hmap2

#ifndef __MEM_H__
#define __MEM_H__

void Q_InitMem( void );
void Q_PrintMem( void );
void Q_ShutdownMem( bool printstats );

void *qmalloc( size_t size );
void qfree( void *data );

#endif