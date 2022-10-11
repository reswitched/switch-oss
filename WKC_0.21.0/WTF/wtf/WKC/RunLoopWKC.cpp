/*
* Copyright (C) 2015-2017 ACCESS CO., LTD. All rights reserved.
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
#include "RunLoop.h"

#include <wkc/wkcpeer.h>

namespace WTF {

void RunLoop::run()
{
}

void RunLoop::stop()
{
}

RunLoop::RunLoop()
{
}

RunLoop::~RunLoop()
{
}

void RunLoop::wakeUp()
{
}

// RunLoop::Timer

bool RunLoop::TimerBase::timerFired(void* runLoop)
{
    TimerBase* self = (TimerBase *)runLoop;
    self->fired();

    if (self->m_isRepeating)
      wkcTimerStartOneShotPeer(self->m_timer, self->m_nextFireInterval, timerFired, self);
    else
        self->m_isRunning = false;

    return false;
}

RunLoop::TimerBase::TimerBase(RunLoop& runLoop)
    : m_runLoop(runLoop)
    , m_isRunning(false)
    , m_isRepeating(false)
    , m_nextFireInterval(0)
{
    m_timer = wkcTimerNewPeer();
    if (!m_timer)
        CRASH();
}

RunLoop::TimerBase::~TimerBase()
{
    if (!m_timer)
        return;

    stop();
    wkcTimerDeletePeer(m_timer);
}

void RunLoop::TimerBase::start(double nextFireInterval, bool repeat)
{
    m_isRepeating = repeat;
    m_isRunning = true;
    m_nextFireInterval = nextFireInterval;
    wkcTimerStartOneShotPeer(m_timer, m_nextFireInterval, timerFired, this);
}

void RunLoop::TimerBase::stop()
{
    wkcTimerCancelPeer(m_timer);
    m_isRunning = false;
}

bool RunLoop::TimerBase::isActive() const
{
    return m_isRunning;
}

} // namespace
