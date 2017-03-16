/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * All rights reserved.
 * Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedTimer.h"

#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>

#include <wkc/wkcpeer.h>

namespace WebCore {

static void* sharedTimer()
{
    // gSharedTimer shold be deleted at finalize / forceTerminate of peer_timer.
    WKC_DEFINE_STATIC_PTR(void*, gSharedTimer, wkcTimerNewPeer());
    return gSharedTimer;
}

static void* sharedTimerFiredFunction(void(*f)(), bool set)
{
    WKC_DEFINE_STATIC_PTR(void*, gSharedTimerFiredFunction, 0);
    if (set) {
        gSharedTimerFiredFunction = (void *)f;
    }
    return gSharedTimerFiredFunction;
}

void setSharedTimerFiredFunction(void (*f)())
{
    sharedTimerFiredFunction(f, true);
}

static bool timeout_cb(void* data)
{
    void(*func)() = (void(*)())sharedTimerFiredFunction(0, false);
    if (func)
        func();

    return false;
}

void setSharedTimerFireInterval(double interval)
{
    stopSharedTimer();
    wkcTimerStartOneShotPeer(sharedTimer(), interval, timeout_cb, NULL);
    // just ignore the error
}

void stopSharedTimer()
{
    wkcTimerCancelPeer(sharedTimer());
}

void invalidateSharedTimer()
{
}

}
