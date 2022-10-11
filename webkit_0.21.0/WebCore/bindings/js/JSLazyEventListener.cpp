/*
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSLazyEventListener.h"

#include "ContentSecurityPolicy.h"
#include "Frame.h"
#include "JSNode.h"
#include "ScriptController.h"
#include <runtime/Executable.h>
#include <runtime/FunctionConstructor.h>
#include <runtime/IdentifierInlines.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>

using namespace JSC;

namespace WebCore {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, eventListenerCounter, ("JSLazyEventListener"));

JSLazyEventListener::JSLazyEventListener(const String& functionName, const String& eventParameterName, const String& code, ContainerNode* node, const String& sourceURL, const TextPosition& position, JSObject* wrapper, DOMWrapperWorld& isolatedWorld)
    : JSEventListener(0, wrapper, true, isolatedWorld)
    , m_functionName(functionName)
    , m_eventParameterName(eventParameterName)
    , m_code(code)
    , m_sourceURL(sourceURL)
    , m_position(position)
    , m_originalNode(node)
{
    // We don't retain the original node because we assume it
    // will stay alive as long as this handler object is around
    // and we need to avoid a reference cycle. If JS transfers
    // this handler to another node, initializeJSFunction will
    // be called and then originalNode is no longer needed.

    // A JSLazyEventListener can be created with a line number of zero when it is created with
    // a setAttribute call from JavaScript, so make the line number 1 in that case.
    if (m_position == TextPosition::belowRangePosition())
        m_position = TextPosition::minimumPosition();

    ASSERT(m_eventParameterName == "evt" || m_eventParameterName == "event");

#ifndef NDEBUG
    eventListenerCounter.increment();
#endif
}

JSLazyEventListener::~JSLazyEventListener()
{
#ifndef NDEBUG
    eventListenerCounter.decrement();
#endif
}

JSObject* JSLazyEventListener::initializeJSFunction(ScriptExecutionContext* executionContext) const
{
    ASSERT(is<Document>(executionContext));
    if (!executionContext)
        return nullptr;

    ASSERT(!m_code.isNull());
    ASSERT(!m_eventParameterName.isNull());
    if (m_code.isNull() || m_eventParameterName.isNull())
        return nullptr;

    // As per the HTML specification [1], if this is an element's event handler, then document should be the
    // element's document. The script execution context may be different from the node's document if the
    // node's document was created by JavaScript.
    // [1] https://html.spec.whatwg.org/multipage/webappapis.html#getting-the-current-value-of-the-event-handler
    Document& document = m_originalNode ? m_originalNode->document() : downcast<Document>(*executionContext);

    if (!document.frame())
        return nullptr;

    if (!document.contentSecurityPolicy()->allowInlineEventHandlers(m_sourceURL, m_position.m_line))
        return nullptr;

    ScriptController& script = document.frame()->script();
    if (!script.canExecuteScripts(AboutToExecuteScript) || script.isPaused())
        return nullptr;

    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(executionContext, isolatedWorld());
    if (!globalObject)
        return nullptr;

    ExecState* exec = globalObject->globalExec();

    MarkedArgumentBuffer args;
    args.append(jsNontrivialString(exec, m_eventParameterName));
    args.append(jsStringWithCache(exec, m_code));

    // We want all errors to refer back to the line on which our attribute was
    // declared, regardless of any newlines in our JavaScript source text.
    int overrideLineNumber = m_position.m_line.oneBasedInt();

    JSObject* jsFunction = constructFunctionSkippingEvalEnabledCheck(
        exec, exec->lexicalGlobalObject(), args, Identifier::fromString(exec, m_functionName),
        m_sourceURL, m_position, overrideLineNumber);

    if (exec->hadException()) {
        reportCurrentException(exec);
        exec->clearException();
        return nullptr;
    }

    JSFunction* listenerAsFunction = jsCast<JSFunction*>(jsFunction);

    if (m_originalNode) {
        if (!wrapper()) {
            // Ensure that 'node' has a JavaScript wrapper to mark the event listener we're creating.
            JSLockHolder lock(exec);
            // FIXME: Should pass the global object associated with the node
            setWrapper(exec->vm(), asObject(toJS(exec, globalObject, m_originalNode)));
        }

        // Add the event's home element to the scope
        // (and the document, and the form - see JSHTMLElement::eventHandlerScope)
        listenerAsFunction->setScope(exec->vm(), jsCast<JSNode*>(wrapper())->pushEventHandlerScope(exec, listenerAsFunction->scope()));
    }
    return jsFunction;
}

static const String& eventParameterName(bool isSVGEvent)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const String> eventString(ASCIILiteral("event"));
    static NeverDestroyed<const String> evtString(ASCIILiteral("evt"));
    return isSVGEvent ? evtString : eventString;
#else
    WKC_DEFINE_STATIC_PTR(const String*, eventString, 0)
    WKC_DEFINE_STATIC_PTR(const String*, evtString, 0)
    if (!eventString) {
        eventString = new String(ASCIILiteral("event"));
        evtString = new String(ASCIILiteral("evt"));
    }
    return isSVGEvent ? *evtString : *eventString;
#endif
}

RefPtr<JSLazyEventListener> JSLazyEventListener::createForNode(ContainerNode& node, const QualifiedName& attributeName, const AtomicString& attributeValue)
{
    if (attributeValue.isNull())
        return nullptr;

    TextPosition position = TextPosition::minimumPosition();
    String sourceURL;

    // FIXME: We should be able to provide source information for frameless documents too (e.g. for importing nodes from XMLHttpRequest.responseXML).
    if (Frame* frame = node.document().frame()) {
        if (!frame->script().canExecuteScripts(AboutToExecuteScript))
            return nullptr;

        position = frame->script().eventHandlerPosition();
        sourceURL = node.document().url().string();
    }

    return adoptRef(*new JSLazyEventListener(attributeName.localName().string(),
        eventParameterName(node.isSVGElement()), attributeValue,
        &node, sourceURL, position, nullptr, mainThreadNormalWorld()));
}

RefPtr<JSLazyEventListener> JSLazyEventListener::createForDOMWindow(Frame& frame, const QualifiedName& attributeName, const AtomicString& attributeValue)
{
    if (attributeValue.isNull())
        return nullptr;

    if (!frame.script().canExecuteScripts(AboutToExecuteScript))
        return nullptr;

    return adoptRef(*new JSLazyEventListener(attributeName.localName().string(),
        eventParameterName(frame.document()->isSVGDocument()), attributeValue,
        nullptr, frame.document()->url().string(), frame.script().eventHandlerPosition(),
        toJSDOMWindow(&frame, mainThreadNormalWorld()), mainThreadNormalWorld()));
}

} // namespace WebCore
