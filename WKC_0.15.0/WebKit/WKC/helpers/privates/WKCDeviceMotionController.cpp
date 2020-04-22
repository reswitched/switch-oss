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

#include "config.h"

#include "helpers/WKCDeviceMotionController.h"
#include "helpers/privates/WKCDeviceMotionControllerPrivate.h"

#include "DeviceMotionController.h"

#include "helpers/privates/WKCDeviceMotionDataPrivate.h"

namespace WKC {

DeviceMotionControllerPrivate::DeviceMotionControllerPrivate(WebCore::DeviceMotionController* parent)
     : m_webcore(parent)
     , m_wkc(*this)
     , m_motion(0)
{
}

DeviceMotionControllerPrivate::~DeviceMotionControllerPrivate()
{
    DeviceMotionData::destroy(m_motion);
}

void
DeviceMotionControllerPrivate::didChangeDeviceMotion(DeviceMotionData* motion)
{
    if (!motion)
        return;
    DeviceMotionData::destroy(m_motion);
    m_motion = motion;
    m_webcore->didChangeDeviceMotion(m_motion->priv()->webcore().get());
}

////////////////////////////////////////////////////////////////////////////////

DeviceMotionController::DeviceMotionController(DeviceMotionControllerPrivate& parent)
     : m_private(parent)
{
}

DeviceMotionController::~DeviceMotionController()
{
}

void
DeviceMotionController::didChangeDeviceMotion(DeviceMotionData* motion)
{
    m_private.didChangeDeviceMotion(motion);
}

} // namespace
