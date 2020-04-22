/*
 *  wkc_NodeAccessor.h
 *
 *  Copyright (c) 2012,2013 ACCESS CO., LTD. All rights reserved.
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
 */

#ifdef WKC_CUSTOMER_PATCH_0304674

#pragma once

#include <helpers/WKCString.h>

namespace WebCore { 

class Node; 
class ContainerNode;
class Element;
class HTMLInputElement;
class NamedNodeMap;
class RenderStyle;
class RenderArena;
class Document;
class TreeScope;
class Event;
class EventTarget;
class KeyboardEvent;
class RenderObject;

} // namespace WebCore


namespace WKC {

class EventTargetAccessor
{
public:
    
    EventTargetAccessor( WebCore::EventTarget* obj )
     : m_webcore( obj )
    {}
    
    WebCore::EventTarget*       ptr()       { return static_cast<WebCore::EventTarget*>( m_webcore ); }
    const WebCore::EventTarget* ptr() const { return static_cast<const WebCore::EventTarget*>( m_webcore ); }

    bool hasEventListeners();
    bool hasEventListeners(const char* eventType);
    bool hasEventListeners(int);  // eventNames_XXXEvent
    
protected:
    void* m_webcore;
};


class NodeAccessor : public EventTargetAccessor
{
public:
    enum NodeType 
    {
        ELEMENT_NODE = 1,
        ATTRIBUTE_NODE = 2,
        TEXT_NODE = 3,
        CDATA_SECTION_NODE = 4,
        ENTITY_REFERENCE_NODE = 5,
        ENTITY_NODE = 6,
        PROCESSING_INSTRUCTION_NODE = 7,
        COMMENT_NODE = 8,
        DOCUMENT_NODE = 9,
        DOCUMENT_TYPE_NODE = 10,
        DOCUMENT_FRAGMENT_NODE = 11,
        NOTATION_NODE = 12,
        XPATH_NAMESPACE_NODE = 13,
        SHADOW_ROOT_NODE = 14
    };

    NodeAccessor( WebCore::Node* obj )
     : EventTargetAccessor( reinterpret_cast<WebCore::EventTarget*>( obj ) )
    {}
    
    WebCore::Node*       ptr()       { return static_cast<WebCore::Node*>( m_webcore ); }
    const WebCore::Node* ptr() const { return static_cast<const WebCore::Node*>( m_webcore ); }

    WebCore::ContainerNode* parent();
    WebCore::Node* firstChild() const;
    WebCore::Node* lastChild() const;

    WebCore::ContainerNode* parentNode() const;
    WebCore::Node* previousSibling() const;
    WebCore::Node* nextSibling() const;

    WebCore::Node* traverseNextNode(const WebCore::Node* stayWithin = 0) const;
    WebCore::Node* traverseNextSibling(const WebCore::Node* stayWithin = 0) const;
    WebCore::Node* traversePreviousNode(const WebCore::Node* stayWithin = 0) const;
    WebCore::Node* traverseNextNodePostOrder() const;
    WebCore::Node* traversePreviousNodePostOrder(const WebCore::Node* stayWithin = 0) const;
    WebCore::Node* traversePreviousSiblingPostOrder(const WebCore::Node* stayWithin = 0) const;
    bool hasChildNodes() const;

    unsigned childNodeCount() const;
    WebCore::Node* childNode(unsigned index) const;

    bool hasAttributes() const;
    WebCore::NamedNodeMap* attributes() const;

    bool hasTagName(const char*) const;
    bool hasTagName(int) const; // HTMLNames_XXXTag
    String nodeName() const;
    String nodeValue() const;
    
    WebCore::Document* ownerDocument() const;
    WebCore::Document* document() const;
    
    WebCore::RenderObject* renderer() const;
    void setRenderer(WebCore::RenderObject* renderer);
    WKCRect getRect() const;
    WKCRect renderRect(bool* isReplaced);
    
    bool disabled() const;
    
    bool isElementNode() const;
    bool isContainerNode() const;
    bool isTextNode() const;
    bool isHTMLElement() const;
    bool isSVGElement() const;
    bool isMediaControlElement() const;
    bool isMediaControls() const;
    bool isStyledElement() const;
    bool isFrameOwnerElement() const;
    bool isAttributeNode() const;
    bool isCharacterDataNode() const;
    bool isDocumentNode() const;
    bool isShadowRoot() const;
    bool isBlockFlow() const;
    bool isBlockFlowOrBlockTable() const;
    bool isSameNode(WebCore::Node* other) const;
    bool isEqualNode(WebCore::Node*) const;
    bool isLink() const;
    bool isFocusable() const;
    bool isKeyboardFocusable(WebCore::KeyboardEvent*) const;
    bool isMouseFocusable() const;
    bool isContentEditable();
    bool inDocument() const;
    bool isReadOnlyNode() const;
    
    WKCFloatPoint convertToPage(const WKCFloatPoint&) const;
    WKCFloatPoint convertFromPage(const WKCFloatPoint&) const;

    NodeType nodeType() const;

    void attach();
    void detach();

    WebCore::Node* toNode();
    WebCore::HTMLInputElement* toInputElement();

    bool isDescendantOf(const WebCore::Node*) const;
    bool contains(const WebCore::Node*) const;
    bool containsIncludingShadowDOM(WebCore::Node*);

    WebCore::Node* lastDescendant() const;
    WebCore::Node* firstDescendant() const;

    WebCore::Node* nextLeafNode() const;
    WebCore::Node* previousLeafNode() const;
    
    bool hasID() const;
    bool hasClass() const;
    bool active() const;
    bool inActiveChain() const;
    bool inDetach() const;
    bool hovered() const;
    bool focused() const;
    bool attached() const;

    short tabIndex() const;
    bool supportsFocus() const;
    bool rendererIsEditable() const;
    bool rendererIsRichlyEditable() const;
    bool shouldUseInputMethod();
    bool hasNonEmptyBoundingBox() const;
    unsigned nodeIndex() const;
};





} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
