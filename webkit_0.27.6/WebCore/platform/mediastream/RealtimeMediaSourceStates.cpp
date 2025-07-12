/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#if ENABLE(MEDIA_STREAM)
#include "RealtimeMediaSourceStates.h"

#include <wtf/NeverDestroyed.h>

namespace WebCore {

const AtomicString& RealtimeMediaSourceStates::facingMode(RealtimeMediaSourceStates::VideoFacingMode mode)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> userFacing("user", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> environmentFacing("environment", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> leftFacing("left", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> rightFacing("right", AtomicString::ConstructFromLiteral);
    
    switch (mode) {
    case RealtimeMediaSourceStates::User:
        return userFacing;
    case RealtimeMediaSourceStates::Environment:
        return environmentFacing;
    case RealtimeMediaSourceStates::Left:
        return leftFacing;
    case RealtimeMediaSourceStates::Right:
        return rightFacing;
    case RealtimeMediaSourceStates::Unknown:
        return emptyAtom;
    }
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, userFacing, 0);
    WKC_DEFINE_STATIC_PTR(AtomicString*, environmentFacing, 0);
    WKC_DEFINE_STATIC_PTR(AtomicString*, leftFacing, 0);
    WKC_DEFINE_STATIC_PTR(AtomicString*, rightFacing, 0);
    if (!userFacing)
        userFacing = new AtomicString("user", AtomicString::ConstructFromLiteral);
    if (!environmentFacing)
        environmentFacing = new AtomicString("environment", AtomicString::ConstructFromLiteral);
    if (!leftFacing)
        leftFacing = new AtomicString("left", AtomicString::ConstructFromLiteral);
    if (!rightFacing)
        rightFacing = new AtomicString("right", AtomicString::ConstructFromLiteral);
    
    switch (mode) {
    case RealtimeMediaSourceStates::User:
        return *userFacing;
    case RealtimeMediaSourceStates::Environment:
        return *environmentFacing;
    case RealtimeMediaSourceStates::Left:
        return *leftFacing;
    case RealtimeMediaSourceStates::Right:
        return *rightFacing;
    case RealtimeMediaSourceStates::Unknown:
        return emptyAtom;
    }
#endif
    
    ASSERT_NOT_REACHED();
    return emptyAtom;
}

const AtomicString& RealtimeMediaSourceStates::sourceType(RealtimeMediaSourceStates::SourceType sourceType)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> none("none", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> camera("camera", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> microphone("microphone", AtomicString::ConstructFromLiteral);
    
    switch (sourceType) {
    case RealtimeMediaSourceStates::None:
        return none;
    case RealtimeMediaSourceStates::Camera:
        return camera;
    case RealtimeMediaSourceStates::Microphone:
        return microphone;
    }
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, none, 0);
    WKC_DEFINE_STATIC_PTR(AtomicString*, camera, 0);
    WKC_DEFINE_STATIC_PTR(AtomicString*, microphone, 0);
    if (!none)
        none = new AtomicString("none", AtomicString::ConstructFromLiteral);
    if (!camera)
        camera = new AtomicString("camera", AtomicString::ConstructFromLiteral);
    if (!microphone)
        microphone = new AtomicString("microphone", AtomicString::ConstructFromLiteral);
    
    switch (sourceType) {
    case RealtimeMediaSourceStates::None:
        return *none;
    case RealtimeMediaSourceStates::Camera:
        return *camera;
    case RealtimeMediaSourceStates::Microphone:
        return *microphone;
    }
#endif
    
    ASSERT_NOT_REACHED();
    return emptyAtom;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
