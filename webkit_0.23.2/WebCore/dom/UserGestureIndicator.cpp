/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "UserGestureIndicator.h"

#include "Document.h"
#include <wtf/MainThread.h>

namespace WebCore {

static bool isDefinite(ProcessingUserGestureState state)
{
    return state == DefinitelyProcessingUserGesture || state == DefinitelyNotProcessingUserGesture || state == DefinitelyProcessingPotentialUserGesture;
}

ProcessingUserGestureState UserGestureIndicator::s_state = DefinitelyNotProcessingUserGesture;

UserGestureIndicator::UserGestureIndicator(ProcessingUserGestureState state, Document* document)
    : m_previousState(s_state)
{
    // Silently ignore UserGestureIndicators on non main threads.
    if (!isMainThread())
        return;
    // We overwrite s_state only if the caller is definite about the gesture state.
    if (isDefinite(state))
        s_state = state;
    ASSERT(isDefinite(s_state));

    if (document && s_state == DefinitelyProcessingUserGesture)
        document->topDocument().updateLastHandledUserGestureTimestamp();
}

UserGestureIndicator::~UserGestureIndicator()
{
    if (!isMainThread())
        return;
    s_state = m_previousState;
    ASSERT(isDefinite(s_state));
}

bool UserGestureIndicator::processingUserGesture()
{
    return isMainThread() ? s_state == DefinitelyProcessingUserGesture : false;
}

bool UserGestureIndicator::processingUserGestureForMedia()
{
    if (!isMainThread())
        return false;

    return s_state == DefinitelyProcessingUserGesture || s_state == DefinitelyProcessingPotentialUserGesture;
}

}
