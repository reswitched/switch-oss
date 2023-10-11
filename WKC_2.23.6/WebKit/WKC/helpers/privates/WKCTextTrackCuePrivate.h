/*
 * Copyright (c) 2013-2022 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_TEXTTRACKCUE_H_
#define _WKC_HELPERS_PRIVATE_TEXTTRACKCUE_H_

#include "helpers/WKCTextTrackCue.h"

namespace WebCore { 
class TextTrackCue;
} // namespace

namespace WKC {

class TextTrackCuePrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    TextTrackCuePrivate(WebCore::TextTrackCue*);
    ~TextTrackCuePrivate();

    WebCore::TextTrackCue* webcore() const { return m_webcore; }
    TextTrackCue& wkc() { return m_wkc; }

    const String& id();
    double startTime() const;
    double endTime() const;
    bool pauseOnExit() const;
    const String& vertical();
    bool snapToLines() const;
    double line() const;
    int position() const;
    int size() const;
    const String& align();
    const String& text();

private:
    WebCore::TextTrackCue* m_webcore;
    TextTrackCue m_wkc;

    String m_id;
    String m_vertical;
    String m_align;
    String m_text;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_TEXTTRACKCUE_H_

