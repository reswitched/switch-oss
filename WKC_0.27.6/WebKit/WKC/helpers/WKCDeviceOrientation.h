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

#ifndef _WKC_HELPERS_WKC_DEVICEORIENTATION_H_
#define _WKC_HELPERS_WKC_DEVICEORIENTATION_H_

namespace WKC {
class DeviceOrientationPrivate;

class WKC_API DeviceOrientation {
public:
    static DeviceOrientation* create(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute);
    static void destroy(DeviceOrientation*);

    DeviceOrientationPrivate* priv() const { return m_private; }

private:
    DeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute);
    ~DeviceOrientation();

    DeviceOrientation(const DeviceOrientation&);
    DeviceOrientation& operator=(const DeviceOrientation&);

private:
    DeviceOrientationPrivate* m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_DEVICEORIENTATION_H_
