/*
 * Copyright (C) 2012, 2014, 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "PolymorphicPutByIdList.h"

#if ENABLE(JIT)

#include "StructureStubInfo.h"

namespace JSC {

PutByIdAccess PutByIdAccess::fromStructureStubInfo(StructureStubInfo& stubInfo)
{
    MacroAssemblerCodePtr initialSlowPath =
        stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase);
    
    PutByIdAccess result;
    
    switch (stubInfo.accessType) {
    case access_put_by_id_replace:
        result.m_type = Replace;
        result.m_oldStructure.copyFrom(stubInfo.u.putByIdReplace.baseObjectStructure);
        result.m_stubRoutine = JITStubRoutine::createSelfManagedRoutine(initialSlowPath);
        break;
        
    case access_put_by_id_transition_direct:
    case access_put_by_id_transition_normal:
        result.m_type = Transition;
        result.m_oldStructure.copyFrom(stubInfo.u.putByIdTransition.previousStructure);
        result.m_newStructure.copyFrom(stubInfo.u.putByIdTransition.structure);
        result.m_conditionSet = ObjectPropertyConditionSet::adoptRawPointer(
            stubInfo.u.putByIdTransition.rawConditionSet);
        result.m_stubRoutine = stubInfo.stubRoutine;
        break;
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    return result;
}

bool PutByIdAccess::visitWeak(RepatchBuffer& repatchBuffer) const
{
    if (!m_conditionSet.areStillLive())
        return false;
    
    switch (m_type) {
    case Replace:
        if (!Heap::isMarked(m_oldStructure.get()))
            return false;
        break;
    case Transition:
        if (!Heap::isMarked(m_oldStructure.get()))
            return false;
        if (!Heap::isMarked(m_newStructure.get()))
            return false;
        break;
    case Setter:
    case CustomSetter:
        if (!Heap::isMarked(m_oldStructure.get()))
            return false;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
    if (!m_stubRoutine->visitWeak(repatchBuffer))
        return false;
    return true;
}

PolymorphicPutByIdList::PolymorphicPutByIdList(
    PutKind putKind, StructureStubInfo& stubInfo)
    : m_kind(putKind)
{
    if (stubInfo.accessType != access_unset)
        m_list.append(PutByIdAccess::fromStructureStubInfo(stubInfo));
}

PolymorphicPutByIdList* PolymorphicPutByIdList::from(
    PutKind putKind, StructureStubInfo& stubInfo)
{
    if (stubInfo.accessType == access_put_by_id_list)
        return stubInfo.u.putByIdList.list;
    
    ASSERT(stubInfo.accessType == access_put_by_id_replace
        || stubInfo.accessType == access_put_by_id_transition_normal
        || stubInfo.accessType == access_put_by_id_transition_direct
        || stubInfo.accessType == access_unset);
    
    PolymorphicPutByIdList* result =
        new PolymorphicPutByIdList(putKind, stubInfo);
    
    stubInfo.initPutByIdList(result);
    
    return result;
}

PolymorphicPutByIdList::~PolymorphicPutByIdList() { }

bool PolymorphicPutByIdList::isFull() const
{
    ASSERT(size() <= POLYMORPHIC_LIST_CACHE_SIZE);
    return size() == POLYMORPHIC_LIST_CACHE_SIZE;
}

bool PolymorphicPutByIdList::isAlmostFull() const
{
    ASSERT(size() <= POLYMORPHIC_LIST_CACHE_SIZE);
    return size() >= POLYMORPHIC_LIST_CACHE_SIZE - 1;
}

void PolymorphicPutByIdList::addAccess(const PutByIdAccess& putByIdAccess)
{
    ASSERT(!isFull());
    // Make sure that the resizing optimizes for space, not time.
    m_list.resizeToFit(m_list.size() + 1);
    m_list.last() = putByIdAccess;
}

bool PolymorphicPutByIdList::visitWeak(RepatchBuffer& repatchBuffer) const
{
    for (unsigned i = 0; i < size(); ++i) {
        if (!at(i).visitWeak(repatchBuffer))
            return false;
    }
    return true;
}

} // namespace JSC

#endif // ENABLE(JIT)
