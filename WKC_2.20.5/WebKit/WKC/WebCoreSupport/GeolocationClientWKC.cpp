/*
 * Copyright (c) 2012, 2016 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(GEOLOCATION)

#include "GeolocationClientWKC.h"
#include "Modules/geolocation/GeolocationPosition.h"
#include "WKCWebView.h"
#include "WKCWebViewPrivate.h"
#include "helpers/GeolocationClientIf.h"

#include <wkc/wkcpeer.h>

#include "helpers/privates/WKCGeolocationPrivate.h"

namespace WKC {

GeolocationClientWKC::GeolocationClientWKC(WKCWebViewPrivate* view)
     : m_view(view)
{
    m_appClient = 0;
}

GeolocationClientWKC::~GeolocationClientWKC()
{
#if 0
    if (m_appClient) {
        m_view->clientBuilders().deleteGeolocationClient(m_appClient);
        m_appClient = 0;
    }
#endif
}

GeolocationClientWKC*
GeolocationClientWKC::create(WKCWebViewPrivate* view)
{
    GeolocationClientWKC* self = 0;
    self = new GeolocationClientWKC(view);
    if (!self)
      return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;

}

bool
GeolocationClientWKC::construct()
{
#if 0
    m_appClient = m_view->clientBuilders().createGeolocationClient(m_view->parent());
    if (!m_appClient) return false;
#endif
    return true;
}

void
GeolocationClientWKC::geolocationDestroyed()
{
    delete this;
}

void
GeolocationClientWKC::startUpdating(const WTF::String& authorizationToken)
{
#if 0
    m_appClient->startUpdating(authorizationToken);
#endif
}

void
GeolocationClientWKC::stopUpdating()
{
#if 0
    m_appClient->stopUpdating();
#endif
}

void
GeolocationClientWKC::setEnableHighAccuracy(bool flag)
{
#if 0
    m_appClient->setEnableHighAccuracy(flag);
#endif
}

Optional<WebCore::GeolocationPositionData>
GeolocationClientWKC::lastPosition()
{
#if 0
    GeolocationPositionWKC pos = {0};
    m_appClient->lastPosition(pos);
    WTF::RefPtr<WebCore::GeolocationPosition> wpos = WebCore::GeolocationPosition::create(
        pos.fTimestamp,
        pos.fLatitude,
        pos.fLongitude,
        pos.fAccuracy,
        pos.fHaveAltitude, pos.fAltitude,
        pos.fHaveAltitudeAccuracy, pos.fAltitudeAccuracy,
        pos.fHaveHeading, pos.fHeading,
        pos.fHaveSpeed, pos.fSpeed);
    return wpos.release().leakRef();
#else
    WTF::RefPtr<WebCore::GeolocationPosition> wpos = WebCore::GeolocationPosition::create(
        0,
        0,
        0,
        0,
        0, 0,
        0, 0,
        0, 0,
        0, 0);
    return wpos.release().leakRef();
#endif
}

void
GeolocationClientWKC::requestPermission(WebCore::Geolocation* webcore)
{
#if 0
    GeolocationPrivate g(webcore);
    m_appClient->requestPermission(&g.wkc());
#endif
}

void
GeolocationClientWKC::cancelPermissionRequest(WebCore::Geolocation* webcore)
{
#if 0
    GeolocationPrivate g(webcore);
    m_appClient->cancelPermissionRequest(&g.wkc());
#endif
}

} // namespace

#else

#include "helpers/GeolocationClientIf.h"

#endif // ENABLE(GEOLOCATION)
