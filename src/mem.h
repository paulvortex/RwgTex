// mem.h
#ifndef H_MEM_H
#define H_MEM_H

void Mem_Init(void);
void Mem_Shutdown(void);

void _mem_free( void **data, char *file, int line);
#define mem_free(data) _mem_free((void **)(&(data)), __FILE__, __LINE__)
void *_mem_alloc( size_t size, char *file, int line);
#define mem_alloc(size) _mem_alloc(size, __FILE__, __LINE__)
void _mem_calloc(void **bufferptr, size_t size, char *file, int line);
#define mem_calloc(buffer,size) _mem_calloc((void **)buffer, size, __FILE__, __LINE__)
void *_mem_realloc(void *data, size_t size, char *file, int line);
#define mem_realloc(data,size) _mem_realloc(data, size, __FILE__, __LINE__)

void _mem_sentinel(char *name, void *ptr, size_t size, char *file, int line);
#define mem_sentinel(name, ptr, size) _mem_sentinel(name, ptr, size, __FILE__, __LINE__)
bool _mem_sentinel_free(char *name, void *ptr, char *file, int line);
#define mem_sentinel_free(name, ptr) _mem_sentinel_free(name, ptr, __FILE__, __LINE__)

#endif