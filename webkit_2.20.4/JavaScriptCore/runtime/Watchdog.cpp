/*
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
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
#include <wtf/CPUTime.h>
#include <wtf/MathExtras.h>
#if PLATFORM(WKC)
#include "WKCWebViewPrivate.h"
#endif

namespace JSC {

#if PLATFORM(WKC)
    static const Seconds cTimeLimit = 1_s;
#endif

Watchdog::Watchdog(VM* vm)
    : m_vm(vm)
    , m_timeLimit(noTimeLimit)
    , m_cpuDeadline(noTimeLimit)
    , m_deadline(MonotonicTime::infinity())
    , m_callback(0)
    , m_callbackData1(0)
    , m_callbackData2(0)
    , m_timerQueue(WorkQueue::create("jsc.watchdog.queue", WorkQueue::Type::Serial, WorkQueue::QOS::Utility))
{
}

void Watchdog::setTimeLimit(Seconds limit,
    ShouldTerminateCallback callback, void* data1, void* data2)
{
    ASSERT(m_vm->currentThreadIsHoldingAPILock());

    m_timeLimit = limit;
    m_callback = callback;
    m_callbackData1 = data1;
    m_callbackData2 = data2;

    if (m_hasEnteredVM && hasTimeLimit())
        startTimer(m_timeLimit);
}

bool Watchdog::shouldTerminate(JSGlobalObject* globalObject)
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
    ASSERT(m_vm->currentThreadIsHoldingAPILock());
    if (MonotonicTime::now() < m_deadline)
        return false; // Just a stale timer firing. Nothing to do.

    // Set m_deadline to MonotonicTime::infinity() here so that we can reject all future
    // spurious wakes.
    m_deadline = MonotonicTime::infinity();

    auto cpuTime = CPUTime::forCurrentThread();
    if (cpuTime < m_cpuDeadline) {
        auto remainingCPUTime = m_cpuDeadline - cpuTime;
        startTimer(remainingCPUTime);
        return false;
    }

    // Note: we should not be holding the lock while calling the callbacks. The callbacks may
    // call setTimeLimit() which will try to lock as well.

    // If m_callback is not set, then we terminate by default.
    // Else, we let m_callback decide if we should terminate or not.
#if !PLATFORM(WKC)
    bool needsTermination = !m_callback
        || m_callback(globalObject, m_callbackData1, m_callbackData2);
#else
    bool needsTermination = WKC::shouldInterruptJavaScript(globalObject, m_callbackData1, m_callbackData2);
#endif
    if (needsTermination)
        return true;

    // If we get here, then the callback above did not want to terminate execution. As a
    // result, the callback may have done one of the following:
    //   1. cleared the time limit (i.e. watchdog is disabled),
    //   2. set a new time limit via Watchdog::setTimeLimit(), or
    //   3. did nothing (i.e. allow another cycle of the current time limit).
    //
    // In the case of 1, we don't have to do anything.
    // In the case of 2, Watchdog::setTimeLimit() would already have started the timer.
    // In the case of 3, we need to re-start the timer here.

    ASSERT(m_hasEnteredVM);
#if !PLATFORM(WKC)
    bool callbackAlreadyStartedTimer = (m_cpuDeadline != noTimeLimit);
    if (hasTimeLimit() && !callbackAlreadyStartedTimer)
        startTimer(m_timeLimit);
#else
    startTimer(m_timeLimit);
#endif

    return false;
}

bool Watchdog::hasTimeLimit()
{
#if !PLATFORM(WKC)
    return (m_timeLimit != noTimeLimit);
#else
    return true;
#endif
}

void Watchdog::enteredVM()
{
    m_hasEnteredVM = true;
    if (hasTimeLimit())
        startTimer(m_timeLimit);
}

void Watchdog::exitedVM()
{
    ASSERT(m_hasEnteredVM);
    stopTimer();
    m_hasEnteredVM = false;
}

void Watchdog::startTimer(Seconds timeLimit)
{
    ASSERT(m_hasEnteredVM);
    ASSERT(m_vm->currentThreadIsHoldingAPILock());
    ASSERT(hasTimeLimit());
    ASSERT(timeLimit <= m_timeLimit);

#if PLATFORM(WKC)
    if (timeLimit < 0_s || cTimeLimit < timeLimit)
        timeLimit = cTimeLimit;
#endif
    m_cpuDeadline = CPUTime::forCurrentThread() + timeLimit;
    auto now = MonotonicTime::now();
    auto deadline = now + timeLimit;

    if ((now < m_deadline) && (m_deadline <= deadline))
        return; // Wait for the current active timer to expire before starting a new one.

    // Else, the current active timer won't fire soon enough. So, start a new timer.
    m_deadline = deadline;

    // We need to ensure that the Watchdog outlives the timer.
    // For the same reason, the timer may also outlive the VM that the Watchdog operates on.
    // So, we always need to null check m_vm before using it. The VM will notify the Watchdog
    // via willDestroyVM() before it goes away.
    RefPtr<Watchdog> protectedThis = this;
    m_timerQueue->dispatchAfter(timeLimit, [this, protectedThis] {
        LockHolder locker(m_lock);
        if (m_vm)
            m_vm->notifyNeedWatchdogCheck();
    });
}

void Watchdog::stopTimer()
{
    ASSERT(m_hasEnteredVM);
    ASSERT(m_vm->currentThreadIsHoldingAPILock());
    m_cpuDeadline = noTimeLimit;
}

void Watchdog::willDestroyVM(VM* vm)
{
    LockHolder locker(m_lock);
    ASSERT_UNUSED(vm, m_vm == vm);
    m_vm = nullptr;
}

} // namespace JSC
