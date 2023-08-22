/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
 * Copyright (c) 2013 Apple Inc. All rights reserved.
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
#include "WorkerScriptDebugServer.h"

#include "JSDOMBinding.h"
#include "Timer.h"
#include "WorkerDebuggerAgent.h"
#include "WorkerGlobalScope.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"
#include <runtime/VM.h>

using namespace Inspector;

namespace WebCore {

WorkerScriptDebugServer::WorkerScriptDebugServer(WorkerGlobalScope* context, const String& mode)
    : ScriptDebugServer(true)
    , m_workerGlobalScope(context)
    , m_debuggerTaskMode(mode)
{
}

void WorkerScriptDebugServer::addListener(ScriptDebugListener* listener)
{
    if (!listener)
        return;

    bool wasEmpty = m_listeners.isEmpty();
    m_listeners.add(listener);

    if (wasEmpty) {
        m_workerGlobalScope->script()->attachDebugger(this);
        recompileAllJSFunctions();
    }
}

void WorkerScriptDebugServer::removeListener(ScriptDebugListener* listener, bool skipRecompile)
{
    if (!listener)
        return;

    m_listeners.remove(listener);

    if (m_listeners.isEmpty()) {
        if (m_workerGlobalScope->script())
            m_workerGlobalScope->script()->detachDebugger(this);
        if (!skipRecompile)
            recompileAllJSFunctions();
    }
}

void WorkerScriptDebugServer::recompileAllJSFunctions()
{
    JSC::VM& vm = m_workerGlobalScope->script()->vm();

    JSC::JSLockHolder lock(vm);
    JSC::Debugger::recompileAllJSFunctions(&vm);
}

void WorkerScriptDebugServer::runEventLoopWhilePaused()
{
    TimerBase::fireTimersInNestedEventLoop();

    MessageQueueWaitResult result;
    do {
        result = m_workerGlobalScope->thread().runLoop().runInMode(m_workerGlobalScope, m_debuggerTaskMode);
    // Keep waiting until execution is resumed.
    } while (result != MessageQueueTerminated && !m_doneProcessingDebuggerEvents);
}

void WorkerScriptDebugServer::reportException(JSC::ExecState* exec, JSC::Exception* exception) const
{
    WebCore::reportException(exec, exception);
}

void WorkerScriptDebugServer::interruptAndRunTask(std::unique_ptr<ScriptDebugServer::Task>)
{
}

} // namespace WebCore
