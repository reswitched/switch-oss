/*
 *  Copyright (C) 2005-2009, 2015 Apple Inc. All rights reserved.
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
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_FastMalloc_h
#define WTF_FastMalloc_h

#include <new>
#include <stdlib.h>
#include <wtf/StdLibExtras.h>

namespace WTF {

class TryMallocReturnValue {
public:
    TryMallocReturnValue(void*);
    TryMallocReturnValue(TryMallocReturnValue&&);
    ~TryMallocReturnValue();
    template<typename T> bool getValue(T*&) WARN_UNUSED_RETURN;
private:
    void operator=(TryMallocReturnValue&&)  = delete;
    mutable void* m_data;
};

// These functions call CRASH() if an allocation fails.
WTF_EXPORT_PRIVATE void* fastMalloc(size_t);
WTF_EXPORT_PRIVATE void* fastZeroedMalloc(size_t);
WTF_EXPORT_PRIVATE void* fastCalloc(size_t numElements, size_t elementSize);
WTF_EXPORT_PRIVATE void* fastRealloc(void*, size_t);
WTF_EXPORT_PRIVATE char* fastStrDup(const char*);

WTF_EXPORT_PRIVATE TryMallocReturnValue tryFastMalloc(size_t);
TryMallocReturnValue tryFastZeroedMalloc(size_t);
WTF_EXPORT_PRIVATE TryMallocReturnValue tryFastCalloc(size_t numElements, size_t elementSize);
WTF_EXPORT_PRIVATE TryMallocReturnValue tryFastRealloc(void*, size_t);

WTF_EXPORT_PRIVATE void fastFree(void*);

// Allocations from fastAlignedMalloc() must be freed using fastAlignedFree().
WTF_EXPORT_PRIVATE void* fastAlignedMalloc(size_t alignment, size_t);
WTF_EXPORT_PRIVATE void fastAlignedFree(void*);

WTF_EXPORT_PRIVATE size_t fastMallocSize(const void*);

// FIXME: This is non-helpful; fastMallocGoodSize will be removed soon.
WTF_EXPORT_PRIVATE size_t fastMallocGoodSize(size_t);

WTF_EXPORT_PRIVATE void releaseFastMallocFreeMemory();
WTF_EXPORT_PRIVATE void releaseFastMallocFreeMemoryForThisThread();

struct FastMallocStatistics {
    size_t reservedVMBytes;
    size_t committedVMBytes;
    size_t freeListBytes;
};
WTF_EXPORT_PRIVATE FastMallocStatistics fastMallocStatistics();

// This defines a type which holds an unsigned integer and is the same
// size as the minimally aligned memory allocation.
typedef unsigned long long AllocAlignmentInteger;

inline TryMallocReturnValue::TryMallocReturnValue(void* data)
    : m_data(data)
{
}

inline TryMallocReturnValue::TryMallocReturnValue(TryMallocReturnValue&& source)
    : m_data(source.m_data)
{
    source.m_data = nullptr;
}

inline TryMallocReturnValue::~TryMallocReturnValue()
{
    ASSERT(!m_data);
}

template<typename T> inline bool TryMallocReturnValue::getValue(T*& data)
{
    data = static_cast<T*>(m_data);
    m_data = nullptr;
    return data;
}

} // namespace WTF

using WTF::fastCalloc;
using WTF::fastFree;
using WTF::fastMalloc;
using WTF::fastMallocGoodSize;
using WTF::fastMallocSize;
using WTF::fastRealloc;
using WTF::fastStrDup;
using WTF::fastZeroedMalloc;
using WTF::tryFastCalloc;
using WTF::tryFastMalloc;
using WTF::tryFastRealloc;
using WTF::tryFastZeroedMalloc;
using WTF::fastAlignedMalloc;
using WTF::fastAlignedFree;

#if COMPILER(GCC) && OS(DARWIN)
#define WTF_PRIVATE_INLINE __private_extern__ inline __attribute__((always_inline))
#elif COMPILER(GCC)
#define WTF_PRIVATE_INLINE inline __attribute__((always_inline))
#elif COMPILER(MSVC)
#define WTF_PRIVATE_INLINE __forceinline
#else
#define WTF_PRIVATE_INLINE inline
#endif

#define WTF_MAKE_FAST_ALLOCATED \
public: \
    void* operator new(size_t, void* p) { return p; } \
    void* operator new[](size_t, void* p) { return p; } \
    \
    void* operator new(size_t size) \
    { \
        return ::WTF::fastMalloc(size); \
    } \
    \
    void* operator new(size_t size, const std::nothrow_t&) throw() \
    { \
        return ::WTF::fastMalloc(size); \
    } \
    \
    void operator delete(void* p) \
    { \
        ::WTF::fastFree(p); \
    } \
    \
    void operator delete(void* p, const std::nothrow_t&) throw() \
    { \
        ::WTF::fastFree(p); \
    } \
    \
    void* operator new[](size_t size) \
    { \
        return ::WTF::fastMalloc(size); \
    } \
    \
    void* operator new[](size_t size, const std::nothrow_t&) throw() \
    { \
        return ::WTF::fastMalloc(size); \
    } \
    \
    void operator delete[](void* p) \
    { \
        ::WTF::fastFree(p); \
    } \
    void operator delete[](void* p, const std::nothrow_t&) throw() \
    { \
        ::WTF::fastFree(p); \
    } \
    void* operator new(size_t, NotNullTag, void* location) \
    { \
        ASSERT(location); \
        return location; \
    } \
private: \
typedef int __thisIsHereToForceASemicolonAfterThisMacro

#if PLATFORM(WKC)

template <class Tp>
struct WKCAllocator {
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef Tp value_type;
    template <class U> struct rebind {
        typedef WKCAllocator<U> other;
    };

    WKCAllocator()
    {}
    ~WKCAllocator()
    {}
    template <class T> WKCAllocator(const WKCAllocator<T>& other)
    {}

    Tp* allocate(std::size_t n)
    {
        return (Tp *)::WTF::fastMalloc(n * sizeof(Tp));
    }
    void deallocate(Tp* p, std::size_t)
    {
        ::WTF::fastFree(p);
    }
    void destroy(Tp* p)
    {
        ((Tp *)p)->~Tp();
    }
};
template<class T1, class T2>
bool operator==(const WKCAllocator<T1>&, const WKCAllocator<T2>&)
{
    return true;
}
template<class T1, class T2>
bool operator!=(const WKCAllocator<T1>&, const WKCAllocator<T2>&)
{
    return false;
}

#include "functional"

namespace WTF {
extern const WKCAllocator<std::function<void()>>& voidFuncAllocator();
} // namespace

#endif

#endif /* WTF_FastMalloc_h */
