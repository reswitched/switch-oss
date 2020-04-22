/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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
#include "Threading.h"

#include <wkc/wkcpeer.h>
#include <wkcmutex.h>

#include <wtf/HashMap.h>
#include <wtf/ThreadingPrimitives.h>

#include "NotImplemented.h"

namespace WTF {

static Mutex& atomicallyInitializedStaticMutex()
{
    WKC_DEFINE_STATIC_TYPE(Mutex*, gAtomicMutex, new Mutex());
    return *gAtomicMutex;
}

Thread::~Thread()
{
}

WKC_DEFINE_GLOBAL_UINT_ZERO(gThreadsIndex);

void Thread::initializeCurrentThreadInternal(const char* threadName)
{
    wkcThreadSetCurrentThreadNamePeer(threadName);
    initializeCurrentThreadEvenIfNonWTFCreated();
}

void Thread::initializeCurrentThreadEvenIfNonWTFCreated()
{
    atomicallyInitializedStaticMutex();
}

void Thread::initializePlatformThreading()
{
    gThreadsIndex = 1;
}

void lockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex().lock();
}

void unlockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex().unlock();
}

static void* wtfThreadEntryPoint(void* param)
{
    Thread::entryPoint(reinterpret_cast<Thread::NewThreadContext*>(param));
    return nullptr;
}

bool Thread::establishHandle(NewThreadContext* data)
{
    PlatformThreadHandle handle = wkcThreadCreatePeer(wtfThreadEntryPoint, data, data->name);
    if (!handle)
        return false;

    establishPlatformSpecificHandle(handle, 0);
    return true;
}

void Thread::changePriority(int delta)
{
    notImplemented();
}

int Thread::waitForCompletion()
{
    PlatformThreadHandle handle;
    {
        auto locker = holdLock(m_mutex);
        handle = m_handle;
    }

    int joinResult = wkcThreadJoinPeer(handle, 0);
    if (joinResult != 0)
        LOG_ERROR("Thread %u was unable to be joined.\n", m_id);

    auto locker = holdLock(m_mutex);
    ASSERT(joinableState() == Joinable);

    // If the thread has already exited, then do nothing. If the thread hasn't exited yet, then just signal that we've already joined on it.
    // In both cases, Thread::destructTLS() will take care of destroying Thread.
    if (!hasExited())
        didJoin();

    return joinResult;
}

void Thread::detach()
{
    auto locker = holdLock(m_mutex);
    wkcThreadDetachPeer(m_handle);
    if (!hasExited())
        didBecomeDetached();
}

auto Thread::suspend() -> Expected<void, PlatformSuspendError>
{
    RELEASE_ASSERT_WITH_MESSAGE(this != &Thread::current(), "We do not support suspending the current thread itself.");
    auto locker = holdLock(m_mutex);
    wkcThreadSuspendPeer(m_handle);
    return { };
}

void Thread::resume()
{
    auto locker = holdLock(m_mutex);
    wkcThreadResumePeer(m_handle);
}

size_t Thread::getRegisters(PlatformRegisters& registers)
{
    notImplemented();
    return 0;
}

Thread& Thread::initializeCurrentTLS()
{
    // Not a WTF-created thread, Thread is not established yet.
    Ref<Thread> thread = adoptRef(*new Thread());
    thread->establishPlatformSpecificHandle(wkcThreadCurrentThreadPeer(), 0);
    thread->initializeInThread();
    initializeCurrentThreadEvenIfNonWTFCreated();

    return initializeTLS(WTFMove(thread));
}

ThreadIdentifier Thread::currentID()
{
    return current().id();
}

void Thread::establishPlatformSpecificHandle(PlatformThreadHandle handle, ThreadIdentifier)
{
    auto locker = holdLock(m_mutex);

    m_handle = handle;
    m_id = gThreadsIndex++;
}

void Thread::initializeTLSKey()
{
    threadSpecificKeyCreate(&s_key, destructTLS);
}

Thread& Thread::initializeTLS(Ref<Thread>&& thread)
{
    // We leak the ref to keep the Thread alive while it is held in TLS. destructTLS will deref it later at thread destruction time.
    auto& threadInTLS = thread.leakRef();
    ASSERT(s_key != InvalidThreadSpecificKey);
    threadSpecificSet(s_key, &threadInTLS);
    return threadInTLS;
}

void Thread::destructTLS(void* data)
{
    Thread* thread = static_cast<Thread*>(data);
    ASSERT(thread);

    if (thread->m_isDestroyedOnce) {
        thread->didExit();
        thread->deref();
        return;
    }

    thread->m_isDestroyedOnce = true;
    // Re-setting the value for key causes another destructTLS() call after all other thread-specific destructors were called.
    ASSERT(s_key != InvalidThreadSpecificKey);
    threadSpecificSet(s_key, thread);
}

void Thread::yield()
{
    wkcThreadYieldPeer();
}

Mutex::Mutex()
{
    wkcLightMutexInitializePeer(&m_mutex);
}

Mutex::~Mutex()
{
    wkcLightMutexFinalizePeer(&m_mutex);
}

void Mutex::lock()
{
    wkcLightMutexLockPeer(&m_mutex);
}

bool Mutex::tryLock()
{
    return wkcLightMutexTryLockPeer(&m_mutex);
}

void Mutex::unlock()
{
    wkcLightMutexUnlockPeer(&m_mutex);
}

ThreadCondition::ThreadCondition()
{
    m_condition = wkcCondNewPeer();
    if (!m_condition)
        wkcMemoryNotifyNoMemoryPeer(sizeof(void*));
}

ThreadCondition::~ThreadCondition()
{
    if (m_condition) {
        wkcCondDeletePeer((void *)m_condition);
    }
}

void ThreadCondition::wait(Mutex& mutex)
{
    wkcCondWaitLightPeer((void *)m_condition, &mutex.impl());
}

bool ThreadCondition::timedWait(Mutex& mutex, WallTime absoluteTime)
{
    // Time is in the past - return right away.
    if (absoluteTime < WallTime::now())
        return false;
    
    // Time is too far in the future for g_cond_timed_wait - wait forever.
    if (absoluteTime > WallTime::now() + Seconds(WKC_INT_MAX)) {
        wait(mutex);
        return true;
    }

    return wkcCondTimedWaitLightPeer((void *)m_condition, &mutex.impl(), (absoluteTime - WallTime::now()).value());
}

void ThreadCondition::signal()
{
    wkcCondSignalPeer((void *)m_condition);
}

void ThreadCondition::broadcast()
{
    wkcCondBroadcastPeer((void *)m_condition);
}

}
