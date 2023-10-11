/*
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
#include "PlatformMouseEvent.h"

#include "GtkUtilities.h"
#include "PlatformKeyboardEvent.h"
#include <gdk/gdk.h>
#include <wtf/Assertions.h>

namespace WebCore {

// FIXME: Would be even better to figure out which modifier is Alt instead of always using GDK_MOD1_MASK.

// Keep this in sync with the other platform event constructors
PlatformMouseEvent::PlatformMouseEvent(GdkEventButton* event)
{
    gdouble x, y;
    gdk_event_get_coords(reinterpret_cast<GdkEvent*>(event), &x, &y);
    gdouble rootX, rootY;
    gdk_event_get_root_coords(reinterpret_cast<GdkEvent*>(event), &rootX, &rootY);
    GdkModifierType state;
    gdk_event_get_state(reinterpret_cast<GdkEvent*>(event), &state);
    guint button;
    gdk_event_get_button(reinterpret_cast<GdkEvent*>(event), &button);

    m_timestamp = wallTimeForEvent(event);
    m_position = IntPoint(static_cast<int>(x), static_cast<int>(y));
    m_globalPosition = IntPoint(static_cast<int>(rootX), static_cast<int>(rootY));
    m_button = NoButton;
    m_clickCount = 0;
    m_modifierFlags = 0;

    if (state & GDK_SHIFT_MASK)
        m_modifiers.add(PlatformEvent::Modifier::ShiftKey);
    if (state & GDK_CONTROL_MASK)
        m_modifiers.add(PlatformEvent::Modifier::ControlKey);
    if (state & GDK_MOD1_MASK)
        m_modifiers.add(PlatformEvent::Modifier::AltKey);
    if (state & GDK_META_MASK)
        m_modifiers.add(PlatformEvent::Modifier::MetaKey);
    if (PlatformKeyboardEvent::modifiersContainCapsLock(state))
        m_modifiers.add(PlatformEvent::Modifier::CapsLockKey);

    GdkEventType type = gdk_event_get_event_type(reinterpret_cast<GdkEvent*>(event));
    switch (type) {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_3BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
        m_type = PlatformEvent::MousePressed;
        if (type == GDK_BUTTON_RELEASE) {
            m_type = PlatformEvent::MouseReleased;
            m_clickCount = 0;
        } else if (type == GDK_BUTTON_PRESS)
            m_clickCount = 1;
        else if (type == GDK_2BUTTON_PRESS)
            m_clickCount = 2;
        else if (type == GDK_3BUTTON_PRESS)
            m_clickCount = 3;

        if (button == 1)
            m_button = LeftButton;
        else if (button == 2)
            m_button = MiddleButton;
        else if (button == 3)
            m_button = RightButton;
        break;

    default:
        ASSERT_NOT_REACHED();
    };
}

PlatformMouseEvent::PlatformMouseEvent(GdkEventMotion* motion)
{
    gdouble x, y, rootX, rootY;
    GdkModifierType state;
    gdk_event_get_coords(reinterpret_cast<GdkEvent*>(motion), &x, &y);
    gdk_event_get_root_coords(reinterpret_cast<GdkEvent*>(motion), &rootX, &rootY);
    gdk_event_get_state(reinterpret_cast<GdkEvent*>(motion), &state);
    m_position = IntPoint(static_cast<int>(x), static_cast<int>(y));
    m_globalPosition = IntPoint(static_cast<int>(rootX), static_cast<int>(rootY));
    m_timestamp = wallTimeForEvent(motion);
    m_button = NoButton;
    m_clickCount = 0;
    m_modifierFlags = 0;

    if (state & GDK_SHIFT_MASK)
        m_modifiers.add(PlatformEvent::Modifier::ShiftKey);
    if (state & GDK_CONTROL_MASK)
        m_modifiers.add(PlatformEvent::Modifier::ControlKey);
    if (state & GDK_MOD1_MASK)
        m_modifiers.add(PlatformEvent::Modifier::AltKey);
    if (state & GDK_META_MASK)
        m_modifiers.add(PlatformEvent::Modifier::MetaKey);
    if (PlatformKeyboardEvent::modifiersContainCapsLock(state))
        m_modifiers.add(PlatformEvent::Modifier::CapsLockKey);

    GdkEventType type = gdk_event_get_event_type(reinterpret_cast<GdkEvent*>(motion));
    switch (type) {
    case GDK_MOTION_NOTIFY:
        m_type = PlatformEvent::MouseMoved;
        m_button = NoButton;
        m_clickCount = 0;
        break;
    default:
        ASSERT_NOT_REACHED();
    };

    if (state & GDK_BUTTON1_MASK)
        m_button = LeftButton;
    else if (state & GDK_BUTTON2_MASK)
        m_button = MiddleButton;
    else if (state & GDK_BUTTON3_MASK)
        m_button = RightButton;
}
}
