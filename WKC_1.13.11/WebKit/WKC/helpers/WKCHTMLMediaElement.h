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

#ifndef _WKC_HELPERS_WKC_HTMLMEDIAELEMENT_H_
#define _WKC_HELPERS_WKC_HTMLMEDIAELEMENT_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHTMLElement.h"

namespace WKC {
class HTMLMediaElementPrivate;
class TextTrackList;

class WKC_API HTMLMediaElement : public WKC::HTMLElement {
public:
    HTMLMediaElement(HTMLMediaElementPrivate&);
    virtual ~HTMLMediaElement();

    void* identifier() const;

    void* platformPlayer() const;
    void togglePlayState();
    bool canPlay() const;
    TextTrackList* textTracks();
    bool couldPlayIfEnoughData() const;
    enum NetworkState { NETWORK_EMPTY, NETWORK_IDLE, NETWORK_LOADING, NETWORK_NO_SOURCE };
    NetworkState networkState() const;
};
} // namespace

#endif // _WKC_HELPERS_WKC_HTMLMEDIAELEMENT_H_
