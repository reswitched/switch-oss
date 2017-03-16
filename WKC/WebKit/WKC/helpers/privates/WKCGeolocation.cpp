/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(GEOLOCATION)

#include "helpers/WKCGeolocation.h"
#include "helpers/privates/WKCGeolocationPrivate.h"

#include "Modules/geolocation/Geolocation.h"
#include "Modules/geolocation/GeolocationError.h"

namespace WKC {

Geolocation*
Geolocation::create(Geolocation* other)
{
    void* p = WTF::fastMalloc(sizeof(Geolocation));
    return new (p) Geolocation(other);
}

void
Geolocation::destroy(Geolocation* instance)
{
    if (!instance)
        return;
    instance->~Geolocation();
    WTF::fastFree(instance);
}

GeolocationPrivate::GeolocationPrivate(WebCore::Geolocation* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

GeolocationPrivate::~GeolocationPrivate()
{
}

Geolocation::Geolocation(GeolocationPrivate& parent)
    : m_private(parent)
{
}

Geolocation::Geolocation(Geolocation* other)
    : m_private(*(new GeolocationPrivate(other->m_private.webcore())))
{
}

Geolocation::~Geolocation()
{
}

void
Geolocation::setPermission(bool permission)
{
    m_private.setPermission(permission);
}

void
GeolocationPrivate::setPermission(bool permission)
{
    m_webcore->setIsAllowed(permission);
}

void
Geolocation::positionChanged()
{
    m_private.positionChanged();
}

void
GeolocationPrivate::positionChanged()
{
    m_webcore->positionChanged();
}

void
Geolocation::setError(int type, const char* message)
{
    m_private.setError(type, message);
}

void
GeolocationPrivate::setError(int type, const char* message)
{
    WTF::RefPtr<WebCore::GeolocationError> err = WebCore::GeolocationError::create((WebCore::GeolocationError::ErrorCode)type, message);
    m_webcore->setError(err.get());
}

} // namespace

#endif // ENABLE(GEOLOCATION)
