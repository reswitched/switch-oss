/*
 * Copyright (c) 2014-2021 ACCESS CO., LTD. All rights reserved.
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

#ifndef NotificationClientWKC_h
#define NotificationClientWKC_h

#include "NotificationClient.h"

namespace WKC {

class WKCWebViewPrivate;
class NotificationClientIf;

class NotificationClientWKC : public WebCore::NotificationClient
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static NotificationClientWKC* create(WKCWebViewPrivate *);
    virtual ~NotificationClientWKC() override;

    virtual bool show(WebCore::Notification*) override;
    virtual void cancel(WebCore::Notification*) override;
    virtual void clearNotifications(WebCore::ScriptExecutionContext*) override;
    virtual void notificationObjectDestroyed(WebCore::Notification*) override;
    virtual void notificationControllerDestroyed() override;
#if ENABLE(NOTIFICATIONS)
    virtual void requestPermission(WebCore::ScriptExecutionContext*, WTF::RefPtr<WebCore::NotificationPermissionCallback>&&) override;
#endif
    virtual void cancelRequestsForPermission(WebCore::ScriptExecutionContext*) override;
    virtual WebCore::NotificationClient::Permission checkPermission(WebCore::ScriptExecutionContext*) override;

    virtual bool hasPendingPermissionRequests(WebCore::ScriptExecutionContext *) const override;

private:
    NotificationClientWKC(WKCWebViewPrivate *);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
};

} // namespace

#endif // NotificationClientWKC_h
