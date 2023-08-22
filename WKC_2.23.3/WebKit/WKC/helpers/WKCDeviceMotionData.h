/*
 * Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_DEVICEMOTIONDATA_H_
#define _WKC_HELPERS_WKC_DEVICEMOTIONDATA_H_

namespace WKC {
class DeviceMotionDataPrivate;
class AccelerationPrivate;
class RotationRatePrivate;

class WKC_API DeviceMotionData {
public:
    class WKC_API Acceleration {
    public:
        static Acceleration* create(bool canProvideX, double x, bool canProvideY, double y, bool canProvideZ, double z);
        static void destroy(Acceleration*);

        AccelerationPrivate* priv() const { return m_private; }

    private:
        Acceleration(bool canProvideX, double x, bool canProvideY, double y, bool canProvideZ, double z);
        ~Acceleration();

        Acceleration(const Acceleration&);
        Acceleration& operator=(const Acceleration&);

    private:
        AccelerationPrivate* m_private;
    };

    class WKC_API RotationRate {
    public:
        static RotationRate* create(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma);
        static void destroy(RotationRate*);

        RotationRatePrivate* priv() const { return m_private; }

    private:
        RotationRate(bool canProvideAlpha, double alpha, bool canProvideBeta,  double beta, bool canProvideGamma, double gamma);
        ~RotationRate();

        RotationRate(const RotationRate&);
        RotationRate& operator=(const RotationRate&);

    private:
        RotationRatePrivate* m_private;
    };

    static DeviceMotionData* create(Acceleration* acceleration, Acceleration* accelerationIncludingGravity, RotationRate* rotationRate,
                                    bool canProvideInterval, double interval);
    static void destroy(DeviceMotionData*);

    DeviceMotionDataPrivate* priv() const { return m_private; }

private:
    DeviceMotionData(Acceleration* acceleration, Acceleration* accelerationIncludingGravity, RotationRate* rotationRate,
                     bool canProvideInterval, double interval);
    ~DeviceMotionData();

    DeviceMotionData(const DeviceMotionData&);
    DeviceMotionData& operator=(const DeviceMotionData&);

private:
    DeviceMotionDataPrivate* m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_DEVICEMOTIONDATA_H_
