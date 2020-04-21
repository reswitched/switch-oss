/*
 *  Copyright (c) 2016-2019 ACCESS CO., LTD. All rights reserved.
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

#include "VibrationClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "helpers/VibrationClientIf.h"

#if ENABLE(VIBRATION)

namespace WKC {

VibrationClientWKC::VibrationClientWKC(WKCWebViewPrivate* view)
    : m_view(view)
    , m_appClient(0)
{
}

VibrationClientWKC::~VibrationClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteVibrationClient(m_appClient);
        m_appClient = nullptr;
    }
}

VibrationClientWKC*
VibrationClientWKC::create(WKCWebViewPrivate* view)
{
    VibrationClientWKC* self = nullptr;
    self = new VibrationClientWKC(view);
    if (!self->construct()) {
        delete self;
        return nullptr;
    }
    return self;
}

bool
VibrationClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createVibrationClient(m_view->parent());
    if (!m_appClient)
        return false;
    return true;
}

void
VibrationClientWKC::vibrate(const unsigned& time)
{
    m_appClient->vibrate(time);
}

void
VibrationClientWKC::cancelVibration()
{
    m_appClient->cancelVibration();
}

void
VibrationClientWKC::vibrationEnd()
{
    m_appClient->vibrationEnd();
}

void
VibrationClientWKC::vibrationDestroyed()
{
    m_appClient->vibrationDestroyed();
}

} // namespace

#endif // ENABLE(VIBRATION)
