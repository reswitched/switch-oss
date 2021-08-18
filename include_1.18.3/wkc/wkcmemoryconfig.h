/*
 *  wkcmemoryconfig.h
 *
 *  Copyright(c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKCMEMORYCONFIG_H_
#define _WKCMEMORYCONFIG_H_

// FastMalloc
#define WKC_ENABLE_FASTMALLOC_SMALL_CLASS_BY_TABLE 1
//#define WKC_ENABLE_REPLACEMENT_SYSTEMMEMORY 1
#define WKC_FASTMALLOC_REVERSE_ORDER_THRESHOLD  (1 * 1024 * 1024) /* bytes */

// for FastMalloc Debug
#ifdef _MSC_VER
#define WKC_ENABLE_FASTMALLOC_USED_MEMORY_INFO     1
#ifdef WKC_ENABLE_FASTMALLOC_USED_MEMORY_INFO
#define WKC_ENABLE_FASTMALLOC_STACK_TRACE          1
#endif // WKC_ENABLE_FASTMALLOC_USED_MEMORY_INFO
#endif // _MSC_VER

#endif /* _WKCMEMORYCONFIG_H_ */
