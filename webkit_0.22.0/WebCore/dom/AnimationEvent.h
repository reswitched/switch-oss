/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef AnimationEvent_h
#define AnimationEvent_h

#include "Event.h"

namespace WebCore {

struct AnimationEventInit : public EventInit {
    AnimationEventInit();

    String animationName;
    double elapsedTime;
};

class AnimationEvent final : public Event {
public:
    static Ref<AnimationEvent> create()
    {
        return adoptRef(*new AnimationEvent);
    }
    static Ref<AnimationEvent> create(const AtomicString& type, const String& animationName, double elapsedTime)
    {
        return adoptRef(*new AnimationEvent(type, animationName, elapsedTime));
    }
    static Ref<AnimationEvent> create(const AtomicString& type, const AnimationEventInit& initializer)
    {
        return adoptRef(*new AnimationEvent(type, initializer));
    }

    virtual ~AnimationEvent();

    const String& animationName() const;
    double elapsedTime() const;

    virtual EventInterface eventInterface() const override;

private:
    AnimationEvent();
    AnimationEvent(const AtomicString& type, const String& animationName, double elapsedTime);
    AnimationEvent(const AtomicString&, const AnimationEventInit&);

    String m_animationName;
    double m_elapsedTime;
};

} // namespace WebCore

#endif // AnimationEvent_h
