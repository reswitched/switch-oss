/*
 *  WKCMemoryInfo.h
 *
 *  Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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

/** @brief Flag to determine which of SpanList::normal and SpanList::returned is referred */
enum {
    /** @brief Refer SpanList::normal */
    FASTMALLCSTATISTICS_SPANLIST_TYPE_NORMAL   = 0x0001,
    /** @brief Refer SpanList::returned */
    FASTMALLCSTATISTICS_SPANLIST_TYPE_RETURNED = 0x0002,
    /** @brief Refer both of SpanList::normal and SpanList::returned */
    FASTMALLCSTATISTICS_SPANLIST_TYPE_ALL      = (FASTMALLCSTATISTICS_SPANLIST_TYPE_NORMAL | FASTMALLCSTATISTICS_SPANLIST_TYPE_RETURNED)
};

/** @brief Structure for storing information of an used memory block in a span */
struct UsedMemoryInfo_ {
    /** @brief Top address of a block */
    void* adr;
    /** @brief Requested size */
    unsigned int requestSize;
    /** @brief Acutually used size */
    unsigned int usedSize;
    /** @brief Class ID */
    unsigned short classID;
    /** @brief  */
    bool outOfRange;
};
/** @brief Type definition of WKC::Heap::UsedMemoryInfo */
typedef struct UsedMemoryInfo_ UsedMemoryInfo;

/** @brief Structure for storing data of a span */
struct SpanInfo_ {
    /** @brief Top address of the span itself
        @attention
        Don't access this member.
    */
    void* span;
    /** @brief Top address of pages managed in this span */
    void* head;
    /** @brief Bottom address of pages managed in this span */
    void* tail;
    /** @brief Address set to Span::objects if Span::objects has a valid address */
    void* nextAddress;
    /** @brief Flag whether this span is used */
    bool used;
    /** @brief Amount of pages (equal to Span::length) */
    unsigned short pages;
    /** @brief Class ID */
    unsigned short classID;
    /** @brief Size of a single block managed in this span (bytes)
        @attention
        Value is valid only when classID > 0.
    */
    unsigned int blockSize;
    /** @brief Amount of used blocks (equal to Span::refcount) */
    unsigned int usedBlocks;
    /** @brief Amount of max blocks in this span */
    unsigned int maxBlocks;
    /** @brief Size of memory managed in this span (equal to (pages * PageSize)) */
    unsigned int size;
    /** @brief Sum of requested size at allocation */
    unsigned int requestedSize;
    /** @brief Size of array of usedMemArray and UsedMemPtrArray */
    unsigned int numUsedMemArray;
    /** @brief Pointer to data of memory usage information
        @attention
        Don't access this member.
    */
    UsedMemoryInfo* usedMemArray;
    /** @brief Pointer to a pointer to data of memory usage information */
    UsedMemoryInfo** usedMemPtrArray;
};
/** @brief Type definition of WKC::Heap::SpanInfo */
typedef struct SpanInfo_ SpanInfo;

/** @brief Structure for storing memory usage information of a span */
struct MemoryInfo_ {
    /** @brief Page size set in TCMalloc */
    unsigned int pageSize;
    /** @brief Size of arrays of spanArray and spanPtrArray */
    unsigned int numSpanArray;
    /** @brief Pointer to a SpanInfo
        @attention
        Don't access this member.
    */
    SpanInfo* spanArray;
    /** @brief Pointer to a pointer to a SpanInfo */
    SpanInfo** spanPtrArray;
};
/** @brief Type definition of WKC::Heap::MemoryInfo */
typedef struct MemoryInfo_ MemoryInfo;

/** @brief Structure for storing actual data of a single stack frame */
struct StackObjectInfo_ {
    /** @brief Address of a function */
    void* adr;
    /** @brief Line number */
    unsigned int line;
    /** @brief (not used) */
    unsigned int displacement;
    /** @brief Function name */
    unsigned char name[kMaxFileNameLength + 1];
};

/** @brief Type definition of WKC::Heap::StackObjectInfo */
typedef struct StackObjectInfo_ StackObjectInfo;

/** @brief Type definition of WKC::Heap::StackTraceInfo */
typedef struct StackTraceInfo_ StackTraceInfo;

/** @brief Structure for storing data of a single stack frame (one element of doubly linked list) */
struct StackTraceInfo_ {
    /** @brief Pointer to data (StackObjectInfo) of this item */
    StackObjectInfo* obj;
    /** @brief Pointer to the next item */
    StackTraceInfo* next;
    /** @brief Pointer to the previous item */
    StackTraceInfo* prev;
};

/** @brief Type definition of WKC::Heap::MemoryLeakInfo */
typedef struct MemoryLeakInfo_ MemoryLeakInfo;

/** @brief Structure for storing data of a single memory leak (one element of doubly linked list) */
struct MemoryLeakInfo_ {
    /** @brief Top address of leaked memory */
    void* adr;
    /** @brief Size of leaked memory */
    unsigned int size;
    /** @brief Requested size when allocated */
    unsigned int reqSize;
    /** @brief Time when allocated */
    time_t curTime;
    /** @brief Pointer to the next item */
    MemoryLeakInfo* next;
    /** @brief Pointer to the previous item */
    MemoryLeakInfo* prev;
};

/** @brief Type definition of WKC::Heap::MemoryLeakNode */
typedef struct MemoryLeakNode_ MemoryLeakNode;

/** @brief Structure for storing a single set of memory leak data (one element of doubly linked list) */
struct MemoryLeakNode_ {
    /** @brief Amount of MemoryLeakInfo */
    unsigned int numInfo;
    /** @brief Pointer to the beginning of MemoryLeakInfo list */
    MemoryLeakInfo* memHead;
    /** @brief Pointer to the end of MemoryLeakInfo list */
    MemoryLeakInfo* memTail;

    /** @brief Amount of StackTraceInfo */
    unsigned int numTrace;
    /** @brief Pointer to the beginning of StackTraceInfo list */
    StackTraceInfo* stHead;
    /** @brief Pointer to the end of StackTraceInfo list */
    StackTraceInfo* stTail;

    /** @brief Pointer to the next item */
    MemoryLeakNode* next;
    /** @brief Pointer to the previous item */
    MemoryLeakNode* prev;
};

/** @brief Structure for storing a list of memory leak data */
struct MemoryLeakRoot_ {
    /** @brief Pointer to the beginning of MemoryLeakNode list */
    MemoryLeakNode* head;
    /** @brief Pointer to the end of MemoryLeakNode list */
    MemoryLeakNode* tail;
    /** @brief Amount of MemoryLeakNodes */
    unsigned int num;
    /** @brief Total size of requested memory (bytes) */
    unsigned int leakReqSum;
    /** @brief Total size of actually consumed memory (bytes) */
    unsigned int leakSum;
};

/** @brief Type definition of WKC::Heap::MemoryLeakRoot */
typedef struct MemoryLeakRoot_ MemoryLeakRoot;


/** @brief Structure for storing memory usage statistics of engine heap (TCMalloc) */
struct Statistics_ {
    /** @brief Consuming size */
    size_t heapSize;
    /** @brief Total size of free memory in CentralCache and ThreadCache */
    size_t freeSizeInHeap;
    /** @brief Total size of free memory in TCMalloc_PageHeap */
    size_t freeSizeInCaches;
    /** @brief Size of memory freed by TCMalloc_SystemRelease() */
    size_t returnedSize;
    /** @brief Page size set in TCMalloc */
    size_t pageSize;
    /** @brief Size of max free block in TCMalloc_PageHeap */
    size_t maxFreeBlockSizeInHeap;
    /** @brief Total size of free memory in CentralCache and ThreadCache of the class ID specified by classInCaches */
    size_t classFreeSizeInCaches;
    /** @brief Size of memory block of the class ID specified by classInCaches */
    size_t classBlockSizeInCaches;
    /** @brief Amount of pages required to allocate requested size of memory (see details of GetStatistics()) */
    unsigned int pageLength;
    /** @brief Class ID corresponding to the requested size (see details of GetStatistics()) */
    unsigned int classInCaches;
    /** @brief Amount of classes */
    size_t numClasses;
    /** @brief Amount of pages assigned to each class ID */
    size_t* eachClassAssignedPagesInCaches;
    /** @brief Size of free memory in ThreadCache of each class ID */
    size_t* eachClassThreadFreeSizeInCaches;
    /** @brief Size of free memory in CentralCache of each class ID */
    size_t* eachClassCentralFreeSizeInCaches;
    /** @brief Size of memory block of each class ID */
    size_t* eachClassBlockSizeInCaches;
    /** @brief  */
    size_t maxPages;
    /** @brief Size of free memory in each page */
    size_t* eachPageFreeSizeInHeap;
    /** @brief Size of array of largeFreeSizeInHeap */
    unsigned int numLargeFreeSize;
    /** @brief Max size of heap */
    size_t maxHeapSize;
    /** @brief Size of free memory in combined blocks of pages over pageSize */
    size_t* largeFreeSizeInHeap;
    /** @brief Current heap usage (See note) */
    size_t currentHeapUsage;
    /** @brief Current physical memory usage */
    size_t currentPhysicalMemoryUsage;
    /** @brief Maximum heap usage (See note) */
    size_t maxHeapUsage;
    size_t allocFailureCount;
    size_t allocFailureMinSize;
    size_t allocFailureTotalSize;
      /* - note -
       * It does not include the amount of heap allocated to the cache in this calculation.
       * The reason is as follows:
       * - Cache size is small.
       * - Calculation process is heavy.
       */
};
/** @brief Type definition of WKC::Heap::Statistics */
typedef struct Statistics_ Statistics;

/** @brief Type definition of WKC::Heap::MemoryLeakDumpProc */
typedef void (*MemoryLeakDumpProc)(void *in_ctx);

/**
@brief Gets memory usage statistics of engine heap
@param stat Reference to a Statistics object, in which data at the call time is set
@param requestSize Requested bytes to be allocated
@return None
@details
requestSize is used to calculate classInCaches and pageLength members of Statistics struct.@n
This function is usually for debugging purpose.
*/
WKC_API void GetStatistics(Statistics& stat, size_t requestSize = 0);

WKC_API size_t GetStatisticsFreeSizeInHeap();
WKC_API size_t GetStatisticsMaxFreeBlockSizeInHeap(size_t requestSize = 0);
WKC_API size_t GetStatisticsCurrentPhysicalMemoryUsage();

/**
@brief Enables / disables memory map function of engine heap
@param in_set Enables / disables
@retval "!= false" Setting succeeded
@retval "== false" Setting failed
@details
Memory map is for debugging purpose.
*/
WKC_API bool EnableMemoryMap(bool in_set);
/**
@brief Gets memory map data from engine heap at the call time
@param memInfo Reference to a MemoryInfo object, in which data at the call time is set
@param needUsedMemory Sets / won't set data to usedMemPtrArray, a member of MemoryInfo
@return None
@attention
- Frequent calls will cause low performance.
- Don't call this function in critical sections. Otherwise, wrong data will be provided.
*/
WKC_API void GetMemoryMap(MemoryInfo& memInfo, bool needUsedMemory = true);
/**
@brief Allocates needed memory for given MemoryInfo object
@param memInfo Reference to a MemoryInfo object
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Memory allocated by this function must be freed using ReleaseMemoryMap().
*/
WKC_API bool AllocMemoryMap(MemoryInfo& memInfo);
/**
@brief Frees memory in given MemoryInfo object
@param memInfo Reference to a MemoryInfo object
@return None
*/
WKC_API void ReleaseMemoryMap(MemoryInfo& memInfo);

/**
@brief Sets a callback function to dump memory leak data
@param in_proc MemoryLeakDumpProc object (function pointer)
@param in_ctx Pointer to an object, provided as an argument to in_proc
@return None
*/
WKC_API void SetMemoryLeakDumpProc(MemoryLeakDumpProc in_proc, void* in_ctx);
/**
@brief Gets memory leak data at the call time
@param leakRoot Reference to a MemoryLeakRoot object, in which data at the call time is set
@param resolveSymbol
- != false Add function name and line number to callstack information
- == false Don't add function name and line number to callstack information (only address of each function)
@retval "!= false" Succeeded
@retval "== false" Failed
@details
If both of WKC_ENABLE_FASTMALLOC_STACK_TRACE and WKC_FASTMALLOC_WIN_STACK_TRACE are defined,
callstack information additionally has function name and line number.
@attention
Don't call this function in critical sections. Otherwise, wrong data will be provided.
*/
WKC_API bool GetMemoryLeakInfo(MemoryLeakRoot& leakRoot, bool resolveSymbol = false);
/**
@brief Clears stack trace information
@return None
*/
WKC_API void ClearStackTrace();
/**
@brief Clear data in given MemoryLeakRoot object
@param leakRoot Reference to a MemoryLeakRoot object
@return None
@note
This function doesn't free any memory.
*/
WKC_API void ReleaseMemoryLeakInfo(MemoryLeakRoot& leakRoot);

/**
@brief Gets amount of browser engine heap available
@retval size_t Size of total free memory (bytes)
*/
WKC_API size_t GetAvailableSize();
/**
@brief Gets maximum size of browser engine heap that can be allocated
@retval size_t Maximum size that can be allocated (bytes)
*/
WKC_API size_t GetMaxAvailableBlockSize();
/**
@brief Returns whether the request size of memory can be allocatable
@param inRequestSize Request bytes
@param outAvailSize Available size (see details section)
@param outCheckPeer (obsolete)
@param outRealRequestSize Request size adjusted due to memory management reasons
@param outAvailPeerSize Size of the largest contiguous memory in engine heap
@retval "!= false" Allocatable
@retval "== false" Not allocatable
@details
For outAvailSize parameter,
- If inRequestSize > kMaxSize, this value is the size of the largest contiguous memory in TCMalloc_PageHeap.
- If inRequestSize <= kMaxSize, this value is the size of total free memory in CentralCache and ThreadCache of the class ID determined by inRequestSize.
@attention
Don't call this function in critical sections.
*/
WKC_API bool CanAllocMemory(size_t inRequestSize, size_t* outAvailSize = NULL, bool* outCheckPeer = NULL, size_t* outRealRequestSize = NULL, size_t* outAvailPeerSize = NULL);


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
