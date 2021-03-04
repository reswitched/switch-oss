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

#include "helpers/WKCDeviceOrientation.h"
#include "helpers/privates/WKCDeviceOrientationPrivate.h"

#include "DeviceOrientationData.h"

namespace WKC {

    DeviceOrientationPrivate::DeviceOrientationPrivate(DeviceOrientation* parent, bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute)
        : m_webcore(WebCore::DeviceOrientationData::create(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma, canProvideAbsolute, absolute))
     , m_wkc(parent)
{
}

DeviceOrientationPrivate::~DeviceOrientationPrivate()
{
}

////////////////////////////////////////////////////////////////////////////////

DeviceOrientation::DeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute)
    : m_private(new DeviceOrientationPrivate(this, canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma, canProvideAbsolute, absolute))
{
}

DeviceOrientation::~DeviceOrientation()
{
    delete m_private;
}

DeviceOrientation*
DeviceOrientation::create(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma, bool canProvideAbsolute, bool absolute)
{
    void* p = WTF::fastMalloc(sizeof(DeviceOrientation));
    return new (p) DeviceOrientation(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma, canProvideAbsolute, absolute);
}

void
DeviceOrientation::destroy(DeviceOrientation* instance)
{
    if (!instance)
        return;
    instance->~DeviceOrientation();
    WTF::fastFree(instance);
}

} // namespace
