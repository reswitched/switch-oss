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

#pragma once

#if PLATFORM(WKC)
#include <wtf/OSAllocator.h>
#endif

namespace WTF {

WTF_EXPORT_PRIVATE size_t pageSize();
WTF_EXPORT_PRIVATE size_t pageMask();
inline bool isPageAligned(void* address) { return !(reinterpret_cast<intptr_t>(address) & (pageSize() - 1)); }
inline bool isPageAligned(size_t size) { return !(size & (pageSize() - 1)); }
inline bool isPowerOfTwo(size_t size) { return !(size & (size - 1)); }

class PageBlock {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PageBlock();
    PageBlock(const PageBlock&);
#if PLATFORM(WKC)
    PageBlock(void*, size_t, bool hasGuardPages, OSAllocator::Usage usage = OSAllocator::UnknownUsage);
#else
    PageBlock(void*, size_t, bool hasGuardPages);
#endif
    
    void* base() const { return m_base; }
    size_t size() const { return m_size; }
#if PLATFORM(WKC)
    bool hasGuardPages() const { return m_hasGuardPages; }
    OSAllocator::Usage usage() const { return m_usage; }
#endif

    operator bool() const { return !!m_realBase; }

    bool contains(void* containedBase, size_t containedSize)
    {
        return containedBase >= m_base
            && (static_cast<char*>(containedBase) + containedSize) <= (static_cast<char*>(m_base) + m_size);
    }

#if PLATFORM(WKC)
    static size_t reservedSize(OSAllocator::Usage usage); // called from code in WKC namespace
    static void changeReservedSize(long diff, OSAllocator::Usage usage);
#endif

private:
    void* m_realBase;
    void* m_base;
    size_t m_size;
#if PLATFORM(WKC)
    bool m_hasGuardPages;
    OSAllocator::Usage m_usage;

    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(size_t, s_reservedBytesJSGCHeap);
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(size_t, s_reservedBytesJSJITCode);
#endif
};

inline PageBlock::PageBlock()
    : m_realBase(0)
    , m_base(0)
    , m_size(0)
#if PLATFORM(WKC)
    , m_hasGuardPages(false)
    , m_usage(OSAllocator::UnknownUsage)
#endif
{
}

inline PageBlock::PageBlock(const PageBlock& other)
    : m_realBase(other.m_realBase)
    , m_base(other.m_base)
    , m_size(other.m_size)
#if PLATFORM(WKC)
    , m_hasGuardPages(other.m_hasGuardPages)
    , m_usage(other.m_usage)
#endif
{
}

#if PLATFORM(WKC)
inline PageBlock::PageBlock(void* base, size_t size, bool hasGuardPages, OSAllocator::Usage usage)
#else
inline PageBlock::PageBlock(void* base, size_t size, bool hasGuardPages)
#endif
    : m_realBase(base)
    , m_base(static_cast<char*>(base) + ((base && hasGuardPages) ? pageSize() : 0))
    , m_size(size)
#if PLATFORM(WKC)
    , m_hasGuardPages(hasGuardPages)
    , m_usage(usage)
#endif
{
}

} // namespace WTF

using WTF::pageSize;
using WTF::isPageAligned;
using WTF::isPowerOfTwo;
