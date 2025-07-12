/*
 * Copyright (c) 2014, 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_TEXTTRACKLIST_H_
#define _WKC_HELPERS_PRIVATE_TEXTTRACKLIST_H_

#include "helpers/WKCTextTrackList.h"

namespace WebCore { 
class TextTrackList;
} // namespace

namespace WKC {
class TextTrackPrivate;

class TextTrackListWrap : public TextTrackList {
    WTF_MAKE_FAST_ALLOCATED;
public:
    TextTrackListWrap(TextTrackListPrivate& parent) : TextTrackList(parent) {}
    ~TextTrackListWrap() {}
};

class TextTrackListPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    TextTrackListPrivate(WebCore::TextTrackList*);
    ~TextTrackListPrivate();

    WebCore::TextTrackList* webcore() const { return m_webcore; }
    TextTrackList& wkc() { return m_wkc; }

    unsigned length() const;
    TextTrack* item(unsigned index);

private:
    WebCore::TextTrackList* m_webcore;
    TextTrackListWrap m_wkc;

    TextTrackPrivate* m_textTrack;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_TEXTTRACKLIST_H_

