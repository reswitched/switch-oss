/*
 * Copyright (c) 2013 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCTextTrackCueList.h"
#include "helpers/privates/WKCTextTrackCueListPrivate.h"

#include "helpers/privates/WKCTextTrackCuePrivate.h"

#include "TextTrackCueList.h"

namespace WKC {

TextTrackCueListPrivate::TextTrackCueListPrivate(WebCore::TextTrackCueList* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_textTrackCue(0)
{
}

TextTrackCueListPrivate::~TextTrackCueListPrivate()
{
    delete m_textTrackCue;
}

unsigned long
TextTrackCueListPrivate::length() const
{
    return m_webcore->length();
}

TextTrackCue*
TextTrackCueListPrivate::item(unsigned index)
{
    WebCore::TextTrackCue* textTrackCue = m_webcore->item(index);
    if (!textTrackCue)
        return 0;
    if (!m_textTrackCue || m_textTrackCue->webcore() != textTrackCue) {
        delete m_textTrackCue;
        m_textTrackCue = new TextTrackCuePrivate(textTrackCue);
    }
    return &m_textTrackCue->wkc();
}


TextTrackCueList::TextTrackCueList(TextTrackCueListPrivate& parent)
    : m_private(parent)
{
}

TextTrackCueList::~TextTrackCueList()
{
}

unsigned long
TextTrackCueList::length() const
{
    return m_private.length();
}

TextTrackCue*
TextTrackCueList::item(unsigned index) const
{
    return m_private.item(index);
}



} // namespace
