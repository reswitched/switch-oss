/*
 * Copyright (c) 2014, 2015 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "NotificationClientWKC.h"

namespace WKC {

NotificationClientWKC::NotificationClientWKC(WKCWebViewPrivate* view)
     : m_view(view)
     , m_appClient(0)
{
}

NotificationClientWKC::~NotificationClientWKC()
{
}

NotificationClientWKC*
NotificationClientWKC::create(WKCWebViewPrivate* view)
{
    NotificationClientWKC* self = 0;
    self = new NotificationClientWKC(view);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
NotificationClientWKC::construct()
{
    return true;
}

// Ugh!: not supported!
// 140211 ACCESS Co.,Ltd.

bool
NotificationClientWKC::show(WebCore::Notification*)
{
    return true;
}

void
NotificationClientWKC::cancel(WebCore::Notification*)
{
}

void
NotificationClientWKC::clearNotifications(WebCore::ScriptExecutionContext*)
{
}

void
NotificationClientWKC::notificationObjectDestroyed(WebCore::Notification*)
{
}

void
NotificationClientWKC::notificationControllerDestroyed()
{
}

#if ENABLE(LEGACY_NOTIFICATIONS)
void
NotificationClientWKC::requestPermission(WebCore::ScriptExecutionContext*, WTF::PassRefPtr<WebCore::VoidCallback>)
{
}
#endif

#if ENABLE(NOTIFICATIONS)
void
NotificationClientWKC::requestPermission(WebCore::ScriptExecutionContext*, WTF::PassRefPtr<WebCore::NotificationPermissionCallback>)
{
}
#endif

void
NotificationClientWKC::cancelRequestsForPermission(WebCore::ScriptExecutionContext*)
{
}

WebCore::NotificationClient::Permission
NotificationClientWKC::checkPermission(WebCore::ScriptExecutionContext*)
{
    return WebCore::NotificationClient::PermissionAllowed;
}

bool
NotificationClientWKC::hasPendingPermissionRequests(WebCore::ScriptExecutionContext *) const
{
    return false;
}

} // namespace
