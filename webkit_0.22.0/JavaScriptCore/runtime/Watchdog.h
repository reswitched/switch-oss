/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef Watchdog_h
#define Watchdog_h

#if OS(DARWIN)
#include <dispatch/dispatch.h>    
#endif

#if !PLATFORM(WKC)
#include <chrono>
#else
#include "wkcchrono.h"
#endif

namespace JSC {

class ExecState;
class VM;

class Watchdog {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    class Scope;

    Watchdog();
    ~Watchdog();

    typedef bool (*ShouldTerminateCallback)(ExecState*, void* data1, void* data2);
    void setTimeLimit(VM&, std::chrono::microseconds limit, ShouldTerminateCallback = 0, void* data1 = 0, void* data2 = 0);

    // This version of didFire() will check the elapsed CPU time and call the
    // callback (if needed) to determine if the watchdog should fire.
    bool didFire(ExecState*);

    bool isEnabled();

    // This version of didFire() is a more efficient version for when we want
    // to know if the watchdog has fired in the past, and not whether it should
    // fire right now.
    bool didFire() { return m_didFire; }
    JS_EXPORT_PRIVATE void fire();

    void* timerDidFireAddress() { return &m_timerDidFire; }

#if PLATFORM(WKC)
    void suspend();
    void resume();
    void restart();
#endif

private:
    void arm();
    void disarm();
    void startCountdownIfNeeded();
    void startCountdown(std::chrono::microseconds limit);
    void stopCountdown();
    bool isArmed() { return !!m_reentryCount; }

    // Platform specific timer implementation:
    void initTimer();
    void destroyTimer();
    void startTimer(std::chrono::microseconds limit);
    void stopTimer();

    // m_timerDidFire (above) indicates whether the timer fired. The Watchdog
    // still needs to check if the allowed CPU time has elapsed. If so, then
    // the Watchdog fires and m_didFire will be set.
    // NOTE: m_timerDidFire is only set by the platform specific timer
    // (probably from another thread) but is only cleared in the script thread.
    bool m_timerDidFire;
    bool m_didFire;

    std::chrono::microseconds m_limit;
    std::chrono::microseconds m_startTime;
    std::chrono::microseconds m_elapsedTime;

    int m_reentryCount;
    bool m_isStopped;

    ShouldTerminateCallback m_callback;
    void* m_callbackData1;
    void* m_callbackData2;

#if OS(DARWIN) && !PLATFORM(EFL) && !PLATFORM(GTK)
    dispatch_queue_t m_queue;
    dispatch_source_t m_timer;
#endif

#if PLATFORM(WKC)
    void* m_timer;
    double m_lastDuration;
    bool m_timerStarted;
#endif

    friend class Watchdog::Scope;
    friend class LLIntOffsetsExtractor;
};

class Watchdog::Scope {
public:
    Scope(Watchdog* watchdog)
        : m_watchdog(watchdog)
    {
        if (!watchdog)
            return;
        m_watchdog->arm();
    }
    
    ~Scope()
    {
        if (!m_watchdog)
            return;
        m_watchdog->disarm();
    }

private:
    Watchdog* m_watchdog;
};

} // namespace JSC

#endif // Watchdog_h
