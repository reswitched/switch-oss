/*
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

#pragma once

#include "EventTarget.h"
#include "SpatialNavigationDirection.h"
#include "UIEvent.h"

namespace WebCore {

class NavigationEvent final : public UIEvent {
    WTF_MAKE_ISO_ALLOCATED(NavigationEvent);
public:
    static Ref<NavigationEvent> create(const AtomString& type, CanBubble canBubble, IsCancelable cancelable, RefPtr<WindowProxy>&& view, int detail, SpatialNavigationDirection dir, RefPtr<EventTarget>&& relatedTarget)
    {
        return adoptRef(*new NavigationEvent(type, canBubble, cancelable, WTFMove(view), detail, dir, WTFMove(relatedTarget)));
    }

    static Ref<NavigationEvent> createForBindings()
    {
        return adoptRef(*new NavigationEvent);
    }

    struct Init : UIEventInit {
        SpatialNavigationDirection dir;
        RefPtr<EventTarget> relatedTarget;
    };

    static Ref<NavigationEvent> create(const AtomString& type, const Init& initializer)
    {
        return adoptRef(*new NavigationEvent(type, initializer));
    }

    SpatialNavigationDirection dir() const { return m_dir; }
    EventTarget* relatedTarget() const final { return m_relatedTarget.get(); }

private:
    NavigationEvent() = default;
    NavigationEvent(const AtomString& type, CanBubble, IsCancelable, RefPtr<WindowProxy>&&, int, SpatialNavigationDirection, RefPtr<EventTarget>&&);
    NavigationEvent(const AtomString& type, const Init&);

    EventInterface eventInterface() const final;
    bool isNavigationEvent() const final;

    void setRelatedTarget(EventTarget* relatedTarget) final { m_relatedTarget = relatedTarget; }

    SpatialNavigationDirection m_dir { SpatialNavigationDirection::Up };
    RefPtr<EventTarget> m_relatedTarget;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_EVENT(NavigationEvent)
