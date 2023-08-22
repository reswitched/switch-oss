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

#ifndef InspectorConsoleInstrumentation_h
#define InspectorConsoleInstrumentation_h

#include "InspectorInstrumentation.h"
#include <inspector/ScriptArguments.h>
#include <inspector/ScriptCallStack.h>

namespace WebCore {

inline void InspectorInstrumentation::addMessageToConsole(Page& page, std::unique_ptr<Inspector::ConsoleMessage> message)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        addMessageToConsoleImpl(*instrumentingAgents, WTF::move(message));
}

inline void InspectorInstrumentation::addMessageToConsole(WorkerGlobalScope* workerGlobalScope, std::unique_ptr<Inspector::ConsoleMessage> message)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForWorkerGlobalScope(workerGlobalScope))
        addMessageToConsoleImpl(*instrumentingAgents, WTF::move(message));
}

inline void InspectorInstrumentation::consoleCount(Page& page, JSC::ExecState* state, RefPtr<Inspector::ScriptArguments>&& arguments)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        consoleCountImpl(*instrumentingAgents, state, WTF::move(arguments));
}

inline void InspectorInstrumentation::startConsoleTiming(Frame& frame, const String& title)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        startConsoleTimingImpl(*instrumentingAgents, frame, title);
}

inline void InspectorInstrumentation::stopConsoleTiming(Frame& frame, const String& title, RefPtr<Inspector::ScriptCallStack>&& stack)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        stopConsoleTimingImpl(*instrumentingAgents, frame, title, WTF::move(stack));
}

inline void InspectorInstrumentation::consoleTimeStamp(Frame& frame, RefPtr<Inspector::ScriptArguments>&& arguments)
{
    FAST_RETURN_IF_NO_FRONTENDS(void());
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForFrame(frame))
        consoleTimeStampImpl(*instrumentingAgents, frame, WTF::move(arguments));
}

inline void InspectorInstrumentation::startProfiling(Page& page, JSC::ExecState* exec, const String &title)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        startProfilingImpl(*instrumentingAgents, exec, title);
}

inline void InspectorInstrumentation::stopProfiling(Page& page, JSC::ExecState* exec, const String &title)
{
    if (InstrumentingAgents* instrumentingAgents = instrumentingAgentsForPage(page))
        stopProfilingImpl(*instrumentingAgents, exec, title);
}

} // namespace WebCore

#endif // !defined(InspectorConsoleInstrumentation_h)
