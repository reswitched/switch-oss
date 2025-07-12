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

#include "config.h"
#include "NavigationEvent.h"

#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(NavigationEvent);

EventInterface NavigationEvent::eventInterface() const
{
    return NavigationEventInterfaceType;
}

bool NavigationEvent::isNavigationEvent() const
{
    return true;
}

NavigationEvent::NavigationEvent(const AtomString& type, CanBubble canBubble, IsCancelable isCancelable, RefPtr<WindowProxy>&& view, int detail, SpatialNavigationDirection dir, RefPtr<EventTarget>&& relatedTarget)
    : UIEvent(type, canBubble, isCancelable, IsComposed::Yes, WTFMove(view), detail)
    , m_dir(dir)
    , m_relatedTarget(WTFMove(relatedTarget))
{
}

NavigationEvent::NavigationEvent(const AtomString& type, const Init& initializer)
    : UIEvent(type, initializer)
    , m_dir(initializer.dir)
    , m_relatedTarget(initializer.relatedTarget)
{
}

} // namespace WebCore
