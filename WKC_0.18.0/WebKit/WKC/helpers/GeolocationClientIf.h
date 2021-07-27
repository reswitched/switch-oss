/*
 * Copyright (c) 2012, 2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCGEOLOCATIONCLIENT_H
#define WKCGEOLOCATIONCLIENT_H

#include <wkc/wkcbase.h>

namespace WKC {

/*@{*/

class Geolocation;

/** @brief struct for storing geolocation informations. */
struct GeolocationPositionWKC_ {
    /** @brief Time-stamp */
    double fTimestamp;

    /** @brief Latitude */
    double fLatitude;
    /** @brief Longitude */
    double fLongitude;
    /** @brief Altitude */
    double fAltitude;
    /** @brief Accuracy */
    double fAccuracy;
    /** @brief Accuracy of altitude */
    double fAltitudeAccuracy;
    /** @brief Heading */
    double fHeading;
    /** @brief Speed */
    double fSpeed;

    /** @brief is fAltitude available */
    bool fHaveAltitude;
    /** @brief is fAltitudeAccuracy available */
    bool fHaveAltitudeAccuracy;
    /** @brief is fHeading available */
    bool fHaveHeading;
    /** @brief is fSpeed available */
    bool fHaveSpeed;
};
/** @brief type definition of  WKC::GeolocationPositionWKC. */
typedef struct GeolocationPositionWKC_ GeolocationPositionWKC;

/** @brief class of notifying geolocation events. */
class WKC_API GeolocationClientIf
{
public:
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @endcond
    */
    virtual void geolocationDestroyed() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @endcond
    */
    virtual void startUpdating() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @endcond
    */
    virtual void stopUpdating() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param bool (TBD) implement description
       @endcond
    */
    virtual void setEnableHighAccuracy(bool) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param GeolocationPositionWKC& (TBD) implement description
       @endcond
    */
    virtual void lastPosition(GeolocationPositionWKC&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Geolocation* (TBD) implement description
       @endcond
    */
    virtual void requestPermission(WKC::Geolocation*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Geolocation* (TBD) implement description
       @endcond
    */
    virtual void cancelPermissionRequest(WKC::Geolocation*) = 0;
};

/*@}*/

} // namespace
#endif // WKCGEOLOCATIONCLIENT_H
