/*
 * Copyright (c) 2012-2019 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(REMOTE_INSPECTOR)

#include "InspectorServerClientWKC.h"
#include "InspectorAgentBase.h"
#include "InspectorController.h"
#include "Page.h"
#include "WKCWebViewPrivate.h"
#include "WebInspectorServer.h"

namespace WKC {

InspectorServerClientWKC*
InspectorServerClientWKC::create(WKCWebViewPrivate* view)
{
    return new InspectorServerClientWKC(view);
}

InspectorServerClientWKC::InspectorServerClientWKC(WKCWebViewPrivate* view)
    : m_view(view),
      m_remoteInspectionPageId(0),
      m_remoteFrontendConnected(false)
{
}

InspectorServerClientWKC::~InspectorServerClientWKC()
{
    if (m_remoteInspectionPageId)
        WebInspectorServer::sharedInstance()->unregisterPage(m_remoteInspectionPageId);
}

void
InspectorServerClientWKC::remoteFrontendConnected()
{
    m_view->core()->inspectorController().connectFrontend(this, true);
    m_remoteFrontendConnected = true;
}

void
InspectorServerClientWKC::remoteFrontendDisconnected()
{
    m_view->core()->inspectorController().disconnectFrontend(this);
    m_remoteFrontendConnected = false;
}

void
InspectorServerClientWKC::dispatchMessageFromRemoteFrontend(const WTF::String& message)
{
    m_view->core()->inspectorController().dispatchMessageFromFrontend(message);
}

void
InspectorServerClientWKC::sendMessageToRemoteFrontend(const WTF::String& message)
{
    if (m_remoteInspectionPageId)
        WebInspectorServer::sharedInstance()->sendMessageOverConnection(m_remoteInspectionPageId, message);
}

void
InspectorServerClientWKC::enableRemoteInspection()
{
    if (!m_remoteInspectionPageId)
        m_remoteInspectionPageId = WebInspectorServer::sharedInstance()->registerPage(this);
}

void
InspectorServerClientWKC::disableRemoteInspection()
{
    if (m_remoteInspectionPageId)
        WebInspectorServer::sharedInstance()->unregisterPage(m_remoteInspectionPageId);
    m_remoteInspectionPageId = 0;
}

Inspector::FrontendChannel::ConnectionType
InspectorServerClientWKC::connectionType() const
{
    if (m_remoteInspectionPageId)
        return Inspector::FrontendChannel::ConnectionType::Remote;
    else
        return Inspector::FrontendChannel::ConnectionType::Local;
}

void
InspectorServerClientWKC::sendMessageToFrontend(const WTF::String& message)
{
    if (!m_remoteInspectionPageId)
        return;
    WebInspectorServer::sharedInstance()->sendMessageOverConnection(m_remoteInspectionPageId, message);
}

} // namespace

#endif // ENABLE(REMOTE_INSPECTOR)

