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

#ifndef _WKC_HELPERS_WKC_FOCUSCONTROLLER_H_
#define _WKC_HELPERS_WKC_FOCUSCONTROLLER_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHelpersEnums.h"

namespace WKC {
class Element;
class Frame;
class FocusControllerPrivate;
class Node;

class WKC_API FocusController {
public:
    Frame* focusedOrMainFrame();
    FocusControllerPrivate& priv() const { return m_private; }

    Element* findNextFocusableElement(const FocusDirection& direction, const WKCRect* scope = 0, Element* base = 0, Node* container = 0);
    Element* findNearestFocusableElementFromPoint(const WKCPoint& point, const WKCRect* scope = 0);
    Element* findNearestClickableElementFromPoint(const WKCPoint& point, const WKCRect* scope = 0);

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    FocusController(FocusControllerPrivate&);
    ~FocusController();

private:
    FocusController(const FocusController&);
    FocusController& operator=(const FocusController&);

    FocusControllerPrivate& m_private;
};

bool isScrollableContainerNode(Node*);
bool hasOffscreenRect(Node*);

}

#endif // _WKC_HELPERS_WKC_FOCUSCONTROLLER_H_
