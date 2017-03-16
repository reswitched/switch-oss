/*
 *  wkcheappeer.h
 *
 *  Copyright(c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_HEAP_PEER_H_
#define _WKC_HEAP_PEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>
#include <wkc/wkcmpeer.h>

WKC_BEGIN_C_LINKAGE

/**
@file
@brief Do not change this file since it operates with the current implementation.
*/

/*@{*/

typedef struct WKCHeapStatistics_ {
    size_t heapSize;
    size_t freeSizeInHeap;
    size_t freeSizeInCaches;
    size_t returnedSize;
    size_t pageSize;                                        // Default Page Size in TCMalloc
    size_t maxFreeBlockSizeInHeap;                          // Maximum Available Block Size in Free Array and Large List for Large Memories
    size_t classFreeSizeInCaches;                           // Available Free Size in Thread Cache and Central Cache
    size_t classBlockSizeInCaches;                          // Block Size in each class
    unsigned int pageLength;                                // Page Length shared with Small and Large Memories
    unsigned int classInCaches;                             // Class ID for Small Memories
    size_t numClasses;                          // Number of the classes of Cache 
    size_t* eachClassAssignedPagesInCaches;     // Assigned Pages for Small Memories
    size_t* eachClassThreadFreeSizeInCaches;    // Free Array in Thread Cache for Small Memories
    size_t* eachClassCentralFreeSizeInCaches;   // Central Cache Array for Small Memories
    size_t* eachClassBlockSizeInCaches;         // Block Size for Small Memories
    size_t maxPages;                            // Max pages of Large Memories
    size_t* eachPageFreeSizeInHeap;               // Free Array for Large Memories
    unsigned int numLargeFreeSize;                          // Number of used array
    size_t maxHeapSize;
    size_t* largeFreeSizeInHeap;              // Large List for Large Memories (Translated into array)
    size_t currentHeapUsage;
    size_t maxHeapUsage;
} WKCHeapStatistics;

WKC_PEER_API void* wkcHeapCallocPeer(size_t p,size_t n);
WKC_PEER_API void wkcHeapFreePeer(void* p);
WKC_PEER_API void* wkcHeapMallocPeer(size_t size);
WKC_PEER_API size_t wkcHeapMallocSizePeer(const void* ptr);
WKC_PEER_API void* wkcHeapReallocPeer(void* old_ptr, size_t new_size);
WKC_PEER_API char* wkcHeapStrDupPeer(const char* src);
WKC_PEER_API void* wkcHeapZeroedMallocPeer(size_t n);
WKC_PEER_API void wkcHeapForbidPeer();
WKC_PEER_API void wkcHeapAllowPeer();
WKC_PEER_API void wkcHeapInitializePeer(void* in_memory, size_t in_memory_size);
WKC_PEER_API void wkcHeapFinalizePeer();
WKC_PEER_API void wkcHeapForceTerminatePeer();
WKC_PEER_API void wkcHeapResetMaxHeapUsagePeer();
WKC_PEER_API size_t wkcHeapGetHeapTotalSizePeer();
WKC_PEER_API size_t wkcHeapGetHeapAvailableSizePeer();
WKC_PEER_API size_t wkcHeapGetHeapMaxAvailableBlockSizePeer();
WKC_PEER_API size_t wkcHeapGetHeapTotalSizeForExecutableRegionPeer();
WKC_PEER_API size_t wkcHeapGetHeapAvailableSizeForExecutableRegionPeer();
WKC_PEER_API size_t wkcHeapGetHeapMaxAvailableBlockSizeForExecutableRegionPeer();
WKC_PEER_API void wkcHeapStatisticsPeer(WKCHeapStatistics& statistics, size_t request_size = 0);
WKC_PEER_API size_t wkcHeapStatisticsFreeSizeInHeapPeer();
WKC_PEER_API size_t wkcHeapStatisticsMaxFreeBlockSizeInHeapPeer(size_t requestSize = 0);
WKC_PEER_API bool wkcHeapCanAllocMemoryPeer(size_t in_request_size, size_t* out_avail_size, bool* out_checkpeer, size_t* out_realrequest_size, size_t* out_availpeer_size);
WKC_PEER_API void* wkcHeapMallocAlignedPeer(size_t align, size_t size, bool writable, bool executable);
WKC_PEER_API void wkcHeapFreeAlignedPeer(void* p, size_t size);
WKC_PEER_API void* wkcHeapReserveUncommittedPeer(size_t size, bool writable, bool executable);
WKC_PEER_API void* wkcHeapReserveAndCommitPeer(size_t size, bool writable, bool executable);
WKC_PEER_API void wkcHeapReleaseDecommittedPeer(void* ptr, size_t size);
WKC_PEER_API void wkcHeapCommitPeer(void* ptr, size_t size, bool writable, bool executable);
WKC_PEER_API void wkcHeapDecommitPeer(void* ptr, size_t size);

WKC_PEER_API size_t wkcHeapGetPageSizePeer(void);
WKC_PEER_API void wkcHeapCacheFlushPeer(void* in_ptr, size_t in_size);

#ifndef WKC_HEAP_PEER_COMPILE_ASSERT
#define WKC_HEAP_PEER_COMPILE_ASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1]
#endif

/*@}*/

WKC_END_C_LINKAGE

#endif /* _WKC_HEAP_PEER_H_ */
