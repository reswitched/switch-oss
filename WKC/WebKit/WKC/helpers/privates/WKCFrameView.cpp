/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCFrameView.h"
#include "helpers/privates/WKCFrameViewPrivate.h"

#include "FrameView.h"
#include "helpers/privates/WKCScrollbarPrivate.h"

namespace WKC {

FrameViewPrivate::FrameViewPrivate(WebCore::FrameView* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_verticalScrollbar(0)
    , m_horizontalScrollbar(0)
{
}

FrameViewPrivate::~FrameViewPrivate()
{
    delete m_verticalScrollbar;
    delete m_horizontalScrollbar;
}

void
FrameViewPrivate::scrollPositionChanged()
{
    WebCore::IntPoint p;
    m_webcore->scrollPositionChanged(p, p);
}


WKCPoint
FrameViewPrivate::contentsToWindow(const WKCPoint& pos)
{
    return m_webcore->contentsToWindow(pos);
}

WKCRect
FrameViewPrivate::contentsToWindow(const WKCRect& rect)
{
    return m_webcore->contentsToWindow(rect);
}

void
FrameViewPrivate::setCannotBlitToWindow()
{
    m_webcore->setCannotBlitToWindow();
}

void
FrameViewPrivate::updateLayoutAndStyleIfNeededRecursive()
{
    m_webcore->updateLayoutAndStyleIfNeededRecursive();
}

void
FrameViewPrivate::forceLayout()
{
    m_webcore->forceLayout();
}

WKCPoint
FrameViewPrivate::windowToContents(const WKCPoint& pos)
{
    return m_webcore->windowToContents(pos);
}

WKCRect
FrameViewPrivate::windowToContents(const WKCRect& rect)
{
    return m_webcore->windowToContents(rect);
}

WKCRect
FrameViewPrivate::convertToContainingWindow(const WKCRect& rect) const
{
    return m_webcore->convertToContainingWindow(rect);
}

Scrollbar*
FrameViewPrivate::verticalScrollbar()
{
    WebCore::Scrollbar* bar = m_webcore->verticalScrollbar();
    if (!bar)
        return 0;
    if (!m_verticalScrollbar || m_verticalScrollbar->webcore()!=bar) {
        delete m_verticalScrollbar ;
        m_verticalScrollbar = new ScrollbarPrivate(bar);
    }
    return &m_verticalScrollbar->wkc();
}

Scrollbar*
FrameViewPrivate::horizontalScrollbar()
{
    WebCore::Scrollbar* bar = m_webcore->horizontalScrollbar();
    if (!bar)
        return 0;
    if (!m_horizontalScrollbar || m_horizontalScrollbar->webcore()!=bar) {
        delete m_horizontalScrollbar ;
        m_horizontalScrollbar = new ScrollbarPrivate(bar);
    }
    return &m_horizontalScrollbar->wkc();
}

Color
FrameViewPrivate::documentBackgroundColor() const
{
    WebCore::Color c = m_webcore->documentBackgroundColor();
    return Color((ColorPrivate*)&c);
}

void
FrameViewPrivate::setWasScrolledByUser(bool wasScrolledByUser)
{
    m_webcore->setWasScrolledByUser(wasScrolledByUser);
}

bool
FrameViewPrivate::flushCompositingStateIncludingSubframes()
{
    return m_webcore->flushCompositingStateIncludingSubframes();
}

////////////////////////////////////////////////////////////////////////////////

FrameView::FrameView(FrameViewPrivate& parent)
    : m_private(parent)
{
}

FrameView::~FrameView()
{
}

void
FrameView::scrollPositionChanged()
{
    m_private.scrollPositionChanged();
}


WKCPoint
FrameView::contentsToWindow(const WKCPoint& pos)
{
    return m_private.contentsToWindow(pos);
}

WKCRect
FrameView::contentsToWindow(const WKCRect& rect)
{
    return m_private.contentsToWindow(rect);
}

void
FrameView::setCannotBlitToWindow()
{
    m_private.setCannotBlitToWindow();
}

void
FrameView::updateLayoutAndStyleIfNeededRecursive()
{
    m_private.updateLayoutAndStyleIfNeededRecursive();
}

void
FrameView::forceLayout()
{
    m_private.forceLayout();
}

WKCPoint
FrameView::windowToContents(const WKCPoint& pos)
{
    return m_private.windowToContents(pos);
}

WKCRect
FrameView::windowToContents(const WKCRect& rect)
{
    return m_private.windowToContents(rect);
}

WKCRect
FrameView::convertToContainingWindow(const WKCRect& rect) const
{
    return m_private.convertToContainingWindow(rect);
}


Scrollbar*
FrameView::verticalScrollbar() const
{
    return m_private.verticalScrollbar();
}

Scrollbar*
FrameView::horizontalScrollbar() const
{
    return m_private.horizontalScrollbar();
}

Color
FrameView::documentBackgroundColor() const
{
    return m_private.documentBackgroundColor();
}

void
FrameView::setWasScrolledByUser(bool wasScrolledByUser)
{
    m_private.setWasScrolledByUser(wasScrolledByUser);
}

bool
FrameView::flushCompositingStateIncludingSubframes()
{
    return m_private.flushCompositingStateIncludingSubframes();
}

} // namespace
