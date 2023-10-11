/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef AnimationControllerPrivate_h
#define AnimationControllerPrivate_h

#include "AnimationBase.h"
#include "CSSPropertyNames.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AnimationBase;
class CompositeAnimation;
class Document;
class Element;
class Frame;
class RenderElement;
class RenderStyle;

enum SetChanged {
    DoNotCallSetChanged = 0,
    CallSetChanged = 1
};

class AnimationControllerPrivate {
    WTF_MAKE_NONCOPYABLE(AnimationControllerPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit AnimationControllerPrivate(Frame&);
    ~AnimationControllerPrivate();

    // Returns the time until the next animation needs to be serviced, or -1 if there are none.
    double updateAnimations(SetChanged callSetChanged = DoNotCallSetChanged);
    void updateAnimationTimer(SetChanged callSetChanged = DoNotCallSetChanged);

    CompositeAnimation& ensureCompositeAnimation(RenderElement&);
    bool clear(RenderElement&);

    void updateStyleIfNeededDispatcherFired();
    void startUpdateStyleIfNeededDispatcher();
    void addEventToDispatch(PassRefPtr<Element> element, const AtomicString& eventType, const String& name, double elapsedTime);
    void addElementChangeToDispatch(Ref<Element>&&);

    bool hasAnimations() const { return !m_compositeAnimations.isEmpty(); }

    bool isSuspended() const { return m_isSuspended; }
    void suspendAnimations();
    void resumeAnimations();
#if ENABLE(REQUEST_ANIMATION_FRAME)
    void animationFrameCallbackFired();
#endif

    void suspendAnimationsForDocument(Document*);
    void resumeAnimationsForDocument(Document*);
    void startAnimationsIfNotSuspended(Document*);

    bool isRunningAnimationOnRenderer(RenderElement&, CSSPropertyID, AnimationBase::RunningState) const;
    bool isRunningAcceleratedAnimationOnRenderer(RenderElement&, CSSPropertyID, AnimationBase::RunningState) const;

    bool pauseAnimationAtTime(RenderElement*, const AtomicString& name, double t);
    bool pauseTransitionAtTime(RenderElement*, const String& property, double t);
    unsigned numberOfActiveAnimations(Document*) const;

    PassRefPtr<RenderStyle> getAnimatedStyleForRenderer(RenderElement&);

    bool computeExtentOfAnimation(RenderElement&, LayoutRect&) const;

    double beginAnimationUpdateTime();
    void setBeginAnimationUpdateTime(double t) { m_beginAnimationUpdateTime = t; }
    
    void beginAnimationUpdate();
    void endAnimationUpdate();
    void receivedStartTimeResponse(double);
    
    void addToAnimationsWaitingForStyle(AnimationBase*);
    void removeFromAnimationsWaitingForStyle(AnimationBase*);

    void addToAnimationsWaitingForStartTimeResponse(AnimationBase*, bool willGetResponse);
    void removeFromAnimationsWaitingForStartTimeResponse(AnimationBase*);

    void animationWillBeRemoved(AnimationBase*);

    void updateAnimationTimerForRenderer(RenderElement&);

    bool allowsNewAnimationsWhileSuspended() const { return m_allowsNewAnimationsWhileSuspended; }
    void setAllowsNewAnimationsWhileSuspended(bool);

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    bool wantsScrollUpdates() const { return !m_animationsDependentOnScroll.isEmpty(); }
    void addToAnimationsDependentOnScroll(AnimationBase*);
    void removeFromAnimationsDependentOnScroll(AnimationBase*);

    void scrollWasUpdated();
    float scrollPosition() const { return m_scrollPosition; }
#endif

private:
    void animationTimerFired();

    void styleAvailable();
    void fireEventsAndUpdateStyle();
    void startTimeResponse(double t);

    HashMap<RenderElement*, RefPtr<CompositeAnimation>> m_compositeAnimations;
    Timer m_animationTimer;
    Timer m_updateStyleIfNeededDispatcher;
    Frame& m_frame;

#if PLATFORM(WKC)
public:
#endif
    class EventToDispatch {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:
        RefPtr<Element> element;
        AtomicString eventType;
        String name;
        double elapsedTime;
    };
#if PLATFORM(WKC)
private:
#endif
    
    Vector<EventToDispatch> m_eventsToDispatch;
    Vector<Ref<Element>> m_elementChangesToDispatch;
    
    double m_beginAnimationUpdateTime;

    typedef HashSet<RefPtr<AnimationBase>> AnimationsSet;
    AnimationsSet m_animationsWaitingForStyle;
    AnimationsSet m_animationsWaitingForStartTimeResponse;

    int m_beginAnimationUpdateCount;

    bool m_waitingForAsyncStartNotification;
    bool m_isSuspended;

    // Used to flag whether we should revert to previous buggy
    // behavior of allowing new transitions and animations to
    // run even when this object is suspended.
    bool m_allowsNewAnimationsWhileSuspended;

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    AnimationsSet m_animationsDependentOnScroll;
    float m_scrollPosition { 0 };
#endif
};

} // namespace WebCore

#endif // AnimationControllerPrivate_h
