/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
#include "PointerEvent.h"

#if ENABLE(TOUCH_EVENTS)

#include "EventNames.h"

namespace WebCore {

static const AtomString& pointerEventType(PlatformEvent::Type type)
{
    switch (type) {
    case PlatformEvent::Type::TouchStart:
        return eventNames().pointerdownEvent;
    case PlatformEvent::Type::TouchMove:
        return eventNames().pointermoveEvent;
    case PlatformEvent::Type::TouchEnd:
        return eventNames().pointerupEvent;
    case PlatformEvent::Type::TouchCancel:
        return eventNames().pointercancelEvent;
    }
    ASSERT_NOT_REACHED();
    return nullAtom();
}

static short buttonForType(const AtomString& type)
{
    return type == eventNames().pointermoveEvent ? -1 : 0;
}

static unsigned short buttonsForType(const AtomString& type)
{
    // We have contact with the touch surface for most events except when we've released the touch or canceled it.
    return (type == eventNames().pointerupEvent || type == eventNames().pointeroutEvent || type == eventNames().pointerleaveEvent || type == eventNames().pointercancelEvent) ? 0 : 1;
}

Ref<PointerEvent> PointerEvent::create(const PlatformTouchEvent& event, unsigned index, bool isPrimary, Ref<WindowProxy>&& view)
{
    const auto& type = pointerEventType(event.type());
    return adoptRef(*new PointerEvent(type, event, typeIsCancelable(type), index, isPrimary, WTFMove(view)));
}

Ref<PointerEvent> PointerEvent::create(const String& type, const PlatformTouchEvent& event, unsigned index, bool isPrimary, Ref<WindowProxy>&& view)
{
    return adoptRef(*new PointerEvent(type, event, typeIsCancelable(type), index, isPrimary, WTFMove(view)));
}

PointerEvent::PointerEvent(const AtomString& type, const PlatformTouchEvent& event, IsCancelable isCancelable, unsigned index, bool isPrimary, Ref<WindowProxy>&& view)
    : MouseEvent(type, typeCanBubble(type), isCancelable, typeIsComposed(type), event.timestamp().approximateMonotonicTime(), WTFMove(view), 0, event.touchPoints()[0].screenPos(), event.touchPoints()[0].pos(), { }, event.modifiers(), buttonForType(type), buttonsForType(type), nullptr, 0, 0, IsSimulated::No, IsTrusted::Yes)
    , m_pointerId(index)
    , m_width(2 * event.touchPoints()[0].radiusX())
    , m_height(2 * event.touchPoints()[0].radiusY())
    , m_pressure(event.touchPoints()[0].force())
    , m_pointerType("touch"_s)
    , m_isPrimary(isPrimary)
{
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
