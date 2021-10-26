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

#include "helpers/WKCFrame.h"
#include "helpers/privates/WKCFramePrivate.h"

#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "Page.h"
#include "MainFrame.h"

#include "helpers/privates/WKCDocumentPrivate.h"
#include "helpers/privates/WKCFrameLoaderPrivate.h"
#include "helpers/privates/WKCFrameViewPrivate.h"
#include "helpers/privates/WKCFrameTreePrivate.h"
#include "helpers/privates/WKCHTMLFrameOwnerElementPrivate.h"
#include "helpers/privates/WKCPagePrivate.h"
#include "helpers/privates/WKCFrameSelectionPrivate.h"
#include "helpers/privates/WKCEditorPrivate.h"

namespace WKC {

FramePrivate::FramePrivate(WebCore::Frame* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_document(0)
    , m_frameLoader(0)
    , m_frameView(0)
    , m_tree(0)
    , m_page(0)
    , m_selection(0)
    , m_ownerElement(0)
    , m_editor(0)
    , m_name()
    , m_mainFrame(0)
{
}

FramePrivate::FramePrivate(const WebCore::Frame* parent)
    : m_webcore(const_cast<WebCore::Frame*>(parent))
    , m_wkc(*this)
    , m_document(0)
    , m_frameLoader(0)
    , m_frameView(0)
    , m_tree(0)
    , m_page(0)
    , m_selection(0)
    , m_ownerElement(0)
    , m_editor(0)
    , m_name()
    , m_mainFrame(0)
{
}

FramePrivate::~FramePrivate()
{
    delete m_frameLoader;
    delete m_document;
    delete m_frameView;
    delete m_page;
    delete m_selection;
    delete m_tree;
    delete m_ownerElement;
    delete m_editor;
    delete m_mainFrame;
}

WKC::Document*
FramePrivate::document()
{
    WebCore::Document* w = m_webcore->document();
    if (!w)
        return 0;
    if (!m_document || m_document->webcore()!=w) {
        delete m_document;
        m_document = new DocumentPrivate(w);
    }
    return &m_document->wkc();
}

WKC::Page*
FramePrivate::page()
{
    WebCore::Page* w = m_webcore->page();
    if (!w)
        return 0;
    if (!m_page || m_page->webcore()!=w) {
        delete m_page;
        m_page = new PagePrivate(w);
    }
    return &m_page->wkc();
}

WKC::FrameView*
FramePrivate::view()
{
    WebCore::FrameView* w = m_webcore->view();
    if (!w)
        return 0;
    if (!m_frameView || m_frameView->webcore()!=w) {
        delete m_frameView;
        m_frameView = new FrameViewPrivate(w);
    }
    return &m_frameView->wkc();
}


WKC::FrameSelection*
FramePrivate::selection()
{
    WebCore::FrameSelection* w = (WebCore::FrameSelection *)&m_webcore->selection();
    if (!w)
        return 0;
    if (!m_selection || m_selection->webcore()!=w) {
        delete m_selection;
        m_selection = new FrameSelectionPrivate(w);
    }
    return &m_selection->wkc();
}

WKC::FrameLoader*
FramePrivate::loader()
{
    WebCore::FrameLoader* w = (WebCore::FrameLoader *)&m_webcore->loader();
    if (!w)
        return 0;

    if (!m_frameLoader || m_frameLoader->webcore()!=w) {
        delete m_frameLoader;
        m_frameLoader = new FrameLoaderPrivate(w);
    }

    return &m_frameLoader->wkc();
}

FrameTree*
FramePrivate::tree()
{
    WebCore::FrameTree* t = (WebCore::FrameTree *)&m_webcore->tree();
    if (!t)
        return 0;

    if (!m_tree || m_tree->webcore()!=t) {
        delete m_tree;
        m_tree = new FrameTreePrivate(t);
    }

    return &m_tree->wkc();
}

WKC::Editor*
FramePrivate::editor()
{
    WebCore::Editor& e = m_webcore->editor();
    if (!m_editor || m_editor->webcore()!=&e) {
        delete m_editor;
        m_editor = new EditorPrivate(&e);
    }
    return &m_editor->wkc();
}

HTMLFrameOwnerElement*
FramePrivate::ownerElement()
{
    WebCore::HTMLFrameOwnerElement* e = m_webcore->ownerElement();
    if (!e)
        return 0;

    if (!m_ownerElement || m_ownerElement->webcore() != e) {
        delete m_ownerElement;
        m_ownerElement = new HTMLFrameOwnerElementPrivate(e);
    }

    return &m_ownerElement->wkc();
}

const unsigned short*
FramePrivate::name()
{
    m_name = m_webcore->tree().name().string();
    return m_name.characters();
}

bool
FramePrivate::isMainFrame() const
{
    return m_webcore->isMainFrame();
}

Frame*
FramePrivate::mainFrame()
{
    WebCore::Frame& mf = m_webcore->mainFrame();
    if (!m_mainFrame || m_mainFrame->webcore() != &mf) {
        delete m_mainFrame;
        m_mainFrame = new FramePrivate(&mf);
    }
    return &m_mainFrame->wkc();
    
}

////////////////////////////////////////////////////////////////////////////////

Frame*
Frame::create(Frame* parent, bool needsRef)
{
    void* p = WTF::fastMalloc(sizeof(Frame));
    return new (p) Frame(parent, needsRef);
}

void
Frame::destroy(Frame* instance)
{
   instance->~Frame();
   WTF::fastFree(instance);
}

Frame::Frame(FramePrivate& parent)
    : m_ownedPrivate(0)
    , m_private(parent)
    , m_needsRef(false)
{
}

Frame::Frame(Frame* parent, bool needsRef)
    : m_ownedPrivate(new FramePrivate(parent->priv().webcore()))
    , m_private(*m_ownedPrivate)    
    , m_needsRef(needsRef)
{
    if (needsRef)
        m_private.webcore()->ref();
}

Frame::~Frame()
{
    if (m_needsRef)
        m_private.webcore()->deref();
    if (m_ownedPrivate)
        delete m_ownedPrivate;
}

bool
Frame::compare(const Frame* other) const
{
    if (this==other)
        return true;
    if (!this || !other)
        return false;
    if (m_private.webcore() == other->m_private.webcore())
        return true;
    return false;
}

WKC::Document*
Frame::document() const
{
    return m_private.document();
}

WKC::Page*
Frame::page() const
{
    return m_private.page();
}

WKC::FrameView*
Frame::view() const
{
    return m_private.view();
}


WKC::FrameSelection*
Frame::selection() const
{
    return m_private.selection();
}

WKC::FrameLoader*
Frame::loader()
{
    return m_private.loader();
}


FrameTree*
Frame::tree() const
{
    return m_private.tree();
}


HTMLFrameOwnerElement*
Frame::ownerElement() const
{
    return m_private.ownerElement();
}

Editor*
Frame::editor() const
{
    return m_private.editor();
}

const unsigned short*
Frame::name()
{
    return m_private.name();
}

bool
Frame::isMainFrame() const
{
    return m_private.isMainFrame();
}

Frame*
Frame::mainFrame()
{
    return m_private.mainFrame();
}

} // namespace
