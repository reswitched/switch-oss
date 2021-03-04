/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (c) 2010, 2015 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DragClientWKC.h"
#include "WKCWebViewPrivate.h"
#include "helpers/DragClientIf.h"

#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"

#include "DragData.h"
#include "Frame.h"
#include "WTFString.h"
#include "URL.h"

#include "helpers/privates/WKCClipboardPrivate.h"
#include "helpers/privates/WKCDragDataPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"

// implementations

namespace WKC {

DragClientWKC::DragClientWKC(WKCWebViewPrivate* view)
     : m_view(view),
       m_appClient(0)
{
}

DragClientWKC::~DragClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteDragClient(m_appClient);
        m_appClient = 0;
    }
}

DragClientWKC*
DragClientWKC::create(WKCWebViewPrivate* view)
{
    DragClientWKC* self = 0;
    self = new DragClientWKC(view);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
DragClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createDragClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void
DragClientWKC::willPerformDragDestinationAction(WebCore::DragDestinationAction action, const WebCore::DragData& data)
{
    DragDataPrivate d(&data);
    m_appClient->willPerformDragDestinationAction((WKC::DragDestinationAction)action, &d.wkc());
}
void
DragClientWKC::willPerformDragSourceAction(WebCore::DragSourceAction action, const WebCore::IntPoint& pos, WebCore::DataTransfer& transfer)
{
//    ClipboardPrivate c(&transfer);
//    m_appClient->willPerformDragSourceAction((WKC::DragSourceAction)action, pos, &c.wkc());
}

WebCore::DragSourceAction
DragClientWKC::dragSourceActionMaskForPoint(const WebCore::IntPoint& windowPoint)
{
    return (WebCore::DragSourceAction)m_appClient->dragSourceActionMaskForPoint(windowPoint);
}

void
DragClientWKC::startDrag(WebCore::DragItem dragItem, WebCore::DataTransfer& transfer, WebCore::Frame& frame)
{
//    FramePrivate fp(&frame);
//    ClipboardPrivate c(clipboard);
//    m_appClient->startDrag((WKC::DragImageRef)dragImage, dragImageOrigin, eventPos, &c.wkc(), &fp.wkc(), linkDrag);
}

void
DragClientWKC::dragEnded()
{
}

void
DragClientWKC::dragControllerDestroyed()
{
    delete this;
}

} // namespace
