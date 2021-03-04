/*
 *  wkcclibwrapper.h
 *
 *  Copyright(c) 2010-2017 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCCLIBWRAPPER_H_
#define _WKCCLIBWRAPPER_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <wkc/wkcclib.h>

#ifdef __BUILDING_IN_VS__

#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
// #define abort    wkc_abort
#define exit     wkc_exit

#define gettimeofday(tv,tz)   wkc_gettimeofday((tv),(tz))
#define nanosleep(req,rem)    wkc_nanosleep((req),(rem))
#define usleep(t)             wkc_usleep((t))
#define sleep(a)              wkc_usleep((a)*1000)

#endif // __BUILDING_IN_VS__


#ifdef __ARMCC_VERSION
/* for __arm__ compiler */

#define time    wkc_time
#define strdup  wkc_strdup

#define fopen   wkc_fopen
#define fclose  wkc_fclose
#define ferror  wkc_ferror
#define fflush  wkc_fflush
#define fgetc   wkc_fgetc
#define fputc   wkc_fputc
#define fread   wkc_fread
#define fseek   wkc_fseek
#define ftell   wkc_ftell
#define fwrite  wkc_fwrite

#define localtime   wkc_localtime
#define localtime_r wkc_localtime_r
#define mktime      wkc_mktime
#define strftime    wkc_strftime

#define gettimeofday(tv,tz)  wkc_gettimeofday((tv),(tz))
#define nanosleep(req,rem)   wkc_nanosleep((req),(rem))
#define usleep(usec)         wkc_usleep((usec))

#ifdef __cplusplus
namespace std {
extern "C" time_t wkc_time(time_t*);
}
#endif

#endif  // __ARMCC_VERSION

#ifdef __aarch64__

#define time    wkc_time
#define localtime   wkc_localtime
#define localtime_r wkc_localtime_r
#define mktime      wkc_mktime
#define strftime    wkc_strftime

#ifdef __cplusplus
namespace std {
extern "C" time_t wkc_time(time_t*);
}
#endif

#endif  // __aarch64__

#ifdef __cplusplus
}
#endif

#endif // _WKCCLIBWRAPPER_H_

/////////////////////////////////////////////////
// Following are just sample
//   DONT define WKCCLIB_CONVERSION_SAMPLE
//
#ifdef WKCCLIB_CONVERSION_SAMPLE

#define strtod wkc_strtod
#define strdup wkc_strdup
#define strcmp wkc_strcmp
#define strcmpi wkc_strcmpi
#define strcasecmp wkc_strcasecmp
#define stricmp wkc_stricmp
#define strlen wkc_strlen
#define strcpy wkc_strcpy
#define strncpy wkc_strncpy
#define strstr wkc_strstr
#define strncmp wkc_strncmp
#define strncasecmp wkc_strncasecmp
#define strchr wkc_strchr
#define strrchr wkc_strrchr
#define strtok wkc_strtok
#define strtol wkc_strtol
#define strtoul wkc_strtoul
#define strcat wkc_strcat
#define strftime wkc_strftime
#define strncat wkc_strncat

#define memcpy wkc_memcpy
#define memmove wkc_memmove
#define memset wkc_memset
#define memcmp wkc_memcmp

#define malloc wkc_malloc
#define calloc wkc_calloc
#define realloc wkc_realloc
#ifndef __XML_PARSER_H__
#define free wkc_free
#endif

#define fclose wkc_fclose
#define ferror wkc_ferror
#define fflush wkc_fflush
#define fgetc wkc_fgetc
#define fopen wkc_fopen
#define fputc wkc_fputc
#define fread wkc_fread
#define fseek wkc_fseek
#define ftell wkc_ftell
#define fwrite wkc_fwrite
#define tmpfile wkc_tmpfile
#ifdef wchar_t
#define _wfopen wkc_wfopen
#endif
#define puts wkc_puts
#define read wkc_read
#define sscanf wkc_sscanf
#define write wkc_write

#define getenv wkc_getenv

#define qsort wkc_qsort

#ifndef isalnum 
#define isalnum wkc_isalnum
#endif
#ifndef isalpha
#define isalpha wkc_isalpha 
#endif
#ifndef isascii
#define isascii wkc_isascii 
#endif
#ifndef isblank
#define isblank wkc_isblank 
#endif
#ifndef iscntrl
#define iscntrl wkc_iscntrl 
#endif
#ifndef isdigit
#define isdigit wkc_isdigit 
#endif
#ifndef isgraph
#define isgraph wkc_isgraph 
#endif
#ifndef islower 
#define islower wkc_islower 
#endif
#ifndef isprint
#define isprint wkc_isprint 
#endif
#ifndef ispunct
#define ispunct wkc_ispunct 
#endif
#ifndef isspace
#define isspace wkc_isspace 
#endif
#ifndef isupper
#define isupper wkc_isupper 
#endif
#ifndef isxdigit
#define isxdigit wkc_isxdigit
#endif
#ifndef toascii
#define toascii wkc_toascii 
#endif
#ifndef tolower
#define tolower wkc_tolower 
#endif
#ifndef toupper
#define toupper wkc_toupper 
#endif


#define srand
#ifdef RAND_MAX
#undef RAND_MAX
#endif
#define RAND_MAX			32767
#define rand wkc_rand

#define atol wkc_atol
#define atoi wkc_atoi
#define mbstowcs wkc_mbstowcs
#define wcstombs wkc_wcstombs
#define abort wkc_abort
#define exit wkc_exit
#define wcslen wkc_wcslen
#define mblen wkc_mblen

#ifdef __BUILDING_IN_VS__
#define snprintf wkc_snprintf
#endif
#define printf wkc_printf
#define fprintf wkc_fprintf
#define sprintf wkc_sprintf

#define difftime wkc_difftime
#define gmtime wkc_gmtime
#define localtime wkc_localtime
#define localtime_r wkc_localtime_r
#define mktime wkc_mktime
#define time wkc_time

#endif  // WKCCLIB_CONVERSION_SAMPLE

//  sample end
/////////////////////////////////////////////////
