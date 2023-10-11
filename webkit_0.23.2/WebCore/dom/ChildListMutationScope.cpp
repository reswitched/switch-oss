/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "ChildListMutationScope.h"

#include "DocumentFragment.h"
#include "MutationObserverInterestGroup.h"
#include "MutationRecord.h"
#include "StaticNodeList.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

typedef HashMap<ContainerNode*, ChildListMutationAccumulator*> AccumulatorMap;
static AccumulatorMap& accumulatorMap()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(AccumulatorMap, map, ());
    return map;
}

ChildListMutationAccumulator::ChildListMutationAccumulator(ContainerNode& target, std::unique_ptr<MutationObserverInterestGroup> observers)
    : m_target(target)
    , m_lastAdded(nullptr)
    , m_observers(WTF::move(observers))
{
}

ChildListMutationAccumulator::~ChildListMutationAccumulator()
{
    if (!isEmpty())
        enqueueMutationRecord();
    accumulatorMap().remove(m_target.ptr());
}

PassRefPtr<ChildListMutationAccumulator> ChildListMutationAccumulator::getOrCreate(ContainerNode& target)
{
    AccumulatorMap::AddResult result = accumulatorMap().add(&target, nullptr);
    RefPtr<ChildListMutationAccumulator> accumulator;
    if (!result.isNewEntry)
        accumulator = result.iterator->value;
    else {
        accumulator = adoptRef(new ChildListMutationAccumulator(target, MutationObserverInterestGroup::createForChildListMutation(target)));
        result.iterator->value = accumulator.get();
    }
    return accumulator.release();
}

inline bool ChildListMutationAccumulator::isAddedNodeInOrder(Node& child)
{
    return isEmpty() || (m_lastAdded == child.previousSibling() && m_nextSibling == child.nextSibling());
}

void ChildListMutationAccumulator::childAdded(Node& childRef)
{
    ASSERT(hasObservers());

    Ref<Node> child(childRef);

    if (!isAddedNodeInOrder(child))
        enqueueMutationRecord();

    if (isEmpty()) {
        m_previousSibling = child->previousSibling();
        m_nextSibling = child->nextSibling();
    }

    m_lastAdded = child.ptr();
    m_addedNodes.append(child.get());
}

inline bool ChildListMutationAccumulator::isRemovedNodeInOrder(Node& child)
{
    return isEmpty() || m_nextSibling == &child;
}

void ChildListMutationAccumulator::willRemoveChild(Node& childRef)
{
    ASSERT(hasObservers());

    Ref<Node> child(childRef);

    if (!m_addedNodes.isEmpty() || !isRemovedNodeInOrder(child))
        enqueueMutationRecord();

    if (isEmpty()) {
        m_previousSibling = child->previousSibling();
        m_nextSibling = child->nextSibling();
        m_lastAdded = child->previousSibling();
    } else
        m_nextSibling = child->nextSibling();

    m_removedNodes.append(child.get());
}

void ChildListMutationAccumulator::enqueueMutationRecord()
{
    ASSERT(hasObservers());
    ASSERT(!isEmpty());

    RefPtr<NodeList> addedNodes = StaticNodeList::adopt(m_addedNodes);
    RefPtr<NodeList> removedNodes = StaticNodeList::adopt(m_removedNodes);
    RefPtr<MutationRecord> record = MutationRecord::createChildList(m_target, addedNodes.release(), removedNodes.release(), m_previousSibling.release(), m_nextSibling.release());
    m_observers->enqueueMutationRecord(record.release());
    m_lastAdded = 0;
    ASSERT(isEmpty());
}

bool ChildListMutationAccumulator::isEmpty()
{
    bool result = m_removedNodes.isEmpty() && m_addedNodes.isEmpty();
#ifndef NDEBUG
    if (result) {
        ASSERT(!m_previousSibling);
        ASSERT(!m_nextSibling);
        ASSERT(!m_lastAdded);
    }
#endif
    return result;
}

} // namespace WebCore
