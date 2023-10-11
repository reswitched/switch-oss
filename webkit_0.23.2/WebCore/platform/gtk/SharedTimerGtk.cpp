/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * All rights reserved.
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
#include "SharedTimer.h"

#include <wtf/glib/GMainLoopSource.h>

namespace WebCore {

static GMainLoopSource gSharedTimer;
static void (*sharedTimerFiredFunction)();

void setSharedTimerFiredFunction(void (*f)())
{
    sharedTimerFiredFunction = f;
    if (!sharedTimerFiredFunction)
        gSharedTimer.cancel();
}

void setSharedTimerFireInterval(double interval)
{
    ASSERT(sharedTimerFiredFunction);

    // This is GDK_PRIORITY_REDRAW, but we don't want to depend on GDK here just to use a constant.
    static const int priority = G_PRIORITY_HIGH_IDLE + 20;
    gSharedTimer.scheduleAfterDelay("[WebKit] sharedTimerTimeoutCallback", std::function<void()>(sharedTimerFiredFunction),
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(interval)), priority);
}

void stopSharedTimer()
{
    gSharedTimer.cancel();
}

void invalidateSharedTimer()
{
}

}
