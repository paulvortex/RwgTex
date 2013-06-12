// tex_compress.h
#ifndef H_TEX_COMPRESS_H
#define H_TEX_COMPRESS_H

#include "tex.h"

// a task that is shipped to codec
// codec should fill it's own values (format type, colorSwizzle etc.)
// and then task is get executed
typedef struct TexEncodeTask_s
{
	// initialized by image loader
	FS_File          *file;
	LoadedImage      *image;
	TexContainer     *container;
	// initialized by codec
	TexCodec         *codec; // if discarded, redirect to fallback codec
	TexFormat        *format;
	TexTool          *tool;
	// initialized right before shipping task to the tool
	byte             *stream;
	size_t            streamLen;
} TexEncodeTask;

// multithreaded write stuff
typedef struct TexWriteData_s
{
	char            outfile[MAX_FPATH];
	byte           *data;
	size_t          datasize;
	TexWriteData_s *next;
} TexWriteData;

typedef struct
{
	// stats
	size_t         num_exported_files;
	size_t        num_original_files;
	double        size_original_files;

	// zip file in memory
	void         *zip_data;
	size_t        zip_len;
	size_t        zip_maxlen;

	// write chain
	TexWriteData *writeData;
} TexCompressData;

// generic
void  TexCompress_WorkerThread(ThreadData *thread);
void  TexCompress_MainThread(ThreadData *thread);
void  TexCompress_Option(const char *section, const char *group, const char *key, const char *val, const char *filename, int linenum);
void  TexCompress_CodecOption(TexCodec *codec, const char *group, const char *key, const char *val, const char *filename, int linenum);
void  TexCompress_ToolOption(TexTool *tool, const char *group, const char *key, const char *val, const char *filename, int linenum);
void  TexCompress_Load(void);

#endif