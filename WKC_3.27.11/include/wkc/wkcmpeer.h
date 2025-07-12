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
typedef void* (*wkcPeerAlignedMallocProc) (unsigned int, unsigned int, int);
typedef void  (*wkcPeerAlignedFreeProc) (void*);

WKC_PEER_API bool wkcMemoryInitializePeer(wkcPeerMallocProc in_malloc, wkcPeerFreeProc in_free, wkcPeerReallocProc in_realloc, wkcPeerAlignedMallocProc in_aligned_malloc, wkcPeerAlignedFreeProc in_aligned_free);
WKC_PEER_API bool wkcMemoryIsInitializedPeer(void);
WKC_PEER_API void wkcMemoryFinalizePeer(void);
WKC_PEER_API void wkcMemoryForceTerminatePeer(void);

WKC_PEER_API void wkcMemorySetNotifyNoMemoryProcPeer(void(*in_proc)(size_t));
WKC_PEER_API void wkcMemorySetNotifyMemoryAllocationErrorProcPeer(void(*in_proc)(size_t, int));
WKC_PEER_API void wkcMemorySetNotifyCrashProcPeer(void(*in_proc)(const char*, int, const char*, const char*));
WKC_PEER_API void wkcMemorySetNotifyStackOverflowProcPeer(void(*in_proc)(size_t, size_t, size_t, void*, void*, void*, const char*, int, const char*));
WKC_PEER_API void wkcMemorySetCheckMemoryAllocatableProcPeer(bool(*in_proc)(size_t, int));
WKC_PEER_API void wkcMemorySetAllocateMemoryPagesProcPeer(bool(*in_proc)(void*, size_t));
WKC_PEER_API void wkcMemorySetFreeMemoryPagesProcPeer(void(*in_proc)(void*, size_t));

WKC_PEER_API void wkcMemoryNotifyNoMemoryPeer(size_t in_size);
WKC_PEER_API void wkcMemorySetCrashReasonPeer(const char* in_file, int in_line, const char* in_function, const char* in_assertion);
WKC_PEER_API void wkcMemoryNotifyCrashPeer(void);
WKC_PEER_API void wkcMemoryNotifyStackOverflowPeer(size_t in_margin, size_t in_consumption, void* in_current_stack_top, const char* in_file, int in_line, const char* in_function);
WKC_PEER_API bool wkcMemoryIsStackOverflowPeer(size_t in_margin, size_t *out_consumption, void** out_current_stack_top);
WKC_PEER_API void wkcMemorySetLongLoopDetectionPeer(wkc_uint64 startTime, wkc_uint64 limitTime, wkc_uint64* lastStartTime, wkc_uint64* lastLimitTime);
WKC_PEER_API void wkcMemoryCrashIfDetectLongLoopPeer(const char* in_file, int in_line, const char* in_function);

enum {
    WKC_MEMORYALLOC_TYPE_IMAGE,
    WKC_MEMORYALLOC_TYPE_JAVASCRIPT,
    WKC_MEMORYALLOC_TYPE_PIXMAN,
    WKC_MEMORYALLOC_TYPES
};
WKC_PEER_API bool wkcMemoryCheckMemoryAllocatablePeer(size_t in_size, int in_reason);
WKC_PEER_API void wkcMemoryNotifyMemoryAllocationErrorPeer(size_t in_size, int in_reason);

WKC_PEER_API void wkcMemoryRegisterGlobalObjPeer(void* in_ptr, size_t in_size, void* in_init);

/* for peer_fastmalloc */

WKC_PEER_API void wkcMemoryInitializeVirtualMemoryPeer(void* in_memory, size_t in_physical_memory_size, size_t in_virtual_memory_size);
WKC_PEER_API bool wkcMemoryCommitPeer(void* ptr, size_t size, bool writable, bool executable);
WKC_PEER_API void wkcMemoryDecommitPeer(void* ptr, size_t size);
WKC_PEER_API size_t wkcMemoryGetPageSizePeer(void);
WKC_PEER_API size_t wkcMemoryGetMaxPhysicalMemorySizePeer(void);
WKC_PEER_API size_t wkcMemoryGetCurrentPhysicalMemoryUsagePeer(void);
WKC_PEER_API void wkcMemorySetExecutablePeer(void* in_ptr, size_t in_size, bool in_executable);
WKC_PEER_API void wkcMemoryCacheFlushPeer(void* in_ptr, size_t in_size);

WKC_PEER_API void wkcMemoryReadWriteBarrierPeer(void);
WKC_PEER_API void wkcMemoryBarrierPeer(void);

WKC_END_C_LINKAGE

/*@}*/

#endif /* _WKC_MEMORY_PEER_H_ */
