/*
 * Copyright (c) 2013, 2014 ACCESS CO., LTD. All rights reserved.
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
#include "helpers/WKCHTMLMediaElement.h"
#include "helpers/privates/WKCHTMLMediaElementPrivate.h"
#include "helpers/privates/WKCTextTrackListPrivate.h"

#include "HTMLMediaElement.h"

namespace WKC {

HTMLMediaElementPrivate::HTMLMediaElementPrivate(WebCore::HTMLMediaElement* parent)
    : HTMLElementPrivate(parent)
    , m_wkc(*this)
    , m_textTrackList(0)
{
}

HTMLMediaElementPrivate::~HTMLMediaElementPrivate()
{
    delete m_textTrackList;
}

void*
HTMLMediaElementPrivate::platformPlayer() const
{
    return reinterpret_cast<WebCore::HTMLMediaElement *>(webcore())->platformPlayer();
}

void
HTMLMediaElementPrivate::togglePlayState()
{
    reinterpret_cast<WebCore::HTMLMediaElement *>(webcore())->togglePlayState();
}

bool
HTMLMediaElementPrivate::canPlay() const
{
    return reinterpret_cast<WebCore::HTMLMediaElement *>(webcore())->canPlay();
}

TextTrackList*
HTMLMediaElementPrivate::textTracks()
{
    WebCore::TextTrackList* textTrackList = reinterpret_cast<WebCore::HTMLMediaElement *>(webcore())->textTracks();
    if (!textTrackList)
        return 0;
    if (!m_textTrackList || m_textTrackList->webcore() != textTrackList) {
        delete m_textTrackList;
        m_textTrackList = new TextTrackListPrivate(textTrackList);
    }
    return &m_textTrackList->wkc();
}

bool
HTMLMediaElementPrivate::couldPlayIfEnoughData() const
{
    return reinterpret_cast<WebCore::HTMLMediaElement *>(webcore())->couldPlayIfEnoughData();
}

HTMLMediaElement::NetworkState
HTMLMediaElementPrivate::networkState() const
{
    WebCore::HTMLMediaElement::NetworkState networkState = reinterpret_cast<WebCore::HTMLMediaElement *>(webcore())->networkState();
    switch (networkState) {
    case WebCore::HTMLMediaElement::NETWORK_EMPTY:
        return HTMLMediaElement::NETWORK_EMPTY;
    case WebCore::HTMLMediaElement::NETWORK_IDLE:
        return HTMLMediaElement::NETWORK_IDLE;
    case WebCore::HTMLMediaElement::NETWORK_LOADING:
        return HTMLMediaElement::NETWORK_LOADING;
    case WebCore::HTMLMediaElement::NETWORK_NO_SOURCE:
        return HTMLMediaElement::NETWORK_NO_SOURCE;
    }
    ASSERT_NOT_REACHED();
    return HTMLMediaElement::NETWORK_EMPTY;
}



HTMLMediaElement::HTMLMediaElement(HTMLMediaElementPrivate& parent)
    : HTMLElement(parent)
{
}

HTMLMediaElement::~HTMLMediaElement()
{
}

void*
HTMLMediaElement::identifier() const
{
    return reinterpret_cast<HTMLMediaElementPrivate&>(priv()).webcore();
}

void*
HTMLMediaElement::platformPlayer() const
{
    return reinterpret_cast<HTMLMediaElementPrivate&>(priv()).platformPlayer();
}

void
HTMLMediaElement::togglePlayState()
{
    reinterpret_cast<HTMLMediaElementPrivate&>(priv()).togglePlayState();
}

bool
HTMLMediaElement::canPlay() const
{
    return reinterpret_cast<HTMLMediaElementPrivate&>(priv()).canPlay();
}

TextTrackList*
HTMLMediaElement::textTracks()
{
    return reinterpret_cast<HTMLMediaElementPrivate&>(priv()).textTracks();
}

bool
HTMLMediaElement::couldPlayIfEnoughData() const
{
    return reinterpret_cast<HTMLMediaElementPrivate&>(priv()).couldPlayIfEnoughData();
}

HTMLMediaElement::NetworkState
HTMLMediaElement::networkState() const
{
    return reinterpret_cast<HTMLMediaElementPrivate&>(priv()).networkState();
}


} // namespace
