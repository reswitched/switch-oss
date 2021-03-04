/*
 * Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_FOCUSCONTROLLER_H_
#define _WKC_HELPERS_PRIVATE_FOCUSCONTROLLER_H_

#include "helpers/WKCFocusController.h"

namespace WebCore {
class FocusController;
} // namespace

namespace WKC {

class FramePrivate;
class ElementPrivate;

class FocusControllerWrap : public FocusController {
WTF_MAKE_FAST_ALLOCATED;
public:
    FocusControllerWrap(FocusControllerPrivate& parent) : FocusController(parent) {}
    ~FocusControllerWrap() {}
};

class FocusControllerPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    FocusControllerPrivate(WebCore::FocusController*);
    ~FocusControllerPrivate();

    WebCore::FocusController* webcore() const { return m_webcore; }
    FocusController& wkc() { return m_wkc; }

    Element* findNextFocusableElement(const FocusDirection& direction, const WKCRect* scope = 0, Element* base = 0, Node* container = 0);
    Element* findNearestFocusableElementFromPoint(const WKCPoint& point, const WKCRect* scope = 0);
    Element* findNearestClickableElementFromPoint(const WKCPoint& point, const WKCRect* scope = 0);
 
    Frame* focusedOrMainFrame();

private:
    WebCore::FocusController* m_webcore;
    FocusControllerWrap m_wkc;

    FramePrivate* m_focusedFrame;
    ElementPrivate* m_focusableElement;
    ElementPrivate* m_clickableElement;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_FOCUSCONTROLLER_H_
