/*
 * Copyright (c) 2013-2016 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(BATTERY_STATUS)

#include "helpers/WKCBatteryController.h"
#include "helpers/privates/WKCBatteryControllerPrivate.h"
#include "helpers/WKCBatteryStatus.h"

#include "BatteryController.h"
#include "BatteryStatus.h"

namespace WKC {

// Private Implementation

BatteryControllerPrivate::BatteryControllerPrivate(WebCore::BatteryController* parent)
     : m_webcore(parent)
     , m_wkc(new BatteryControllerWrap(*this))
{
}

BatteryControllerPrivate::~BatteryControllerPrivate()
{
    delete m_wkc;
}

void
BatteryControllerPrivate::updateBatteryStatus(const BatteryStatus& status)
{
    m_status = WebCore::BatteryStatus::create(status.m_charging, status.m_chargingTime, status.m_dischargingTime, status.m_level);
    m_webcore->updateBatteryStatus(m_status);
}

void
BatteryControllerPrivate::didChangeBatteryStatus(const String& eventType, const BatteryStatus& status)
{
    m_status = WebCore::BatteryStatus::create(status.m_charging, status.m_chargingTime, status.m_dischargingTime, status.m_level);
    WTF::String type(eventType);
    m_webcore->didChangeBatteryStatus(type, m_status);
}


// Implementation

BatteryController::BatteryController(BatteryControllerPrivate& parent)
     : m_private(parent)
{
}

BatteryController::~BatteryController()
{
}

void
BatteryController::updateBatteryStatus(const BatteryStatus& status)
{
    m_private.updateBatteryStatus(status);
}

void
BatteryController::didChangeBatteryStatus(const String& eventType, const BatteryStatus& status)
{
    m_private.didChangeBatteryStatus(eventType, status);
}

} // namespace

#endif // ENABLE(BATTERY_STATUS)
