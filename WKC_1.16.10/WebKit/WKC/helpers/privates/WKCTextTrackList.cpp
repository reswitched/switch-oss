/*
 * Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCTextTrackList.h"
#include "helpers/privates/WKCTextTrackListPrivate.h"

#include "helpers/privates/WKCTextTrackPrivate.h"

#include "TextTrackList.h"

namespace WKC {

TextTrackListPrivate::TextTrackListPrivate(WebCore::TextTrackList* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_textTrack(0)
{
}

TextTrackListPrivate::~TextTrackListPrivate()
{
    delete m_textTrack;
}

unsigned
TextTrackListPrivate::length() const
{
    return m_webcore->length();
}

TextTrack*
TextTrackListPrivate::item(unsigned index)
{
    WebCore::TextTrack* textTrack = m_webcore->item(index);
    if (!textTrack)
        return 0;
    if (!m_textTrack || m_textTrack->webcore() != textTrack) {
        delete m_textTrack;
        m_textTrack = new TextTrackPrivate(textTrack);
    }
    return &m_textTrack->wkc();
}

////////////////////////////////////////////////////////////////////////////////

TextTrackList::TextTrackList(TextTrackListPrivate& parent)
    : m_private(parent)
{
}

TextTrackList::~TextTrackList()
{
}

unsigned
TextTrackList::length() const
{
    return m_private.length();
}

TextTrack*
TextTrackList::item(unsigned index)
{
    return m_private.item(index);
}


} // namespace
