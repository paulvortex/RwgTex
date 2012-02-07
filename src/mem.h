// mem.h

#pragma once

void Mem_Init(void);
void Mem_Shutdown(void);

void _mem_free( void *data, char *file, int line);
#define mem_free(data) _mem_free(data, __FILE__, __LINE__)
void *_mem_alloc( size_t size, char *file, int line);
#define mem_alloc(size) _mem_alloc(size, __FILE__, __LINE__)

void _mem_sentinel(char *name, void *ptr, size_t size, char *file, int line);
#define mem_sentinel(name, ptr, size) _mem_sentinel(name, ptr, size, __FILE__, __LINE__)
bool _mem_sentinel_free(char *name, void *ptr, char *file, int line);
#define mem_sentinel_free(name, ptr) _mem_sentinel_free(name, ptr, __FILE__, __LINE__)