/*
* Copyright (C) 2018-2019 ACCESS CO., LTD. All rights reserved.
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
#include "WorkQueue.h"

namespace WTF {

WKC_DEFINE_GLOBAL_TYPE_ZERO(RunLoop*, gRunLoop);

void WorkQueue::initialize()
{
    auto thread = Thread::create("WKC: WorkQueue", [] {
        gRunLoop = &RunLoop::current();
        gRunLoop->run();
    }).ptr();

    thread->detach();
}

void WorkQueue::finalize()
{
    if (gRunLoop) {
        gRunLoop->stop();
        gRunLoop = nullptr;
    }
}

void WorkQueue::forceTerminate()
{
    finalize();
}

void WorkQueue::platformInitialize(const char*, Type, QOS)
{
}

void WorkQueue::platformInvalidate()
{
}

void WorkQueue::dispatch(Function<void()>&& function)
{
    if (!gRunLoop)
        return;

    gRunLoop->dispatch([function = WTFMove(function)] {
        function();
    });
}

void WorkQueue::dispatchAfter(Seconds delay, Function<void()>&& function)
{
    if (!gRunLoop)
        return;

    gRunLoop->dispatchAfter(delay, [function = WTFMove(function)] {
        function();
    });
}

} // namespace
