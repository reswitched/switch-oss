/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 */

#ifndef HeapVerifier_h
#define HeapVerifier_h

#include "Heap.h"
#include <wtf/Vector.h>

namespace JSC {

class JSObject;
class MarkedBlock;

struct LiveObjectData {
    LiveObjectData(JSObject* obj, bool isConfirmedDead = false)
        : obj(obj)
        , isConfirmedDead(isConfirmedDead)
    {
    }

    JSObject* obj;
    bool isConfirmedDead;
};

struct LiveObjectList {
    LiveObjectList(const char* name)
        : name(name)
        , hasLiveObjects(true)
    {
    }

    void reset()
    {
        liveObjects.clear();
        hasLiveObjects = true; // Presume to have live objects until the list is trimmed.
    }

    LiveObjectData* findObject(JSObject*);

    const char* name;
    Vector<LiveObjectData> liveObjects;
    bool hasLiveObjects;
};

class HeapVerifier {
public:
    enum class Phase {
        BeforeGC,
        BeforeMarking,
        AfterMarking,
        AfterGC
    };

    HeapVerifier(Heap*, unsigned numberOfGCCyclesToRecord);

    void initializeGCCycle();
    void gatherLiveObjects(Phase);
    void trimDeadObjects();
    void verify(Phase);

    // Scans all previously recorded LiveObjectLists and checks if the specified
    // object was in any of those lists.
    JS_EXPORT_PRIVATE void checkIfRecorded(JSObject*);

    static const char* collectionTypeName(HeapOperation);
    static const char* phaseName(Phase);

#if !PLATFORM(WKC)
private:
#endif
    struct GCCycle {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
    public:
#endif
        GCCycle()
            : before("Before Marking")
            , after("After Marking")
        {
        }

        HeapOperation collectionType;
        LiveObjectList before;
        LiveObjectList after;

        const char* collectionTypeName() const
        {
            return HeapVerifier::collectionTypeName(collectionType);
        }
    };

#if PLATFORM(WKC)
private:
#endif

    void incrementCycle() { m_currentCycle = (m_currentCycle + 1) % m_numberOfCycles; }
    GCCycle& currentCycle() { return m_cycles[m_currentCycle]; }
    GCCycle& cycleForIndex(int cycleIndex)
    {
        ASSERT(cycleIndex <= 0 && cycleIndex > -m_numberOfCycles);
        cycleIndex += m_currentCycle;
        if (cycleIndex < 0)
            cycleIndex += m_numberOfCycles;
        ASSERT(cycleIndex < m_numberOfCycles);
        return m_cycles[cycleIndex];
    }

    LiveObjectList* liveObjectListForGathering(Phase);
    bool verifyButterflyIsInStorageSpace(Phase, LiveObjectList&);

    static void reportObject(LiveObjectData&, int cycleIndex, HeapVerifier::GCCycle&, LiveObjectList&);

    Heap* m_heap;
    int m_currentCycle;
    int m_numberOfCycles;
    std::unique_ptr<GCCycle[]> m_cycles;
};

} // namespace JSC

#endif // HeapVerifier
