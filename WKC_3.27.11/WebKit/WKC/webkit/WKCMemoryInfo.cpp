/*
 *  WKCMemoryInfo.cpp
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

#include "config.h"
#include "WKCMemoryInfo.h"
#include <wkc/wkcheappeer.h>
#include <wkc/wkcpeer.h>

#include <wtf/OSAllocator.h>
#include <wtf/PageBlock.h>
#include "Options.h"


namespace WKC {

namespace Heap {

bool
GetNumericProperty(const char* property, size_t* value)
{
    return wkcHeapGetNumericPropertyPeer(property, value);
}

bool
SetNumericProperty(const char* property, size_t value)
{
    return wkcHeapSetNumericPropertyPeer(property, value);
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
    JSC::Options::AllowUnfinalizedAccessScope scope;
    JSC::Options::maxPerThreadStackUsage() = size;
}


} // namespace Heap

} // namespace WKC

#ifndef WKC_DISABLE_OVERRIDE_PLATFORMNEWDELETE

#ifdef _MSC_VER

void* operator new(size_t size) throw (std::bad_alloc)
{
#if 0
    wkcDebugPrintfPeer("system alloc: %d", size);
    void *bt[50];
    int len = wkcDebugGetBacktracePeer(bt, 50, 1);
    int skip = WKC_MIN(len, 1);
    wkcDebugPrintBacktracePeer(bt + skip, len - skip);
#endif
    return wkc_malloc(size);
}
void* operator new(size_t size, const std::nothrow_t&) throw()
{
#if 0
    wkcDebugPrintfPeer("system alloc: %d", size);
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
#if 0
    wkcDebugPrintfPeer("system alloc[array]: %d", size);
    void *bt[50];
    int len = wkcDebugGetBacktracePeer(bt, 50, 1);
    int skip = WKC_MIN(len, 1);
    wkcDebugPrintBacktracePeer(bt + skip, len - skip);
#endif
    return wkc_malloc(size);
}
void* operator new[](size_t size, const std::nothrow_t&) throw()
{
#if 0
    wkcDebugPrintfPeer("system alloc[array]: %d", size);
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
