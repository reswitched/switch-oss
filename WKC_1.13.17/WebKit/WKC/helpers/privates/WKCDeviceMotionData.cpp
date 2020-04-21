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

#include "helpers/WKCDeviceMotionData.h"
#include "helpers/privates/WKCDeviceMotionDataPrivate.h"

#include "DeviceMotionData.h"

namespace WKC {

// Private Implementation

// DeviceMotionDataPrivate

DeviceMotionDataPrivate::DeviceMotionDataPrivate(DeviceMotionData* parent,
                                                 DeviceMotionData::Acceleration* acceleration, DeviceMotionData::Acceleration* accelerationIncludingGravity, DeviceMotionData::RotationRate* rotationRate,
                                                 bool canProvideInterval, double interval)
     : m_wkc(parent)
     , m_acceleration(acceleration)
     , m_accelerationIncludingGravity(accelerationIncludingGravity)
     , m_rotation(rotationRate)
{
    std::optional<double> d;
    if (canProvideInterval)
        d = interval;
    else
        d = std::nullopt;
    m_webcore = WebCore::DeviceMotionData::create(acceleration->priv()->webcore(), accelerationIncludingGravity->priv()->webcore(), rotationRate->priv()->webcore(), d);
}

DeviceMotionDataPrivate::~DeviceMotionDataPrivate()
{
    if (m_acceleration) {
        DeviceMotionData::Acceleration::destroy(m_acceleration);
    }
    if (m_accelerationIncludingGravity) {
        DeviceMotionData::Acceleration::destroy(m_accelerationIncludingGravity);
    }
    if (m_rotation) {
        DeviceMotionData::RotationRate::destroy(m_rotation);
    }
}

// AccelerationPrivate

AccelerationPrivate::AccelerationPrivate(DeviceMotionData::Acceleration* parent,
                                         bool canProvideX, double x, bool canProvideY, double y, bool canProvideZ, double z)
     : m_wkc(parent)
{
    std::optional<double> optx, opty, optz;
    if (canProvideX)
        optx = x;
    else
        optx = std::nullopt;
    if (canProvideY)
        opty = y;
    else
        opty = std::nullopt;
    if (canProvideZ)
        optz = z;
    else
        optz = std::nullopt;
    m_webcore = WebCore::DeviceMotionData::Acceleration::create(optx, opty, optz);
}

AccelerationPrivate::~AccelerationPrivate()
{
}

// RotationRatePrivate

RotationRatePrivate::RotationRatePrivate(DeviceMotionData::RotationRate* parent,
                                         bool canProvideAlpha, double alpha, bool canProvideBeta,  double beta, bool canProvideGamma, double gamma)
     : m_wkc(parent)
{
    std::optional<double> opta, optb, optg;
    if (canProvideAlpha)
        opta = alpha;
    else
        opta = std::nullopt;
    if (canProvideBeta)
        optb = beta;
    else
        optb = std::nullopt;
    if (canProvideGamma)
        optg = gamma;
    else
        optg = std::nullopt;
    m_webcore = WebCore::DeviceMotionData::RotationRate::create(opta, optb, optg);
}

RotationRatePrivate::~RotationRatePrivate()
{
}


// Implementation

// DeviceMotionData

DeviceMotionData::DeviceMotionData(Acceleration* acceleration, Acceleration* accelerationIncludingGravity, RotationRate* rotationRate,
                                   bool canProvideInterval, double interval)
     : m_private(new DeviceMotionDataPrivate(this, acceleration, accelerationIncludingGravity, rotationRate, canProvideInterval, interval))
{
}

DeviceMotionData::~DeviceMotionData()
{
    delete m_private;
}

DeviceMotionData*
DeviceMotionData::create(Acceleration* acceleration, Acceleration* accelerationIncludingGravity, RotationRate* rotationRate,
                         bool canProvideInterval, double interval)
{
    void* p = WTF::fastMalloc(sizeof(DeviceMotionData));
    return new (p) DeviceMotionData(acceleration, accelerationIncludingGravity, rotationRate, canProvideInterval, interval);
}

void
DeviceMotionData::destroy(DeviceMotionData* instance)
{
    if (!instance)
        return;
    instance->~DeviceMotionData();
    WTF::fastFree(instance);
}

// DeviceMotionData::Acceleration

DeviceMotionData::Acceleration::Acceleration(bool canProvideX, double x, bool canProvideY, double y, bool canProvideZ, double z)
     : m_private(new AccelerationPrivate(this, canProvideX, x, canProvideY, y, canProvideZ, z))
{
}

DeviceMotionData::Acceleration::~Acceleration()
{
    delete m_private;
}

DeviceMotionData::Acceleration*
DeviceMotionData::Acceleration::create(bool canProvideX, double x, bool canProvideY, double y, bool canProvideZ, double z)
{
    void* p = WTF::fastMalloc(sizeof(Acceleration));
    return new (p) Acceleration(canProvideX, x, canProvideY, y, canProvideZ, z);
}

void
DeviceMotionData::Acceleration::destroy(Acceleration* instance)
{
    if (!instance)
        return;
    instance->~Acceleration();
    WTF::fastFree(instance);
}

// DeviceMotionData::RotationRate

DeviceMotionData::RotationRate::RotationRate(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
     : m_private(new RotationRatePrivate(this, canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma))
{
}

DeviceMotionData::RotationRate::~RotationRate()
{
    delete m_private;
}

DeviceMotionData::RotationRate*
DeviceMotionData::RotationRate::create(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    void* p = WTF::fastMalloc(sizeof(DeviceMotionData::RotationRate));
    return new (p) DeviceMotionData::RotationRate(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma);
}

void
DeviceMotionData::RotationRate::destroy(RotationRate* instance)
{
    if (!instance)
        return;
    instance->~RotationRate();
    WTF::fastFree(instance);
}

} // namespace
