/*
 *  wkcconfig.h
 *
 *  Copyright(c) 2009-2021 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCCONFIG_H_
#define _WKCCONFIG_H_

#define WTF_PLATFORM_WKC 1

// for exporting WKC / peer APIs

#if defined(_WIN32)
# define _WKC_SYMBOL_EXPORT  __declspec(dllexport)
# define _WKC_SYMBOL_IMPORT  __declspec(dllimport)
# define _WKC_SYMBOL_PRIVATE
#elif defined(__GNUC__) && (__GNUC__ >= 4)
# define _WKC_SYMBOL_EXPORT  __attribute__ ((visibility("default")))
# define _WKC_SYMBOL_IMPORT  __attribute__ ((visibility("default")))
# define _WKC_SYMBOL_PRIVATE __attribute__ ((visibility("hidden")))
#elif defined(_ARMCC_VERSION)
# define _WKC_SYMBOL_EXPORT  __declspec(dllexport)
# define _WKC_SYMBOL_IMPORT  __declspec(dllimport)
# define _WKC_SYMBOL_PRIVATE
#else
# define _WKC_SYMBOL_EXPORT
# define _WKC_SYMBOL_IMPORT
# define _WKC_SYMBOL_PRIVATE
#endif

#ifndef WKC_API
# ifndef BUILDING_WKC_STATIC
#  ifdef BUILDING_WKC_API
#   define WKC_API _WKC_SYMBOL_EXPORT
#  else
#   define WKC_API _WKC_SYMBOL_IMPORT
#  endif
# else
#  define WKC_API
# endif
#endif

#ifndef WKC_PEER_API
# ifndef BUILDING_WKC_STATIC
#  ifdef BUILDING_WKC_PEER
#   define WKC_PEER_API _WKC_SYMBOL_EXPORT
#  else
#   define WKC_PEER_API _WKC_SYMBOL_IMPORT
#  endif
# else
#  define WKC_PEER_API
# endif
#endif

// force-inline
#if defined(__CC_ARM) || defined(__ARMCC__) || defined(__ARMCC_VERSION)
# define WKC_FORCEINLINE __forceinline
#elif defined (__GNUC__) && !defined(__MINGW32__)
# define WKC_FORCEINLINE inline __attribute__((__always_inline__))
#else
# define WKC_FORCEINLINE inline
#endif

#if defined(__CC_ARM) || defined(__ARMCC__) || defined(__ARMCC_VERSION)
# define WKC_RESTRICT __restrict
#else
# define WKC_RESTRICT __restrict
#endif

#if defined(__CC_ARM) || defined(__ARMCC__) || defined(__ARMCC_VERSION)
# define WKC_ALIGN __align(4)
#else
# define WKC_ALIGN
#endif

// booleans
#ifndef __cplusplus
  #ifndef _MSC_VER
    #ifndef bool
      typedef unsigned int bool;
    #endif
    #ifndef true
      #define true 1
    #endif
    #ifndef false
      #define false 0
    #endif
  #else
    #define bool int
  #endif
#endif

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

// inttypes
typedef unsigned int wkc_uint32;
typedef int wkc_int32;
typedef unsigned long long wkc_uint64;
typedef long long wkc_int64;

// offscreens 
#define WKC_USE_OFFSCREEN_POLYGON  0
#define WKC_USE_OFFSCREEN_HW_ACCELERATION 0

// margin of stack overflow (Note: This is heuristic and needs readjustment for core upgrade.)
#define WKC_STACK_MARGIN_DEFAULT                            (16 * 1024)

// I18n
//#define WKC_ENABLE_I18N_IDN_SUPPORT 1

// memory
#include "wkcmemoryconfig.h"

// accelerated compositing
#define WKC_MAX_LAYERS 1024

// cairo
#define USE_WKC_CAIRO 1

// cairo-filter
// enable below to gain rendering speed (and lower quality of rendering image)
//#define WTF_USE_WKC_CAIRO_FILTER_NEAREST 1

// CustomJS
#if defined(__linux__)
#define WKC_ENABLE_CUSTOMJS 1
#endif

// disable platform's mutex / condition_variable
#ifdef _MSC_VER
# define _MUTEX_
# define _CONDITION_VARIABLE_
#elif defined __clang__
#define _LIBCPP_MUTEX
#define _LIBCPP_CONDITION_VARIABLE
#endif

#define WKC_RELEASE_LOG_DISABLED 1

#endif /* _WKCCONFIG_H_ */
