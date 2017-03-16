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

#ifndef GeolocationClientWKC_h
#define GeolocationClientWKC_h

#include "GeolocationClient.h"

namespace WKC {
class GeolocationClientIf;
class WKCWebViewPrivate;

class GeolocationClientWKC : public WebCore::GeolocationClient
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static GeolocationClientWKC* create(WKCWebViewPrivate* view);
    virtual ~GeolocationClientWKC() override;

    virtual void geolocationDestroyed() override;

    virtual void startUpdating() override;
    virtual void stopUpdating() override;
    virtual void setEnableHighAccuracy(bool) override;
    virtual WebCore::GeolocationPosition* lastPosition() override;

    virtual void requestPermission(WebCore::Geolocation*) override;
    virtual void cancelPermissionRequest(WebCore::Geolocation*) override;

private:
    GeolocationClientWKC(WKCWebViewPrivate* view);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    GeolocationClientIf* m_appClient;
};

}

#endif // GeolocationClientWKC_h
