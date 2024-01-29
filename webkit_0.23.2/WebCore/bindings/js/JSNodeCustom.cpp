/*
 * Copyright (C) 2007, 2009, 2010 Apple Inc. All rights reserved.
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
#include "JSNode.h"

#include "Attr.h"
#include "CDATASection.h"
#include "Comment.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Entity.h"
#include "EntityReference.h"
#include "ExceptionCode.h"
#include "HTMLAudioElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLImageElement.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLScriptElement.h"
#include "HTMLStyleElement.h"
#include "JSAttr.h"
#include "JSCDATASection.h"
#include "JSComment.h"
#include "JSDOMBinding.h"
#include "JSDocument.h"
#include "JSDocumentFragment.h"
#include "JSDocumentType.h"
#include "JSEntity.h"
#include "JSEntityReference.h"
#include "JSEventListener.h"
#include "JSHTMLElement.h"
#include "JSHTMLElementWrapperFactory.h"
#include "JSProcessingInstruction.h"
#include "JSSVGElementWrapperFactory.h"
#include "JSText.h"
#include "Node.h"
#include "ProcessingInstruction.h"
#include "RegisteredEventListener.h"
#include "SVGElement.h"
#include "ShadowRoot.h"
#include "StyleSheet.h"
#include "StyledElement.h"
#include "Text.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

using namespace JSC;

namespace WebCore {

using namespace HTMLNames;

static inline bool isReachableFromDOM(Node* node, SlotVisitor& visitor)
{
    if (!node->inDocument()) {
        if (is<Element>(*node)) {
            auto& element = downcast<Element>(*node);

            // If a wrapper is the last reference to an image element
            // that is loading but not in the document, the wrapper is observable
            // because it is the only thing keeping the image element alive, and if
            // the element is destroyed, its load event will not fire.
            // FIXME: The DOM should manage this issue without the help of JavaScript wrappers.
            if (is<HTMLImageElement>(element)) {
                if (downcast<HTMLImageElement>(element).hasPendingActivity())
                    return true;
            }
#if ENABLE(VIDEO)
            else if (is<HTMLAudioElement>(element)) {
                if (!downcast<HTMLAudioElement>(element).paused())
                    return true;
            }
#endif
        }

        // If a node is firing event listeners, its wrapper is observable because
        // its wrapper is responsible for marking those event listeners.
        if (node->isFiringEventListeners())
            return true;
    }

    return visitor.containsOpaqueRoot(root(node));
}

bool JSNodeOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    JSNode* jsNode = jsCast<JSNode*>(handle.slot()->asCell());
    return isReachableFromDOM(&jsNode->impl(), visitor);
}

JSValue JSNode::insertBefore(ExecState* exec)
{
    ExceptionCode ec = 0;
    bool ok = impl().insertBefore(JSNode::toWrapped(exec->argument(0)), JSNode::toWrapped(exec->argument(1)), ec);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(0);
    return jsNull();
}

JSValue JSNode::replaceChild(ExecState* exec)
{
    ExceptionCode ec = 0;
    bool ok = impl().replaceChild(JSNode::toWrapped(exec->argument(0)), JSNode::toWrapped(exec->argument(1)), ec);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(1);
    return jsNull();
}

JSValue JSNode::removeChild(ExecState* exec)
{
    ExceptionCode ec = 0;
    bool ok = impl().removeChild(JSNode::toWrapped(exec->argument(0)), ec);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(0);
    return jsNull();
}

JSValue JSNode::appendChild(ExecState* exec)
{
    ExceptionCode ec = 0;
    bool ok = impl().appendChild(JSNode::toWrapped(exec->argument(0)), ec);
    setDOMException(exec, ec);
    if (ok)
        return exec->argument(0);
    return jsNull();
}

JSScope* JSNode::pushEventHandlerScope(ExecState* exec, JSScope* node) const
{
    if (inherits(JSHTMLElement::info()))
        return jsCast<const JSHTMLElement*>(this)->pushEventHandlerScope(exec, node);
    return node;
}

void JSNode::visitAdditionalChildren(SlotVisitor& visitor)
{
    visitor.addOpaqueRoot(root(impl()));
}

static ALWAYS_INLINE JSValue createWrapperInline(ExecState* exec, JSDOMGlobalObject* globalObject, Node* node)
{
    ASSERT(node);
    ASSERT(!getCachedWrapper(globalObject->world(), node));
    
    JSDOMWrapper* wrapper;    
    switch (node->nodeType()) {
        case Node::ELEMENT_NODE:
            if (is<HTMLElement>(*node))
                wrapper = createJSHTMLWrapper(globalObject, downcast<HTMLElement>(node));
            else if (is<SVGElement>(*node))
                wrapper = createJSSVGWrapper(globalObject, downcast<SVGElement>(node));
            else
                wrapper = CREATE_DOM_WRAPPER(globalObject, Element, node);
            break;
        case Node::ATTRIBUTE_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, Attr, node);
            break;
        case Node::TEXT_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, Text, node);
            break;
        case Node::CDATA_SECTION_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, CDATASection, node);
            break;
        case Node::ENTITY_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, Entity, node);
            break;
        case Node::PROCESSING_INSTRUCTION_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, ProcessingInstruction, node);
            break;
        case Node::COMMENT_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, Comment, node);
            break;
        case Node::DOCUMENT_NODE:
            // we don't want to cache the document itself in the per-document dictionary
            return toJS(exec, globalObject, downcast<Document>(node));
        case Node::DOCUMENT_TYPE_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, DocumentType, node);
            break;
        case Node::DOCUMENT_FRAGMENT_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, DocumentFragment, node);
            break;
        case Node::ENTITY_REFERENCE_NODE:
            wrapper = CREATE_DOM_WRAPPER(globalObject, EntityReference, node);
            break;
        default:
            wrapper = CREATE_DOM_WRAPPER(globalObject, Node, node);
    }

    return wrapper;    
}

JSValue createWrapper(ExecState* exec, JSDOMGlobalObject* globalObject, Node* node)
{
    return createWrapperInline(exec, globalObject, node);
}
    
JSValue toJSNewlyCreated(ExecState* exec, JSDOMGlobalObject* globalObject, Node* node)
{
    if (!node)
        return jsNull();
    
    return createWrapperInline(exec, globalObject, node);
}

JSC::JSObject* getOutOfLineCachedWrapper(JSDOMGlobalObject* globalObject, Node* node)
{
    ASSERT(!globalObject->world().isNormal());
    return globalObject->world().m_wrappers.get(node);
}

void willCreatePossiblyOrphanedTreeByRemovalSlowCase(Node* root)
{
    JSC::ExecState* scriptState = mainWorldExecState(root->document().frame());
    if (!scriptState)
        return;

    JSLockHolder lock(scriptState);
    toJS(scriptState, static_cast<JSDOMGlobalObject*>(scriptState->lexicalGlobalObject()), root);
}

} // namespace WebCore
