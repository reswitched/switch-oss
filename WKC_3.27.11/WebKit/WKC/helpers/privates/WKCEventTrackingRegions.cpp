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

#include "config.h"

#include "helpers/WKCEventTrackingRegions.h"
#include "helpers/privates/WKCEventTrackingRegionsPrivate.h"

#include "helpers/WKCString.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

namespace WKC {

EventTrackingRegionsPrivate*
EventTrackingRegionsPrivate::create(const WebCore::EventTrackingRegions& parent)
{
    return new EventTrackingRegionsPrivate(parent);
}

EventTrackingRegionsPrivate::EventTrackingRegionsPrivate(const WebCore::EventTrackingRegions& parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

EventTrackingRegionsPrivate::~EventTrackingRegionsPrivate()
{
}

WebCore::EventTrackingRegions&
EventTrackingRegionsPrivate::webcore()
{
    return m_webcore;
}

TrackingType
EventTrackingRegionsPrivate::trackingTypeForPoint(const String& eventName, const WKCPoint& point)
{
    return toWKCTrackingType(webcore().trackingTypeForPoint(eventName, point));
}

////////////////////////////////////////////////////////////////////////////////

EventTrackingRegions::EventTrackingRegions(EventTrackingRegionsPrivate& parent)
    : m_private(parent)
{
}

EventTrackingRegions::~EventTrackingRegions()
{
}

TrackingType
EventTrackingRegions::trackingTypeForPoint(const String& eventName, const WKCPoint& point)
{
    return m_private.trackingTypeForPoint(eventName, point);
}

} // namespace
