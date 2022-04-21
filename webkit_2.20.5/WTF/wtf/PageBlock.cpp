/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/PageBlock.h>

#if PLATFORM(WKC)
#include <wtf/Threading.h>
#include <wtf/ThreadingPrimitives.h>
#include <wkc/wkcmpeer.h>
#endif

#if OS(UNIX)
#include <unistd.h>
#endif

#if OS(WINDOWS)
#include <malloc.h>
#include <windows.h>
#endif

namespace WTF {

#if PLATFORM(WKC)
WKC_DEFINE_GLOBAL_SIZET_ZERO(s_pageSize);
WKC_DEFINE_GLOBAL_SIZET_ZERO(s_pageMask);
#else
static size_t s_pageSize;
static size_t s_pageMask;
#endif

#if OS(UNIX)

inline size_t systemPageSize()
{
    return getpagesize();
}

#elif OS(WINDOWS)

inline size_t systemPageSize()
{
    static size_t size = 0;
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    size = system_info.dwPageSize;
    return size;
}

#elif PLATFORM(WKC)

inline size_t systemPageSize()
{
    return wkcMemoryGetPageSizePeer();
}

WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(size_t, PageBlock, s_reservedBytesJSGCHeap);
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(size_t, PageBlock, s_reservedBytesJSJITCode);

// Main logic of the following is equivalent to
//     AtomicallyInitializedStatic(Mutex&, mutex = *new Mutex);
// in normal WebKit code.
extern void lockAtomicallyInitializedStaticMutex();
extern void unlockAtomicallyInitializedStaticMutex();
#define WKC_DEFINE_MUTEX_PROVIDER(functionName) \
    static Mutex& functionName() \
    { \
        lockAtomicallyInitializedStaticMutex(); \
        WKC_DEFINE_STATIC_TYPE(Mutex*, mutex_, new Mutex); \
        Mutex& mutex = *mutex_; \
        unlockAtomicallyInitializedStaticMutex(); \
        \
        return mutex; \
    }
WKC_DEFINE_MUTEX_PROVIDER(reservedBytesJSGCHeapCountMutex)
WKC_DEFINE_MUTEX_PROVIDER(reservedBytesJSJITCodeCountMutex)
#undef WKC_DEFINE_MUTEX_PROVIDER

size_t
PageBlock::reservedSize(OSAllocator::Usage usage)
{
    size_t size = 0;

    // Won't lock mutexes here because we don't need precise values.
    switch (usage) {
    case OSAllocator::JSGCHeapPages:
        size = s_reservedBytesJSGCHeap;
        break;
    case OSAllocator::JSJITCodePages:
        size = s_reservedBytesJSJITCode;
        break;
    default:
        break;
    }

    return size;
}

void
PageBlock::changeReservedSize(long diff, OSAllocator::Usage usage)
{
    switch (usage) {
    case OSAllocator::JSGCHeapPages:
        {
            MutexLocker lock(reservedBytesJSGCHeapCountMutex());
            s_reservedBytesJSGCHeap += diff;
        }
        break;
    case OSAllocator::JSJITCodePages:
        {
            MutexLocker lock(reservedBytesJSJITCodeCountMutex());
            s_reservedBytesJSJITCode += diff;
        }
        break;
    default:
        break;
    }
}

#endif

size_t pageSize()
{
    if (!s_pageSize)
        s_pageSize = systemPageSize();
    ASSERT(isPowerOfTwo(s_pageSize));
    return s_pageSize;
}

size_t pageMask()
{
    if (!s_pageMask)
        s_pageMask = ~(pageSize() - 1);
    return s_pageMask;
}

} // namespace WTF
