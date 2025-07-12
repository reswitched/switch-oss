/*
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

#ifndef _WKC_HELPERS_PRIVATE_EVENTTRACKINGREGIONS_H_
#define _WKC_HELPERS_PRIVATE_EVENTTRACKINGREGIONS_H_

#include "helpers/WKCEventTrackingRegions.h"

#include "EventTrackingRegions.h"

namespace WKC {

class EventTrackingRegionsWrap : public EventTrackingRegions {
WTF_MAKE_FAST_ALLOCATED;
public:
    EventTrackingRegionsWrap(EventTrackingRegionsPrivate& parent) : EventTrackingRegions(parent) {}
    ~EventTrackingRegionsWrap() {}
};

class EventTrackingRegionsPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    static EventTrackingRegionsPrivate* create(const WebCore::EventTrackingRegions&);
    virtual ~EventTrackingRegionsPrivate();

    WebCore::EventTrackingRegions& webcore();
    EventTrackingRegions& wkc() { return m_wkc; }

    TrackingType trackingTypeForPoint(const String& eventName, const WKCPoint&);

private:
    EventTrackingRegionsPrivate(const WebCore::EventTrackingRegions&);

private:
    WebCore::EventTrackingRegions m_webcore;
    EventTrackingRegionsWrap m_wkc;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_EVENTTRACKINGREGIONS_H_
