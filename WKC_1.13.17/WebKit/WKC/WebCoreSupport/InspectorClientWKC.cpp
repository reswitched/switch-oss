/*
 *  Copyright (C) 2008 Gustavo Noronha Silva
 *  Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "InspectorClientWKC.h"
#if ENABLE(REMOTE_INSPECTOR)
#include "InspectorServerClientWKC.h"
#endif

#include "WKCOverlayPrivate.h"

#include "InspectorController.h"
#include "RenderObject.h"
#include "RenderElement.h"
#include "RenderStyle.h"

#include "WKCWebViewPrivate.h"

#include "helpers/InspectorClientIf.h"

#include "Frame.h"
#include "FrameView.h"
#include "Page.h"
#include "WTFString.h"

// implementations

namespace WKC {

InspectorClientWKC::InspectorClientWKC(WKCWebViewPrivate* view)
     : m_view(view),
       m_appClient(0),
       m_inspectedWebView(0),
       m_webInspector(0)
#if ENABLE(REMOTE_INSPECTOR)
       , m_inspectorServerClient(0)
#endif
{
}
InspectorClientWKC::~InspectorClientWKC()
{
    if (m_appClient) {
        if (m_view) {
            m_view->clientBuilders().deleteInspectorClient(m_appClient);
        } else {
            // Ugh!: no way to destroy!
        }
    }
}

InspectorClientWKC*
InspectorClientWKC::create(WKCWebViewPrivate* view)
{
    InspectorClientWKC* self = 0;
    self = new InspectorClientWKC(view);
    if (!self) {
        return 0;
    }
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
InspectorClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createInspectorClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void
InspectorClientWKC::webViewDestroyed()
{
    m_view = 0;
}
void
InspectorClientWKC::inspectedPageDestroyed()
{
    delete this;
}

WebCore::Page*
InspectorClientWKC::createPage()
{
    return 0;
}

WTF::String
InspectorClientWKC::localizedStringsURL()
{
    return WTF::String();
}

WTF::String
InspectorClientWKC::hiddenPanels()
{
    return WTF::String();
}

void
InspectorClientWKC::showWindow()
{
}

void
InspectorClientWKC::closeWindow()
{
}

void
InspectorClientWKC::attachWindow()
{
}

void
InspectorClientWKC::detachWindow()
{
}

void
InspectorClientWKC::setAttachedWindowHeight(unsigned height)
{
}

void
InspectorClientWKC::highlight()
{
#if ENABLE(REMOTE_INSPECTOR)
    int fixedDirectionFlag = EFixedDirectionNone;

    WebCore::InspectorController& controller = m_view->core()->inspectorController();
    if (controller.enabled()) {
        WebCore::Node* node = controller.highlightedNode();
        if (node && node->renderer()) {
            const WebCore::RenderStyle& style = node->renderer()->style();
            if (style.position() == WebCore::PositionType::Fixed) {
                fixedDirectionFlag |= style.left().isSpecified() ? EFixedDirectionLeft : 0;
                fixedDirectionFlag |= style.right().isSpecified() ? EFixedDirectionRight : 0;
                fixedDirectionFlag |= style.top().isSpecified() ? EFixedDirectionTop : 0;
                fixedDirectionFlag |= style.bottom().isSpecified() ? EFixedDirectionBottom : 0;
            }
        }
    }
    m_view->addOverlay(this, 0, fixedDirectionFlag);
#endif
}

void
InspectorClientWKC::hideHighlight()
{
#if ENABLE(REMOTE_INSPECTOR)
    m_view->removeOverlay(this);
#endif
}

void
InspectorClientWKC::paintOverlay(WebCore::GraphicsContext& ctx)
{
#if ENABLE(REMOTE_INSPECTOR)
    WebCore::Frame& frame = m_view->core()->mainFrame();
    if (frame.view()) {
        WebCore::InspectorController& controller = m_view->core()->inspectorController();
        if (controller.enabled()) {
            controller.drawHighlight(ctx);
        }
    }
#endif
}

void
InspectorClientWKC::inspectedURLChanged(const WTF::String& newURL)
{
}

void
InspectorClientWKC::populateSetting(const WTF::String& key, WTF::String* value)
{
}

void
InspectorClientWKC::storeSetting(const WTF::String& key, const WTF::String& value)
{
}

void
InspectorClientWKC::inspectorWindowObjectCleared()
{
}

Inspector::FrontendChannel*
InspectorClientWKC::openLocalFrontend(WebCore::InspectorController *)
{
    return 0;
}

void
InspectorClientWKC::closeLocalFrontend()
{
}

void
InspectorClientWKC::bringFrontendToFront()
{
}

bool
InspectorClientWKC::sendMessageToFrontend(const WTF::String& message)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (m_inspectorServerClient && m_inspectorServerClient->hasRemoteFrontendConnected()) {
        m_inspectorServerClient->sendMessageToRemoteFrontend(message);
        return true;
    }
#endif

    return false;
}

} // namespace
