/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 * Copyright (c) 2010-2017 ACCESS CO., LTD. All rights reserved.
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

#if !PLATFORM(WKC)
#include <mutex>
#else
#include <wkcmutex.h>
#endif
#include "CurrentTime.h"
#include "DateMath.h"
#include "dtoa.h"
#include "HashMap.h"
#include "MainThread.h"
#include "RandomNumberSeed.h"
#include <wtf/WTFThreadData.h>

#include "NotImplemented.h"

namespace WTF {

static Mutex& threadMapMutex()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Mutex, gMutex, ());
    return gMutex;
}

static Mutex& atomicallyInitializedStaticMutex()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Mutex, gAtomicMutex, ());
    return gAtomicMutex;
}

typedef struct WKCThreadIdentifier_ {
    void* m_instance;
    ThreadIdentifier m_identifier;
} WKCThreadIdentifier;
WKC_DEFINE_GLOBAL_PTR(Vector<WKCThreadIdentifier>*, gThreads, 0);
WKC_DEFINE_GLOBAL_UINT(gThreadsIndex, 0);

static ThreadIdentifier mainThreadIdentifier()
{
    // main thread identifier must be 1
    return 1;
}

void initializeThreading()
{
    // StringImpl::empty() does not construct its static string in a threadsafe fashion,
    // so ensure it has been initialized from here.
    StringImpl::empty();

    atomicallyInitializedStaticMutex();
    threadMapMutex();

    gThreads = new Vector<WKCThreadIdentifier>;
    WKCThreadIdentifier cur = {0};
    cur.m_instance = wkcThreadCurrentThreadPeer();
    cur.m_identifier = mainThreadIdentifier();
    gThreads->append(cur);
    gThreadsIndex = mainThreadIdentifier() + 1;

    initializeRandomNumberGenerator();
    initializeMainThread();
    wtfThreadData();
    s_dtoaP5Mutex = new Mutex;
    initializeDates();
}

void initializeCurrentThreadInternal(const char* threadName)
{
    wkcThreadSetCurrentThreadNamePeer(threadName);
}

void lockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex().lock();
}

void unlockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex().unlock();
}

ThreadIdentifier
createThreadInternal(ThreadFunction entryPoint, void* data, const char* name)
{
    void* instance = NULL;
    wkcThreadProc proc = (wkcThreadProc)entryPoint;
    instance = wkcThreadCreatePeer(proc, data, name);
    if (!instance)
        return 0;
    const WKCThreadIdentifier th = {
        instance,
        gThreadsIndex++
    };
    gThreads->append(th);
    return (ThreadIdentifier)th.m_identifier;
}

void setThreadNameInternal(const char*)
{
}

static void*
getThreadInstance(ThreadIdentifier id)
{
    int count = gThreads->size();
    for (int i=0; i<count; i++) {
        if (gThreads->at(i).m_identifier==id)
            return gThreads->at(i).m_instance;
    }
    return 0;
}

static void
removeThreadIdentifier(ThreadIdentifier id)
{
    int count = gThreads->size();
    for (int i=0; i<count; i++) {
        if (gThreads->at(i).m_identifier==id) {
            gThreads->remove(i);
            return;
        }
    }
}

int waitForThreadCompletion(ThreadIdentifier threadID)
{
    void* instance = getThreadInstance(threadID);
    if (!instance)
        return 0;
    int ret = wkcThreadJoinPeer(instance, 0);
    removeThreadIdentifier(threadID);
    return ret;
}

void detachThread(ThreadIdentifier threadID)
{
    void* instance = getThreadInstance(threadID);
    removeThreadIdentifier(threadID);
    wkcThreadDetachPeer(instance);
}

void yield()
{
    wkcThreadYieldPeer();
}

ThreadIdentifier currentThread()
{
    void* instance = NULL;
    instance = wkcThreadCurrentThreadPeer();

    int count = gThreads->size();
    for (int i=0; i<count; i++) {
        if (gThreads->at(i).m_instance==instance) {
            return gThreads->at(i).m_identifier;
        }
    }
    return 0;
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

bool ThreadCondition::timedWait(Mutex& mutex, double absoluteTime)
{
    // Time is in the past - return right away.
    if (absoluteTime < currentTime())
        return false;
    
    // Time is too far in the future for g_cond_timed_wait - wait forever.
    if (absoluteTime > WKC_INT_MAX) {
        wait(mutex);
        return true;
    }

    return wkcCondTimedWaitLightPeer((void *)m_condition, &mutex.impl(), (absoluteTime - currentTime()));
}

void ThreadCondition::signal()
{
    wkcCondSignalPeer((void *)m_condition);
}

void ThreadCondition::broadcast()
{
    wkcCondBroadcastPeer((void *)m_condition);
}

void changeThreadPriority(ThreadIdentifier, int)
{
    notImplemented();
}

}
