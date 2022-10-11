/*
 * Copyright (C) 2008, 2009, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
 * Copyright (C) 2013 University of Washington. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScriptDebugServer.h"

#include "DebuggerCallFrame.h"
#include "DebuggerScope.h"
#include "Exception.h"
#include "JSJavaScriptCallFrame.h"
#include "JSLock.h"
#include "JavaScriptCallFrame.h"
#include "ScriptValue.h"
#include "SourceProvider.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/TemporaryChange.h>
#include <wtf/text/WTFString.h>

using namespace JSC;

namespace Inspector {

ScriptDebugServer::ScriptDebugServer(bool isInWorkerThread)
    : Debugger(isInWorkerThread)
{
}

ScriptDebugServer::~ScriptDebugServer()
{
}

JSC::BreakpointID ScriptDebugServer::setBreakpoint(JSC::SourceID sourceID, const ScriptBreakpoint& scriptBreakpoint, unsigned* actualLineNumber, unsigned* actualColumnNumber)
{
    if (!sourceID)
        return JSC::noBreakpointID;

    JSC::Breakpoint breakpoint(sourceID, scriptBreakpoint.lineNumber, scriptBreakpoint.columnNumber, scriptBreakpoint.condition, scriptBreakpoint.autoContinue, scriptBreakpoint.ignoreCount);
    JSC::BreakpointID id = Debugger::setBreakpoint(breakpoint, *actualLineNumber, *actualColumnNumber);
    if (id != JSC::noBreakpointID && !scriptBreakpoint.actions.isEmpty()) {
#ifndef NDEBUG
        BreakpointIDToActionsMap::iterator it = m_breakpointIDToActions.find(id);
        ASSERT(it == m_breakpointIDToActions.end());
#endif
        const BreakpointActions& actions = scriptBreakpoint.actions;
        m_breakpointIDToActions.set(id, actions);
    }
    return id;
}

void ScriptDebugServer::removeBreakpoint(JSC::BreakpointID id)
{
    ASSERT(id != JSC::noBreakpointID);
    BreakpointIDToActionsMap::iterator it = m_breakpointIDToActions.find(id);
    if (it != m_breakpointIDToActions.end())
        m_breakpointIDToActions.remove(it);

    Debugger::removeBreakpoint(id);
}

bool ScriptDebugServer::evaluateBreakpointAction(const ScriptBreakpointAction& breakpointAction)
{
    DebuggerCallFrame* debuggerCallFrame = currentDebuggerCallFrame();

    switch (breakpointAction.type) {
    case ScriptBreakpointActionTypeLog: {
        dispatchBreakpointActionLog(debuggerCallFrame->exec(), breakpointAction.data);
        break;
    }
    case ScriptBreakpointActionTypeEvaluate: {
        NakedPtr<Exception> exception;
        debuggerCallFrame->evaluate(breakpointAction.data, exception);
        if (exception)
            reportException(debuggerCallFrame->exec(), exception);
        break;
    }
    case ScriptBreakpointActionTypeSound:
        dispatchBreakpointActionSound(debuggerCallFrame->exec(), breakpointAction.identifier);
        break;
    case ScriptBreakpointActionTypeProbe: {
        NakedPtr<Exception> exception;
        JSValue result = debuggerCallFrame->evaluate(breakpointAction.data, exception);
        if (exception)
            reportException(debuggerCallFrame->exec(), exception);
        
        JSC::ExecState* state = debuggerCallFrame->scope()->globalObject()->globalExec();
        Deprecated::ScriptValue wrappedResult = Deprecated::ScriptValue(state->vm(), exception ? exception->value() : result);
        dispatchBreakpointActionProbe(state, breakpointAction, wrappedResult);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }

    return true;
}

void ScriptDebugServer::clearBreakpoints()
{
    Debugger::clearBreakpoints();
    m_breakpointIDToActions.clear();
}

void ScriptDebugServer::dispatchDidPause(ScriptDebugListener* listener)
{
    ASSERT(isPaused());
    DebuggerCallFrame* debuggerCallFrame = currentDebuggerCallFrame();
    JSGlobalObject* globalObject = debuggerCallFrame->scope()->globalObject();
    JSC::ExecState* state = globalObject->globalExec();
    RefPtr<JavaScriptCallFrame> javaScriptCallFrame = JavaScriptCallFrame::create(debuggerCallFrame);
    JSValue jsCallFrame = toJS(state, globalObject, javaScriptCallFrame.get());

    listener->didPause(state, Deprecated::ScriptValue(state->vm(), jsCallFrame), exceptionOrCaughtValue(state));
}

void ScriptDebugServer::dispatchBreakpointActionLog(ExecState* exec, const String& message)
{
    if (m_callingListeners)
        return;

    ListenerSet& listeners = getListeners();
    if (listeners.isEmpty())
        return;

    TemporaryChange<bool> change(m_callingListeners, true);

    Vector<ScriptDebugListener*> listenersCopy;
    copyToVector(listeners, listenersCopy);
    for (auto* listener : listenersCopy)
        listener->breakpointActionLog(exec, message);
}

void ScriptDebugServer::dispatchBreakpointActionSound(ExecState*, int breakpointActionIdentifier)
{
    if (m_callingListeners)
        return;

    ListenerSet& listeners = getListeners();
    if (listeners.isEmpty())
        return;

    TemporaryChange<bool> change(m_callingListeners, true);

    Vector<ScriptDebugListener*> listenersCopy;
    copyToVector(listeners, listenersCopy);
    for (auto* listener : listenersCopy)
        listener->breakpointActionSound(breakpointActionIdentifier);
}

void ScriptDebugServer::dispatchBreakpointActionProbe(ExecState* exec, const ScriptBreakpointAction& action, const Deprecated::ScriptValue& sampleValue)
{
    if (m_callingListeners)
        return;

    ListenerSet& listeners = getListeners();
    if (listeners.isEmpty())
        return;

    TemporaryChange<bool> change(m_callingListeners, true);

    unsigned sampleId = m_nextProbeSampleId++;

    Vector<ScriptDebugListener*> listenersCopy;
    copyToVector(listeners, listenersCopy);
    for (auto* listener : listenersCopy)
        listener->breakpointActionProbe(exec, action, m_currentProbeBatchId, sampleId, sampleValue);
}

void ScriptDebugServer::dispatchDidContinue(ScriptDebugListener* listener)
{
    listener->didContinue();
}

void ScriptDebugServer::dispatchDidParseSource(const ListenerSet& listeners, SourceProvider* sourceProvider, bool isContentScript)
{
    JSC::SourceID sourceID = sourceProvider->asID();

    ScriptDebugListener::Script script;
    script.url = sourceProvider->url();
    script.source = sourceProvider->source();
    script.startLine = sourceProvider->startPosition().m_line.zeroBasedInt();
    script.startColumn = sourceProvider->startPosition().m_column.zeroBasedInt();
    script.isContentScript = isContentScript;
    script.sourceURL = sourceProvider->sourceURL();
    script.sourceMappingURL = sourceProvider->sourceMappingURL();

    int sourceLength = script.source.length();
    int lineCount = 1;
    int lastLineStart = 0;
    for (int i = 0; i < sourceLength; ++i) {
        if (script.source[i] == '\n') {
            lineCount += 1;
            lastLineStart = i + 1;
        }
    }

    script.endLine = script.startLine + lineCount - 1;
    if (lineCount == 1)
        script.endColumn = script.startColumn + sourceLength;
    else
        script.endColumn = sourceLength - lastLineStart;

    Vector<ScriptDebugListener*> copy;
    copyToVector(listeners, copy);
    for (size_t i = 0; i < copy.size(); ++i)
        copy[i]->didParseSource(sourceID, script);
}

void ScriptDebugServer::dispatchFailedToParseSource(const ListenerSet& listeners, SourceProvider* sourceProvider, int errorLine, const String& errorMessage)
{
    String url = sourceProvider->url();
    const String& data = sourceProvider->source();
    int firstLine = sourceProvider->startPosition().m_line.oneBasedInt();

    Vector<ScriptDebugListener*> copy;
    copyToVector(listeners, copy);
    for (size_t i = 0; i < copy.size(); ++i)
        copy[i]->failedToParseSource(url, data, firstLine, errorLine, errorMessage);
}

void ScriptDebugServer::sourceParsed(ExecState* exec, SourceProvider* sourceProvider, int errorLine, const String& errorMessage)
{
    if (m_callingListeners)
        return;

    ListenerSet& listeners = getListeners();
    if (listeners.isEmpty())
        return;

    TemporaryChange<bool> change(m_callingListeners, true);

    bool isError = errorLine != -1;
    if (isError)
        dispatchFailedToParseSource(listeners, sourceProvider, errorLine, errorMessage);
    else
        dispatchDidParseSource(listeners, sourceProvider, isContentScript(exec));
}

void ScriptDebugServer::dispatchFunctionToListeners(JavaScriptExecutionCallback callback)
{
    if (m_callingListeners)
        return;

    TemporaryChange<bool> change(m_callingListeners, true);

    ListenerSet& listeners = getListeners();
    if (!listeners.isEmpty())
        dispatchFunctionToListeners(listeners, callback);
}

void ScriptDebugServer::dispatchFunctionToListeners(const ListenerSet& listeners, JavaScriptExecutionCallback callback)
{
    Vector<ScriptDebugListener*> copy;
    copyToVector(listeners, copy);
    for (size_t i = 0; i < copy.size(); ++i)
        (this->*callback)(copy[i]);
}

void ScriptDebugServer::notifyDoneProcessingDebuggerEvents()
{
    m_doneProcessingDebuggerEvents = true;
}

void ScriptDebugServer::handleBreakpointHit(JSC::JSGlobalObject* globalObject, const JSC::Breakpoint& breakpoint)
{
    ASSERT(isAttached(globalObject));

    m_currentProbeBatchId++;

    BreakpointIDToActionsMap::iterator it = m_breakpointIDToActions.find(breakpoint.id);
    if (it != m_breakpointIDToActions.end()) {
        BreakpointActions actions = it->value;
        for (size_t i = 0; i < actions.size(); ++i) {
            if (!evaluateBreakpointAction(actions[i]))
                return;
            if (!isAttached(globalObject))
                return;
        }
    }
}

void ScriptDebugServer::handleExceptionInBreakpointCondition(JSC::ExecState* exec, JSC::Exception* exception) const
{
    reportException(exec, exception);
}

void ScriptDebugServer::handlePause(JSGlobalObject* vmEntryGlobalObject, Debugger::ReasonForPause)
{
    dispatchFunctionToListeners(&ScriptDebugServer::dispatchDidPause);
    didPause(vmEntryGlobalObject);

    m_doneProcessingDebuggerEvents = false;
    runEventLoopWhilePaused();

    didContinue(vmEntryGlobalObject);
    dispatchFunctionToListeners(&ScriptDebugServer::dispatchDidContinue);
}

const BreakpointActions& ScriptDebugServer::getActionsForBreakpoint(JSC::BreakpointID breakpointID)
{
    ASSERT(breakpointID != JSC::noBreakpointID);

    if (m_breakpointIDToActions.contains(breakpointID))
        return m_breakpointIDToActions.find(breakpointID)->value;
    
#if !PLATFORM(WKC)
    static NeverDestroyed<BreakpointActions> emptyActionVector = BreakpointActions();
    return emptyActionVector;
#else
    WKC_DEFINE_STATIC_PTR(BreakpointActions*, emptyActionVector, new BreakpointActions());
    return *emptyActionVector;
#endif
}

Deprecated::ScriptValue ScriptDebugServer::exceptionOrCaughtValue(JSC::ExecState* state)
{
    if (reasonForPause() == PausedForException)
        return Deprecated::ScriptValue(state->vm(), currentException());

    RefPtr<DebuggerCallFrame> debuggerCallFrame = currentDebuggerCallFrame();
    while (debuggerCallFrame) {
        DebuggerScope* scope = debuggerCallFrame->scope();
        if (scope->isCatchScope())
            return Deprecated::ScriptValue(state->vm(), scope->caughtValue());
        debuggerCallFrame = debuggerCallFrame->callerFrame();
    }

    return Deprecated::ScriptValue();
}

} // namespace Inspector
