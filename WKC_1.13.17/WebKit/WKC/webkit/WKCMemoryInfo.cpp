/*
 *  WKCMemoryInfo.cpp
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

#include "config.h"
#include "WKCMemoryInfo.h"
#include "peer_heap_debug.h"
#include <wkc/wkcheappeer.h>
#include <wkc/wkcpeer.h>

#include <wtf/OSAllocator.h>
#include <wtf/PageBlock.h>
#include "Options.h"


namespace WKC {

namespace Heap {

void
GetStatistics(Statistics& stat, size_t requestSize)
{
    wkcHeapStatisticsPeer(*(WKCHeapStatistics*)&stat, requestSize);
}

size_t
GetStatisticsFreeSizeInHeap()
{
    return wkcHeapStatisticsFreeSizeInHeapPeer();
}

size_t
GetStatisticsMaxFreeBlockSizeInHeap(size_t requestSize)
{
    return wkcHeapStatisticsMaxFreeBlockSizeInHeapPeer(requestSize);
}

size_t
GetStatisticsCurrentPhysicalMemoryUsage()
{
    return wkcMemoryGetCurrentPhysicalMemoryUsagePeer();
}


bool
EnableMemoryMap(bool in_set)
{
    return wkcHeapDebugEnableMemoryMapPeer(in_set);
}

void
GetMemoryMap(MemoryInfo& memInfo, bool needUsedMemory)
{
    COMPILE_ASSERT(sizeof(UsedMemoryInfo) == sizeof(UsedMemoryInfoWKC), sizeof__UsedMemoryInfo);
    COMPILE_ASSERT(sizeof(SpanInfo) == sizeof(SpanInfoWKC), sizeof__SpanInfo);
    COMPILE_ASSERT(sizeof(MemoryInfo) == sizeof(MemoryInfoWKC), sizeof_MemoryInfo);

    wkcHeapDebugGetMemoryMapPeer(*(MemoryInfoWKC*)&memInfo, needUsedMemory);
}

bool
AllocMemoryMap(MemoryInfo& memInfo)
{
    bool ret = false;

    COMPILE_ASSERT(sizeof(UsedMemoryInfo) == sizeof(UsedMemoryInfoWKC), sizeof__UsedMemoryInfo);
    COMPILE_ASSERT(sizeof(SpanInfo) == sizeof(SpanInfoWKC), sizeof__SpanInfo);
    COMPILE_ASSERT(sizeof(MemoryInfo) == sizeof(MemoryInfo), sizeof__MemoryInfo);
    ret = wkcHeapDebugAllocMemoryMapPeer(*(MemoryInfoWKC*)&memInfo);
    return ret;
}

void
ReleaseMemoryMap(MemoryInfo& memInfo)
{
    COMPILE_ASSERT(sizeof(UsedMemoryInfo) == sizeof(UsedMemoryInfoWKC), sizeof__UsedMemoryInfo);
    COMPILE_ASSERT(sizeof(SpanInfo) == sizeof(SpanInfoWKC), sizeof__SpanInfo);
    COMPILE_ASSERT(sizeof(MemoryInfo) == sizeof(MemoryInfoWKC), sizeof__MemoryInfo);

    wkcHeapDebugFreeMemoryMapPeer(*(MemoryInfoWKC*)&memInfo);
}

void
SetMemoryLeakDumpProc(MemoryLeakDumpProc in_proc, void* in_ctx)
{
    wkcHeapDebugSetMemoryLeakDumpProcPeer((WKCMemoryLeakDumpProc)in_proc, in_ctx);
}

bool
GetMemoryLeakInfo(MemoryLeakRoot& leakRoot, bool resolveSymbol)
{
    COMPILE_ASSERT(sizeof(StackObjectInfo) == sizeof(StackObjectInfoWKC), sizeof__StackObjectInfo);
    COMPILE_ASSERT(sizeof(StackTraceInfo) == sizeof(StackTraceInfoWKC), sizeof__StackTraceInfo);
    COMPILE_ASSERT(sizeof(MemoryLeakInfo) == sizeof(MemoryLeakInfoWKC), sizeof__MemoryLeakInfo);
    COMPILE_ASSERT(sizeof(MemoryLeakNode) == sizeof(MemoryLeakNodeWKC), sizeof__MemoryLeakNode);
    COMPILE_ASSERT(sizeof(MemoryLeakRoot) == sizeof(MemoryLeakRootWKC), sizeof__MemoryLeakRoot);
    COMPILE_ASSERT(kMaxFileNameLength <= kMaxNameLengthWKC, kMaxFileNameLength);

    return wkcHeapDebugGetMemoryLeakInfoPeer(*(MemoryLeakRootWKC*)&leakRoot, resolveSymbol);
}

void
ClearStackTrace()
{
    wkcHeapDebugClearStackTracePeer();
}

void
ReleaseMemoryLeakInfo(MemoryLeakRoot& leakRoot)
{
    wkcHeapDebugClearMemoryLeakInfoPeer(*(MemoryLeakRootWKC*)&leakRoot);
}

size_t
GetAvailableSize()
{
    return wkcHeapGetHeapAvailableSizePeer();
}

size_t
GetMaxAvailableBlockSize()
{
    return wkcHeapGetHeapMaxAvailableBlockSizePeer();
}

bool
CanAllocMemory(size_t inRequestSize, size_t* outAvailSize, bool* outCheckPeer, size_t* outRealRequestSize, size_t* outAvailPeerSize)
{
    return wkcHeapCanAllocMemoryPeer(inRequestSize, outAvailSize, outCheckPeer, outRealRequestSize, outAvailPeerSize);
}

size_t
GetJSHeapAllocatedBlockBytes()
{
    return WTF::PageBlock::reservedSize(OSAllocator::JSGCHeapPages);
}

void
GetJSJITCodePageAllocatedBytes(size_t& allocated_bytes, size_t& total_bytes, size_t& max_allocatable_bytes)
{
#if ENABLE(JIT)
    allocated_bytes = WTF::PageBlock::reservedSize(OSAllocator::JSJITCodePages);
    total_bytes = wkcHeapGetHeapTotalSizeForExecutableRegionPeer();
    max_allocatable_bytes = wkcHeapGetHeapMaxAvailableBlockSizeForExecutableRegionPeer();
#else
    allocated_bytes = 0;
    total_bytes = 0;
    max_allocatable_bytes = 0;
#endif
}

void SetJSRegisterFileDefaultCapacity(unsigned int size)
{
    JSC::Options::maxPerThreadStackUsage() = size;
}


} // namespace Heap

} // namespace WKC

#ifndef WKC_DISABLE_OVERRIDE_PLATFORMNEWDELETE

#ifdef _MSC_VER

void* operator new(size_t size) throw (std::bad_alloc)
{
    wkcDebugPrintfPeer("system alloc: %d", size);
#if 0
    void *bt[50];
    int len = wkcDebugGetBacktracePeer(bt, 50, 1);
    int skip = WKC_MIN(len, 1);
    wkcDebugPrintBacktracePeer(bt + skip, len - skip);
#endif
    return wkc_malloc(size);
}
void* operator new(size_t size, const std::nothrow_t&) throw()
{
    wkcDebugPrintfPeer("system alloc: %d", size);
#if 0
    void *bt[50];
    int len = wkcDebugGetBacktracePeer(bt, 50, 1);
    int skip = WKC_MIN(len, 1);
    wkcDebugPrintBacktracePeer(bt + skip, len - skip);
#endif
    return wkc_malloc(size);
}
void operator delete(void* p) throw()
{
    wkc_free(p);
}
void operator delete(void* p, const std::nothrow_t&) throw()
{
    wkc_free(p);
}
void* operator new[](size_t size) throw (std::bad_alloc)
{
    wkcDebugPrintfPeer("system alloc[array]: %d", size);
#if 0
    void *bt[50];
    int len = wkcDebugGetBacktracePeer(bt, 50, 1);
    int skip = WKC_MIN(len, 1);
    wkcDebugPrintBacktracePeer(bt + skip, len - skip);
#endif
    return wkc_malloc(size);
}
void* operator new[](size_t size, const std::nothrow_t&) throw()
{
    wkcDebugPrintfPeer("system alloc[array]: %d", size);
#if 0
    void *bt[50];
    int len = wkcDebugGetBacktracePeer(bt, 50, 1);
    int skip = WKC_MIN(len, 1);
    wkcDebugPrintBacktracePeer(bt + skip, len - skip);
#endif
    return wkc_malloc(size);
}
void operator delete[](void* p) throw()
{
    wkc_free(p);
}
void operator delete[](void* p, const std::nothrow_t&) throw()
{
    wkc_free(p);
}

#endif

#endif // WKC_DISABLE_OVERRIDE_PLATFORMNEWDELETE
