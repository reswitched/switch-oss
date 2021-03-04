/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCDocument.h"
#include "helpers/privates/WKCDocumentPrivate.h"

#include "Document.h"
#include "RenderView.h"
#include "helpers/privates/WKCCachedResourceLoaderPrivate.h"
#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCHitTestRequestPrivate.h"
#include "helpers/privates/WKCHTMLCollectionPrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCRenderViewPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/WKCHitTestRequest.h"
#include "helpers/WKCKURL.h"

namespace WKC {

DocumentPrivate::DocumentPrivate(WebCore::Document* parent)
    : NodePrivate(parent)
    , m_webcore(parent)
    , m_wkc(*this)
    , m_cachedResourceLoader(0)
    , m_focusedElement(0)
    , m_renderView(0)
    , m_frame(0)
    , m_firstChild(0)
{
}

DocumentPrivate::~DocumentPrivate()
{
    delete m_firstChild;
    delete m_frame;
    delete m_renderView;
    delete m_focusedElement;
    delete m_cachedResourceLoader;
}

bool
DocumentPrivate::isImageDocument() const
{
    return m_webcore->isImageDocument();
}

bool
DocumentPrivate::isSVGDocument() const
{
    return m_webcore->isSVGDocument();
}

void
DocumentPrivate::updateLayoutIgnorePendingStylesheets()
{
    m_webcore->updateLayoutIgnorePendingStylesheets();
}

void
DocumentPrivate::updateHoverActiveState(const HitTestRequest& request, Element* element)
{
    WebCore::Element* elm = element ? ((WebCore::Element*)(((NodePrivate&)(((Node*)element)->priv())).webcore())) : nullptr;
    m_webcore->updateHoverActiveState(*request.priv()->webcore(), elm);
}

KURL
DocumentPrivate::completeURL(const String& url) const
{
    return m_webcore->completeURL(url);    
}

Element*
DocumentPrivate::focusedElement()
{
    WebCore::Element* element = m_webcore->focusedElement();
    if (!element)
        return 0;
    if (!m_focusedElement || m_focusedElement->webcore()!=element) {
        delete m_focusedElement;
        m_focusedElement = ElementPrivate::create(element);
    }
    return &m_focusedElement->wkc();
}

RenderView*
DocumentPrivate::renderView() 
{
    WebCore::RenderView* view = m_webcore->renderView();
    if (!view)
        return 0;
    if (!m_renderView || m_renderView->webcore()!=view) {
        delete m_renderView;
        m_renderView = new RenderViewPrivate(view);
    }
    return &m_renderView->wkc();
}

CachedResourceLoader*
DocumentPrivate::cachedResourceLoader()
{
    WebCore::CachedResourceLoader* loader = &m_webcore->cachedResourceLoader();
    if (!loader)
        return 0;
    if (!m_cachedResourceLoader || m_cachedResourceLoader->webcore()!=loader) {
        delete m_cachedResourceLoader;
        m_cachedResourceLoader = new CachedResourceLoaderPrivate(loader);
    }
    return &m_cachedResourceLoader->wkc();
}

Frame*
DocumentPrivate::frame()
{
    WebCore::Frame* frame = m_webcore->frame();
    if (!frame)
        return 0;
    if (!m_frame || m_frame->webcore()!=frame) {
        delete m_frame;
        m_frame = new FramePrivate(frame);
    }
    return &m_frame->wkc();
}

Node*
DocumentPrivate::firstChild()
{
    WebCore::Node* node = m_webcore->firstChild();
    if (!node)
        return 0;
    if (!m_firstChild || m_firstChild->webcore()!=node) {
        delete m_firstChild;
        m_firstChild = NodePrivate::create(node);
    }
    return &m_firstChild->wkc();
}

HTMLCollection*
DocumentPrivate::forms()
{
    RefPtr<WebCore::HTMLCollection> coll = m_webcore->forms();
    if (!coll)
        return 0;
    HTMLCollectionPrivate* collection = new HTMLCollectionPrivate(coll);
    return &collection->wkc();
}

bool
DocumentPrivate::loadEventFinished() const
{
    return m_webcore->loadEventFinished();
}

void
DocumentPrivate::webkitWillEnterFullScreenForElement(Element* element)
{
    WebCore::Element* e = ((WebCore::Element*)(((NodePrivate&)(((Node*)element)->priv())).webcore()));
    m_webcore->webkitWillEnterFullScreenForElement(e);
}

void
DocumentPrivate::webkitDidEnterFullScreenForElement(Element* element)
{
    WebCore::Element* e = ((WebCore::Element*)(((NodePrivate&)(((Node*)element)->priv())).webcore()));
    m_webcore->webkitDidEnterFullScreenForElement(e);
}

void
DocumentPrivate::webkitWillExitFullScreenForElement(Element* element)
{
    WebCore::Element* e = ((WebCore::Element*)(((NodePrivate&)(((Node*)element)->priv())).webcore()));
    m_webcore->webkitWillExitFullScreenForElement(e);
}

void
DocumentPrivate::webkitDidExitFullScreenForElement(Element* element)
{
    WebCore::Element* e = ((WebCore::Element*)(((NodePrivate&)(((Node*)element)->priv())).webcore()));
    m_webcore->webkitDidExitFullScreenForElement(e);
}

////////////////////////////////////////////////////////////////////////////////

Document::Document(DocumentPrivate& parent)
    : Node(parent)
    , m_private(parent)
{
}

Document::~Document()
{
}

Element*
Document::focusedElement() const
{
    return m_private.focusedElement();
}


Frame*
Document::frame() const
{
    return m_private.frame();
}

Node*
Document::firstChild() const
{
    return m_private.firstChild();
}

CachedResourceLoader*
Document::cachedResourceLoader() const
{
    return m_private.cachedResourceLoader();
}

bool
Document::isImageDocument() const
{
    return m_private.isImageDocument();
}

bool
Document::isSVGDocument() const
{
    return m_private.isSVGDocument();
}

RenderView*
Document::renderView() const
{
    return m_private.renderView();
}

void
Document::updateLayoutIgnorePendingStylesheets()
{
    m_private.updateLayoutIgnorePendingStylesheets();
}

void
Document::updateHoverActiveState(const HitTestRequest& request, Element* element)
{
    m_private.updateHoverActiveState(request, element);
}

KURL
Document::completeURL(const String& url) const
{
    return m_private.completeURL(url);
}

HTMLCollection*
Document::forms() const
{
    return m_private.forms();
}

bool
Document::loadEventFinished() const
{
    return m_private.loadEventFinished();
}

void
Document::webkitWillEnterFullScreenForElement(Element* element)
{
    m_private.webkitWillEnterFullScreenForElement(element);
}

void
Document::webkitDidEnterFullScreenForElement(Element* element)
{
    m_private.webkitDidEnterFullScreenForElement(element);
}

void
Document::webkitWillExitFullScreenForElement(Element* element)
{
    m_private.webkitWillExitFullScreenForElement(element);
}

void
Document::webkitDidExitFullScreenForElement(Element* element)
{
    m_private.webkitDidExitFullScreenForElement(element);
}

} // namespace
