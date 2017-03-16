/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_GEOLOCATION_H_
#define _WKC_HELPERS_PRIVATE_GEOLOCATION_H_

#include "helpers/WKCGeolocation.h"

namespace WebCore {
class Geolocation;
} // namespace

namespace WKC {

class GeolocationWrap : public Geolocation {
WTF_MAKE_FAST_ALLOCATED;
public:
    GeolocationWrap(GeolocationPrivate& parent) : Geolocation(parent) {}
    ~GeolocationWrap() {}
};

class GeolocationPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    GeolocationPrivate(WebCore::Geolocation*);
    ~GeolocationPrivate();

    WebCore::Geolocation* webcore() const { return m_webcore; }
    Geolocation& wkc() { return m_wkc; }

    void setPermission(bool permission);
    void positionChanged();
    void setError(int type, const char* message);

private:
    WebCore::Geolocation* m_webcore;
    GeolocationWrap m_wkc;

};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_GEOLOCATION_H_

