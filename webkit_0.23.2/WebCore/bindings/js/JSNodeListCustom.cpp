/*
 * Copyright (C) 2007, 2014 Apple Inc. All rights reserved.
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
#include "JSNodeList.h"

#include "ChildNodeList.h"
#include "JSNode.h"
#include "LiveNodeList.h"
#include "Node.h"
#include "NodeList.h"
#include <wtf/text/AtomicString.h>

using namespace JSC;

namespace WebCore {

bool JSNodeListOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    JSNodeList* jsNodeList = jsCast<JSNodeList*>(handle.slot()->asCell());
    if (!jsNodeList->hasCustomProperties())
        return false;
    if (jsNodeList->impl().isLiveNodeList())
        return visitor.containsOpaqueRoot(root(static_cast<LiveNodeList&>(jsNodeList->impl()).ownerNode()));
    if (jsNodeList->impl().isChildNodeList())
        return visitor.containsOpaqueRoot(root(static_cast<ChildNodeList&>(jsNodeList->impl()).ownerNode()));
    if (jsNodeList->impl().isEmptyNodeList())
        return visitor.containsOpaqueRoot(root(static_cast<EmptyNodeList&>(jsNodeList->impl()).ownerNode()));
    return false;
}

bool JSNodeList::getOwnPropertySlotDelegate(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    if (Node* item = impl().namedItem(propertyNameToAtomicString(propertyName))) {
        slot.setValue(this, ReadOnly | DontDelete | DontEnum, toJS(exec, globalObject(), item));
        return true;
    }
    return false;
}

JSC::JSValue createWrapper(JSDOMGlobalObject& globalObject, NodeList& nodeList)
{
    // FIXME: Adopt reportExtraMemoryVisited, and switch to reportExtraMemoryAllocated.
    // https://bugs.webkit.org/show_bug.cgi?id=142595
    globalObject.vm().heap.deprecatedReportExtraMemory(nodeList.memoryCost());
    return createNewWrapper<JSNodeList>(&globalObject, &nodeList);
}

} // namespace WebCore
