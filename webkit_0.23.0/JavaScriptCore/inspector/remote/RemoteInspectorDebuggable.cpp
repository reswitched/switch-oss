/*
 * Copyright (C) 2013 Apple Inc. All Rights Reserved.
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
#include "RemoteInspectorDebuggable.h"

#if ENABLE(REMOTE_INSPECTOR)

#include "EventLoop.h"
#include "InspectorFrontendChannel.h"
#include "RemoteInspector.h"

namespace Inspector {

RemoteInspectorDebuggable::RemoteInspectorDebuggable()
    : m_identifier(0)
    , m_allowed(false)
{
}

RemoteInspectorDebuggable::~RemoteInspectorDebuggable()
{
    RemoteInspector::singleton().unregisterDebuggable(this);
}

void RemoteInspectorDebuggable::init()
{
    RemoteInspector::singleton().registerDebuggable(this);
}

void RemoteInspectorDebuggable::update()
{
    RemoteInspector::singleton().updateDebuggable(this);
}

void RemoteInspectorDebuggable::setRemoteDebuggingAllowed(bool allowed)
{
    if (m_allowed == allowed)
        return;

    m_allowed = allowed;

    if (m_allowed && automaticInspectionAllowed())
        RemoteInspector::singleton().updateDebuggableAutomaticInspectCandidate(this);
    else
        RemoteInspector::singleton().updateDebuggable(this);
}

RemoteInspectorDebuggableInfo RemoteInspectorDebuggable::info() const
{
    RemoteInspectorDebuggableInfo info;
    info.identifier = identifier();
    info.type = type();
    info.name = name();
    info.url = url();
    info.hasLocalDebugger = hasLocalDebugger();
    info.remoteDebuggingAllowed = remoteDebuggingAllowed();
    return info;
}

void RemoteInspectorDebuggable::pauseWaitingForAutomaticInspection()
{
    ASSERT(m_identifier);
    ASSERT(m_allowed);
    ASSERT(automaticInspectionAllowed());

    EventLoop loop;
    while (RemoteInspector::singleton().waitingForAutomaticInspection(identifier()) && !loop.ended())
        loop.cycle();
}

void RemoteInspectorDebuggable::unpauseForInitializedInspector()
{
    RemoteInspector::singleton().setupCompleted(identifier());
}

} // namespace Inspector

#endif // ENABLE(REMOTE_INSPECTOR)
