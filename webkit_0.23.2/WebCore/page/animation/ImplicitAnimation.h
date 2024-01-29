/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#ifndef ImplicitAnimation_h
#define ImplicitAnimation_h

#include "AnimationBase.h"
#include "CSSPropertyNames.h"
#include "Document.h"

namespace WebCore {

class RenderElement;

// An ImplicitAnimation tracks the state of a transition of a specific CSS property
// for a single RenderElement.
class ImplicitAnimation : public AnimationBase {
public:
    static Ref<ImplicitAnimation> create(Animation& animation, CSSPropertyID animatingProperty, RenderElement* renderer, CompositeAnimation* compositeAnimation, RenderStyle* fromStyle)
    {
        return adoptRef(*new ImplicitAnimation(animation, animatingProperty, renderer, compositeAnimation, fromStyle));
    };
    
    CSSPropertyID transitionProperty() const { return m_transitionProperty; }
    CSSPropertyID animatingProperty() const { return m_animatingProperty; }

    virtual void onAnimationEnd(double elapsedTime) override;
    virtual bool startAnimation(double timeOffset) override;
    virtual void pauseAnimation(double timeOffset) override;
    virtual void endAnimation(bool fillingForwards = false) override;

    virtual bool animate(CompositeAnimation*, RenderElement*, const RenderStyle* currentStyle, RenderStyle* targetStyle, RefPtr<RenderStyle>& animatedStyle) override;
    virtual void getAnimatedStyle(RefPtr<RenderStyle>& animatedStyle) override;
    virtual void reset(RenderStyle* to);

    bool computeExtentOfTransformAnimation(LayoutRect&) const override;

    void setOverridden(bool);
    virtual bool overridden() const override { return m_overridden; }

    virtual bool affectsProperty(CSSPropertyID) const override;

    bool hasStyle() const { return m_fromStyle && m_toStyle; }

    bool isTargetPropertyEqual(CSSPropertyID, const RenderStyle*);

    void blendPropertyValueInStyle(CSSPropertyID, RenderStyle*);

    virtual double timeToNextService() override;
    
    bool active() const { return m_active; }
    void setActive(bool b) { m_active = b; }

protected:
    bool shouldSendEventForListener(Document::ListenerType) const;    
    bool sendTransitionEvent(const AtomicString&, double elapsedTime);

    void validateTransformFunctionList();
    void checkForMatchingFilterFunctionLists();
#if ENABLE(FILTERS_LEVEL_2)
    void checkForMatchingBackdropFilterFunctionLists();
#endif

private:
    ImplicitAnimation(Animation&, CSSPropertyID, RenderElement*, CompositeAnimation*, RenderStyle*);
    virtual ~ImplicitAnimation();

    // The two styles that we are blending.
    RefPtr<RenderStyle> m_fromStyle;
    RefPtr<RenderStyle> m_toStyle;

    CSSPropertyID m_transitionProperty; // Transition property as specified in the RenderStyle.
    CSSPropertyID m_animatingProperty; // Specific property for this ImplicitAnimation

    bool m_active { true }; // Used for culling the list of transitions.
    bool m_overridden { false }; // True when there is a keyframe animation that overrides the transitioning property
};

} // namespace WebCore

#endif // ImplicitAnimation_h
