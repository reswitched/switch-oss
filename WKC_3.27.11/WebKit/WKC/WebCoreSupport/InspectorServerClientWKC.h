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

#ifndef InspectorServerClientWKC_h
#define InspectorServerClientWKC_h

#if ENABLE(REMOTE_INSPECTOR)

#include "WebInspectorServerClient.h"
#include "WKCWebViewPrivate.h"

#include "InspectorFrontendChannel.h"

namespace WKC {
class WKCWebView;

class InspectorServerClientWKC : public WebInspectorServerClient, public Inspector::FrontendChannel {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static InspectorServerClientWKC* create(WKCWebViewPrivate*);
    ~InspectorServerClientWKC();

    // from WebInspectorServerClient
    virtual void remoteFrontendConnected();
    virtual void remoteFrontendDisconnected();
    virtual void dispatchMessageFromRemoteFrontend(const WTF::String& message);
    virtual WKCWebView* webView() { return m_view->parent(); }

    void sendMessageToRemoteFrontend(const WTF::String& message);
    void enableRemoteInspection();
    void disableRemoteInspection();
    bool hasRemoteFrontendConnected() { return m_remoteFrontendConnected; }

    // InspectorFrontendChannel
    virtual Inspector::FrontendChannel::ConnectionType connectionType() const;
    virtual void sendMessageToFrontend(const WTF::String& message);

private:
   InspectorServerClientWKC(WKCWebViewPrivate*);

   WKCWebViewPrivate* m_view;
   int m_remoteInspectionPageId;
   bool m_remoteFrontendConnected;
};

} // namespace

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // InspectorServerClientWKC_h
