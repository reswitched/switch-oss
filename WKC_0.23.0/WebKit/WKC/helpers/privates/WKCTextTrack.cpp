/*
 * Copyright (c) 2013-2018 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCTextTrack.h"
#include "helpers/privates/WKCTextTrackPrivate.h"

#include "helpers/WKCString.h"
#include "helpers/privates/WKCTextTrackCueListPrivate.h"

#include "TextTrack.h"
#include "NotImplemented.h"

namespace WKC {

TextTrackPrivate::TextTrackPrivate(WebCore::TextTrack* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_textTrackCueList(0)
{
}

TextTrackPrivate::~TextTrackPrivate()
{
    delete m_textTrackCueList;
}

String
TextTrackPrivate::kind() const
{
    return m_webcore->kind().string();
}

String
TextTrackPrivate::label() const
{
    return m_webcore->label().string();
}

String
TextTrackPrivate::language() const
{
    return m_webcore->language().string();
}

bool
TextTrackPrivate::isDefault() const
{
    return m_webcore->isDefault();
}

TextTrackCueList*
TextTrackPrivate::cues()
{
    WebCore::TextTrackCueList* textTrackCueList = m_webcore->cues();
    if (!textTrackCueList)
        return 0;
    if (!m_textTrackCueList || m_textTrackCueList->webcore() != textTrackCueList) {
        delete m_textTrackCueList;
        m_textTrackCueList = new TextTrackCueListPrivate(textTrackCueList);
    }
    return &m_textTrackCueList->wkc();
}



TextTrack::TextTrack(TextTrackPrivate& parent)
    : m_ownedPrivate(0)
    , m_private(parent)
    , m_needsRef(false)
{
}

TextTrack::TextTrack(TextTrack* parent, bool needsRef)
    : m_ownedPrivate(new TextTrackPrivate(parent->priv().webcore()))
    , m_private(*m_ownedPrivate)
    , m_needsRef(needsRef)
{
    if (needsRef)
        m_private.webcore()->ref();
}

TextTrack::~TextTrack()
{
    if (m_needsRef)
        m_private.webcore()->deref();
    if (m_ownedPrivate)
        delete m_ownedPrivate;
}

bool
TextTrack::compare(const TextTrack* other) const
{
    if (this==other)
        return true;
    if (!this || !other)
        return false;
    if (m_private.webcore() == other->m_private.webcore())
        return true;
    return false;
}

String
TextTrack::kind() const
{
    return m_private.kind();
}

String
TextTrack::label() const
{
    return m_private.label();
}

String
TextTrack::language() const
{
    return m_private.language();
}

bool
TextTrack::isDefault() const
{
    return m_private.isDefault();
}

TextTrackCueList*
TextTrack::cues()
{
    return m_private.cues();
}


} // namespace
