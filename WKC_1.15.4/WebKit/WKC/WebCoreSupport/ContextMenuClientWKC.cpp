/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(CONTEXT_MENUS)

#include "ContextMenuClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "ContextMenu.h"
#include "ContextMenuItem.h"
#include "Frame.h"
#include "URL.h"
#include "NotImplemented.h"

#include "helpers/ContextMenuClientIf.h"
#include "helpers/WKCString.h"
#include "helpers/WKCKURL.h"

#include "helpers/privates/WKCContextMenuPrivate.h"
#include "helpers/privates/WKCContextMenuItemPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"

// implementations

namespace WKC {

ContextMenuClientWKC::ContextMenuClientWKC(WKCWebViewPrivate* view)
     :m_view(view)
{
    m_appClient = 0;
}

ContextMenuClientWKC::~ContextMenuClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteContextMenuClient(m_appClient);
        m_appClient = 0;
    }
}

ContextMenuClientWKC*
ContextMenuClientWKC::create(WKCWebViewPrivate* view)
{
    ContextMenuClientWKC* self = 0;
    self = new ContextMenuClientWKC(view);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
ContextMenuClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createContextMenuClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void
ContextMenuClientWKC::contextMenuDestroyed()
{
    delete this;
}

WebCore::PlatformMenuDescription
ContextMenuClientWKC::getCustomMenuFromDefaultItems(WebCore::ContextMenu* menu)
{
    ContextMenuPrivate wm(menu);
    return (WebCore::PlatformMenuDescription)m_appClient->getCustomMenuFromDefaultItems(&wm.wkc());
}
void
ContextMenuClientWKC::contextMenuItemSelected(WebCore::ContextMenuItem* item, const WebCore::ContextMenu* menu)
{
    ContextMenuPrivate wm(const_cast<WebCore::ContextMenu*>(menu));
    ContextMenuItemPrivate wi(item);
    m_appClient->contextMenuItemSelected(&wi.wkc(), &wm.wkc());
}

void
ContextMenuClientWKC::downloadURL(const WebCore::URL& url)
{
    m_appClient->downloadURL(url);
}
void
ContextMenuClientWKC::searchWithGoogle(const WebCore::Frame* frame)
{
    FramePrivate fp(frame);
    m_appClient->searchWithGoogle(&fp.wkc());
}
void
ContextMenuClientWKC::lookUpInDictionary(WebCore::Frame* frame)
{
    FramePrivate fp(frame);
    m_appClient->lookUpInDictionary(&fp.wkc());
}
void
ContextMenuClientWKC::speak(const WTF::String& string)
{
    m_appClient->speak(string);
}
void
ContextMenuClientWKC::stopSpeaking()
{
    m_appClient->stopSpeaking();
}
bool
ContextMenuClientWKC::isSpeaking()
{
    return m_appClient->isSpeaking();
}
WebCore::ContextMenuItem
ContextMenuClientWKC::shareMenuItem(const WebCore::HitTestResult&)
{
    notImplemented();

    WKC_DEFINE_STATIC_TYPE(WebCore::ContextMenuItem*, item, 0);
    if (!item)
        item = new WebCore::ContextMenuItem();
    return *item;
}

} // namespace

#else

#include "helpers/ContextMenuClientIf.h"

#endif // ENABLE(CONTEXT_MENUS)
