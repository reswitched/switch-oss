/*
 * Copyright (C) 2008, 2012-2015 Apple Inc. All rights reserved.
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

#ifndef StructureStubInfo_h
#define StructureStubInfo_h

#include "CodeOrigin.h"
#include "Instruction.h"
#include "JITStubRoutine.h"
#include "MacroAssembler.h"
#include "ObjectPropertyConditionSet.h"
#include "Opcode.h"
#include "PolymorphicAccessStructureList.h"
#include "RegisterSet.h"
#include "SpillRegistersMode.h"
#include "Structure.h"
#include "StructureStubClearingWatchpoint.h"

namespace JSC {

#if ENABLE(JIT)

class PolymorphicGetByIdList;
class PolymorphicPutByIdList;

enum AccessType {
    access_get_by_id_self,
    access_get_by_id_list,
    access_put_by_id_transition_normal,
    access_put_by_id_transition_direct,
    access_put_by_id_replace,
    access_put_by_id_list,
    access_unset,
    access_in_list
};

inline bool isGetByIdAccess(AccessType accessType)
{
    switch (accessType) {
    case access_get_by_id_self:
    case access_get_by_id_list:
        return true;
    default:
        return false;
    }
}
    
inline bool isPutByIdAccess(AccessType accessType)
{
    switch (accessType) {
    case access_put_by_id_transition_normal:
    case access_put_by_id_transition_direct:
    case access_put_by_id_replace:
    case access_put_by_id_list:
        return true;
    default:
        return false;
    }
}

inline bool isInAccess(AccessType accessType)
{
    switch (accessType) {
    case access_in_list:
        return true;
    default:
        return false;
    }
}

struct StructureStubInfo {
    StructureStubInfo()
        : accessType(access_unset)
        , seen(false)
        , resetByGC(false)
        , tookSlowPath(false)
    {
    }

    void initGetByIdSelf(VM& vm, JSCell* owner, Structure* baseObjectStructure)
    {
        accessType = access_get_by_id_self;

        u.getByIdSelf.baseObjectStructure.set(vm, owner, baseObjectStructure);
    }

    void initGetByIdList(PolymorphicGetByIdList* list)
    {
        accessType = access_get_by_id_list;
        u.getByIdList.list = list;
    }

    // PutById*

    void initPutByIdTransition(VM& vm, JSCell* owner, Structure* previousStructure, Structure* structure, ObjectPropertyConditionSet conditionSet, bool isDirect)
    {
        if (isDirect)
            accessType = access_put_by_id_transition_direct;
        else
            accessType = access_put_by_id_transition_normal;

        u.putByIdTransition.previousStructure.set(vm, owner, previousStructure);
        u.putByIdTransition.structure.set(vm, owner, structure);
        u.putByIdTransition.rawConditionSet = conditionSet.releaseRawPointer();
    }

    void initPutByIdReplace(VM& vm, JSCell* owner, Structure* baseObjectStructure)
    {
        accessType = access_put_by_id_replace;
    
        u.putByIdReplace.baseObjectStructure.set(vm, owner, baseObjectStructure);
    }
        
    void initPutByIdList(PolymorphicPutByIdList* list)
    {
        accessType = access_put_by_id_list;
        u.putByIdList.list = list;
    }
    
    void initInList(PolymorphicAccessStructureList* list, int listSize)
    {
        accessType = access_in_list;
        u.inList.structureList = list;
        u.inList.listSize = listSize;
    }
        
    void reset()
    {
        deref();
        accessType = access_unset;
        stubRoutine = nullptr;
        watchpoints = nullptr;
    }

    void deref();

    // Check if the stub has weak references that are dead. If there are dead ones that imply
    // that the stub should be entirely reset, this should return false. If there are dead ones
    // that can be handled internally by the stub and don't require a full reset, then this
    // should reset them and return true. If there are no dead weak references, return true.
    // If this method returns true it means that it has left the stub in a state where all
    // outgoing GC pointers are known to point to currently marked objects; this method is
    // allowed to accomplish this by either clearing those pointers somehow or by proving that
    // they have already been marked. It is not allowed to mark new objects.
    bool visitWeakReferences(RepatchBuffer&);
        
    bool seenOnce()
    {
        return seen;
    }

    void setSeen()
    {
        seen = true;
    }
        
    StructureStubClearingWatchpoint* addWatchpoint(
        CodeBlock* codeBlock, const ObjectPropertyCondition& condition = ObjectPropertyCondition())
    {
        return WatchpointsOnStructureStubInfo::ensureReferenceAndAddWatchpoint(
            watchpoints, codeBlock, this, condition);
    }
    
    int8_t accessType;
    bool seen : 1;
    bool resetByGC : 1;
    bool tookSlowPath : 1;

    CodeOrigin codeOrigin;

#if !PLATFORM(WKC)
    struct {
#else
    struct _dummy {
        _dummy()
        : usedRegisters(RegisterSet::EmptyValue)
        {}
#endif
        unsigned spillMode : 8;
        int8_t baseGPR;
#if USE(JSVALUE32_64)
        int8_t valueTagGPR;
#endif
        int8_t valueGPR;
        RegisterSet usedRegisters;
        int32_t deltaCallToDone;
        int32_t deltaCallToStorageLoad;
        int32_t deltaCallToJump;
        int32_t deltaCallToSlowCase;
        int32_t deltaCheckImmToCall;
#if USE(JSVALUE64)
        int32_t deltaCallToLoadOrStore;
#else
        int32_t deltaCallToTagLoadOrStore;
        int32_t deltaCallToPayloadLoadOrStore;
#endif
    } patch;

    union {
        struct {
            // It would be unwise to put anything here, as it will surely be overwritten.
        } unset;
        struct {
            WriteBarrierBase<Structure> baseObjectStructure;
        } getByIdSelf;
        struct {
            PolymorphicGetByIdList* list;
        } getByIdList;
        struct {
            WriteBarrierBase<Structure> previousStructure;
            WriteBarrierBase<Structure> structure;
            void* rawConditionSet;
        } putByIdTransition;
        struct {
            WriteBarrierBase<Structure> baseObjectStructure;
        } putByIdReplace;
        struct {
            PolymorphicPutByIdList* list;
        } putByIdList;
        struct {
            PolymorphicAccessStructureList* structureList;
            int listSize;
        } inList;
    } u;

    RefPtr<JITStubRoutine> stubRoutine;
    CodeLocationCall callReturnLocation;
    RefPtr<WatchpointsOnStructureStubInfo> watchpoints;
};

inline CodeOrigin getStructureStubInfoCodeOrigin(StructureStubInfo& structureStubInfo)
{
    return structureStubInfo.codeOrigin;
}

typedef HashMap<CodeOrigin, StructureStubInfo*, CodeOriginApproximateHash> StubInfoMap;

#else

typedef HashMap<int, void*> StubInfoMap;

#endif // ENABLE(JIT)

} // namespace JSC

#endif // StructureStubInfo_h
