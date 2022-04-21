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

#ifndef _WKC_HELPERS_PRIVATE_TEXTTRACK_H_
#define _WKC_HELPERS_PRIVATE_TEXTTRACK_H_

#include "helpers/WKCTextTrack.h"

namespace WebCore { 
class TextTrack;
} // namespace

namespace WKC {

class TextTrackCueListPrivate;

class TextTrackPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    TextTrackPrivate(WebCore::TextTrack*);
    ~TextTrackPrivate();

    WebCore::TextTrack* webcore() const { return m_webcore; }
    TextTrack& wkc() { return m_wkc; }

    String kind() const;
    String label() const;
    String language() const;
    bool isDefault() const;
    TextTrackCueList* cues();

private:
    WebCore::TextTrack* m_webcore;
    TextTrack m_wkc;

    TextTrackCueListPrivate* m_textTrackCueList;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_TEXTTRACK_H_

