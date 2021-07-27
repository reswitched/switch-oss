/*
 *  Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(WKC_WEB_NFC)

#include "WebNfcClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "helpers/privates/WKCWebNfcControllerPrivate.h"
#include "helpers/privates/WKCWebNfcMessagePrivate.h"
#include "helpers/WebNfcClientIf.h"
#include "helpers/WKCString.h"

namespace WKC {

WebNfcClientWKC::WebNfcClientWKC(WKCWebViewPrivate* view)
    : m_view(view)
    , m_appClient(0)
    , m_controller(0)
{
}

WebNfcClientWKC::~WebNfcClientWKC()
{
    delete m_controller;

    if (m_appClient) {
        m_view->clientBuilders().deleteWebNfcClient(m_appClient);
        m_appClient = 0;
    }
}

WebNfcClientWKC*
WebNfcClientWKC::create(WKCWebViewPrivate* view)
{
    WebNfcClientWKC* self = new WebNfcClientWKC(view);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
WebNfcClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createWebNfcClient(m_view->parent());
    return true;
}

void
WebNfcClientWKC::setController(WebCore::WebNfcController* controller)
{
    if (!m_controller || m_controller->webcore() != controller) {
        delete m_controller;
        m_controller = new WebNfcControllerPrivate(controller);
    }
    if (m_appClient) {
        m_appClient->setController(m_controller->wkc());
    }
}

void
WebNfcClientWKC::webNfcControllerDestroyed()
{
    delete this;
}

bool
WebNfcClientWKC::requestPermission()
{
    if (m_appClient) {
        return m_appClient->requestPermission();
    }
    return false;
}

void
WebNfcClientWKC::requestAdapter(int id)
{
    if (m_appClient) {
        m_appClient->requestAdapter(id);
    }
}

void
WebNfcClientWKC::startRequestMessage()
{
    if (m_appClient) {
        m_appClient->startRequestMessage();
    }
}

void
WebNfcClientWKC::stopRequestMessage()
{
    if (m_appClient) {
        m_appClient->stopRequestMessage();
    }
}

void
WebNfcClientWKC::send(int id, RefPtr<WebCore::WebNfcMessage> message, const WKC::String& target)
{
    WebNfcMessagePrivate* priv = new WebNfcMessagePrivate(message);

    if (m_appClient) {
        switch (priv->dataType()) {
        case EWebNfcMessageDataTypeString:
        case EWebNfcMessageDataTypeURL:
        case EWebNfcMessageDataTypeJSON:
            m_appClient->send(id, priv->dataType(), priv->scope(), priv->scopeLength(), priv->stringDatas(), priv->stringDataLengths(), priv->length(), target.characters(), target.length());
            break;
        case EWebNfcMessageDataTypeBlob:
            m_appClient->send(id, priv->scope(), priv->scopeLength(), priv->byteDatas(), priv->byteDataLengths(), priv->contentTypes(), priv->contentTypeLengths(), priv->length(), target.characters(), target.length());
            break;
        default:
            break;
        }
    }
}

} // namespace WKC

#endif // ENABLE(WKC_WEB_NFC)
