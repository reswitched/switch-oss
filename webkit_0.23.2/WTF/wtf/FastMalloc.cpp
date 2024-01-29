// Copyright (c) 2005, 2007, Google Inc. All rights reserved.

/*
 * Copyright (C) 2005-2009, 2011, 2015 Apple Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "FastMalloc.h"

#include "CheckedArithmetic.h"
#include "CurrentTime.h"
#include <limits>
#include <string.h>
#include <wtf/DataLog.h>

#if OS(WINDOWS) || OS(WINDOWS_WKC)
#include <windows.h>
#else
#include <pthread.h>
#endif

#if OS(DARWIN)
#include <mach/mach_init.h>
#include <malloc/malloc.h>
#endif

namespace WTF {

void* fastZeroedMalloc(size_t n) 
{
    void* result = fastMalloc(n);
    memset(result, 0, n);
    return result;
}

char* fastStrDup(const char* src)
{
    size_t len = strlen(src) + 1;
    char* dup = static_cast<char*>(fastMalloc(len));
    memcpy(dup, src, len);
    return dup;
}

TryMallocReturnValue tryFastZeroedMalloc(size_t n) 
{
    void* result;
    if (!tryFastMalloc(n).getValue(result))
        return 0;
    memset(result, 0, n);
    return result;
}

} // namespace WTF

#if defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC

#if OS(WINDOWS)
#include <malloc.h>
#endif

namespace WTF {

size_t fastMallocGoodSize(size_t bytes)
{
#if OS(DARWIN)
    return malloc_good_size(bytes);
#else
    return bytes;
#endif
}

#if OS(WINDOWS)

void* fastAlignedMalloc(size_t alignment, size_t size) 
{
    return _aligned_malloc(size, alignment);
}

void fastAlignedFree(void* p) 
{
    _aligned_free(p);
}

#else

void* fastAlignedMalloc(size_t alignment, size_t size) 
{
    void* p = nullptr;
    posix_memalign(&p, alignment, size);
    return p;
}

void fastAlignedFree(void* p) 
{
    free(p);
}

#endif // OS(WINDOWS)

TryMallocReturnValue tryFastMalloc(size_t n) 
{
    return malloc(n);
}

void* fastMalloc(size_t n) 
{
    void* result = malloc(n);
    if (!result)
        CRASH();

    return result;
}

TryMallocReturnValue tryFastCalloc(size_t n_elements, size_t element_size)
{
    return calloc(n_elements, element_size);
}

void* fastCalloc(size_t n_elements, size_t element_size)
{
    void* result = calloc(n_elements, element_size);
    if (!result)
        CRASH();

    return result;
}

void fastFree(void* p)
{
    free(p);
}

void* fastRealloc(void* p, size_t n)
{
    void* result = realloc(p, n);
    if (!result)
        CRASH();
    return result;
}

void releaseFastMallocFreeMemory() { }
void releaseFastMallocFreeMemoryForThisThread() { }
    
FastMallocStatistics fastMallocStatistics()
{
    FastMallocStatistics statistics = { 0, 0, 0 };
    return statistics;
}

size_t fastMallocSize(const void* p)
{
#if OS(DARWIN)
    return malloc_size(p);
#elif OS(WINDOWS)
    return _msize(const_cast<void*>(p));
#else
    UNUSED_PARAM(p);
    return 1;
#endif
}

} // namespace WTF

#else // defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC

#include <bmalloc/bmalloc.h>

namespace WTF {

void* fastMalloc(size_t size)
{
    return bmalloc::api::malloc(size);
}

void* fastCalloc(size_t numElements, size_t elementSize)
{
    Checked<size_t> checkedSize = elementSize;
    checkedSize *= numElements;
    void* result = fastZeroedMalloc(checkedSize.unsafeGet());
    if (!result)
        CRASH();
    return result;
}

void* fastRealloc(void* object, size_t size)
{
    return bmalloc::api::realloc(object, size);
}

void fastFree(void* object)
{
    bmalloc::api::free(object);
}

size_t fastMallocSize(const void*)
{
    // FIXME: This is incorrect; best fix is probably to remove this function.
    // Caller currently are all using this for assertion, not to actually check
    // the size of the allocation, so maybe we can come up with something for that.
    return 1;
}

size_t fastMallocGoodSize(size_t size)
{
    return size;
}

void* fastAlignedMalloc(size_t alignment, size_t size) 
{
    return bmalloc::api::memalign(alignment, size);
}

void fastAlignedFree(void* p) 
{
    bmalloc::api::free(p);
}

TryMallocReturnValue tryFastMalloc(size_t size)
{
    return bmalloc::api::tryMalloc(size);
}
    
TryMallocReturnValue tryFastCalloc(size_t numElements, size_t elementSize)
{
    Checked<size_t, RecordOverflow> checkedSize = elementSize;
    checkedSize *= numElements;
    if (checkedSize.hasOverflowed())
        return nullptr;
    return tryFastZeroedMalloc(checkedSize.unsafeGet());
}
    
void releaseFastMallocFreeMemoryForThisThread()
{
    bmalloc::api::scavengeThisThread();
}

void releaseFastMallocFreeMemory()
{
    bmalloc::api::scavenge();
}

FastMallocStatistics fastMallocStatistics()
{
    // FIXME: This is incorrect; needs an implementation or to be removed.
    FastMallocStatistics statistics = { 0, 0, 0 };
    return statistics;
}

} // namespace WTF

#endif // defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC
