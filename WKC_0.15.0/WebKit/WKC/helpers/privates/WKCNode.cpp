/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCNode.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCNodeListPrivate.h"

#include "Node.h"
#include "HTMLNames.h"
#include "Element.h"
#include "HTMLInputElement.h"
#include "HTMLButtonElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLAreaElement.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "AtomicString.h"
#include "NodeList.h"

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#endif

#include "helpers/WKCString.h"

#include "helpers/privates/WKCDocumentPrivate.h"
#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCHTMLInputElementPrivate.h"
#include "helpers/privates/WKCHTMLTextAreaElementPrivate.h"
#include "helpers/privates/WKCHTMLAreaElementPrivate.h"
#include "helpers/privates/WKCHTMLFormElementPrivate.h"
#include "helpers/privates/WKCHTMLMediaElementPrivate.h"
#include "helpers/privates/WKCRenderObjectPrivate.h"
#include "helpers/privates/WKCNamedAttrMapPrivate.h"
#include "helpers/privates/WKCAtomicStringPrivate.h"

#if USE(ACCELERATED_COMPOSITING)
#include "helpers/privates/WKCGraphicsLayerPrivate.h"
namespace WebCore {
WKC::GraphicsLayerPrivate* GraphicsLayerWKC_wkc(const WebCore::GraphicsLayer* layer);
}
#endif

namespace WKC {

NodePrivate*
NodePrivate::create(WebCore::Node* parent)
{
    if (!parent)
        return 0;

    if (parent->isElementNode()) {
        return ElementPrivate::create(static_cast<WebCore::Element*>(parent));
    }
    return new NodePrivate(parent);
}

NodePrivate*
NodePrivate::create(const WebCore::Node* parent)
{
    return create(const_cast<WebCore::Node*>(parent));
}

NodePrivate*
NodePrivate::create(const NodePrivate& wparent)
{
    WebCore::Node* parent = wparent.webcore();
    return create(parent);
}

void 
NodePrivate::destroy()
{
    delete this;
}

NodePrivate::NodePrivate(WebCore::Node* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_document(0)
    , m_parentElement(0)
    , m_renderer(0)
    , m_parentNamedNodeMap(0)
    , m_parent(0)
    , m_parentNode(0)
    , m_firstChild(0)
    , m_traverseNextNode(0)
    , m_traverseNextSibling(0)
    , m_shadowHost(0)
    , m_HTMLElement(0)
    , m_parentOrShadowHostElement(0)
    , m_querySelectorElement(0)
{
}

NodePrivate::~NodePrivate()
{
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
    delete m_renderer;

    if (m_document)
        delete m_document;
    if (m_parentElement)
        delete m_parentElement;
    if (m_parent)
        delete m_parent;
    if (m_parentNode)
        delete m_parentNode;
    if (m_firstChild)
        delete m_firstChild;
    if (m_traverseNextNode)
        delete m_traverseNextNode;
    if (m_traverseNextSibling)
        delete m_traverseNextSibling;
    if (m_shadowHost)
        delete m_shadowHost;
    if (m_parentNamedNodeMap)
        delete m_parentNamedNodeMap;
    if (m_HTMLElement)
        delete m_HTMLElement;
    if (m_parentOrShadowHostElement)
        delete m_parentOrShadowHostElement;
    if (m_querySelectorElement)
        delete m_querySelectorElement;
}

WebCore::Node*
NodePrivate::webcore() const
{
    return m_webcore;
}

String
NodePrivate::nodeName() const
{
    return webcore()->nodeName();
}

bool
NodePrivate::hasTagName(int id) const
{
    switch (id) {
    case HTMLNames_inputTag:
        return webcore()->hasTagName(WebCore::HTMLNames::inputTag);
    case HTMLNames_buttonTag:
        return webcore()->hasTagName(WebCore::HTMLNames::buttonTag);
    case HTMLNames_textareaTag:
        return webcore()->hasTagName(WebCore::HTMLNames::textareaTag);
    case HTMLNames_selectTag:
        return webcore()->hasTagName(WebCore::HTMLNames::selectTag);
    case HTMLNames_formTag:
        return webcore()->hasTagName(WebCore::HTMLNames::formTag);
    case HTMLNames_frameTag:
        return webcore()->hasTagName(WebCore::HTMLNames::frameTag);
    case HTMLNames_iframeTag:
        return webcore()->hasTagName(WebCore::HTMLNames::iframeTag);
    case HTMLNames_videoTag:
        return m_webcore->hasTagName(WebCore::HTMLNames::videoTag);
    case HTMLNames_areaTag:
        return webcore()->hasTagName(WebCore::HTMLNames::areaTag);
    case HTMLNames_aTag:
        return webcore()->hasTagName(WebCore::HTMLNames::aTag);
    default:
        return false;
    }
}

bool
NodePrivate::isHTMLElement() const
{
    return webcore()->isHTMLElement();
}

bool
NodePrivate::isElementNode() const
{
    return webcore()->isElementNode();
}

bool
NodePrivate::isFrameOwnerElement() const
{
    return webcore()->isFrameOwnerElement();
}

bool
NodePrivate::inDocument() const
{
    return webcore()->inDocument();
}

bool
NodePrivate::hasEventListeners(int id)
{
    switch (id ){
    case eventNames_clickEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().clickEvent);
    case eventNames_mousedownEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().mousedownEvent);
    case eventNames_mousemoveEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().mousemoveEvent);
    case eventNames_dragEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().dragEvent);
    case eventNames_dragstartEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().dragstartEvent);
    case eventNames_dragendEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().dragendEvent);
#if ENABLE(TOUCH_EVENTS)
    case eventNames_touchstartEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().touchstartEvent);
    case eventNames_touchmoveEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().touchmoveEvent);
    case eventNames_touchendEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().touchendEvent);
    case eventNames_touchcancelEvent:
        return webcore()->hasEventListeners(WebCore::eventNames().touchcancelEvent);
#endif
    }
    return false;
}


Document*
NodePrivate::document()
{
    WebCore::Document* doc = &webcore()->document();
    if (!doc)
        return 0;
    if (!m_document || m_document->webcore()!=doc) {
        delete m_document;
        m_document = new DocumentPrivate(doc);
    }
    return &m_document->wkc();
}

RenderObject*
NodePrivate::renderer()
{
    WebCore::RenderObject* render = webcore()->renderer();
    if (!render)
        return 0;
    if (!m_renderer || m_renderer->webcore()!=render) {
        delete m_renderer;
        m_renderer = new RenderObjectPrivate(render);

    }
    return &m_renderer->wkc();
}

Element*
NodePrivate::parentElement()
{
    WebCore::Element* elem = webcore()->parentElement();
    if (!elem)
        return 0;
    if (!m_parentElement || m_parentElement->webcore()!=elem) {
        delete m_parentElement;
        m_parentElement = ElementPrivate::create(elem);
    }
    return &m_parentElement->wkc();
}

Element*
NodePrivate::shadowHost()
{
    WebCore::Element* elem = webcore()->shadowHost();
    if (!elem)
        return 0;
    if (!m_shadowHost || m_shadowHost->webcore() != elem) {
        delete m_shadowHost;
        m_shadowHost = ElementPrivate::create(elem);
    }
    return &m_shadowHost->wkc();
}

Node*
NodePrivate::parent()
{
    if (!webcore())
        return 0;

    WebCore::ContainerNode* n = webcore()->parentNode();
    if (!n)
        return 0;
    if (n==this->webcore())
        return &wkc();

    if (!m_parent || m_parent->webcore()!=n) {
        delete m_parent;
        m_parent = create(n);
    }
    return &m_parent->wkc();
}

Node*
NodePrivate::parentNode()
{
    if (!webcore())
        return 0;

    WebCore::Node* n = webcore()->parentNode();
    if (!n)
        return 0;
    if (n==this->webcore())
        return &wkc();

    if (!m_parentNode || m_parentNode->webcore()!=n) {
        delete m_parentNode;
        m_parentNode = create(n);
    }
    return &m_parentNode->wkc();
}

Node*
NodePrivate::firstChild()
{
    if (!webcore())
        return 0;

    WebCore::Node* n = webcore()->firstChild();
    if (!n)
        return 0;
    if (n==this->webcore())
        return &wkc();

    if (!m_firstChild || m_firstChild->webcore()!=n) {
        delete m_firstChild;
        m_firstChild = create(n);
    }
    return &m_firstChild->wkc();
}

bool
NodePrivate::isInShadowTree() const
{
    if (!m_webcore)
        return false;

    return m_webcore->isInShadowTree();
}

NodeList*
NodePrivate::getElementsByTagName(const String& localName)
{
    if (!m_webcore)
        return 0;

    PassRefPtr<WebCore::NodeList> list = ((WebCore::ContainerNode *)m_webcore)->getElementsByTagName(WTF::AtomicString(localName));
    if (!list)
        return 0;
    NodeListPrivate* nodeList = new NodeListPrivate(list);
    return &nodeList->wkc();
}

Element*
NodePrivate::querySelector(const String& selectors)
{
    if (!m_webcore)
        return 0;

    WebCore::ExceptionCode ec;
    WebCore::Element* elem = ((WebCore::ContainerNode *)m_webcore)->querySelector(selectors, ec);
    if (!elem)
        return 0;
    if (!m_querySelectorElement || m_querySelectorElement->webcore() != elem) {
        delete m_querySelectorElement;
        m_querySelectorElement = ElementPrivate::create(elem);
    }
    return &m_querySelectorElement->wkc();
}

NodeList*
NodePrivate::querySelectorAll(const String& selectors)
{
    if (!m_webcore)
        return 0;

    WebCore::ExceptionCode ec;
    PassRefPtr<WebCore::NodeList> list = ((WebCore::ContainerNode *)m_webcore)->querySelectorAll(selectors, ec);
    if (!list)
        return 0;
    NodeListPrivate* nodeList = new NodeListPrivate(list);
    return &nodeList->wkc();
}

NamedNodeMap*
NodePrivate::attributes()
{
    WebCore::NamedNodeMap* n;
    n = webcore()->attributes();

    if (!n)
        return 0;

    if (!m_parentNamedNodeMap){
        m_parentNamedNodeMap = new NamedNodeMapPrivate(n);
    } else if (m_parentNamedNodeMap->webcore() != n) {
        delete m_parentNamedNodeMap;
        m_parentNamedNodeMap = new NamedNodeMapPrivate(n);
    }

    return m_parentNamedNodeMap->wkc();
}

bool
NodePrivate::isContentEditable() const
{
    return webcore()->isContentEditable();
}

String
NodePrivate::textContent(bool conv) const
{
    return webcore()->textContent(conv);
}

void
NodePrivate::setTextContent(const String& text, int& ec)
{
    webcore()->setTextContent(text, ec);
}

HTMLElement*
NodePrivate::toHTMLElement()
{
    if( !(this->isHTMLElement()) )
        return 0;

    WebCore::HTMLElement* elem = (WebCore::HTMLElement *)(webcore());
    if (!elem)
        return 0;
    if (!m_HTMLElement || m_HTMLElement->webcore()!=elem) {
        delete m_HTMLElement;
        m_HTMLElement = HTMLElementPrivate::create(elem);
    }
    return &m_HTMLElement->wkc();
}

void
NodePrivate::dispatchChangeEvent()
{
    if (is<WebCore::HTMLFormControlElement>(*webcore())) {
        WebCore::HTMLFormControlElement* el = downcast<WebCore::HTMLFormControlElement>(webcore());
        el->dispatchChangeEvent();
    }
}

const GraphicsLayer*
NodePrivate::enclosingGraphicsLayer() const
{
#if USE(ACCELERATED_COMPOSITING)
    WebCore::RenderObject* renderer = webcore()->renderer();
    if (!renderer)
        return 0;
    WebCore::RenderLayer* renderlayer = renderer->enclosingLayer();
    if (!renderlayer)
        return 0;
    WebCore::RenderLayer* compositedlayer = renderlayer->enclosingCompositingLayer();
    if (!compositedlayer)
        return 0;
    WebCore::GraphicsLayer* graphicslayer = compositedlayer->backing()->graphicsLayer();
    if (!graphicslayer)
        return 0;
    GraphicsLayerPrivate* wkc = GraphicsLayerWKC_wkc(graphicslayer);
    return &wkc->wkc();
#else
    return 0;
#endif
}

bool
NodePrivate::isScrollableOverFlowBlockNode() const
{
    return m_webcore->isScrollableOverFlowBlockNode();
}

void
NodePrivate::getNodeCompositeRect(WKCRect* rect, int tx, int ty)
{
    WebCore::RenderObject* renderer = m_webcore->renderer();
    if (!renderer)
        return;

    WebCore::RenderStyle& style = renderer->style();
    bool isOverflowXHidden = (style.overflowX() == WebCore::OHIDDEN);
    bool isOverflowYHidden = (style.overflowY() == WebCore::OHIDDEN);

    WebCore::LayoutRect core_rect = WebCore::LayoutRect(rect->fX, rect->fY, rect->fWidth, rect->fHeight);
    m_webcore->getNodeCompositeRect(&core_rect, isOverflowXHidden, isOverflowYHidden, tx, ty);
    rect->fX = core_rect.x();
    rect->fY = core_rect.y();
    rect->fWidth = core_rect.width();
    rect->fHeight = core_rect.height();
}

Element*
NodePrivate::parentOrShadowHostElement()
{
    WebCore::Element* elem = webcore()->parentOrShadowHostElement();
    if (!elem)
        return 0;
    if (!m_parentOrShadowHostElement || m_parentOrShadowHostElement->webcore() != elem) {
        delete m_parentOrShadowHostElement;
        m_parentOrShadowHostElement = ElementPrivate::create(elem);
    }
    return &m_parentOrShadowHostElement->wkc();
}

void
NodePrivate::showNode(const char* prefix) const
{
#if ENABLE(TREE_DEBUGGING)
    webcore()->showNode(prefix);
#endif
}

void
NodePrivate::showTreeForThis() const
{
#if ENABLE(TREE_DEBUGGING)
    webcore()->showTreeForThis();
#endif
}

void
NodePrivate::showNodePathForThis() const
{
#if ENABLE(TREE_DEBUGGING)
    webcore()->showNodePathForThis();
#endif
}

void
NodePrivate::showTreeForThisAcrossFrame() const
{
#if ENABLE(TREE_DEBUGGING)
    webcore()->showTreeForThisAcrossFrame();
#endif
}

////////////////////////////////////////////////////////////////////////////////

Node*
Node::create(Node* parent, bool needsRef)
{
    void* p = WTF::fastMalloc(sizeof(Node));
    return new (p) Node(parent, needsRef);
}

void
Node::destroy(Node* instance)
{
    if (!instance)
        return;
    instance->~Node();
    WTF::fastFree(instance);
}

Node::Node(NodePrivate& parent)
    : m_ownedPrivate(0)
    , m_private(parent)
    , m_needsRef(false)
{
}

Node::Node(Node* parent, bool needsRef)
    : m_ownedPrivate(NodePrivate::create(parent->priv().webcore()))
    , m_private(*m_ownedPrivate)
    , m_needsRef(needsRef)
{
    if (needsRef)
        m_private.webcore()->ref();
}

Node::~Node()
{
    if (m_needsRef)
        m_private.webcore()->deref();
    if (m_ownedPrivate)
        delete m_ownedPrivate;
}

bool
Node::compare(const Node* other) const
{
    if (this==other)
        return true;
    if (!this || !other)
        return false;
    if (priv().webcore() == other->priv().webcore())
        return true;
    return false;
}

bool
Node::hasTagName(int id) const
{
    return priv().hasTagName(id);
}

bool
Node::isHTMLElement() const
{
    return priv().isHTMLElement();
}

bool
Node::isElementNode() const
{
    return priv().isElementNode();
}

bool
Node::isFrameOwnerElement() const
{
    return priv().isFrameOwnerElement();
}

bool
Node::inDocument() const
{
    return priv().inDocument();
}

String
Node::nodeName() const
{
    return priv().nodeName();
}

bool
Node::hasEventListeners(int id)
{
    return priv().hasEventListeners(id);
}


Document*
Node::document() const
{
    return priv().document();
}

RenderObject*
Node::renderer() const
{
    return priv().renderer();
}

Element*
Node::parentElement() const
{
    return priv().parentElement();
}

Element*
Node::shadowHost() const
{
    return priv().shadowHost();
}


Node*
Node::parent() const
{
    return priv().parent();
}

Node*
Node::parentNode() const
{
    return priv().parentNode();
}

Node*
Node::firstChild() const
{
    return priv().firstChild();
}

NamedNodeMap*
Node::attributes() const
{
    return priv().attributes();
}

bool
Node::isContentEditable() const
{
    return priv().isContentEditable();
}

String
Node::textContent(bool conv) const
{
    return priv().textContent(conv);
}

void
Node::setTextContent(const String& text, int& ec)
{
    priv().setTextContent(text, ec);
}

HTMLElement*
Node::toHTMLElement() const
{
    return priv().toHTMLElement();
}

void
Node::dispatchChangeEvent()
{
    priv().dispatchChangeEvent();
}

const GraphicsLayer*
Node::enclosingGraphicsLayer() const
{
    return priv().enclosingGraphicsLayer();
}


bool
Node::isScrollableOverFlowBlockNode() const
{
    return m_private.isScrollableOverFlowBlockNode();
}

void
Node::getNodeCompositeRect(WKCRect* rect, int tx, int ty)
{
    m_private.getNodeCompositeRect(rect, tx, ty);
}

bool
Node::isInShadowTree() const
{
    return m_private.isInShadowTree();
}

NodeList*
Node::getElementsByTagName(const String& localName)
{
    return m_private.getElementsByTagName(localName);
}

Element*
Node::querySelector(const String& selectors)
{
    return m_private.querySelector(selectors);
}

NodeList*
Node::querySelectorAll(const String& selectors)
{
    return m_private.querySelectorAll(selectors);
}

Element*
Node::parentOrShadowHostElement()
{
    return priv().parentOrShadowHostElement();
}

void
Node::showNode(const char* prefix) const
{
    priv().showNode(prefix);
}

void
Node::showTreeForThis() const
{
    priv().showTreeForThis();
}

void
Node::showNodePathForThis() const
{
    priv().showNodePathForThis();
}

void
Node::showTreeForThisAcrossFrame() const
{
    priv().showTreeForThisAcrossFrame();
}

} // namespace
