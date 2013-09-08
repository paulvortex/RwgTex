////////////////////////////////////////////////////////////////
//
// RwgTex / Commandline Functions Library
// (c) Pavel [VorteX] Timofeyev
// based on functions picked from Darkplaces hmap2
// See LICENSE text file for a license agreement
//
////////////////////////////////

#define __USE_BSD 1

#include <sys/types.h>
#include <sys/stat.h>
#if defined(WIN32) || defined(_WIN64)
#include <limits.h>
#include <direct.h>
#include <windows.h>
#endif
#include "cmd.h"

#include "mem.h"
#include "main.h"
#include "thread.h"

// console stuff
bool verbose;
bool noprint;
bool solidpacifier;

// set these before calling CheckParm
int myargc;
char **myargv;
char com_token[MAXTOKEN];
bool com_eof;

//========================================================
// strlcat and strlcpy, from OpenBSD

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*	$OpenBSD: strlcat.c,v 1.11 2003/06/17 21:56:24 millert Exp $	*/
/*	$OpenBSD: strlcpy.c,v 1.8 2003/06/17 21:56:24 millert Exp $	*/


#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
#endif  // #ifndef HAVE_STRLCAT

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

#endif  // #ifndef HAVE_STRLCPY


/*
=================
console stuff
=================
*/

void Print(char *str, ...)
{
	va_list argptr;

	if (noprint)
		return;

	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
}

void Verbose(char *str, ...)
{
	va_list argptr;

	if (!verbose || noprint)
		return;

	va_start(argptr, str);
	vprintf(str, argptr);
	va_end(argptr);
}

void PercentPacifier(char *str, ...)
{
	va_list argptr;
	char msg[10000];

	va_start(argptr, str);
	vsprintf(msg, str, argptr);
	va_end(argptr);
	if (!solidpacifier)
		printf("\r%s%%\r", msg);
	else
		printf("\r%s%%                                      \r", msg);
	fflush(stdout);
}

void Pacifier(char *format, ...)
{
	va_list argptr;
	char msg[10000];

	va_start(argptr, format);
	vsprintf(msg, format, argptr);
	va_end(argptr);
	printf("\r%s                                        \r", msg);
	fflush(stdout);
}

char SimplePacifierChars[5] = "-\\|/";
int SimplePacifierCharNum = 0;
void SimplePacifier()
{
	if (noprint)
		return;

	SimplePacifierCharNum++;
	if (SimplePacifierCharNum >= 4)
		SimplePacifierCharNum = 0;
	printf("\r  %c                                        \r", SimplePacifierChars[SimplePacifierCharNum]);
	fflush(stdout);
}

void PacifierEnd() 
{
	if (noprint)
		return;

	printf("\n");
}

void Warning(char *str, ...)
{
	va_list argptr;
	char msg[10000];

	va_start(argptr, str);
	vsprintf(msg, str, argptr);
	va_end(argptr);
	printf("Warning: %s\n", msg);
}


/*
=================
Error

For abnormal program terminations
=================
*/
void Error (char *error, ...)
{
	va_list argptr;
	char logfile[MAX_FPATH], msgstr[10384];
	FILE *f;

	va_start(argptr, error);
	vsprintf(msgstr, error, argptr);
	va_end(argptr);
	printf("\n*** ERROR ***\n%s\n", msgstr);
	
	// write error log
	if (errorlog)
	{
		sprintf(logfile, "%sberror.txt", progpath);
		f = fopen(logfile, "wb");
		if (f)
		{
			fwrite(msgstr, strlen(msgstr), 1, f);
			fclose(f);
		}
	}
#if _MSC_VER
	if (waitforkey)
	{
		printf("press any key\n");
		getchar();
	}
#endif
	exit(1);
}

/*
================
I_DoubleTime
================
*/
double I_DoubleTime (void)
{
#if defined(WIN32) || defined(_WIN64)
	static DWORD starttime;
	static bool first = true;
	DWORD now;

	if (first)
		timeBeginPeriod (1);

	now = timeGetTime ();

	if (first)
	{
		first = false;
		starttime = now;
		return 0.0;
	}

	if (now < starttime)				// wrapped?
		return (now / 1000.0) + (LONG_MAX - starttime / 1000.0);

	if (now - starttime == 0)
		return 0.0;

	return (now - starttime) / 1000.0;
#else
	time_t	t;

	time (&t);

	return t;
#endif
}

// single mkdir
void Q_mkdir (char *path)
{
#if defined(WIN32) || defined(_WIN64)
  if (mkdir (path) != -1)
#else
  if (mkdir (path, 0777) != -1)
#endif
    return;
  if (errno != EEXIST)
    Error ("mkdir '%s': %s",path, strerror(errno));
}

/*
============
CreatePath
============
*/

void AddSlash(char *path)
{
	int l;

	l = strlen(path) - 1;
	if (l < 0)
		return;
	if (path[l] == '/' || path[l] == '\\')
		return;
	path[l+1] = '/';
	path[l+2] = 0;
}


void AddSlash(string &path)
{
	int l;

	l = strlen(path.c_str()) - 1;
	if (l < 0)
		return;
	if (path.c_str()[l] == '/' || path.c_str()[l] == '\\')
		return;
	path.append("/");
}

void CreatePath (char *createpath)
{
	char *ofs, save, *opath;
	char path[MAX_FPATH];

	strncpy(path, createpath, sizeof(path)); 
	opath = path;
	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/' || *ofs == '\\')
		{
			// create the directory
			save = *ofs;
			*ofs = 0;
			if (path[0] && path[strlen(path)-1] != ':' && strcmp(path, "..") && strcmp(path, "."))
			{
				#if defined(WIN32) || defined(_WIN64)
				  if (mkdir (path) != -1)
				#else
				  if (mkdir (path, 0777) != -1)
				#endif
				if (errno != 0 && errno != EEXIST)
					Error ("CreatePath '%s': %s %i", opath, strerror(errno), errno);
			}
			*ofs = save;
		}
	}
}

/*
============
ChangeDirectory
changes program working directory
============
*/
void ChangeDirectory (char *path)
{
	ConvSlashW2U(path);
	if (_chdir(path) != -1)
		return;
	if (errno != EEXIST)
		 Error("chdir %s: %s", path, strerror(errno));
}
void GetDirectory(char *path, int size_bytes)
{
	_getcwd(path, size_bytes);
}
void GetRealPath(char *outpath, char *inpath)
{
	char cdir[MAX_FPATH];
	_getcwd(cdir, MAX_FPATH);
	if (cdir[strlen(cdir)-1] == '/' || cdir[strlen(cdir)-1] == '\\' )
		sprintf(outpath, "%s%s", cdir, inpath);
	else
		sprintf(outpath, "%s/%s", cdir, inpath);
}

/*
============
TempFileName
get temp directory
============
*/
void TempFileName(char *out)
{
	char tempdir[MAX_FPATH], tempfile[MAX_FPATH];
	int l;

#ifdef WIN32
	l = GetTempPath(MAX_FPATH, tempdir);
	tmpnam(tempfile);
	if (!l)
		Error("TempFileName: error %s", strerror(GetLastError()));
	sprintf(out, "%s%s", tempdir, tempfile);
#else
	Error("TempFileName: only implemented on Win32\n");
#endif
}

/*
============
FileTime

returns -1 if not present
============
*/
int	FileTime (char *path)
{
  struct	stat	buf;

  if (stat (path,&buf) == -1)
    return -1;

  return (int)buf.st_mtime;
}



/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
  int		c;
  int		len;

  len = 0;
  com_token[0] = 0;

  if (!data)
    return NULL;

  // skip whitespace
skipwhite:
  while ( (c = *data) <= ' ')
    {
      if (c == 0)
	{
	  com_eof = true;
	  return NULL;			// end of file;
	}
      data++;
    }

  // skip // comments
  if (c=='/' && data[1] == '/')
    {
      while (*data && *data != '\n')
	data++;
      goto skipwhite;
    }

// skip /* */ comments
	if (c == '/' && data[1] == '*')
	{
		data += 2;
		while (1)
		{
			if (!*data)
				break;
			if (*data != '*' || *(data+1) != '/')
				data++;
			else
			{
				data += 2;
				break;
			}
		}
		goto skipwhite;
	}

  // handle quoted strings specially
  if (c == '\"')
    {
      data++;
      do
	{
	  c = *data++;
	  if (c=='\"')
	    {
		  if (len == MAXTOKEN)
			  len = 0;
	      com_token[len] = 0;
	      return data;
	    }
	  if (len < MAXTOKEN)
		 com_token[len++] = c;
	} while (1);
    }

  // parse single characters
  if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
    {
	  if (len < MAXTOKEN)
	      com_token[len++] = c;
	  if (len == MAXTOKEN)
		  len = 0;
      com_token[len] = 0;
      return data+1;
    }

  // parse a regular word
  do
    {
	  if (len < MAXTOKEN)
	      com_token[len++] = c;
      data++;
      c = *data;
      if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
	break;
    } while (c>32);

  if (len == MAXTOKEN)
	  len = 0;
  com_token[len] = 0;
  return data;
}


int Q_strncasecmp (char *s1, char *s2, int n)
{
	int		c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
		if (!c1)
			return 0;		// strings are equal
	}

	return -1;
}

int Q_strcasecmp (char *s1, char *s2)
{
  return Q_strncasecmp (s1, s2, 99999);
}


char *Q_strupr (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = toupper(*in);
      in++;
    }
  return start;
}

char *Q_strlower (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = tolower(*in);
      in++;
    }
  return start;
}

// convert to stupid Windows slashes
byte unixtowin(byte c)
{
	if (c == '/')
		return '\\';
	return c;

}
byte wintounix(byte c)
{
	if (c == '\\')
		return '/';
	return c;

}
byte dotconv(byte c)
{
	if (c == '.')
		return '-';
	return c;
}
char *ConvSlashU2W (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = unixtowin(*in);
      in++;
    }
  return start;
}
char *ConvSlashW2U (char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = wintounix(*in);
      in++;
    }
  return start;
}
char *ConvDot(char *start)
{
  char	*in;
  in = start;
  while (*in)
    {
      *in = dotconv(*in);
      in++;
    }
  return start;
}

/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
bool CheckParm(char *check)
{
	char *str, *next, parm[64];
	int i;

	// check exe name
	str = strstr(progname, "-");
	if (!strcmp(check, "-"))
		return (str == NULL) ? false : true;
	while(str)
	{
		next = strstr(str + 1, "-");
		if (next)
		{
			strncpy(parm, str, (int)(next - str));
			parm[(int)(next - str)] = 0;
		}
		else
		{
			strcpy(parm, str);
			parm[strlen(str)] = 0;
		}
		if (!Q_strcasecmp(check, parm))
			return true;
		str = next;
	}
	// check commandline parms
	for (i = 1;i < myargc;i++)
		if (!Q_strcasecmp(check, myargv[i]))
			return true;
	return 0;
}

/*
================
Q_filelength
================
*/

unsigned int Q_filelength (FILE *f)
{
  int		pos;
  int		end;

  pos = ftell (f);
  fseek (f, 0, SEEK_END);
  end = ftell (f);
  fseek (f, pos, SEEK_SET);

  return end;
}

size_t FileSize(const char *filename)
{
	FILE *f;
	size_t s;

	f = fopen(filename, "rb");
	if (!f)
		return 0;
	fseek(f, 0, SEEK_END);
	s = ftell(f);
	fclose(f);
	return s;
}

FILE *SafeOpen (char *filename, char mode[])
{
  FILE	*f;

  f = fopen(filename, mode);

  if (!f)
    Error ("SafeOpen %s: %s",filename,strerror(errno));

  return f;
}


void SafeRead (FILE *f, void *buffer, int count)
{
  if ( fread (buffer, 1, count, f) != (size_t)count)
    Error ("File read failure");
}


void SafeWrite (FILE *f, void *buffer, int count)
{
  if (fwrite (buffer, 1, count, f) != (size_t)count)
    Error ("File write failure");
}

/*
==============
LoadFile
==============
*/

int LoadFile(char *filename, byte **bufferptr)
{
  FILE *f;
  int length;
  byte *buffer;

  f = SafeOpen(filename, "rb");
  length = Q_filelength(f);
  buffer = (byte *)mem_alloc(length+1);
  ((char *)buffer)[length] = 0;
  SafeRead (f, buffer, length);
  fclose(f);

  *bufferptr = buffer;
  return length;
}


/*
==============
LoadFileUnsafe
==============
*/

size_t LoadFileUnsafe(char *filename, byte **bufferptr)
{
  FILE *f;
  size_t length;
  byte *buffer;

  f = fopen(filename, "rb");
  if (!f)
  {
	  *bufferptr = NULL;
	  return 0;
  }
  length = Q_filelength (f);
  buffer = (byte *)mem_alloc(length+1);
  ((char *)buffer)[length] = 0;
  SafeRead (f, buffer, length);
  fclose(f);
  *bufferptr = buffer;
  return length;
}

void DefaultPath (char *path, char *basepath)
{
  char    temp[128];

  if (path[0] == '/' || path[0] == '\\')
    return; // absolute path location
  strcpy (temp,path);
  strcpy (path,basepath);
  strcat (path,temp);
}

void ReplaceExtension (char *path, char *oldextension, char *replacementextension, char *missingextension)
{
	char    *src;
	// if path has a .EXT, replace it with extension
	// if path doesn't have a .EXT, append extension
	// (extension should include the .)

	for (src = path + strlen(path) - 1;src >= path && *src != '/' && *src != '\\' && *src != ':';src--)
	{
		if (*src == '.')
		{
			if (!oldextension || !Q_strncasecmp(src, oldextension, strlen(oldextension)))
			{
				*src = 0;
				strcat (path, replacementextension);
			}
			return;
		}
	}

	strcat (path, missingextension);
}

void DefaultExtension (char *path, const char *extension, size_t size_path)
{
	const char *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strlcat (path, extension, size_path);
}

/*
====================
Extract file parts
====================
*/
void ExtractFilePath (char *path, char *dest)
{
  char *src;

  src = path + strlen(path) - 1;

  // back up until a \ or the start
  while (src != path && (*(src-1) != '/' && *(src-1) != '\\'))
    src--;
  if (path == dest)
	*src = 0;
  else
  {
	 memcpy(dest, path, src-path);
	 dest[src-path] = 0;
  }
}

void ExtractFileBase (char *path, char *dest)
{
  char *src, *ext = NULL;

  src = path + strlen(path) - 1;

  //
  // back up until a \ or the start
  //
  while(src != path && (*(src-1) != '/' && *(src-1) != '\\'))
    src--;
  while(*src)
  {
	if (*src == '.')
		ext = dest;
	*dest++ = *src++;
  }
  if (ext)
	*ext = 0;
  else
	*dest = 0;
}

void ExtractFileName(char *path, char *dest)
{
  char *src;

  src = path + strlen(path) - 1;
  // back up until a \ or the start
  while (src != path && (*(src-1) != '/' && *(src-1) != '\\'))
	src--;
  while(*src)
	*dest++ = *src++;
  *dest = 0;
}

void ExtractFileSuffix(char *path, char *dest, char suffix_sym)
{
	char *src;

	src = path + strlen(path) - 1;
	// back up until a _ or the start
	while (src != path && *(src-1) != suffix_sym)
		src--;
	if (src == path)
	{
		*dest = 0;
		return;
	}
	strcpy(dest, src);
}

void StripFileExtension(char *path, char *dest)
{
	int l;

	l = strlen(path) - 1;
	while(l >= 0 && path[l] != '.')
		l--;
	if (l == 0)
		return;
	if (path == dest)
		path[l] = 0;
	else
		strlcpy(dest, path, l+1);
}

void ExtractFileExtension (char *path, char *dest)
{
  char    *src;

  src = path + strlen(path) - 1;

  //
  // back up until a . or the start
  //
  while (src != path && *(src-1) != '.')
    src--;
  if (src == path)
    {
      *dest = 0;	// no extension
      return;
    }

  strcpy (dest,src);
}

void AddSuffix(char *outpath, char *inpath, char *suffix)
{
	char basename[MAX_FPATH], ext[128];

	StripFileExtension(inpath, basename);
	ExtractFileExtension(inpath, ext);
	if (ext[0])
		sprintf(outpath, "%s%s.%s", basename, suffix, ext);
	else
		sprintf(outpath, "%s%s", basename, suffix);
}

bool FileExists(char *filename) 
{
	struct stat fileinfo;
	int statsucc;

	statsucc = stat(filename, &fileinfo);
	if (statsucc == 0)
		return true;
	return false;
}

/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex (char *hex)
{
	char *str;
	int num;

	num = 0;
	str = hex;
	while (*str && *str != '\n')
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			Error ("Bad hex number: '%s'", hex);
		str++;
	}
	return num;
}

int ParseNum (char *str)
{
  if (str[0] == '$')
    return ParseHex (str+1);
  if (str[0] == '0' && str[1] == 'x')
    return ParseHex (str+2);
  return atol (str);
}



/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

int ReadInt(byte *buffer)
{
	int i;
	memcpy(&i, buffer, 4);
	return i;
}

unsigned int ReadUInt(byte *buffer)
{
	return buffer[3]*16777216 + buffer[2]*65536 + buffer[1]*256 + buffer[0];
}

unsigned int ReadUShort(byte *buffer)
{
	return buffer[1]*256 + buffer[0];
}

int ReadShort(byte *buffer)
{
	union {byte b[2]; signed short f;} in;
	in.b[0] = buffer[0];
	in.b[1] = buffer[1];
	return in.f;
}

int ReadSignedByte(byte *buffer)
{
	return (signed char)buffer[0];
}

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

short   LittleShort (short l)
{
  byte    b1,b2;

  b1 = l&255;
  b2 = (l>>8)&255;

  return (b1<<8) + b2;
}

short   BigShort (short l)
{
  return l;
}


short   LittleShort (short l)
{
  byte    b1,b2;

  b1 = l&255;
  b2 = (l>>8)&255;

  return (b1<<8) + b2;
}
int    LittleLong (int l)
{
  byte    b1,b2,b3,b4;

  b1 = l&255;
  b2 = (l>>8)&255;
  b3 = (l>>16)&255;
  b4 = (l>>24)&255;

  return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    BigLong (int l)
{
  return l;
}


float	LittleFloat (float l)
{
  union {byte b[4]; float f;} in, out;

  in.f = l;
  out.b[0] = in.b[3];
  out.b[1] = in.b[2];
  out.b[2] = in.b[1];
  out.b[3] = in.b[0];

  return out.f;
}

float	BigFloat (float l)
{
  return l;
}


#else


short   BigShort (short l)
{
  byte    b1,b2;

  b1 = l&255;
  b2 = (l>>8)&255;

  return (b1<<8) + b2;
}

short   LittleShort (short l)
{
  return l;
}


int    BigLong (int l)
{
  byte    b1,b2,b3,b4;

  b1 = l&255;
  b2 = (l>>8)&255;
  b3 = (l>>16)&255;
  b4 = (l>>24)&255;

  return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    LittleLong (int l)
{
  return l;
}

float	BigFloat (float l)
{
  union {byte b[4]; float f;} in, out;

  in.f = l;
  out.b[0] = in.b[3];
  out.b[1] = in.b[2];
  out.b[2] = in.b[1];
  out.b[3] = in.b[0];

  return out.f;
}


#endif

//=======================================================
// File wrapping routines
// used to write files into temporary place and query their contents afterwards
//=======================================================

#define FILE_START_SIZE 1048576 // 1 meg
bool wrapfilestomem;

// wrapped file struct
typedef struct wrapfile_s
{
	char realname[MAX_FPATH];
	unsigned int filesize;
	struct wrapfile_s *next;
	FILE *f;
}wrapfile_t;

// wrapped file linkchain
wrapfile_t *wrapfiles = NULL;

// free all wrapped file buffers
void FreeWrappedFiles()
{
	wrapfile_t *current, *next;

	current = wrapfiles;
	while(current != NULL)
	{
		next = current->next;
		fclose(current->f);
		mem_free(current);
		current = next;
	}
	wrapfiles = NULL;
}

// return count of wrapped files
int CountWrappedFiles()
{
	wrapfile_t *wrapf;
	int c = 0;

	for (wrapf = wrapfiles; wrapf != NULL; wrapf = wrapf->next)
		c++;
	return c;
}

// retrieve contents of wrapped file
int LoadWrappedFile(int wrapnum, byte **bufferptr, char **realfilename)
{
	wrapfile_t *wrapf;
	byte *buffer;
	int c = 0;

	// find wrapfile
	for (wrapf = wrapfiles; wrapf != NULL; wrapf = wrapf->next, c++)
		if (c == wrapnum)
			break;

	// if not found - generate error
	if (wrapf == NULL)
		Error("LoadWrappedFile: can open file on index %i", wrapnum);

	// open file
	buffer = (byte *)mem_alloc(wrapf->filesize+1);
	fseek(wrapf->f, 0, SEEK_SET);
	SafeRead(wrapf->f, buffer, wrapf->filesize);
	((char *)buffer)[wrapf->filesize] = 0;
	*bufferptr = buffer;
	*realfilename = wrapf->realname;
	return wrapf->filesize;
}

// setup fiels wrapping
void WrapFileWritesToMemory()
{
	FreeWrappedFiles();
	wrapfilestomem = true;
}

// open file stream for writing
FILE *SafeOpenWrite (char *filename)
{
	FILE		*f;
	wrapfile_t	*wrapf, *current;
	char path[MAX_FPATH];

	// check if opening file that was wrapped
	for (wrapf = wrapfiles; wrapf != NULL; wrapf = wrapf->next)
		if (!strcmp(filename, wrapf->realname))
			return wrapf->f;

	// automatically make directory structure
	ExtractFilePath(filename, path);
	// open file or stream
	if (!wrapfilestomem)
	{
		CreatePath(path);
		f = fopen(filename, "wb");
		if (!f)
			Error("Error opening real file %s: %s",filename,strerror(errno));
	}
	else
	{
		// link to chain
		wrapf = (wrapfile_t *)mem_alloc(sizeof(wrapfile_t));
		wrapf->filesize = 0;
		wrapf->next = NULL;
		strcpy(wrapf->realname, filename);
		if (!wrapfiles)
			wrapfiles = wrapf;
		else
		{
			current = wrapfiles;
			while(current->next != NULL)
				current = current->next;
			current->next = wrapf;
		}
		// return a tempfile
		wrapf->f = tmpfile();
		f = wrapf->f;
		if (!f)
			Error("Error opening temp file %s: %s",filename,strerror(errno));
	}
	return f;
}

// open stream  for readind and writing
FILE *OpenReadWrite(char *filename)
{
	wrapfile_t	*wrapf;

	// check if opening file that was wrapped
	for (wrapf = wrapfiles; wrapf != NULL; wrapf = wrapf->next)
		if (!strcmp(filename, wrapf->realname))
			return wrapf->f;

	// open normal
	return fopen(filename, "r+b");
}

// wrapped fclose function, as wrapped files should not be closed on fclose, but once wrapping code received them
// also read the filesize on wrapped close
void WriteClose(FILE *f)
{
	wrapfile_t	*wrapf;

	// find in wrapped chain and bail if it is in
	for (wrapf = wrapfiles; wrapf != NULL; wrapf = wrapf->next)
	{
		if (wrapf->f == f)
		{
			wrapf->filesize = ftell(f);
			return;
		}
	}
	fclose(f);
}

//=======================================================

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM

#define CRC_INIT_VALUE	0xffff
#define CRC_XOR_VALUE	0x0000

static unsigned short crctable[256] =
{
  0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
  0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
  0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
  0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
  0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
  0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
  0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
  0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
  0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
  0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
  0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
  0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
  0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
  0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
  0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
  0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
  0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
  0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
  0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
  0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
  0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
  0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
  0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
  0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
  0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
  0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
  0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
  0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
  0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
  0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
  0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
  0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

void CRC_Init(unsigned short *crcvalue)
{
  *crcvalue = CRC_INIT_VALUE;
}

void CRC_ProcessByte(unsigned short *crcvalue, byte data)
{
  *crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data];
}

unsigned short CRC_Value(unsigned short crcvalue)
{
  return crcvalue ^ CRC_XOR_VALUE;
}

//=======================================================

/* Return a 32-bit CRC of the contents of the buffer. */
unsigned int crc_tab[256];
bool crc_initialized = false;

void crc32_init()
{
	unsigned long crc, poly;
	int i, j;

	poly = 0xEDB88320L;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 8; j > 0; j--)
		{
			if (crc & 1)
			{
				crc = (crc >> 1) ^ poly;
			}
			else
			{
				crc >>= 1;
			}
		}
		crc_tab[i] = crc;
	}

	crc_initialized = true;
}

unsigned int crc32(unsigned char *block, unsigned int length)
{
   register unsigned long crc;
   unsigned long i;

   if (!crc_initialized)
	   crc32_init();

   crc = 0xFFFFFFFF;
   for (i = 0; i < length; i++)
   {
      crc = ((crc >> 8) & 0x00FFFFFF) ^ crc_tab[(crc ^ *block++) & 0xFF];
   }
   return (crc ^ 0xFFFFFFFF);
}