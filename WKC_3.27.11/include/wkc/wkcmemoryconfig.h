/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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
