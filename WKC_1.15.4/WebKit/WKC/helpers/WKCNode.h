/*
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_NODE_H_
#define _WKC_HELPERS_WKC_NODE_H_

#include <wkc/wkcbase.h>

namespace WKC {
class Document;
class Element;
class RenderObject;
class String;
class NodeList;

class NodePrivate;
class NamedNodeMap;

class HTMLElement;

class GraphicsLayer;

enum {
    HTMLNames_inputTag,
    HTMLNames_buttonTag,
    HTMLNames_textareaTag,
    HTMLNames_selectTag,
    HTMLNames_formTag,
    HTMLNames_frameTag,
    HTMLNames_iframeTag,
    HTMLNames_objectTag,
    HTMLNames_videoTag,
    HTMLNames_areaTag,
    HTMLNames_aTag,
};

enum {
    eventNames_clickEvent,
    eventNames_mousedownEvent,
    eventNames_mousemoveEvent,
    eventNames_dragEvent,
    eventNames_dragstartEvent,
    eventNames_dragendEvent,
    eventNames_touchstartEvent,
    eventNames_touchmoveEvent,
    eventNames_touchendEvent,
    eventNames_touchcancelEvent,
};

class WKC_API Node {
public:
    static Node* create(Node*, bool needsRef = false);
    static void destroy(Node*);

    NodePrivate& priv() const { return m_private; }

    bool compare(const Node*) const;

    bool hasTagName(int) const;
    bool isHTMLElement() const;
    bool isDocumentNode() const;
    bool isElementNode() const;
    bool isFrameOwnerElement() const;
    bool isConnected() const;
    String nodeName() const;

    bool hasEventListeners(int);

    Document* document() const;
    RenderObject* renderer() const;
    Element* parentElement() const;
    Element* shadowHost() const;

    Node* parent() const;
    Node* parentNode() const;
    Node* firstChild() const;

    bool isInShadowTree() const;

    NamedNodeMap* attributes() const;

    bool isContentEditable() const;
    String textContent(bool conv) const;
    void setTextContent(const String& text, int& ec);
    HTMLElement* toHTMLElement() const;
    void dispatchChangeEvent();

    const GraphicsLayer* enclosingGraphicsLayer() const;

    bool isScrollableOverFlowBlockNode() const;
    WKCRect absoluteClippedCompositeTransformedRect() const;

    // NOTE: NodeList needs to be freed by calling release() on the application side.
    NodeList* getElementsByTagName(const String&);
    Element* querySelector(const String& selectors);
    NodeList* querySelectorAll(const String& selectors);

    Element* parentOrShadowHostElement();

    void showNode(const char* prefix = "") const;
    void showTreeForThis() const;
    void showNodePathForThis() const;
    void showTreeForThisAcrossFrame() const;

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    // Use create()/destroy() instead.
    Node(NodePrivate&);
    Node(Node*, bool needsRef);
    virtual ~Node();

private:
    Node(const Node&);
    Node& operator=(const Node&);

    NodePrivate* m_ownedPrivate;
    NodePrivate& m_private;
    bool m_needsRef;
};
}

#endif // _WKC_HELPERS_WKC_NODE_H_
