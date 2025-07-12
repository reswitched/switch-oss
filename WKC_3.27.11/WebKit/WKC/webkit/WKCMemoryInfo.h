/*
 *  WKCMemoryInfo.h
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef _WKCMEMORYINFO_H_
#define _WKCMEMORYINFO_H_

// Note: This file must be synchronized with FastMallocWKC.h

#include <time.h>
#include <WKC/wkcconfig.h>

#ifdef MAX_PATH
#undef MAX_PATH
#endif
#define MAX_PATH  260

#define kMaxFileNameLength MAX_PATH

#define WKC_FASTMALLOC_HOWMANY(a, b) (((unsigned int)(a) + ((unsigned int)(b - 1))) / (unsigned int)(b))

/*@{*/
/** @brief Namespace of API layer of NetFront Browser NX WKC */
namespace WKC {

/*@{*/
/** @brief Namespace of API layer of NetFront Browser NX WKC Heap */
namespace Heap {

/**
@brief Gets a numeric property of engine heap
@details
See the explanation of GetNumericProperty in WebKitLibraries/TCMalloc/src/gperftools/malloc_extension.h
*/
WKC_API bool GetNumericProperty(const char* property, size_t* value);

/**
@brief Sets a numeric property of engine heap
@details
See the explanation of SetNumericProperty in WebKitLibraries/TCMalloc/src/gperftools/malloc_extension.h
*/
WKC_API bool SetNumericProperty(const char* property, size_t value);

/**
@brief Gets size of memory allocatad for JavaScript heap from engine heap
@return size_t Size of memory allocatad for JavaScript heap (bytes)
*/
WKC_API size_t GetJSHeapAllocatedBlockBytes();

/**
@brief Gets size of memory allocatad for JavaScript JIT code (executable area)
@param allocated_bytes Allocated size (bytes)
@param total_bytes Total size of executable area (bytes)
@param max_allocatable_bytes Max size of allocatable memory in executable area (bytes)
@return None
@details
The param total_bytes will be useful if the target environment has a limited area allowed to be executable only there.@n
The param max_allocatable_bytes means size of the largest contiguous memory.@n
*/
WKC_API void GetJSJITCodePageAllocatedBytes(size_t& allocated_bytes, size_t& total_bytes, size_t& max_allocatable_bytes);

/**
@brief Sets default capacity of JavaScript register file
@param size size of default capacity
@details
Sets default capacity of JavaScript register file
*/
WKC_API void SetJSRegisterFileDefaultCapacity(unsigned int size);

} // namespace Heap
/*@}*/

} // namespace WKC
/*@}*/

#endif // _WKCMEMORYINFO_H_
