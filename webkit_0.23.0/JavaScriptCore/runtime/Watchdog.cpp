/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (c) 2016 ACCESS CO.,LTD. All rights reserved.
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
#include "Watchdog.h"

#include "CallFrame.h"
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>

namespace JSC {

#define NO_LIMIT std::chrono::microseconds::max()

Watchdog::Watchdog()
    : m_timerDidFire(false)
    , m_didFire(false)
    , m_limit(NO_LIMIT)
    , m_startTime(0)
    , m_elapsedTime(0)
    , m_reentryCount(0)
    , m_isStopped(true)
    , m_callback(0)
    , m_callbackData1(0)
    , m_callbackData2(0)
{
    initTimer();
}

Watchdog::~Watchdog()
{
    ASSERT(!isArmed());
    stopCountdown();
    destroyTimer();
}

void Watchdog::setTimeLimit(VM& vm, std::chrono::microseconds limit,
    ShouldTerminateCallback callback, void* data1, void* data2)
{
    bool wasEnabled = isEnabled();

    if (!m_isStopped)
        stopCountdown();

    m_didFire = false; // Reset the watchdog.

    m_limit = limit;
    m_callback = callback;
    m_callbackData1 = data1;
    m_callbackData2 = data2;

    // If this is the first time that timeout is being enabled, then any
    // previously JIT compiled code will not have the needed polling checks.
    // Hence, we need to flush all the pre-existing compiled code.
    //
    // However, if the timeout is already enabled, and we're just changing the
    // timeout value, then any existing JITted code will have the appropriate
    // polling checks. Hence, there is no need to re-do this flushing.
    if (!wasEnabled) {
        // And if we've previously compiled any functions, we need to revert
        // them because they don't have the needed polling checks yet.
        vm.releaseExecutableMemory();
    }

    startCountdownIfNeeded();
}

bool Watchdog::didFire(ExecState* exec)
{
#if PLATFORM(WKC)
    wkcThreadCheckAlivePeer();
    if (wkcThreadCurrentIsMainThreadPeer()) {
        wkcThreadYieldPeer();
    } else {
        // For Worker threads
        // Worker threads are created at the almost same time.
        // It means each watchdog fires and yields everytime at close timing.
        // Then the same thread would be activated again.
        // So we need to omit yield sometimes.

        // No srand() done and rand() is not thread safe
        // but we just want to obtain a chance of about 50% probability.
        if (rand() % 2 == 1) {
            wkcThreadYieldPeer();
        }
    }
#endif
    if (m_didFire)
        return true;

    if (!m_timerDidFire)
        return false;
    m_timerDidFire = false;
    stopCountdown();

    auto currentTime = currentCPUTime();
    auto deltaTime = currentTime - m_startTime;
    auto totalElapsedTime = m_elapsedTime + deltaTime;
#if !PLATFORM(WKC)
    if (totalElapsedTime > m_limit) {
#else
    if (totalElapsedTime >= m_limit) {
#endif
        // Case 1: the allowed CPU time has elapsed.

        // If m_callback is not set, then we terminate by default.
        // Else, we let m_callback decide if we should terminate or not.
        bool needsTermination = !m_callback
            || m_callback(exec, m_callbackData1, m_callbackData2);
        if (needsTermination) {
            m_didFire = true;
            return true;
        }

        // The m_callback may have set a new limit. So, we may need to restart
        // the countdown.
        startCountdownIfNeeded();

    } else {
        // Case 2: the allowed CPU time has NOT elapsed.

        // Tell the timer to alarm us again when it thinks we've reached the
        // end of the allowed time.
        auto remainingTime = m_limit - totalElapsedTime;
        m_elapsedTime = totalElapsedTime;
        m_startTime = currentTime;
        startCountdown(remainingTime);
    }

    return false;
}

bool Watchdog::isEnabled()
{
    return (m_limit != NO_LIMIT);
}

void Watchdog::fire()
{
    m_didFire = true;
}

void Watchdog::arm()
{
    m_reentryCount++;
    if (m_reentryCount == 1)
        startCountdownIfNeeded();
}

void Watchdog::disarm()
{
    ASSERT(m_reentryCount > 0);
    if (m_reentryCount == 1)
        stopCountdown();
    m_reentryCount--;
}

void Watchdog::startCountdownIfNeeded()
{
    if (!m_isStopped)
        return; // Already started.

    if (!isArmed())
        return; // Not executing JS script. No need to start.

    if (isEnabled()) {
        m_elapsedTime = std::chrono::microseconds::zero();
        m_startTime = currentCPUTime();
        startCountdown(m_limit);
    }
}

void Watchdog::startCountdown(std::chrono::microseconds limit)
{
    ASSERT(m_isStopped);
    m_isStopped = false;
    startTimer(limit);
}

void Watchdog::stopCountdown()
{
    if (m_isStopped)
        return;
    stopTimer();
    m_isStopped = true;
}

} // namespace JSC
