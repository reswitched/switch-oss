/*
 *  Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "DeviceMotionClientWKC.h"

#include "WKCWebViewPrivate.h"

#include "helpers/DeviceMotionClientIf.h"

#include "helpers/WKCDeviceMotionData.h"
#include "helpers/privates/WKCDeviceMotionDataPrivate.h"
#include "helpers/privates/WKCDeviceMotionControllerPrivate.h"

namespace WKC {

DeviceMotionClientWKC::DeviceMotionClientWKC(WKCWebViewPrivate* view)
     : m_view(view)
     , m_appClient(0)
     , m_controller(0)
     , m_motion(0)
{
}

DeviceMotionClientWKC::~DeviceMotionClientWKC()
{
    DeviceMotionData::destroy(m_motion);
    delete m_controller;

    if (m_appClient) {
        m_view->clientBuilders().deleteDeviceMotionClient(m_appClient);
        m_appClient = 0;
    }
}

DeviceMotionClientWKC*
DeviceMotionClientWKC::create(WKCWebViewPrivate* view)
{
    DeviceMotionClientWKC* self = 0;
    self = new DeviceMotionClientWKC(view);
    if (!self) {
        return 0;
    }
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
DeviceMotionClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createDeviceMotionClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void
DeviceMotionClientWKC::setController(WebCore::DeviceMotionController* controller)
{
    if (!m_controller || m_controller->webcore() != controller) {
        delete m_controller;
        m_controller = new DeviceMotionControllerPrivate(controller);
    }
    m_appClient->setController(&m_controller->wkc());
}

void
DeviceMotionClientWKC::startUpdating()
{
    m_appClient->startUpdating();
}

void
DeviceMotionClientWKC::stopUpdating()
{
    m_appClient->stopUpdating();
}

WebCore::DeviceMotionData*
DeviceMotionClientWKC::lastMotion() const
{
    DeviceMotionData::destroy(m_motion);
    m_motion = m_appClient->lastMotion();
    if (!m_motion)
        return 0;
    return m_motion->priv()->webcore().get();
}

void
DeviceMotionClientWKC::deviceMotionControllerDestroyed()
{
    delete this;
}

} // namespace WKC
