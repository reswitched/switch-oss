/*
 *  WKCPlatformEvents.h
 *
 *  Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCPlatformEvents_h
#define WKCPlatformEvents_h

#include "WKCEnums.h"

namespace WKC {

/*@{*/

/** @brief Structure for storing key event. */
struct WKCKeyEvent_ {
    /** @brief Key event type */
    WKC::KeyEventType m_type;
    /** @brief Key */
    WKC::Key m_key;
    /** @brief Character */
    unsigned int m_char;
    /** @brief Modifiers */
    WKC::Modifier m_modifiers;
    /** @brief Auto repeat */
    bool m_autoRepeat;
};
/** @brief Type definition of WKCKeyEvent. */
typedef struct WKCKeyEvent_ WKCKeyEvent;

/** @brief Structure for storing mouse event. */
struct WKCMouseEvent_ {
    /** @brief Mouse event type */
    WKC::MouseEventType m_type;
    /** @brief Mouse button */
    WKC::MouseButton m_button;
    /** @brief Mouse position X*/
    int m_x;
    /** @brief Mouse position Y*/
    int m_y;
    /** @brief Modifiers */
    WKC::Modifier m_modifiers;
    /** @brief Timestamp */
    unsigned int m_timestampinsec;
};
/** @brief Type definition of WKCMouseEvent. */
typedef struct WKCMouseEvent_ WKCMouseEvent;

/** @brief Structure for storing wheel event. */
struct WKCWheelEvent_ {
    /** @brief Delta X */
    int m_dx;
    /** @brief Delta Y*/
    int m_dy;
    /** @brief Position X */
    int m_x;
    /** @brief Position Y */
    int m_y;
    /** @brief Modifiers */
    WKC::Modifier m_modifiers;
};
/** @brief Type definition of WKCWheelEvent. */
typedef struct WKCWheelEvent_ WKCWheelEvent;

/** @brief Structure for storing touch point. */
struct WKCTouchPoint_ {
    /** @brief Id */
    int m_id;
    /** @brief State */
    int m_state;
    /** @brief Position */
    WKCPoint m_pos;
};
/** @brief Type definition of WKCTouchPoint. */
typedef struct WKCTouchPoint_ WKCTouchPoint;

/** @brief Structure for storing touch event. */
struct WKCTouchEvent_ {
    /** @brief Type */
    int m_type;
    /** @brief Touch points */
    const WKCTouchPoint* m_points;
    /** @brief Number of touch points */
    int m_npoints;
    /** @brief Modifiers */
    WKC::Modifier m_modifiers;
    /** @brief Timestamp */
    unsigned int m_timestampinsec;
};
/** @brief Type definition of WKCTouchEvent. */
typedef struct WKCTouchEvent_ WKCTouchEvent;

/*@}*/

} // namespace

#endif // WKCPlatformEvents_h
