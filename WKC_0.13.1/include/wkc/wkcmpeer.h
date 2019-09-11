/*
 *  wkcmpeer.h
 *
 *  Copyright(c) 2011-2018 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_MEMORY_PEER_H_
#define _WKC_MEMORY_PEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>

/**
@file
@brief Do not change this file since it operates with the current implementation.
*/

/*@{*/

WKC_BEGIN_C_LINKAGE

#ifndef WKC_MPEER_COMPILE_ASSERT
#define WKC_MPEER_COMPILE_ASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1]
#endif

// memory peer
typedef void* (*wkcPeerMallocProc) (unsigned int, int);
typedef void  (*wkcPeerFreeProc) (void*);
typedef void* (*wkcPeerReallocProc) (void*, unsigned int, int);
typedef void* (*wkcPeerAlignedMallocProc) (unsigned int, unsigned int);
typedef void  (*wkcPeerAlignedFreeProc) (void*);

WKC_PEER_API bool wkcMemoryInitializePeer(wkcPeerMallocProc in_malloc, wkcPeerFreeProc in_free, wkcPeerReallocProc in_realloc, wkcPeerAlignedMallocProc in_aligned_malloc, wkcPeerAlignedFreeProc in_aligned_free);
WKC_PEER_API bool wkcMemoryIsInitializedPeer(void);
WKC_PEER_API void wkcMemoryFinalizePeer(void);
WKC_PEER_API void wkcMemoryForceTerminatePeer(void);

WKC_PEER_API void wkcMemorySetNotifyNoMemoryProcPeer(void*(*in_proc)(unsigned int));
WKC_PEER_API void wkcMemorySetNotifyMemoryAllocationErrorProcPeer(void(*in_proc)(unsigned int, int));
WKC_PEER_API void wkcMemorySetNotifyCrashProcPeer(void(*in_proc)(const char*, int, const char*, const char*));
WKC_PEER_API void wkcMemorySetNotifyStackOverflowProcPeer(void(*in_proc)(bool, unsigned int, unsigned int, unsigned int, void*, void*, void*, const char*, int, const char*));
WKC_PEER_API void wkcMemorySetCheckMemoryAllocatableProcPeer(bool(*in_proc)(unsigned int, int));

WKC_PEER_API void* wkcMemoryNotifyNoMemoryPeer(unsigned int in_size);
WKC_PEER_API void wkcMemorySetCrashReasonPeer(const char* in_file, int in_line, const char* in_function, const char* in_assertion);
WKC_PEER_API void wkcMemoryNotifyCrashPeer(void);
WKC_PEER_API void wkcMemoryNotifyStackOverflowPeer(bool in_need_restart, unsigned int in_margin, unsigned int in_consumption, void* in_current_stack_top, const char* in_file, int in_line, const char* in_function);
WKC_PEER_API bool wkcMemoryIsStackOverflowPeer(unsigned int in_margin, unsigned int *out_consumption, void** out_current_stack_top);

WKC_PEER_API void wkcMemorySetAllocatingForImagesPeer(bool in_flag);
WKC_PEER_API void wkcMemorySetAllocatingForSVGPeer(bool in_flag);
WKC_PEER_API void wkcMemorySetAllocationForAnimatedImagePeer(bool in_flag);
WKC_PEER_API bool wkcMemoryIsAllocatingForSVGPeer();
WKC_PEER_API int wkcMemoryGetAllocationStatePeer();

WKC_PEER_API bool wkcMemoryIsCrashingPeer(void);

enum {
    WKC_MEMORYALLOC_TYPE_IMAGE,
    WKC_MEMORYALLOC_TYPE_ANIMATED_IMAGE,
    WKC_MEMORYALLOC_TYPE_JAVASCRIPT,
    WKC_MEMORYALLOC_TYPE_IMAGE_SHAREDBUFFER,
    WKC_MEMORYALLOC_TYPE_LAYER,
    WKC_MEMORYALLOC_TYPE_PIXMAN,
    WKC_MEMORYALLOC_TYPE_JAVASCRIPT_HEAP,
    WKC_MEMORYALLOC_TYPE_ASSEMBLERBUFFER,
    WKC_MEMORYALLOC_TYPES
};
WKC_PEER_API bool wkcMemoryCheckMemoryAllocatablePeer(unsigned int in_size, int in_reason);
WKC_PEER_API void wkcMemoryNotifyMemoryAllocationErrorPeer(unsigned int in_size, int in_reason);

WKC_PEER_API void wkcMemoryRegisterGlobalObjPeer(volatile void* in_ptr, int in_size);

/* for peer_fastmalloc */

WKC_PEER_API void wkcMemoryInitializeVirtualMemoryPeer(void* in_memory, size_t in_physical_memory_size, size_t in_virtual_memory_size);
WKC_PEER_API bool wkcMemoryCommitPeer(void* ptr, size_t size, bool writable, bool executable);
WKC_PEER_API void wkcMemoryDecommitPeer(void* ptr, size_t size);
WKC_PEER_API size_t wkcMemoryGetPageSizePeer(void);
WKC_PEER_API size_t wkcMemoryGetCurrentPhysicalMemoryUsagePeer(void);
WKC_PEER_API void wkcMemorySetExecutablePeer(void* in_ptr, size_t in_size, bool in_executable);
WKC_PEER_API void wkcMemoryCacheFlushPeer(void* in_ptr, size_t in_size);

WKC_PEER_API void wkcMemoryReadWriteBarrierPeer(void);
WKC_PEER_API void wkcMemoryBarrierPeer(void);

WKC_END_C_LINKAGE

/*@}*/

#endif /* _WKC_MEMORY_PEER_H_ */
