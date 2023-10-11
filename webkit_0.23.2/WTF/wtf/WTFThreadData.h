/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 *
 */

#ifndef WTFThreadData_h
#define WTFThreadData_h

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/StackBounds.h>
#include <wtf/StackStats.h>
#include <wtf/text/StringHash.h>

#if OS(DARWIN)
#if defined(__has_include) && __has_include(<System/pthread_machdep.h>)
#include <System/pthread_machdep.h>
#endif
#endif

#if defined(__PTK_FRAMEWORK_JAVASCRIPTCORE_KEY1)
#define USE_PTHREAD_GETSPECIFIC_DIRECT 1
#endif

#if !USE(PTHREAD_GETSPECIFIC_DIRECT)
#include <wtf/ThreadSpecific.h>
#include <wtf/Threading.h>
#endif

namespace WTF {

class AtomicStringTable;

typedef void (*AtomicStringTableDestructor)(AtomicStringTable*);

class WTFThreadData {
    WTF_MAKE_NONCOPYABLE(WTFThreadData);
public:
    WTF_EXPORT_PRIVATE WTFThreadData();
    WTF_EXPORT_PRIVATE ~WTFThreadData();

    AtomicStringTable* atomicStringTable()
    {
        return m_currentAtomicStringTable;
    }

    AtomicStringTable* setCurrentAtomicStringTable(AtomicStringTable* atomicStringTable)
    {
        AtomicStringTable* oldAtomicStringTable = m_currentAtomicStringTable;
        m_currentAtomicStringTable = atomicStringTable;
        return oldAtomicStringTable;
    }

    const StackBounds& stack()
    {
        // We need to always get a fresh StackBounds from the OS due to how fibers work.
        // See https://bugs.webkit.org/show_bug.cgi?id=102411
#if OS(WINDOWS)
        m_stackBounds = StackBounds::currentThreadStackBounds();
#endif
        return m_stackBounds;
    }

#if ENABLE(STACK_STATS)
    StackStats::PerThreadStats& stackStats()
    {
        return m_stackStats;
    }
#endif

    void* savedStackPointerAtVMEntry()
    {
        return m_savedStackPointerAtVMEntry;
    }

    void setSavedStackPointerAtVMEntry(void* stackPointerAtVMEntry)
    {
        m_savedStackPointerAtVMEntry = stackPointerAtVMEntry;
    }

    void* savedLastStackTop()
    {
        return m_savedLastStackTop;
    }

    void setSavedLastStackTop(void* lastStackTop)
    {
        m_savedLastStackTop = lastStackTop;
    }

    void* m_apiData;

private:
    AtomicStringTable* m_currentAtomicStringTable;
    AtomicStringTable* m_defaultAtomicStringTable;
    AtomicStringTableDestructor m_atomicStringTableDestructor;

    StackBounds m_stackBounds;
#if ENABLE(STACK_STATS)
    StackStats::PerThreadStats m_stackStats;
#endif
    void* m_savedStackPointerAtVMEntry;
    void* m_savedLastStackTop;

#if USE(PTHREAD_GETSPECIFIC_DIRECT)
    static const pthread_key_t directKey = __PTK_FRAMEWORK_JAVASCRIPTCORE_KEY1;
    WTF_EXPORT_PRIVATE static WTFThreadData& createAndRegisterForGetspecificDirect();
#else
#if !PLATFORM(WKC)
    static WTF_EXPORTDATA ThreadSpecific<WTFThreadData>* staticData;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(ThreadSpecific<WTFThreadData>*, staticData);
#endif
#endif

    friend WTFThreadData& wtfThreadData();
    friend class AtomicStringTable;
};

inline WTFThreadData& wtfThreadData()
{
    // WRT WebCore:
    //    WTFThreadData is used on main thread before it could possibly be used
    //    on secondary ones, so there is no need for synchronization here.
    // WRT JavaScriptCore:
    //    wtfThreadData() is initially called from initializeThreading(), ensuring
    //    this is initially called in a pthread_once locked context.
#if !USE(PTHREAD_GETSPECIFIC_DIRECT)
    if (!WTFThreadData::staticData)
        WTFThreadData::staticData = new ThreadSpecific<WTFThreadData>;
    return **WTFThreadData::staticData;
#else
    if (WTFThreadData* data = static_cast<WTFThreadData*>(_pthread_getspecific_direct(WTFThreadData::directKey)))
        return *data;
    return WTFThreadData::createAndRegisterForGetspecificDirect();
#endif
}

} // namespace WTF

using WTF::WTFThreadData;
using WTF::wtfThreadData;
using WTF::AtomicStringTable;

#endif // WTFThreadData_h
