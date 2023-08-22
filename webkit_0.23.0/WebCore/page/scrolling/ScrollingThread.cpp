/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollingThread.h"

#if ENABLE(ASYNC_SCROLLING)

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

ScrollingThread::ScrollingThread()
    : m_threadIdentifier(0)
{
}

bool ScrollingThread::isCurrentThread()
{
    auto threadIdentifier = ScrollingThread::singleton().m_threadIdentifier;
    return threadIdentifier && currentThread() == threadIdentifier;
}

void ScrollingThread::dispatch(std::function<void ()> function)
{
    auto& scrollingThread = ScrollingThread::singleton();
    scrollingThread.createThreadIfNeeded();

    {
        std::lock_guard<std::mutex> lock(singleton().m_functionsMutex);
        scrollingThread.m_functions.append(function);
    }

    scrollingThread.wakeUpRunLoop();
}

void ScrollingThread::dispatchBarrier(std::function<void ()> function)
{
    dispatch([function]() mutable {
        callOnMainThread(WTF::move(function));
    });
}

ScrollingThread& ScrollingThread::singleton()
{
    static NeverDestroyed<ScrollingThread> scrollingThread;

    return scrollingThread;
}

void ScrollingThread::createThreadIfNeeded()
{
    if (m_threadIdentifier)
        return;

    // Wait for the thread to initialize the run loop.
    {
        std::unique_lock<std::mutex> lock(m_initializeRunLoopMutex);

        m_threadIdentifier = createThread(threadCallback, this, "WebCore: Scrolling");
        
#if PLATFORM(COCOA)
        m_initializeRunLoopConditionVariable.wait(lock, [this]{ return m_threadRunLoop; });
#endif
    }
}

void ScrollingThread::threadCallback(void* scrollingThread)
{
    WTF::setCurrentThreadIsUserInteractive();
    static_cast<ScrollingThread*>(scrollingThread)->threadBody();
}

void ScrollingThread::threadBody()
{
    initializeRunLoop();
}

void ScrollingThread::dispatchFunctionsFromScrollingThread()
{
    ASSERT(isCurrentThread());

    Vector<std::function<void ()>> functions;
    
    {
        std::lock_guard<std::mutex> lock(m_functionsMutex);
        functions = WTF::move(m_functions);
    }

    for (auto& function : functions)
        function();
}

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING)
