/*
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
 *
 */

#include "config.h"

#if ENABLE(REMOTE_INSPECTOR)

#include "InspectorServerClientWKC.h"
#include "InspectorController.h"
#include "Page.h"
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
    m_view->core()->inspectorController().connectFrontend(*this, true);
    m_remoteFrontendConnected = true;
}

void
InspectorServerClientWKC::remoteFrontendDisconnected()
{
    m_view->core()->inspectorController().disconnectFrontend(*this);
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

