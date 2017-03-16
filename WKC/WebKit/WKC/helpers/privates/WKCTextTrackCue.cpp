/*
 * Copyright (c) 2013, 2016 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCTextTrackCue.h"
#include "helpers/privates/WKCTextTrackCuePrivate.h"

#include "TextTrackCue.h"
#include "VTTCue.h"

#include "NotImplemented.h"

namespace WKC {

TextTrackCuePrivate::TextTrackCuePrivate(WebCore::TextTrackCue* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_id()
    , m_vertical()
    , m_align()
    , m_text()
{
}

TextTrackCuePrivate::~TextTrackCuePrivate()
{
}

const String&
TextTrackCuePrivate::id()
{
    m_id = m_webcore->id();
    return m_id;
}
double
TextTrackCuePrivate::startTime() const
{
    return m_webcore->startTime();
}
double
TextTrackCuePrivate::endTime() const
{
    return m_webcore->endTime();
}
bool
TextTrackCuePrivate::pauseOnExit() const
{
    return m_webcore->pauseOnExit();
}
const String&
TextTrackCuePrivate::vertical()
{
    m_vertical = (static_cast<WebCore::VTTCue*>(m_webcore))->vertical();
    return m_vertical;
}
bool
TextTrackCuePrivate::snapToLines() const
{
    return (static_cast<WebCore::VTTCue*>(m_webcore))->snapToLines();
}
int
TextTrackCuePrivate::line() const
{
    return (static_cast<WebCore::VTTCue*>(m_webcore))->line();
}
int
TextTrackCuePrivate::position() const
{
    return (static_cast<WebCore::VTTCue*>(m_webcore))->position();
}
int
TextTrackCuePrivate::size() const
{
    return (static_cast<WebCore::VTTCue*>(m_webcore))->size();
}
const String&
TextTrackCuePrivate::align()
{
    m_align = (static_cast<WebCore::VTTCue*>(m_webcore))->align();
    return m_align;
}
const String&
TextTrackCuePrivate::text()
{
    m_text = (static_cast<WebCore::VTTCue*>(m_webcore))->text();
    return m_text;
}


TextTrackCue::TextTrackCue(TextTrackCuePrivate& parent)
    : m_private(parent)
{
}

TextTrackCue::~TextTrackCue()
{
}

const String&
TextTrackCue::id() const
{
    return m_private.id();
}
double
TextTrackCue::startTime() const
{
    return m_private.startTime();
}
double
TextTrackCue::endTime() const
{
    return m_private.endTime();
}
bool
TextTrackCue::pauseOnExit() const
{
    return m_private.pauseOnExit();
}
const String&
TextTrackCue::vertical()
{
    return m_private.vertical();
}
bool
TextTrackCue::snapToLines() const
{
    return m_private.snapToLines();
}
int
TextTrackCue::line() const
{
    return m_private.line();
}
int
TextTrackCue::position() const
{
    return m_private.position();
}
int
TextTrackCue::size() const
{
    return m_private.size();
}
const String&
TextTrackCue::align() const
{
    return m_private.align();
}
const String&
TextTrackCue::text() const
{
    return m_private.text();
}


} // namespace
