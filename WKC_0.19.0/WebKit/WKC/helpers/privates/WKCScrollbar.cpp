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

#include "helpers/WKCScrollbar.h"
#include "helpers/privates/WKCScrollbarPrivate.h"

#include "Scrollbar.h"

#include "WKCWebView.h"

namespace WebCore {
extern WebCore::IntRect scrollbarTrackRect(Scrollbar& scrollbar);
extern void scrollbarSplitTrack(Scrollbar& scrollbar, const WebCore::IntRect& track, WebCore::IntRect& startTrack, WebCore::IntRect& thumb, WebCore::IntRect& endTrack);
extern WebCore::IntRect scrollbarBackButtonRect(Scrollbar& scrollbar, ScrollbarPart part);
extern WebCore::IntRect scrollbarForwardButtonRect(Scrollbar& scrollbar, ScrollbarPart part);
} // namespace

namespace WKC {

ScrollbarPrivate::ScrollbarPrivate(WebCore::Scrollbar* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

ScrollbarPrivate::~ScrollbarPrivate()
{
}

bool
ScrollbarPrivate::enabled() const
{
    return m_webcore->enabled();
}

int
ScrollbarPrivate::x() const
{
    return m_webcore->x();
}

int
ScrollbarPrivate::y() const
{
    return m_webcore->y();
}

WKCRect
ScrollbarPrivate::frameRect() const
{
    return m_webcore->frameRect();
}

WKCPoint
ScrollbarPrivate::convertFromContainingWindow(const WKCPoint& pos) const
{
    return m_webcore->convertFromContainingWindow(pos);
}

WKCRect
scrollbarTrackRect(Scrollbar* scrollbar)
{
    return WebCore::scrollbarTrackRect(*scrollbar->priv().webcore());
}

void
scrollbarSplitTrack(Scrollbar* scrollbar, const WKCRect& track, WKCRect& startTrack, WKCRect& thumb, WKCRect& endTrack)
{
    WebCore::IntRect st = startTrack;
    WebCore::IntRect et = endTrack;
    WebCore::IntRect tm = thumb;
    WebCore::scrollbarSplitTrack(*scrollbar->priv().webcore(), track, st, tm, et);
    startTrack = st;
    endTrack = et;
    thumb = tm;
}

static WebCore::ScrollbarPart
_corePart(WKC::WKCWebView::ScrollbarPart wpart)
{
    WebCore::ScrollbarPart part = WebCore::NoPart;

    switch (wpart) {
    case WKC::WKCWebView::NoPart:
        part = WebCore::NoPart; break;
    case WKC::WKCWebView::BackTrackPart:
        part = WebCore::BackTrackPart; break;
    case WKC::WKCWebView::ThumbPart:
        part = WebCore::ThumbPart; break;
    case WKC::WKCWebView::ForwardTrackPart:
        part = WebCore::ForwardTrackPart; break;
    case WKC::WKCWebView::ScrollbarBGPart:
        part = WebCore::ScrollbarBGPart; break;
    case WKC::WKCWebView::TrackBGPart:
        part = WebCore::TrackBGPart; break;
    case WKC::WKCWebView::BackButtonStartPart:
        part = WebCore::BackButtonStartPart; break;
    case WKC::WKCWebView::BackButtonEndPart:
        part = WebCore::BackButtonEndPart; break;
    case WKC::WKCWebView::ForwardButtonStartPart:
        part = WebCore::ForwardButtonStartPart; break;
    case WKC::WKCWebView::ForwardButtonEndPart:
        part = WebCore::ForwardButtonEndPart; break;

    case WKC::WKCWebView::BackButtonPart:
    case WKC::WKCWebView::ForwardButtonPart:
    default:
        part = WebCore::NoPart; break;
    }
    return part;
}

WKCRect
scrollbarBackButtonRect(Scrollbar* scrollbar, WKC::WKCWebView::ScrollbarPart part)
{
    return WebCore::scrollbarBackButtonRect(*scrollbar->priv().webcore(), _corePart(part));
}

WKCRect
scrollbarForwardButtonRect(Scrollbar* scrollbar, WKC::WKCWebView::ScrollbarPart part)
{
    return WebCore::scrollbarForwardButtonRect(*scrollbar->priv().webcore(), _corePart(part));
}


Scrollbar*
Scrollbar::create(Scrollbar* parent, bool needsRef)
{
    void* p = WTF::fastMalloc(sizeof(Scrollbar));
    return new (p) Scrollbar(parent, needsRef);
}

void
Scrollbar::destroy(Scrollbar* instance)
{
    if (!instance)
        return;
    instance->~Scrollbar();
    WTF::fastFree(instance);
}

Scrollbar::Scrollbar(ScrollbarPrivate& parent)
    : m_ownedPrivate(0)
    , m_private(parent)
    , m_needsRef(false)
{
}

Scrollbar::Scrollbar(Scrollbar* parent, bool needsRef)
    : m_ownedPrivate(new ScrollbarPrivate(parent->priv().webcore()))
    , m_private(*m_ownedPrivate)
    , m_needsRef(needsRef)
{
    if (needsRef)
        m_private.webcore()->ref();
}

Scrollbar::~Scrollbar()
{
    if (m_needsRef)
        m_private.webcore()->deref();
    if (m_ownedPrivate)
        delete m_ownedPrivate;
}

bool
Scrollbar::compare(const Scrollbar* other) const
{
    if (this==other)
        return true;
    if (!this || !other)
        return false;
    if (m_private.webcore() == other->m_private.webcore())
        return true;
    return false;
}

bool
Scrollbar::enabled() const
{
    return m_private.enabled();
}

int
Scrollbar::x() const
{
    return m_private.x();
}

int
Scrollbar::y() const
{
    return m_private.y();
}


WKCRect
Scrollbar::frameRect() const
{
    return m_private.frameRect();
}


WKCPoint
Scrollbar::convertFromContainingWindow(const WKCPoint& pos) const
{
    return m_private.convertFromContainingWindow(pos);
}

} // namespace
