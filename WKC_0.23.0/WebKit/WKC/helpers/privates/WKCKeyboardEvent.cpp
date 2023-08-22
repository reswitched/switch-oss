/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCKeyboardEvent.h"
#include "helpers/privates/WKCKeyboardEventPrivate.h"

#include "helpers/privates/WKCEventPrivate.h"

#include "KeyboardEvent.h"
#include "PlatformKeyboardEvent.h"

#include <string.h>

static bool
_platformKeyEvent(WebCore::KeyboardEvent* event, WKC::WKCKeyEvent& ev)
{
    const WebCore::PlatformKeyboardEvent* pe = 0;
    unsigned int mod = 0;
    unsigned int pmod = 0;

    if (!event)
        return false;
    pe = event->keyEvent();
    if (!pe)
        return false;

    switch (pe->type()) {
    case WebCore::PlatformKeyboardEvent::KeyDown:
    case WebCore::PlatformKeyboardEvent::RawKeyDown:
        ev.m_type = WKC::EKeyEventPressed;
        ev.m_key = (WKC::Key)event->keyCode();
        break;
    case WebCore::PlatformKeyboardEvent::KeyUp:
        ev.m_type = WKC::EKeyEventReleased;
        ev.m_key = (WKC::Key)event->keyCode();
        break;
    case WebCore::PlatformKeyboardEvent::Char:
        ev.m_type = WKC::EKeyEventChar;
        ev.m_char = event->charCode();
        break;
    default:
        return false;
    }
    mod = pe->modifiers();
    pmod = WKC::EModifierNone;
    if (mod&WebCore::PlatformKeyboardEvent::AltKey)
        pmod |= WKC::EModifierAlt;
    if (mod&WebCore::PlatformKeyboardEvent::CtrlKey)
        pmod |= WKC::EModifierCtrl;
    if (mod&WebCore::PlatformKeyboardEvent::ShiftKey)
        pmod |= WKC::EModifierShift;
    if (mod&WebCore::PlatformKeyboardEvent::MetaKey)
        pmod |= WKC::EModifierMod1;
    ev.m_modifiers = (WKC::Modifier)pmod;
    return true;
}

namespace WKC {

KeyboardEventPrivate::KeyboardEventPrivate(WebCore::KeyboardEvent* parent)
    : EventPrivate(parent)
    , m_wkc(*this)
{
    ::memset(&m_keyEvent, 0, sizeof(m_keyEvent));
    if (!_platformKeyEvent(parent, m_keyEvent)) {
        ::memset(&m_keyEvent, 0, sizeof(m_keyEvent));
    }
}

KeyboardEventPrivate::~KeyboardEventPrivate()
{
}

////////////////////////////////////////////////////////////////////////////////

KeyboardEvent::KeyboardEvent(KeyboardEventPrivate& parent)
    : Event(parent)
{
}

KeyboardEvent::~KeyboardEvent()
{
}

WKCKeyEvent
KeyboardEvent::keyEvent() const
{
    return static_cast<KeyboardEventPrivate&>(priv()).keyEvent();
}

} // namespace
