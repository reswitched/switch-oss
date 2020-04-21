/*
 * Copyright (c) 2011-2017 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCFocusController.h"
#include "helpers/privates/WKCFocusControllerPrivate.h"

#include "FocusController.h"
#include "IntRect.h"

#include "SpatialNavigation.h"

#include "helpers/WKCNode.h"
#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCNodePrivate.h"

namespace WKC {

FocusControllerPrivate::FocusControllerPrivate(WebCore::FocusController* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_focusedFrame(0)
    , m_focusableElement(0)
    , m_clickableElement(0)
{
}

FocusControllerPrivate::~FocusControllerPrivate()
{
    delete m_clickableElement;
    delete m_focusableElement;
    delete m_focusedFrame;
}

Frame*
FocusControllerPrivate::focusedOrMainFrame()
{
    WebCore::Frame* frame = &m_webcore->focusedOrMainFrame();
    if (!frame)
        return 0;
    if (!m_focusedFrame || m_focusedFrame->webcore()!=frame) {
        delete m_focusedFrame;
        m_focusedFrame = new FramePrivate(frame);
    }
    return &m_focusedFrame->wkc();
}

Element*
FocusControllerPrivate::findNextFocusableElement(const FocusDirection& direction, const WKCRect* scope, Element* base)
{
    WebCore::FocusDirection webcore_dir = toWebCoreFocusDirection(direction);
    WebCore::Element* element;
    WebCore::Element* baseWebCore = base ? ((WebCore::Element*)(((NodePrivate&)(((Node*)base)->priv())).webcore())) : nullptr;
    if (scope) {
        WebCore::IntRect r(scope->fX, scope->fY, scope->fWidth, scope->fHeight);
        element = m_webcore->findNextFocusableElement(webcore_dir, &r, baseWebCore);
    } else {
        element = m_webcore->findNextFocusableElement(webcore_dir, 0, baseWebCore);
    }
    if (!element)
        return 0;

    if (m_focusableElement)
        delete m_focusableElement;
    m_focusableElement = ElementPrivate::create(element);
    if (!m_focusableElement)
        return 0;

    return &m_focusableElement->wkc();
}

Element*
FocusControllerPrivate::findNearestFocusableElementFromPoint(const WKCPoint& point, const WKCRect* scope)
{
    WebCore::IntPoint p(point.fX, point.fY);
    WebCore::Element* element;
    if (scope) {
        WebCore::IntRect r(scope->fX, scope->fY, scope->fWidth, scope->fHeight);
        element = m_webcore->findNearestFocusableElementFromPoint(p, &r);
    } else {
        element = m_webcore->findNearestFocusableElementFromPoint(p, 0);
    }
    if (!element)
        return 0;

    if (m_focusableElement)
        delete m_focusableElement;
    m_focusableElement = ElementPrivate::create(element);
    if (!m_focusableElement)
        return 0;

    return &m_focusableElement->wkc();
}

Element*
FocusControllerPrivate::findNearestClickableElementFromPoint(const WKCPoint& point, const WKCRect* scope)
{
    WebCore::IntPoint p(point.fX, point.fY);
    WebCore::Element* element;
    if (scope) {
        WebCore::IntRect r(scope->fX, scope->fY, scope->fWidth, scope->fHeight);
        element = m_webcore->findNearestClickableElementFromPoint(p, &r);
    } else {
        element = m_webcore->findNearestClickableElementFromPoint(p, 0);
    }
    if (!element)
        return 0;

    if (m_clickableElement)
        delete m_clickableElement;
    m_clickableElement = ElementPrivate::create(element);
    if (!m_clickableElement)
        return 0;

    return &m_clickableElement->wkc();
}

bool
isScrollableContainerNode(Node* node)
{
    if (!node)
        return false;

    WebCore::FocusCandidate fc(node->priv().webcore(), WebCore::FocusDirectionNone);

    return fc.inScrollableContainer();
}

bool
hasOffscreenRect(Node* node)
{
    return WebCore::hasOffscreenRect(node ? node->priv().webcore() : 0);
}


FocusController::FocusController(FocusControllerPrivate& parent)
    : m_private(parent)
{
}

FocusController::~FocusController()
{
}

Frame*
FocusController::focusedOrMainFrame()
{
    return m_private.focusedOrMainFrame();
}

Element* FocusController::findNextFocusableElement(const FocusDirection& direction, const WKCRect* scope, Element* base)
{
    return m_private.findNextFocusableElement(direction, scope, base);
}

Element*
FocusController::findNearestFocusableElementFromPoint(const WKCPoint& point, const WKCRect* scope)
{
    return m_private.findNearestFocusableElementFromPoint(point, scope);
}

Element*
FocusController::findNearestClickableElementFromPoint(const WKCPoint& point, const WKCRect* scope)
{
    return m_private.findNearestClickableElementFromPoint(point, scope);
}

} // namespace
