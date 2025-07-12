/*
 *  Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(BATTERY_STATUS)

#include "BatteryClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "helpers/privates/WKCBatteryControllerPrivate.h"
#include "helpers/BatteryClientIf.h"

namespace WKC {
BatteryClientWKC::BatteryClientWKC(WKCWebViewPrivate* view)
     : m_view(view)
     , m_appClient(0)
     , m_controller(0)
{
}

BatteryClientWKC::~BatteryClientWKC()
{
    delete m_controller;

    if (m_appClient) {
        m_view->clientBuilders().deleteBatteryClient(m_appClient);
        m_appClient = 0;
    }
}

BatteryClientWKC*
BatteryClientWKC::create(WKCWebViewPrivate* view)
{
    BatteryClientWKC* self = 0;
    self = new BatteryClientWKC(view);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
BatteryClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createBatteryClient(m_view->parent());
    return true;
}

void
BatteryClientWKC::setController(WebCore::BatteryController* controller)
{
    if (!m_controller || m_controller->webcore() != controller) {
        delete m_controller;
        m_controller = new BatteryControllerPrivate(controller);
    }
    if (m_appClient)
        m_appClient->setController(m_controller->wkc());
}

void
BatteryClientWKC::startUpdating()
{
    if (m_appClient)
        m_appClient->startUpdating();
}

void
BatteryClientWKC::stopUpdating()
{
    if (m_appClient)
        m_appClient->stopUpdating();
}

void
BatteryClientWKC::batteryControllerDestroyed()
{
    delete this;
}

} // namespace

#endif // ENABLE(BATTERY_STATUS)
