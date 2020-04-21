/*
 * Copyright (c) 2011, 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_DEVICEMOTIONDATAPRIVATE_H_
#define _WKC_HELPERS_WKC_DEVICEMOTIONDATAPRIVATE_H_

#include "helpers/WKCDeviceMotionData.h"

#include "DeviceMotionData.h"

namespace WKC {

class AccelerationPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    AccelerationPrivate(DeviceMotionData::Acceleration* parent,
                        bool canProvideX, double x, bool canProvideY, double y, bool canProvideZ, double z);
    ~AccelerationPrivate();

    RefPtr<WebCore::DeviceMotionData::Acceleration> webcore() const { return m_webcore; }
    DeviceMotionData::Acceleration* wkc() { return m_wkc; }

private:
    RefPtr<WebCore::DeviceMotionData::Acceleration> m_webcore;
    DeviceMotionData::Acceleration* m_wkc;
};


class RotationRatePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    RotationRatePrivate(DeviceMotionData::RotationRate* parent,
                        bool canProvideAlpha, double alpha, bool canProvideBeta,  double beta, bool canProvideGamma, double gamma);
    ~RotationRatePrivate();

    RefPtr<WebCore::DeviceMotionData::RotationRate> webcore() const { return m_webcore; }
    DeviceMotionData::RotationRate* wkc() { return m_wkc; }

private:
    RefPtr<WebCore::DeviceMotionData::RotationRate> m_webcore;
    DeviceMotionData::RotationRate* m_wkc;
};


class DeviceMotionDataPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    DeviceMotionDataPrivate(DeviceMotionData* parent,
                            DeviceMotionData::Acceleration* acceleration, DeviceMotionData::Acceleration* accelerationIncludingGravity, DeviceMotionData::RotationRate* rotationRate,
                            bool canProvideInterval, double interval);
    ~DeviceMotionDataPrivate();

    RefPtr<WebCore::DeviceMotionData> webcore() const { return m_webcore; }
    DeviceMotionData* wkc() { return m_wkc; }

private:
    RefPtr<WebCore::DeviceMotionData> m_webcore;
    DeviceMotionData* m_wkc;

    DeviceMotionData::Acceleration* m_acceleration;
    DeviceMotionData::Acceleration* m_accelerationIncludingGravity;
    DeviceMotionData::RotationRate* m_rotation;
};

} // namespace

#endif // _WKC_HELPERS_WKC_DEVICEMOTIONDATAPRIVATE_H_
