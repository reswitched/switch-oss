/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
#include "PolymorphicGetByIdList.h"

#if ENABLE(JIT)

#include "CodeBlock.h"
#include "Heap.h"
#include "JSCInlines.h"
#include "StructureStubInfo.h"

namespace JSC {

GetByIdAccess::GetByIdAccess(
    VM& vm, JSCell* owner, AccessType type, PassRefPtr<JITStubRoutine> stubRoutine,
    Structure* structure, const ObjectPropertyConditionSet& conditionSet)
    : m_type(type)
    , m_structure(vm, owner, structure)
    , m_conditionSet(conditionSet)
    , m_stubRoutine(stubRoutine)
{
}

GetByIdAccess::~GetByIdAccess()
{
}

GetByIdAccess GetByIdAccess::fromStructureStubInfo(StructureStubInfo& stubInfo)
{
    MacroAssemblerCodePtr initialSlowPath =
        stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase);
    
    GetByIdAccess result;
    
    RELEASE_ASSERT(stubInfo.accessType == access_get_by_id_self);
    
    result.m_type = SimpleInline;
    result.m_structure.copyFrom(stubInfo.u.getByIdSelf.baseObjectStructure);
    result.m_stubRoutine = JITStubRoutine::createSelfManagedRoutine(initialSlowPath);
    
    return result;
}

bool GetByIdAccess::visitWeak(RepatchBuffer& repatchBuffer) const
{
    if (m_structure && !Heap::isMarked(m_structure.get()))
        return false;
    if (!m_conditionSet.areStillLive())
        return false;
    if (!m_stubRoutine->visitWeak(repatchBuffer))
        return false;
    return true;
}

PolymorphicGetByIdList::PolymorphicGetByIdList(StructureStubInfo& stubInfo)
{
    if (stubInfo.accessType == access_unset)
        return;
    
    m_list.append(GetByIdAccess::fromStructureStubInfo(stubInfo));
}

PolymorphicGetByIdList* PolymorphicGetByIdList::from(StructureStubInfo& stubInfo)
{
    if (stubInfo.accessType == access_get_by_id_list)
        return stubInfo.u.getByIdList.list;
    
    ASSERT(
        stubInfo.accessType == access_get_by_id_self
        || stubInfo.accessType == access_unset);
    
    PolymorphicGetByIdList* result = new PolymorphicGetByIdList(stubInfo);
    
    stubInfo.initGetByIdList(result);
    
    return result;
}

PolymorphicGetByIdList::~PolymorphicGetByIdList() { }

MacroAssemblerCodePtr PolymorphicGetByIdList::currentSlowPathTarget(
    StructureStubInfo& stubInfo) const
{
    if (isEmpty())
        return stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.deltaCallToSlowCase);
    return m_list.last().stubRoutine()->code().code();
}

void PolymorphicGetByIdList::addAccess(const GetByIdAccess& access)
{
    ASSERT(!isFull());
    // Make sure that the resizing optimizes for space, not time.
    m_list.resizeToFit(m_list.size() + 1);
    m_list.last() = access;
}

bool PolymorphicGetByIdList::isFull() const
{
    ASSERT(size() <= POLYMORPHIC_LIST_CACHE_SIZE);
    return size() == POLYMORPHIC_LIST_CACHE_SIZE;
}

bool PolymorphicGetByIdList::isAlmostFull() const
{
    ASSERT(size() <= POLYMORPHIC_LIST_CACHE_SIZE);
    return size() >= POLYMORPHIC_LIST_CACHE_SIZE - 1;
}

bool PolymorphicGetByIdList::didSelfPatching() const
{
    for (unsigned i = size(); i--;) {
        if (at(i).type() == GetByIdAccess::SimpleInline)
            return true;
    }
    return false;
}

bool PolymorphicGetByIdList::visitWeak(RepatchBuffer& repatchBuffer) const
{
    for (unsigned i = size(); i--;) {
        if (!at(i).visitWeak(repatchBuffer))
            return false;
    }
    return true;
}

} // namespace JSC

#endif // ENABLE(JIT)


