/*
* Copyright (C) 2016 ACCESS CO., LTD. All rights reserved.
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
#include "Watchdog.h"

#include <wkc/wkcpeer.h>

namespace JSC {

void Watchdog::initTimer()
{
    m_timer = wkcTimerNewPeer();
    if (!m_timer)
        CRASH();
    m_lastDuration = 0;
    m_timerStarted = false;
}

void Watchdog::destroyTimer()
{
    wkcTimerDeletePeer(m_timer);
}

static bool
timerProc(void* ctx)
{
    Watchdog* self = static_cast<Watchdog *>(ctx);
    bool* ptr = (bool *)self->timerDidFireAddress();
    *ptr = true;
    return false;
}

void Watchdog::startTimer(std::chrono::microseconds duration)
{
    m_lastDuration = (double)duration.count() / 1000 / 1000;
    m_timerStarted = true;
    wkcTimerStartOneShotDirectPeer(m_timer, m_lastDuration, timerProc, this);
}

void Watchdog::stopTimer()
{
    wkcTimerCancelPeer(m_timer);
    m_timerStarted = false;
}

void Watchdog::suspend()
{
    wkcTimerCancelPeer(m_timer);
}

void Watchdog::resume()
{
    if (m_timerStarted)
        wkcTimerStartOneShotDirectPeer(m_timer, m_lastDuration, timerProc, this);
}

void Watchdog::restart()
{
    m_timerStarted = true;
    m_timerDidFire = false;
    m_didFire = false;
}

} // namespace JSC
