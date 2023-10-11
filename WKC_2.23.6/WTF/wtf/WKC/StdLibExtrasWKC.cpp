/*
 * Copyright (c) 2015-2021 ACCESS CO., LTD. All rights reserved.
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

#include "wkcmutex.h"
#include "wkcconditionvariable.h"
#include "wkcchrono.h"
#include "wkcthread.h"
#include <wkc/wkcclib.h>

#include <wkc/wkcpeer.h>

#include "limits"

#include "NotImplemented.h"

namespace std {

condition_variable::condition_variable()
{
    m_cond = wkcCondNewPeer();
}

condition_variable::~condition_variable()
{
    wkcCondDeletePeer(m_cond);
}

void
condition_variable::wait(std::unique_lock<std::mutex>& lock)
{
    if (!m_cond)
        return;
    WKCLightMutex* mutex = lock.platformLock().platformLock();
    wkcCondWaitLightPeer(m_cond, mutex);
}

void
condition_variable::wait_for(std::unique_lock<std::mutex>& lock, const std::chrono::milliseconds& timeout)
{
    if (!m_cond)
        return;
    WKCLightMutex* mutex = lock.platformLock().platformLock();
    wkcCondTimedWaitLightPeer(m_cond, mutex, (double)timeout.count() / 1000);
}

void
condition_variable::notify_one()
{
    if (!m_cond)
        return;
    wkcCondSignalPeer(m_cond);
}

void
condition_variable::notify_all()
{
    if (!m_cond)
        return;
    wkcCondBroadcastPeer(m_cond);
}

try_to_lock_t try_to_lock;

chrono::wkc_steady_clock::wkc_steady_clock()
{
}

chrono::wkc_steady_clock::~wkc_steady_clock()
{
}

chrono::wkc_time_point<chrono::wkc_steady_clock>
chrono::wkc_steady_clock::now()
{
    time_t t = wkc_time(NULL);
    chrono::seconds d((long long)t);
    chrono::wkc_time_point<chrono::wkc_steady_clock> ret(d);
    return ret;
}

chrono::wkc_time_point<chrono::wkc_system_clock>
chrono::wkc_system_clock::now()
{
    time_t t = wkc_time(NULL);
    chrono::seconds d((long long)t);
    chrono::wkc_time_point<chrono::wkc_system_clock> ret(d);
    return ret;
}

template<>
chrono::microseconds
chrono::wkc_time_point<chrono::wkc_steady_clock, chrono::duration<long long, std::micro> >::time_since_epoch() const
{
    long long diff = (long long)wkcGetBaseTickCountPeer();
    return chrono::microseconds(m_value.count() + diff);
}

template<>
chrono::microseconds
chrono::wkc_time_point<chrono::wkc_system_clock, chrono::duration<long long, std::micro> >::time_since_epoch() const
{
    long long diff = (long long)wkcGetBaseTickCountPeer();
    return chrono::microseconds(m_value.count() + diff);
}

time_t
chrono::wkc_system_clock::to_time_t(const std::chrono::wkc_time_point<system_clock>& point)
{
    return point.get().count() / (1000 * 1000);
}

// thread

#if !defined(__NX_TOOLCHAIN_VERSION__) || (__NX_TOOLCHAIN_MAJOR__  < 1) || ((__NX_TOOLCHAIN_MAJOR__  == 1) && (__NX_TOOLCHAIN_MINOR__ < 11)) || ((__NX_TOOLCHAIN_MAJOR__  == 1) && (__NX_TOOLCHAIN_MINOR__ == 11) && (__NX_TOOLCHAIN_PATCHLEVEL__ < 5))
thread::id::id()
    : m_id(0)
{
}

this_thread::this_thread()
{
}

this_thread::~this_thread()
{
}

void
this_thread::yield()
{
}

thread::id
this_thread::get_id()
{
    thread::id v(wkcThreadCurrentThreadPeer());
    return v;
}
#endif

template <class Rep, class Period>
void sleep_for(const chrono::duration<Rep, Period>& rel_time)
{
    notImplemented();
}

} // namespace std
