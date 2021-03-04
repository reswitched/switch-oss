/*
 *  Copyright (c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "PlatformTouchEvent.h"
#include <wtf/CurrentTime.h>

#include "WKCPlatformEvents.h"
#include "WKCEnums.h"

#if ENABLE(TOUCH_EVENTS)

namespace WebCore {

PlatformTouchEvent::PlatformTouchEvent(const void* event)
{
    const WKC::WKCTouchEvent* ev = reinterpret_cast<const WKC::WKCTouchEvent *>(event);

    m_timestamp = ev->m_timestampinsec;

    switch (ev->m_type) {
    case WKC::ETouchTypeStart:
        m_type = TouchStart; break;
    case WKC::ETouchTypeEnd:
        m_type = TouchEnd; break;
    case WKC::ETouchTypeMove:
        m_type = TouchMove; break;
    case WKC::ETouchTypeCancel:
    default:
        m_type = TouchCancel; break;
    }

    for (int i=0; i<ev->m_npoints; i++) {
        m_touchPoints.append(PlatformTouchPoint((void *)&ev->m_points[i]));
    }

    if (ev->m_modifiers & WKC::EModifierShift) {
        m_modifiers |= ShiftKey;
    }
    if (ev->m_modifiers & WKC::EModifierCtrl) {
        m_modifiers |= CtrlKey;
    }
    if (ev->m_modifiers & WKC::EModifierAlt) {
        m_modifiers |= AltKey;
    }
    if (ev->m_modifiers & WKC::EModifierMod1) {
        m_modifiers |= MetaKey;
    }
}

PlatformTouchPoint::PlatformTouchPoint(const void* obj)
{
    const WKC::WKCTouchPoint* pos = reinterpret_cast<const WKC::WKCTouchPoint *>(obj);

    m_id = pos->m_id;

    switch (pos->m_state) {
    case WKC::ETouchPointStateReleased:
        m_state = TouchReleased; break;
    case WKC::ETouchPointStatePressed:
        m_state = TouchPressed; break;
    case WKC::ETouchPointStateMoved:
        m_state = TouchMoved; break;
    case WKC::ETouchPointStateStationary:
        m_state = TouchStationary; break;
    case WKC::ETouchPointStateCancelled:
    default:
        m_state = TouchCancelled; break;
    }

    m_screenPos = IntPoint(pos->m_pos.fX, pos->m_pos.fY);
    m_pos = m_screenPos;

    m_radiusX = 0;
    m_radiusY = 0;
    m_rotationAngle = 0.f;
    m_force = 0.f;
}

} // namespace

#endif // ENABLE(TOUCH_EVENTS)
