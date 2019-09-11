/*
 *  wkcclib.h
 *
 *  Copyright(c) 2009-2017 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCCLIB_H_
#define _WKCCLIB_H_

#include <wkc/wkcbase.h>

/**
   @file
   @brief clib wrappers
*/
/*@{*/

WKC_BEGIN_C_LINKAGE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __ARMCC_VERSION
#include <sys/stat.h>
#endif

#ifdef _WIN64
#define ssize_t __int64
#elif (defined(__LP64__) && __LP64__) || defined(__aarch64__)
#define ssize_t long long
#else
#define ssize_t int
#endif

/**
@brief Standard C library abs() function
@param i Conforms to standard C library abs()
@return Conforms to standard C library abs()
@attention
The behavior must be the same as the standard C library abs() function.
*/
WKC_PEER_API int wkc_abs(int i);
/**
@brief Standard C library strdup() function
@param string Conforms to standard C library strdup()
@return Conforms to standard C library strdup()
@attention
- The behavior must be the same as the standard C library strdup() function.
- wkc_malloc() must be used for allocating memory.
*/
WKC_PEER_API char * wkc_strdup(const char * string);
/**
@brief Standard C library strcmp() function
@param str1 Conforms to standard C library strcmp()
@param str2 Conforms to standard C library strcmp()
@return Conforms to standard C library strcmp()
@attention
The behavior must be the same as the standard C library strcmp() function.
*/
WKC_PEER_API int wkc_strcmp(const char * str1, const char * str2);
/**
@brief Equivalent to Windows C runtime library strcmpi()
@param str1 Conforms to Windows C runtime library strcmpi()
@param str2 Conforms to Windows C runtime library strcmpi()
@return Conforms to Windows C runtime library strcmpi()
@attention
The behavior must be the same as the Windows C runtime library strcmpi() function.
*/
WKC_PEER_API int wkc_strcmpi(const char * str1, const char * str2);
/**
@brief POSIX strcasecmp() function
@param str1 Conforms to POSIX strcasecmp()
@param str2 Conforms to POSIX strcasecmp()
@return Conforms to POSIX strcasecmp()
@attention
The behavior must be the same as the POSIX strcasecmp() function.
*/
WKC_PEER_API int wkc_strcasecmp(const char * str1, const char * str2);
/**
@brief Equivalent to Windows C runtime library _stricmp()
@param str1 Conforms to Windows C runtime library _stricmp()
@param str2 Conforms to Windows C runtime library _stricmp()
@return Conforms to Windows C runtime library _stricmp()
@attention
The behavior must be the same as the Windows C runtime library _stricmp() function.
*/
WKC_PEER_API int wkc_stricmp(const char * str1, const char * str2);
/**
@brief Standard C library strlen() function
@param str Conforms to standard C library strlen()
@return Conforms to standard C library strlen()
@attention
The behavior must be the same as the standard C library strlen() function.
*/
WKC_PEER_API size_t wkc_strlen(const char * str);
/**
@brief Standard C library strcpy() function
@param dest Conforms to standard C library strcpy()
@param src Conforms to standard C library strcpy()
@return Conforms to standard C library strcpy()
@attention
The behavior must be the same as the standard C library strcpy() function.
*/
WKC_PEER_API char * wkc_strcpy(char * dest, const char * src);
/**
@brief Standard C library strncpy() function
@param strDest Conforms to standard C library strncpy()
@param strSource Conforms to standard C library strncpy()
@param count Conforms to standard C library strncpy()
@return Conforms to standard C library strncpy()
@attention
The behavior must be the same as the standard C library strncpy() function.
*/
WKC_PEER_API char * wkc_strncpy(char * strDest, const char * strSource, size_t count);
/**
@brief Standard C library strstr() function
@param pszHaystack Conforms to standard C library strstr()
@param pszNeedle Conforms to standard C library strstr()
@return Conforms to standard C library strstr()
@attention
The behavior must be the same as the standard C library strstr() function.
*/
WKC_PEER_API char * wkc_strstr(const char * pszHaystack, const char * pszNeedle);
/**
@brief Standard C library strncmp() function
@param a Conforms to standard C library strncmp()
@param b Conforms to standard C library strncmp()
@param length Conforms to standard C library strncmp()
@return Conforms to standard C library strncmp()
@attention
The behavior must be the same as the standard C library strncmp() function.
*/
WKC_PEER_API int wkc_strncmp(const char *a, const char *b, size_t length );
/**
@brief POSIX strncasecmp() function
@param str1 Conforms to POSIX strncasecmp()
@param str2 Conforms to POSIX strncasecmp()
@param length Conforms to POSIX strncasecmp()
@return Conforms to POSIX strncasecmp()
@attention
The behavior must be the same as the POSIX strncasecmp() function.
*/
WKC_PEER_API int wkc_strncasecmp(const char * str1, const char * str2, size_t length);
/**
@brief Standard C library strchr() function
@param string Conforms to standard C library strchr()
@param c Conforms to standard C library strchr()
@return Conforms to standard C library strchr()
@attention
The behavior must be the same as the standard C library strchr() function.
*/
WKC_PEER_API char *wkc_strchr( const char *string, int c);
/**
@brief Standard C library strrchr() function
@param string Conforms to standard C library strrchr()
@param c Conforms to standard C library strrchr()
@return Conforms to standard C library strrchr()
@attention
The behavior must be the same as the standard C library strrchr() function.
*/
WKC_PEER_API char *wkc_strrchr( const char *string, int c );
/**
@brief Standard C library strtod() function
@param pszFloat Conforms to standard C library strtod()
@param ppszEnd Conforms to standard C library strtod()
@return Conforms to standard C library strtod()
@attention
The behavior must be the same as the standard C library strtod() function.
*/
WKC_PEER_API double wkc_strtod(const char *pszFloat, char ** ppszEnd);
/**
@brief Standard C library strtok() function
@param strToken Conforms to standard C library strtok()
@param strDelimit Conforms to standard C library strtok()
@return Conforms to standard C library strtok()
@attention
The behavior must be the same as the standard C library strtok() function.
*/
WKC_PEER_API char *wkc_strtok( char *strToken, const char *strDelimit );
/**
@brief Standard C library strtol() function
@param nptr Conforms to standard C library strtol()
@param endptr Conforms to standard C library strtol()
@param base Conforms to standard C library strtol()
@return Conforms to standard C library strtol()
@attention
The behavior must be the same as the standard C library strtol() function.
*/
WKC_PEER_API long int wkc_strtol(const char * nptr, char * * endptr, int base);
/**
@brief Standard C library strtoul() function
@param nptr Conforms to standard C library strtoul()
@param endptr Conforms to standard C library strtoul()
@param base Conforms to standard C library strtoul()
@return Conforms to standard C library strtoul()
@attention
The behavior must be the same as the standard C library strtoul() function.
*/
WKC_PEER_API unsigned long  wkc_strtoul( const char *nptr, char **endptr, int base );
/**
@brief Standard C library strcat() function
@param dest Conforms to standard C library strcat()
@param src Conforms to standard C library strcat()
@return Conforms to standard C library strcat()
@attention
The behavior must be the same as the standard C library strcat() function.
*/
WKC_PEER_API char * wkc_strcat(char * dest, const char * src);
/**
@brief Standard C library strftime() function
@param dest Conforms to standard C library strftime()
@param maxsize Conforms to standard C library strftime()
@param format Conforms to standard C library strftime()
@param timeptr Conforms to standard C library strftime()
@return Conforms to standard C library strftime()
@attention
The behavior must be the same as the standard C library strftime() function.
*/
WKC_PEER_API size_t wkc_strftime(char *dest, size_t maxsize, const char *format, const struct tm *timeptr);
/**
@brief Standard C library strncat() function
@param dest Conforms to standard C library strncat()
@param src Conforms to standard C library strncat()
@param len Conforms to standard C library strncat()
@return Conforms to standard C library strncat()
@attention
The behavior must be the same as the standard C library strncat() function.
*/
WKC_PEER_API char * wkc_strncat(char * dest, const char * src, int len);

/**
@brief wkc_strdup() function that supports wide characters
@param s Wide strings
@retval !=0 Pointer to duplicated wide strings
@retval ==0 Failed"
@details
The behavior is basically the same as wkc_strdup(), and wide strings are used for arguments and return values respectively.
@attention
wkc_malloc() must be used for allocating memory.
*/
WKC_PEER_API unsigned short* wkc_wstrdup(const unsigned short *s);
/**
@brief Duplicates wide strings.
@param s Wide strings
@param n Maximum length of strings to duplicate
@retval !=0 Pointer to duplicated wide strings
@retval ==0 Failed
@details
The behavior is basically equivalent to wkc_wstrdup(), however this function duplicates up to n characters.
@attention
wkc_malloc() must be used for allocating memory.
*/
WKC_PEER_API unsigned short* wkc_wstrndup(const unsigned short *s, unsigned int n);
/**
@brief Standard C library wcslen() function
@param s Conforms to standard C library wcslen()
@return Conforms to standard C library wcslen()
@attention
The behavior must be the same as the standard C library wcslen() function.
*/
WKC_PEER_API int wkc_wstrlen(const unsigned short *s);
/**
@brief Copies wide strings
@param dest Pointer to area to which wide strings are copied
@param destmax Number of characters of area to which wide strings are copied (including terminal symbol)
@param src Pointer to wide strings to be copied
@param srclen Length of strings to be copied
@details
Copies wide characters of up to destmax, including terminal L'\0'.@n
It can be assumed that memory area that can store destmax or more characters is allocated for dest.
*/
WKC_PEER_API void wkc_wstrncpy(unsigned short* dest, unsigned int destmax, const unsigned short* src, unsigned int srclen);

/**
@brief Standard C library memcpy() function
@param dest Conforms to standard C library memcpy()
@param src Conforms to standard C library memcpy()
@param count Conforms to standard C library memcpy()
@return Conforms to standard C library memcpy()
@attention
The behavior must be the same as the standard C library memcpy() function.
*/
WKC_PEER_API void * wkc_memcpy(void * dest, const void * src, size_t count);
/**
@brief Standard C library memmove() function
@param dest Conforms to standard C library memmove()
@param src Conforms to standard C library memmove()
@param count Conforms to standard C library memmove()
@return Conforms to standard C library memmove()
@attention
The behavior must be the same as the standard C library memmove() function.
*/
WKC_PEER_API void * wkc_memmove(void * dest, const void * src, size_t count);
/**
@brief Standard C library memset() function
@param dest Conforms to standard C library memset()
@param b Conforms to standard C library memset()
@param count Conforms to standard C library memset()
@return Conforms to standard C library memset()
@attention
The behavior must be the same as the standard C library memset() function.
*/
WKC_PEER_API void * wkc_memset(void * dest, int b, size_t count);
/**
@brief Standard C library memcmp() function
@param p1 Conforms to standard C library memcmp()
@param p2 Conforms to standard C library memcmp()
@param count Conforms to standard C library memcmp()
@return Conforms to standard C library memcmp()
@attention
The behavior must be the same as the standard C library memcmp() function.
*/
WKC_PEER_API int wkc_memcmp(const void * p1, const void * p2, size_t count);

#ifdef _CRTDBG_MAP_ALLOC
#define wkc_malloc malloc
#define wkc_calloc calloc
#define wkc_realloc realloc
#define wkc_malloc_crashonfailure malloc
#define wkc_calloc_crashonfailure calloc
#define wkc_realloc_crashonfailure realloc
#define wkc_free free
#else
/**
@brief Standard C library malloc() function
@param dwSize Conforms to standard C library malloc()
@return Conforms to standard C library malloc()
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void * wkc_malloc(size_t dwSize);
/**
@brief Standard C library calloc() function
@param p Conforms to standard C library calloc()
@param q Conforms to standard C library calloc()
@return Conforms to standard C library calloc()
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void * wkc_calloc(size_t p, size_t q);
/**
@brief Standard C library realloc() function
@param pSrc Conforms to standard C library realloc()
@param dwSize Conforms to standard C library realloc()
@return Conforms to standard C library realloc()
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void * wkc_realloc(void * pSrc, size_t dwSize);
/**
@brief Allocates memory of size specified
@param dwSize Size of memory to allocate
@return Pointer to allocated memory area
@details
It is basically the same as wkc_malloc(), however, performs crash processing if allocating memory fails.
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void * wkc_malloc_crashonfailure(size_t dwSize);
/**
@brief Allocates memory of size specified
@param p Size of one memory block
@param q Number of memory blocks to allocate
@return Pointer to allocated memory area
@details
It is basically the same as wkc_calloc(), however, performs crash processing if allocating memory fails.
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void * wkc_calloc_crashonfailure(size_t p, size_t q);
/**
@brief Reallocates memory area that was already allocated
@param pSrc Pointer to memory area to reallocate 
@param dwSize Size of memory to change
@return Pointer to memory area after reallocation
@details
It is basically the same as wkc_realloc(), however, performs crash processing if allocating memory fails.
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void * wkc_realloc_crashonfailure(void * pSrc, size_t dwSize);
/**
@brief Standard C library free() function
@param po Conforms to standard C library free()
@return Conforms to standard C library free()
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void wkc_free(void * po);
#endif

/**
@brief Allocate memory with alignment
@param size size to allocate
@param align_size size of alignment
@return Allocated memory. The memory must be freed by wkc_free_aligned().
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void *wkc_malloc_aligned(size_t size, size_t align_size);
/**
@brief Free memory allocated by wkc_malloc_aligned()
@param ptr Pointer to free
@return None
@attention
Do not change this function since it operates with the current implementation. 
*/
WKC_PEER_API void wkc_free_aligned(void *ptr);

/**
@brief Standard C library fopen() function
@param filename Conforms to standard C library fopen()
@param mode Conforms to standard C library fopen()
@return Conforms to standard C library fopen()
@attention
The behavior must be the same as the standard C library fopen() function.
*/
WKC_PEER_API FILE   *wkc_fopen(const char *filename, const char *mode);
/**
@brief Standard C library tmpfile() function
@return Conforms to standard C library tmpfile()
@attention
The behavior must be the same as the standard C library tmpfile() function.
*/
WKC_PEER_API FILE   *wkc_tmpfile(void);
/**
@brief Standard C library fclose() function
@param stream Conforms to standard C library fclose()
@return Conforms to standard C library fclose()
@attention
The behavior must be the same as the standard C library fclose() function.
*/
WKC_PEER_API int    wkc_fclose(FILE *stream);
/**
@brief Standard C library ferror() function
@param stream Conforms to standard C library ferror()
@return Conforms to standard C library ferror()
@attention
The behavior must be the same as the standard C library ferror() function.
*/
WKC_PEER_API int    wkc_ferror(FILE *stream);
/**
@brief Standard C library fflush() function
@param stream Conforms to standard C library fflush()
@return Conforms to standard C library fflush()
@attention
The behavior must be the same as the standard C library fflush() function.
*/
WKC_PEER_API int    wkc_fflush(FILE *stream);
/**
@brief Standard C library feof() function
@param stream Conforms to standard C library feof()
@return Conforms to standard C library feof()
@attention
The behavior must be the same as the standard C library feof() function.
*/
WKC_PEER_API int    wkc_feof(FILE *stream);
/**
@brief Standard C library ferror() function
@param stream Conforms to standard C library ferror()
@return Conforms to standard C library ferror()
@attention
The behavior must be the same as the standard C library ferror() function.
*/
WKC_PEER_API int    wkc_ferror(FILE *stream);
/**
@brief POSIX fileno() function
@param stream Conforms to POSIX fileno()
@return Conforms to POSIX fileno()
@attention
The behavior must be the same as the POSIX fileno() function.
*/ 
WKC_PEER_API int    wkc_fileno(FILE *stream); 
/**
@brief Standard C library fgetc() function
@param stream Conforms to standard C library fgetc()
@return Conforms to standard C library fgetc()
@attention
The behavior must be the same as the standard C library fgetc() function.
*/
WKC_PEER_API int    wkc_fgetc(FILE *stream);
/**
@brief Standard C library fputc() function
@param c Conforms to standard C library fputc()
@param stream Conforms to standard C library fputc()
@return Conforms to standard C library fputc()
@attention
The behavior must be the same as the standard C library fputc() function.
*/
WKC_PEER_API int    wkc_fputc(int c, FILE *stream);
/**
@brief Standard C library fgets() function
@param s Conforms to standard C library fgets()
@param size Conforms to standard C library fgets()
@param stream Conforms to standard C library fgets()
@return Conforms to standard C library fgets()
@attention
The behavior must be the same as the standard C library fgets() function.
*/
WKC_PEER_API char   *wkc_fgets(char *s, int size, FILE *stream);
/**
@brief Standard C library fputs() function
@param s Conforms to standard C library fputs()
@param stream Conforms to standard C library fputs()
@return Conforms to standard C library fputs()
@attention
The behavior must be the same as the standard C library fputs() function.
*/
WKC_PEER_API int    wkc_fputs(const char *s, FILE *stream);
/**
@brief Standard C library fwrite() function
@param buffer Conforms to standard C library fwrite()
@param size Conforms to standard C library fwrite()
@param count Conforms to standard C library fwrite()
@param stream Conforms to standard C library fwrite()
@return Conforms to standard C library fwrite()
@attention
The behavior must be the same as the standard C library fwrite() function.
*/
WKC_PEER_API size_t wkc_fwrite(const void *buffer, size_t size, size_t count, FILE *stream);
/**
@brief Standard C library fread() function
@param buffer Conforms to standard C library fread()
@param size Conforms to standard C library fread()
@param count Conforms to standard C library fread()
@param stream Conforms to standard C library fread()
@return Conforms to standard C library fread()
@attention
The behavior must be the same as the standard C library fread() function.
*/
WKC_PEER_API size_t wkc_fread(void *buffer, size_t size, size_t count, FILE *stream);
/**
@brief Standard C library fseek() function
@param stream Conforms to standard C library fseek()
@param offset Conforms to standard C library fseek()
@param origin Conforms to standard C library fseek()
@return Conforms to standard C library fseek()
@attention
The behavior must be the same as the standard C library fseek() function.
*/
WKC_PEER_API int    wkc_fseek(FILE *stream, long offset, int origin);
/**
@brief Standard C library ftell() function
@param stream Conforms to standard C library ftell()
@return Conforms to standard C library ftell()
@attention
The behavior must be the same as the standard C library ftell() function.
*/
WKC_PEER_API long   wkc_ftell(FILE *stream);
/**
@brief Gets the size of file
@param filename Filename
@param size Pointer to store file size
@retval "!=false" Succeeded
@retval "==false" Failed
*/
WKC_PEER_API bool   wkc_fgetsize(const char* filename, long long* size);

#ifdef wchar_t
/**
@brief wkc_fopen() function that supports wide characters
@param filename File name (wide strings)
@param mode Type of access permission (wide strings)
@retval != 0 Pointer to file structure
@retval == 0 Failed
@details
Only the difference is that the argument is specified by wide strings, however the behavior is basically the same as wkc_fopen().
*/
WKC_PEER_API FILE   *wkc_wfopen(const wchar_t *filename, const wchar_t *mode);
#endif
/**
@brief POSIX mkdir() function
@param path Conforms to POSIX mkdir()
@param mode Conforms to POSIX mkdir()
@return Conforms to POSIX mkdir()
@attention
The behavior must be the same as the POSIX mkdir() function.
*/
WKC_PEER_API int    wkc_mkdir(const char* path, int mode);

/**
@brief Standard C library puts() function
@param string Conforms to standard C library puts()
@return Conforms to standard C library puts()
@attention
The behavior must be the same as the standard C library puts() function.
*/
WKC_PEER_API int     wkc_puts(const char *string);
/**
@brief Standard C library gets() function
@param s Conforms to standard C library gets()
@return Conforms to standard C library gets()
@attention
The behavior must be the same as the standard C library gets() function.
*/
WKC_PEER_API char    *wkc_gets(char *s);
/**
@brief Standard C library sscanf() function
@param buffer Conforms to standard C library sscanf()
@param format Conforms to standard C library sscanf()
@return Conforms to standard C library sscanf()
@attention
The behavior must be the same as the standard C library sscanf() function.
*/
WKC_PEER_API int     wkc_sscanf(const char *buffer, const char *format, ... );
/**
@brief POSIX read() function
@param handle Conforms to POSIX read()
@param buffer Conforms to POSIX read()
@param count Conforms to POSIX read()
@return Conforms to POSIX read()
@attention
The behavior must be the same as the POSIX read() function.
*/
WKC_PEER_API ssize_t wkc_read(int handle, void *buffer, unsigned int count);
/**
@brief POSIX write() function
@param handle Conforms to POSIX write()
@param buffer Conforms to POSIX write()
@param count Conforms to POSIX write()
@return Conforms to POSIX write()
@attention
The behavior must be the same as the POSIX write() function.
*/
WKC_PEER_API ssize_t wkc_write(int handle, const void *buffer, unsigned int count);

/**
@brief Standard C library getenv() function
@param varname Conforms to standard C library getenv()
@return Conforms to standard C library getenv()
@attention
The behavior must be the same as the standard C library getenv() function.
*/
WKC_PEER_API char *wkc_getenv(const char *varname);
/**
@brief POSIX gethostname() function
@param name Conforms to POSIX gethostname()
@param len Conforms to POSIX gethostname()
@return Conforms to POSIX gethostname()
@attention
The behavior must be the same as the POSIX gethostname() function.
*/ 
WKC_PEER_API int  wkc_gethostname(char *name, size_t len); 

// sys/stat.h
#ifdef __ARMCC_VERSION
typedef long  off_t;
typedef unsigned long int dev_t;
typedef unsigned long int ino_t;
typedef unsigned int      mode_t;
typedef unsigned int      nlink_t;
typedef unsigned int      uid_t;
typedef unsigned int      gid_t;
typedef long int          blkcnt_t;
typedef long int          blksize_t;

#ifndef __ARMCC_VERSION
struct stat {
    dev_t     st_dev;
    ino_t     st_ino;
    mode_t    st_mode;
    nlink_t   st_nlink;
    uid_t     st_uid;
    gid_t     st_gid;
    dev_t     st_rdev;
    off_t     st_size;
    blksize_t st_blksize;
    blkcnt_t  st_blocks;
    time_t    st_atime;
    time_t    st_mtime;
    time_t    st_ctime;
};
#endif
#else
struct stat;
#endif

typedef int pid_t;
/**
@brief POSIX getpid() function
@return Conforms to POSIX getpid()
@attention
The behavior must be the same as the POSIX getpid() function.
*/
WKC_PEER_API pid_t wkc_getpid(void);
/**
@brief POSIX getppid() function
@return Conforms to POSIX getppid()
@attention
The behavior must be the same as the POSIX getppid() function.
*/
WKC_PEER_API pid_t wkc_getppid(void);

/**
@brief POSIX open() function
@param pathname Conforms to POSIX open()
@param flags Conforms to POSIX open()
@return Conforms to POSIX open()
@attention
The behavior must be the same as the POSIX open() function.
*/
WKC_PEER_API int   wkc_open(const char *pathname, int flags, ...);
/**
@brief POSIX close() function
@param fd Conforms to POSIX close()
@return Conforms to POSIX close()
@attention
The behavior must be the same as the POSIX close() function.
*/
WKC_PEER_API int   wkc_close(int fd);
/**
@brief POSIX stat() function
@param path Conforms to POSIX stat()
@param buf Conforms to POSIX stat()
@return Conforms to POSIX stat()
@attention
The behavior must be the same as the POSIX stat() function.
*/
WKC_PEER_API int   wkc_stat(const char *path, struct stat *buf);
/**
@brief POSIX fstat() function
@param fd Conforms to POSIX fstat()
@param buf Conforms to POSIX fstat()
@return Conforms to POSIX fstat()
@attention
The behavior must be the same as the POSIX fstat() function.
*/ 
WKC_PEER_API int   wkc_fstat(int fd, struct stat *buf); 
/**
@brief POSIX lseek() function
@param fd Conforms to POSIX lseek()
@param offset Conforms to POSIX lseek()
@param whence Conforms to POSIX lseek()
@return Conforms to POSIX lseek()
@attention
- The behavior must be the same as the POSIX lseek() function.
- In Windows environment, the types of offset and return value are long, respectively.
*/
#if defined(__ARMCC_VERSION)
WKC_PEER_API off_t wkc_lseek(int fd, off_t offset, int whence);
#elif defined(__BUILDING_IN_VS__)
WKC_PEER_API long wkc_lseek(int fd, long offset, int whence);
#endif

/**
@brief Standard C library qsort() function
@param base Conforms to standard C library qsort()
@param nmemb Conforms to standard C library qsort()
@param size Conforms to standard C library qsort()
@param compar Conforms to standard C library qsort()
@return Conforms to standard C library qsort()
@attention
The behavior must be the same as the standard C library qsort() function.
*/
WKC_PEER_API void wkc_qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

/**
@brief Standard C library rand() function
@return Conforms to standard C library rand()
@attention
The behavior must be the same as the standard C library rand() function.
*/
WKC_PEER_API int wkc_rand(void);

/**
@brief Standard C library isalnum() function
@param c Conforms to standard C library isalnum()
@return Conforms to standard C library isalnum()
@attention
The behavior must be the same as the standard C library isalnum() function.
*/
WKC_PEER_API int wkc_isalnum(int c);
/**
@brief Standard C library isalpha() function
@param c Conforms to standard C library isalpha()
@return Conforms to standard C library isalpha()
@attention
The behavior must be the same as the standard C library isalpha() function.
*/
WKC_PEER_API int wkc_isalpha(int c);
/**
@brief POSIX isascii() function
@param c Conforms to POSIX isascii()
@return Conforms to POSIX isascii()
@attention
The behavior must be the same as the POSIX isascii() function.
*/
WKC_PEER_API int wkc_isascii(int c);
/**
@brief Standard C library isblank() function
@param c Conforms to standard C library isblank()
@return Conforms to standard C library isblank()
@attention
The behavior must be the same as the standard C library isblank() function.
*/
WKC_PEER_API int wkc_isblank(int c);
/**
@brief Standard C library iscntrl() function
@param c Conforms to standard C library iscntrl()
@return Conforms to standard C library iscntrl()
@attention
The behavior must be the same as the standard C library iscntrl() function.
*/
WKC_PEER_API int wkc_iscntrl(int c);
/**
@brief Standard C library isdigit() function
@param c Conforms to standard C library isdigit()
@return Conforms to standard C library isdigit()
@attention
The behavior must be the same as the standard C library isdigit() function.
*/
WKC_PEER_API int wkc_isdigit(int c);
/**
@brief Standard C library isgraph() function
@param c Conforms to standard C library isgraph()
@return Conforms to standard C library isgraph()
@attention
The behavior must be the same as the standard C library isgraph() function.
*/
WKC_PEER_API int wkc_isgraph(int c);
/**
@brief Standard C library islower() function
@param c Conforms to standard C library islower()
@return Conforms to standard C library islower()
@attention
The behavior must be the same as the standard C library islower() function.
*/
WKC_PEER_API int wkc_islower(int c);
/**
@brief Standard C library isprint() function
@param c Conforms to standard C library isprint()
@return Conforms to standard C library isprint()
@attention
The behavior must be the same as the standard C library isprint() function.
*/
WKC_PEER_API int wkc_isprint(int c);
/**
@brief Standard C library ispunct() function
@param c Conforms to standard C library ispunct()
@return Conforms to standard C library ispunct()
@attention
The behavior must be the same as the standard C library ispunct() function.
*/
WKC_PEER_API int wkc_ispunct(int c);
/**
@brief Standard C library isspace() function
@param c Conforms to standard C library isspace()
@return Conforms to standard C library isspace()
@attention
The behavior must be the same as the standard C library isspace() function.
*/
WKC_PEER_API int wkc_isspace(int c);
/**
@brief Standard C library isupper() function
@param c Conforms to standard C library isupper()
@return Conforms to standard C library isupper()
@attention
The behavior must be the same as the standard C library isupper() function.
*/
WKC_PEER_API int wkc_isupper(int c);
/**
@brief Standard C library isxdigit() function
@param c Conforms to standard C library isxdigit()
@return Conforms to standard C library isxdigit()
@attention
The behavior must be the same as the standard C library isxdigit() function.
*/
WKC_PEER_API int wkc_isxdigit(int c);
/**
@brief Standard C library toascii() function
@param c Conforms to standard C library toascii()
@return Conforms to standard C library toascii()
@attention
The behavior must be the same as the standard C library toascii() function.
*/
WKC_PEER_API int wkc_toascii(int c);
/**
@brief Standard C library tolower() function
@param c Conforms to standard C library tolower()
@return Conforms to standard C library tolower()
@attention
The behavior must be the same as the standard C library tolower() function.
*/
WKC_PEER_API int wkc_tolower(int c);
/**
@brief Standard C library toupper() function
@param c Conforms to standard C library toupper()
@return Conforms to standard C library toupper()
@attention
The behavior must be the same as the standard C library toupper() function.
*/
WKC_PEER_API int wkc_toupper(int c);

#if defined(WIN_32)
__declspec(noreturn) void abort(void);
__declspec(noreturn) void exit(int);
#endif

/**
@brief Standard C library abort() function
@return Conforms to standard C library abort()
@attention
The behavior must be the same as the standard C library abort() function.
*/
WKC_PEER_API void wkc_abort(void);
#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
/**
@brief Standard C library exit() function

@return Conforms to standard C library exit()
@attention
The behavior must be the same as the standard C library exit() function.
*/
WKC_PEER_API void wkc_exit(int);

/**
@brief Standard C library atol() function
@param a Conforms to standard C library atol()
@return Conforms to standard C library atol()
@attention
The behavior must be the same as the standard C library atol() function.
*/
WKC_PEER_API long wkc_atol(const char* a);
/**
@brief Standard C library atoi() function
@param a Conforms to standard C library atoi()
@return Conforms to standard C library atoi()
@attention
The behavior must be the same as the standard C library atoi() function.
*/
WKC_PEER_API int wkc_atoi(const char* a);

/**
@brief Standard C library mblen() function
@param mbstr Conforms to standard C library mblen()
@param count Conforms to standard C library mblen()
@return Conforms to standard C library mblen()
@attention
The behavior must be the same as the standard C library mblen() function.
*/
WKC_PEER_API int wkc_mblen( const char *mbstr, size_t count );

/**
@brief TBD
@param n TBD
@param buffer TBD
@param size TBD
@return TBD
*/
WKC_PEER_API int wkc_doubleToString(double n, char* buffer, size_t size);

#if defined(WIN32)
/**
@brief Standard C library snprintf() function
@param buf Conforms to standard C library snprintf()
@param len Conforms to standard C library snprintf()
@param cpszFormat Conforms to standard C library snprintf()
@return Conforms to standard C library snprintf()
@attention
The behavior must be the same as the standard C library snprintf() function.
*/
WKC_PEER_API int wkc_snprintf(char *buf, size_t len, const char *cpszFormat, ...);
#endif
/**
@brief Standard C library printf() function
@param cpszFormat Conforms to standard C library printf()
@return Conforms to standard C library printf()
@attention
The behavior must be the same as the standard C library printf() function.
*/
WKC_PEER_API int wkc_printf(const char *cpszFormat, ...);
/**
@brief Standard C library fprintf() function
@param stream Conforms to standard C library fprintf()
@param cpszFormat Conforms to standard C library fprintf()
@return Conforms to standard C library fprintf()
@attention
The behavior must be the same as the standard C library fprintf() function.
*/
WKC_PEER_API int wkc_fprintf(FILE *stream, const char *cpszFormat, ...);
/**
@brief Standard C library sprintf() function
@param buf Conforms to standard C library sprintf()
@param cpszFormat Conforms to standard C library sprintf()
@return Conforms to standard C library sprintf()
@attention
The behavior must be the same as the standard C library sprintf() function.
*/
WKC_PEER_API int wkc_sprintf(char *buf, const char *cpszFormat, ...);

#if defined(__ARMCC_VERSION)
#ifndef WKC_DECLARED_TIMEVAL
struct timeval {
    long long tv_sec;
    long tv_usec;
};
#define WKC_DECLARED_TIMEVAL
#endif
#endif //__ARMCC_VERSION

#ifndef __WKC_OMIT_DEFINE_TIMEZONE
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#else
struct timezone;
#endif

#ifndef  __WKC_OMIT_DEFINE_TIMESPEC
struct timespec {
    time_t  tv_sec;
    long    tv_nsec;
};
#endif

/**
@brief Standard C library difftime() function
@param time1 Conforms to standard C library difftime()
@param time0 Conforms to standard C library difftime()
@return Conforms to standard C library difftime()
@attention
The behavior must be the same as the standard C library difftime() function.
*/
WKC_PEER_API double wkc_difftime(time_t time1, time_t time0);
/**
@brief Sets time zone offset
@param offset Offset time in minutes. [-840,840]
@param is_summer Whether the zone is in summer time or not.
@return 0:succeed -1:failed
*/
WKC_PEER_API int wkcSetTimeZonePeer(int offset, bool is_summer);

/** 
@brief Function that is set proc for getting current time
@param out_posix_time Current time in posix time
@attention
This function must returns immediately.
*/
typedef void (*wkcGetCurrentTimeProc)(wkc_int64* out_posix_time);

WKC_PEER_API void wkcRegisterGetCurrentTimeProcPeer(wkcGetCurrentTimeProc in_proc);

/**
@brief Standard C library gmtime() function
@param timep Conforms to standard C library gmtime()
@return Conforms to standard C library gmtime()
@attention
The behavior must be the same as the standard C library gmtime() function.
*/
WKC_PEER_API struct tm *wkc_gmtime(const time_t *timep);
/**
@brief Standard C library localtime() function
@param timep Conforms to standard C library localtime()
@return Conforms to standard C library localtime()
@attention
The behavior must be the same as the standard C library localtime() function.
*/
WKC_PEER_API struct tm *wkc_localtime(const time_t *timep);
/**
@brief POSIX localtime_r() function
@param timep Conforms to POSIX localtime_r()
@param result Conforms to POSIX localtime_r()
@return Conforms to POSIX localtime_r()
@attention
The behavior must be the same as the POSIX localtime_r() function.
*/
WKC_PEER_API struct tm *wkc_localtime_r(const time_t *timep, struct tm *result);
/**
@brief Standard C library mktime() function
@param tim Conforms to standard C library mktime()
@return Conforms to standard C library mktime()
@attention
The behavior must be the same as the standard C library mktime() function.
*/
WKC_PEER_API time_t wkc_mktime(struct tm *tim);
/**
@brief Standard C library time() function
@param t Conforms to standard C library time()
@return Conforms to standard C library time()
@attention
The behavior must be the same as the standard C library time() function.
*/
WKC_PEER_API time_t wkc_time(time_t *t);
/**
@brief POSIX gettimeofday() function
@param tv Conforms to POSIX gettimeofday()
@param tz Conforms to POSIX gettimeofday()
@return Conforms to POSIX gettimeofday()
@attention
The behavior must be the same as the POSIX gettimeofday() function.
*/
WKC_PEER_API int wkc_gettimeofday(struct timeval *tv, struct timezone *tz);
/**
@brief POSIX usleep() function
@param usleep Conforms to POSIX usleep()
@return Conforms to POSIX usleep()
@attention
The behavior must be the same as the POSIX usleep() function.
*/
WKC_PEER_API int wkc_usleep(unsigned long usleep);
/**
@brief POSIX nanosleep() function
@param req Conforms to POSIX nanosleep()
@param rem Conforms to POSIX nanosleep()
@return Conforms to POSIX nanosleep()
@attention
The behavior must be the same as the POSIX nanosleep() function.
*/
WKC_PEER_API int wkc_nanosleep(const struct timespec *req, struct timespec *rem);

/**
@brief Issues an exception for interrupting processing in libpng
@attention
This function must be implemented only in an environment that does not use setjmp/longjmp.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkc_pngabort(void);

/**
@brief POSIX htonl() function
@param in_x Conforms to POSIX htonl()
@return Conforms to POSIX htonl()
@attention
POSIX The behavior must be the same as the htonl() function.
*/
WKC_PEER_API unsigned int wkc_htonl(unsigned int in_x);
/**
@brief POSIX htons() function
@param in_x Conforms to POSIX htons()
@return Conforms to POSIX htons()
@attention
POSIX The behavior must be the same as the htons() function.
*/
WKC_PEER_API unsigned int wkc_htons(unsigned int in_x);
/**
@brief POSIX ntohl() function
@param in_x Conforms to POSIX ntohl()
@return Conforms to POSIX ntohl()
@attention
POSIX The behavior must be the same as the ntohl() function.
*/
WKC_PEER_API unsigned int wkc_ntohl(unsigned int in_x);
/**
@brief POSIX ntohs() function
@param in_x Conforms to POSIX ntohs()
@return Conforms to POSIX ntohs()
@attention
POSIX The behavior must be the same as the ntos() function.
*/
WKC_PEER_API unsigned int wkc_ntohs(unsigned int in_x);
struct in_addr;
/**
@brief POSIX inet_ntoa() function
@param in_x Conforms to POSIX inet_ntoa()
@return Conforms to POSIX inet_ntoa()
@attention
POSIX The behavior must be the same as the inet_ntoa() function.
*/
WKC_PEER_API char* wkc_inet_ntoa(struct in_addr in_x);

/* for y2038 issue specials */

typedef long long wkc_time64_t;

struct wkc_timeval64 {
    wkc_time64_t tv_sec;
    unsigned long long tv_usec;
};

/**
@brief Standard C library time() function with supporting 64-bit time_t
@param t Conforms to standard C library time()
@return Conforms to standard C library time()
@attention
The behavior must be the same as the standard C library time() function.
*/
WKC_PEER_API wkc_time64_t wkc_time64(wkc_time64_t* t);
/**
@brief Standard C library gmtime() function with supporting 64-bit time_t
@param timep Conforms to standard C library gmtime()
@return Conforms to standard C library gmtime()
@attention
The behavior must be the same as the standard C library gmtime() function.
*/
WKC_PEER_API struct tm* wkc_gmtime64(const wkc_time64_t* timep);
/**
@brief Standard C library gmtime() function with supporting 64-bit time_t
@param timep Conforms to standard C library gmtime()
@param store Conforms to standard C library gmtime()
@return Conforms to standard C library gmtime()
@attention
The behavior must be the same as the standard C library gmtime() function.
*/
WKC_PEER_API struct tm* wkc_gmtime64_r(const wkc_time64_t* timep, struct tm* store);
/**
@brief POSIX gettimeofday() function with supporting 64-bit time_t
@param tv Conforms to POSIX gettimeofday()
@param tz Conforms to POSIX gettimeofday()
@return Conforms to POSIX gettimeofday()
@attention
The behavior must be the same as the POSIX gettimeofday() function.
*/
WKC_PEER_API int wkc_gettimeofday64(struct wkc_timeval64* tv, struct timezone* tz);

WKC_END_C_LINKAGE

/*@}*/

#if defined(__cplusplus) && defined(__ARMCC_VERSION)
namespace std {
WKC_PEER_API FILE   *wkc_fopen(const char *filename, const char *mode);
WKC_PEER_API int    wkc_fclose(FILE *stream);
WKC_PEER_API int    wkc_ferror(FILE *stream);
WKC_PEER_API int    wkc_fflush(FILE *stream);
WKC_PEER_API int    wkc_fgetc(FILE *stream);
WKC_PEER_API int    wkc_fputc(int c, FILE *stream);
WKC_PEER_API size_t wkc_fread(void *buffer, size_t size, size_t count, FILE *stream);
WKC_PEER_API size_t wkc_fwrite(const void *buffer, size_t size, size_t count, FILE *stream);
WKC_PEER_API int    wkc_fseek(FILE *stream, long offset, int origin);
WKC_PEER_API long   wkc_ftell(FILE *stream);
} // namespace std
#endif

#endif /* _WKCCLIB_H_ */
