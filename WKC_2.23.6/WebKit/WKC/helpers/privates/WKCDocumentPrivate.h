/*
 * Copyright (c) 2011-2022 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_DOCUMENT_H_
#define _WKC_HELPERS_PRIVATE_DOCUMENT_H_

#include "helpers/WKCDocument.h"
#include "WKCNodePrivate.h"

namespace WebCore {
class Document;
} // namespace

namespace WKC {

class CachedResourceLoader;
class Frame;
class Element;
class RenderView;
class CachedResourceLoaderPrivate;
class FramePrivate;
class HTMLCollectionPrivate;
class ElementPrivate;
class NodePrivate;
class RenderViewPrivate;

class DocumentWrap : public Document {
WTF_MAKE_FAST_ALLOCATED;
public:
    DocumentWrap(DocumentPrivate& parent) : Document(parent) {}
    ~DocumentWrap() {}
};

class DocumentPrivate : public NodePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    DocumentPrivate(WebCore::Document*);
    ~DocumentPrivate();

    WebCore::Document* webcore() const { return m_webcore; }
    Document& wkc() { return m_wkc; }

    bool isImageDocument() const;
    bool isMediaDocument() const;
    bool isSVGDocument() const;
    void updateLayoutIgnorePendingStylesheets();
    void updateHoverActiveState(const HitTestRequest&, Element*);
    KURL completeURL(const String&) const;

    Element* focusedElement();
    RenderView* renderView();
    CachedResourceLoader* cachedResourceLoader();

    Frame* frame();
    Node* firstChild();

    HTMLCollection* forms();

    bool loadEventFinished() const;

    void webkitWillEnterFullScreenForElement(Element*);
    void webkitDidEnterFullScreenForElement(Element*);
    void webkitWillExitFullScreenForElement(Element*);
    void webkitDidExitFullScreenForElement(Element*);

private:
    WebCore::Document* m_webcore;
    DocumentWrap m_wkc;

    CachedResourceLoaderPrivate* m_cachedResourceLoader;
    ElementPrivate* m_focusedElement;
    RenderViewPrivate* m_renderView;
    FramePrivate* m_frame;
    NodePrivate* m_firstChild;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_DOCUMENT_H_

