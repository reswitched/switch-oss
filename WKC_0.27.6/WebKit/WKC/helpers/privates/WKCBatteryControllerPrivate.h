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

#ifndef _WKC_HELPERS_WKC_BATTERYCONTROLLERPRIVATE_H_
#define _WKC_HELPERS_WKC_BATTERYCONTROLLERPRIVATE_H_

#if ENABLE(BATTERY_STATUS)

#include "helpers/WKCBatteryController.h"

#include "BatteryController.h"
#include "BatteryStatus.h"
#include <wtf/RefPtr.h>

namespace WKC {

class BatteryStatus;

class BatteryControllerWrap : public BatteryController {
WTF_MAKE_FAST_ALLOCATED;
public:
    BatteryControllerWrap(BatteryControllerPrivate& parent) : BatteryController(parent) {}
    ~BatteryControllerWrap() {}
};

class BatteryControllerPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    BatteryControllerPrivate(WebCore::BatteryController* parent);
    ~BatteryControllerPrivate();

    WebCore::BatteryController* webcore() const { return m_webcore; }
    BatteryController* wkc() { return m_wkc; }

    void updateBatteryStatus(const BatteryStatus&);
    void didChangeBatteryStatus(const String& eventType, const BatteryStatus&);

private:
    WebCore::BatteryController* m_webcore;
    BatteryControllerWrap* m_wkc;

    WTF::RefPtr<WebCore::BatteryStatus> m_status;
};

} // namespace

#endif // ENABLE(BATTERY_STATUS)

#endif // _WKC_HELPERS_WKC_BATTERYCONTROLLERPRIVATE_H_
