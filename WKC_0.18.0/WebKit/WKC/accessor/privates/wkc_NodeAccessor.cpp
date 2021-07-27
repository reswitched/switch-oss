/*
 *  wkc_NodeAccessor.cpp
 *
 *  Copyright (c) 2012-2017 ACCESS CO., LTD. All rights reserved.
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

#include <wkc_NodeAccessor.h>

#undef ASSERT
#include <wkcplatform.h>
#include <wkcglobalwrapper.h>
#include <WebCore/config.h>
#include <WebCore/dom/Node.h>
#include <WebCore/dom/ContainerNode.h>
#include <WebCore/dom/QualifiedName.h>
#include <WebCore/dom/NamedNodeMap.h>
#include <WebCore/dom/Element.h>
#include <WebCore/dom/Document.h>
#include <WebCore/platform/graphics/FloatPoint.h>

#include <helpers/WKCNode.h>

#include "HTMLNames.h"


namespace WKC {

bool
EventTargetAccessor::hasEventListeners()
{
    return ptr()->hasEventListeners();
}

bool
EventTargetAccessor::hasEventListeners(const char* eventType)
{
    return ptr()->hasEventListeners( WTF::AtomicString( eventType ) );
}

bool
EventTargetAccessor::hasEventListeners(int id)
{
    switch (id ){
    case eventNames_clickEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().clickEvent);
    case eventNames_mousedownEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().mousedownEvent);
    case eventNames_mousemoveEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().mousemoveEvent);
    case eventNames_dragEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().dragEvent);
    case eventNames_dragstartEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().dragstartEvent);
    case eventNames_dragendEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().dragendEvent);
#if ENABLE(TOUCH_EVENTS)
    case eventNames_touchstartEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().touchstartEvent);
    case eventNames_touchmoveEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().touchmoveEvent);
    case eventNames_touchendEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().touchendEvent);
    case eventNames_touchcancelEvent:
        return ptr()->hasEventListeners(WebCore::eventNames().touchcancelEvent);
#endif
    }
    return false;
}


WebCore::ContainerNode*
NodeAccessor::parent()
{
//    return ptr()->parent();
    return ptr()->parentOrHostNode();
}

WebCore::Node*
NodeAccessor::firstChild() const
{
    return ptr()->firstChild();
}

WebCore::Node*
NodeAccessor::lastChild() const
{
    return ptr()->lastChild();
}


WebCore::ContainerNode*
NodeAccessor::parentNode() const
{
    return ptr()->parentNode();
}

WebCore::Node*
NodeAccessor::previousSibling() const
{
    return ptr()->previousSibling();
}

WebCore::Node*
NodeAccessor::nextSibling() const
{
    return ptr()->nextSibling();
}


WebCore::Node*
NodeAccessor::traverseNextNode(const WebCore::Node* stayWithin) const
{
    return ptr()->traverseNextNode(stayWithin);
}

WebCore::Node*
NodeAccessor::traverseNextSibling(const WebCore::Node* stayWithin) const
{
    return ptr()->traverseNextSibling(stayWithin);
}

WebCore::Node*
NodeAccessor::traversePreviousNode(const WebCore::Node* stayWithin) const
{
    return ptr()->traversePreviousNode(stayWithin);
}

WebCore::Node*
NodeAccessor::traverseNextNodePostOrder() const
{
    return ptr()->traverseNextNodePostOrder();
}

WebCore::Node*
NodeAccessor::traversePreviousNodePostOrder(const WebCore::Node* stayWithin) const
{
    return ptr()->traversePreviousNodePostOrder(stayWithin);
}

WebCore::Node*
NodeAccessor::traversePreviousSiblingPostOrder(const WebCore::Node* stayWithin) const
{
    return ptr()->traversePreviousSiblingPostOrder(stayWithin);
}

bool
NodeAccessor::hasChildNodes() const
{
    return ptr()->hasChildNodes();
}


unsigned
NodeAccessor::childNodeCount() const
{
    return ptr()->childNodeCount();
}

WebCore::Node*
NodeAccessor::childNode(unsigned index) const
{
    return ptr()->childNode(index);
}


bool
NodeAccessor::hasAttributes() const
{
    return ptr()->hasAttributes();
}

WebCore::NamedNodeMap*
NodeAccessor::attributes() const
{
    return ptr()->attributes();
}


bool
NodeAccessor::hasTagName(const char* tagName) const
{
    return ptr()->hasTagName( WebCore::QualifiedName( WTF::AtomicString(), tagName, WTF::AtomicString("http://www.w3.org/1999/xhtml") ) );
}

bool
NodeAccessor::hasTagName(int id) const
{
    switch (id) {
    case HTMLNames_inputTag:
        return ptr()->hasTagName(WebCore::HTMLNames::inputTag);
    case HTMLNames_buttonTag:
        return ptr()->hasTagName(WebCore::HTMLNames::buttonTag);
    case HTMLNames_textareaTag:
        return ptr()->hasTagName(WebCore::HTMLNames::textareaTag);
    case HTMLNames_selectTag:
        return ptr()->hasTagName(WebCore::HTMLNames::selectTag);
    case HTMLNames_formTag:
        return ptr()->hasTagName(WebCore::HTMLNames::formTag);
    case HTMLNames_frameTag:
        return ptr()->hasTagName(WebCore::HTMLNames::frameTag);
    case HTMLNames_iframeTag:
        return ptr()->hasTagName(WebCore::HTMLNames::iframeTag);
    case HTMLNames_videoTag:
        return ptr()->hasTagName(WebCore::HTMLNames::videoTag);
    case HTMLNames_areaTag:
        return ptr()->hasTagName(WebCore::HTMLNames::areaTag);
    case HTMLNames_aTag:
        return ptr()->hasTagName(WebCore::HTMLNames::aTag);
    default:
        return false;
    }
}


String
NodeAccessor::nodeName() const
{
    return String( ptr()->nodeName().characters() );
}

String
NodeAccessor::nodeValue() const
{
    return String( ptr()->nodeValue().characters() );
}


WebCore::Document*
NodeAccessor::ownerDocument() const
{
    return ptr()->ownerDocument();
}

WebCore::Document*
NodeAccessor::document() const
{
    return ptr()->document();
}


WebCore::RenderObject*
NodeAccessor::renderer() const
{
    return ptr()->renderer();
}

void
NodeAccessor::setRenderer(WebCore::RenderObject* renderer)
{
    return ptr()->setRenderer(renderer);
}

WKCRect
NodeAccessor::getRect() const
{
    WebCore::IntRect rect = ptr()->getRect();
    WKCRect result = { rect.x(), rect.y(), rect.width(), rect.height() };
    return result;
}

WKCRect
NodeAccessor::renderRect(bool* isReplaced)
{
    WebCore::IntRect rect = ptr()->renderRect(isReplaced);
    WKCRect result = { rect.x(), rect.y(), rect.width(), rect.height() };
    return result;
}


bool
NodeAccessor::disabled() const
{
    return ptr()->disabled();
}


bool
NodeAccessor::isElementNode() const
{
    return ptr()->isElementNode();
}

bool
NodeAccessor::isContainerNode() const
{
    return ptr()->isContainerNode();
}

bool
NodeAccessor::isTextNode() const
{
    return ptr()->isTextNode();
}

bool
NodeAccessor::isHTMLElement() const
{
    return ptr()->isHTMLElement();
}

bool
NodeAccessor::isSVGElement() const
{
    return ptr()->isSVGElement();
}

bool
NodeAccessor::isMediaControlElement() const
{
    return ptr()->isMediaControlElement();
}

bool
NodeAccessor::isMediaControls() const
{
    return ptr()->isMediaControls();
}

bool
NodeAccessor::isStyledElement() const
{
    return ptr()->isStyledElement();
}

bool
NodeAccessor::isFrameOwnerElement() const
{
    return ptr()->isFrameOwnerElement();
}

bool
NodeAccessor::isAttributeNode() const
{
    return ptr()->isAttributeNode();
}

bool
NodeAccessor::isCharacterDataNode() const
{
    return ptr()->isCharacterDataNode();
}

bool
NodeAccessor::isDocumentNode() const
{
    return ptr()->isDocumentNode();
}

bool
NodeAccessor::isShadowRoot() const
{
    return ptr()->isShadowRoot();
}

bool
NodeAccessor::isBlockFlow() const
{
    return ptr()->isBlockFlow();
}

bool
NodeAccessor::isBlockFlowOrBlockTable() const
{
    return ptr()->isBlockFlowOrBlockTable();
}

bool
NodeAccessor::isSameNode(WebCore::Node* other) const
{
    return ptr()->isSameNode(other);
}

bool
NodeAccessor::isEqualNode(WebCore::Node* node) const
{
    return ptr()->isEqualNode(node);
}

bool
NodeAccessor::isLink() const
{
    return ptr()->isLink();
}

bool
NodeAccessor::isFocusable() const
{
    return ptr()->isFocusable();
}

bool
NodeAccessor::isKeyboardFocusable(WebCore::KeyboardEvent* event) const
{
    return ptr()->isKeyboardFocusable(event);
}

bool
NodeAccessor::isMouseFocusable() const
{
    return ptr()->isMouseFocusable();
}

bool
NodeAccessor::isContentEditable()
{
    return ptr()->isContentEditable();
}

bool
NodeAccessor::inDocument() const
{
    return ptr()->inDocument();
}

bool
NodeAccessor::isReadOnlyNode() const
{
    return ptr()->isReadOnlyNode();
}


WKCFloatPoint
NodeAccessor::convertToPage(const WKCFloatPoint& point) const
{
    WebCore::FloatPoint wcPoint(point.fX, point.fY);
    WebCore::FloatPoint wcResult = ptr()->convertToPage(wcPoint);
    WKCFloatPoint result = { wcResult.x(), wcResult.y() };
    return result;
}

WKCFloatPoint
NodeAccessor::convertFromPage(const WKCFloatPoint& point) const
{
    WebCore::FloatPoint wcPoint(point.fX, point.fY);
    WebCore::FloatPoint wcResult = ptr()->convertFromPage(wcPoint);
    WKCFloatPoint result = { wcResult.x(), wcResult.y() };
    return result;
}


NodeAccessor::NodeType
NodeAccessor::nodeType() const
{
    return static_cast<NodeAccessor::NodeType>( ptr()->nodeType() );
}


void
NodeAccessor::attach()
{
    ptr()->attach();
}

void
NodeAccessor::detach()
{
    ptr()->detach();
}


WebCore::Node*
NodeAccessor::toNode()
{
    return ptr()->toNode();
}

WebCore::HTMLInputElement*
NodeAccessor::toInputElement()
{
    return ptr()->toInputElement();
}


bool
NodeAccessor::isDescendantOf(const WebCore::Node* node) const
{
    return ptr()->isDescendantOf(node);
}

bool
NodeAccessor::contains(const WebCore::Node* node) const
{
    return ptr()->contains(node);
}

bool
NodeAccessor::containsIncludingShadowDOM(WebCore::Node* node)
{
    return ptr()->containsIncludingShadowDOM(node);
}


WebCore::Node*
NodeAccessor::lastDescendant() const
{
    return ptr()->lastDescendant();
}

WebCore::Node*
NodeAccessor::firstDescendant() const
{
    return ptr()->firstDescendant();
}


WebCore::Node*
NodeAccessor::nextLeafNode() const
{
    return ptr()->nextLeafNode();
}

WebCore::Node*
NodeAccessor::previousLeafNode() const
{
    return ptr()->previousLeafNode();
}


bool
NodeAccessor::hasID() const
{
    return ptr()->hasID();
}

bool
NodeAccessor::hasClass() const
{
    return ptr()->hasClass();
}

bool
NodeAccessor::active() const
{
    return ptr()->active();
}

bool
NodeAccessor::inActiveChain() const
{
    return ptr()->inActiveChain();
}

bool
NodeAccessor::inDetach() const
{
    return ptr()->inDetach();
}

bool
NodeAccessor::hovered() const
{
    return ptr()->hovered();
}

bool
NodeAccessor::focused() const
{
    return ptr()->focused();
}

bool
NodeAccessor::attached() const
{
    return ptr()->attached();
}


short
NodeAccessor::tabIndex() const
{
    return ptr()->tabIndex();
}

bool
NodeAccessor::supportsFocus() const
{
    return ptr()->supportsFocus();
}

bool
NodeAccessor::rendererIsEditable() const
{
    return ptr()->rendererIsEditable();
}

bool
NodeAccessor::rendererIsRichlyEditable() const
{
    return ptr()->rendererIsRichlyEditable();
}

bool
NodeAccessor::shouldUseInputMethod()
{
    return ptr()->shouldUseInputMethod();
}

bool
NodeAccessor::hasNonEmptyBoundingBox() const
{
    return ptr()->hasNonEmptyBoundingBox();
}

unsigned
NodeAccessor::nodeIndex() const
{
    return ptr()->nodeIndex();
}



} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
