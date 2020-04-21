/*
 *  wkc_ConstNodeAccessor.cpp
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

#include <wkc_ConstNodeAccessor.h>

#undef ASSERT
#include <wkcplatform.h>
#include <wkcglobalwrapper.h>
#include <WebCore/config.h>
#include <WebCore/dom/Node.h>
#include <WebCore/dom/QualifiedName.h>
#include <WebCore/dom/NamedNodeMap.h>
#include <WebCore/dom/Element.h>
#include <WebCore/dom/Document.h>
#include <WebCore/platform/graphics/FloatPoint.h>

#include <helpers/WKCNode.h>

#include "HTMLNames.h"


namespace WKC {


WebCore::ContainerNode*
ConstNodeAccessor::parent() const
{
//    return ptr()->parent();
    return ptr()->parentOrHostNode();
}

WebCore::Node*
ConstNodeAccessor::firstChild() const
{
    return ptr()->firstChild();
}

WebCore::Node*
ConstNodeAccessor::lastChild() const
{
    return ptr()->lastChild();
}


WebCore::ContainerNode*
ConstNodeAccessor::parentNode() const
{
    return ptr()->parentNode();
}

WebCore::Node*
ConstNodeAccessor::previousSibling() const
{
    return ptr()->previousSibling();
}

WebCore::Node*
ConstNodeAccessor::nextSibling() const
{
    return ptr()->nextSibling();
}


WebCore::Node*
ConstNodeAccessor::traverseNextNode(const WebCore::Node* stayWithin) const
{
    return ptr()->traverseNextNode(stayWithin);
}

WebCore::Node*
ConstNodeAccessor::traverseNextSibling(const WebCore::Node* stayWithin) const
{
    return ptr()->traverseNextSibling(stayWithin);
}

WebCore::Node*
ConstNodeAccessor::traversePreviousNode(const WebCore::Node* stayWithin) const
{
    return ptr()->traversePreviousNode(stayWithin);
}

WebCore::Node*
ConstNodeAccessor::traverseNextNodePostOrder() const
{
    return ptr()->traverseNextNodePostOrder();
}

WebCore::Node*
ConstNodeAccessor::traversePreviousNodePostOrder(const WebCore::Node* stayWithin) const
{
    return ptr()->traversePreviousNodePostOrder(stayWithin);
}

WebCore::Node*
ConstNodeAccessor::traversePreviousSiblingPostOrder(const WebCore::Node* stayWithin) const
{
    return ptr()->traversePreviousSiblingPostOrder(stayWithin);
}

bool
ConstNodeAccessor::hasChildNodes() const
{
    return ptr()->hasChildNodes();
}


unsigned
ConstNodeAccessor::childNodeCount() const
{
    return ptr()->childNodeCount();
}

WebCore::Node*
ConstNodeAccessor::childNode(unsigned index) const
{
    return ptr()->childNode(index);
}


bool
ConstNodeAccessor::hasAttributes() const
{
    return ptr()->hasAttributes();
}

WebCore::NamedNodeMap*
ConstNodeAccessor::attributes() const
{
    return ptr()->attributes();
}


bool
ConstNodeAccessor::hasTagName(const char* tagName) const
{
    return ptr()->hasTagName( WebCore::QualifiedName( WTF::AtomicString(), tagName, WTF::AtomicString("http://www.w3.org/1999/xhtml") ) );
}

bool
ConstNodeAccessor::hasTagName(int id) const
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
ConstNodeAccessor::nodeName() const
{
    return String( ptr()->nodeName().characters() );
}

String
ConstNodeAccessor::nodeValue() const
{
    return String( ptr()->nodeValue().characters() );
}


WebCore::Document*
ConstNodeAccessor::ownerDocument() const
{
    return ptr()->ownerDocument();
}

WebCore::Document*
ConstNodeAccessor::document() const
{
    return ptr()->document();
}


WebCore::RenderObject*
ConstNodeAccessor::renderer() const
{
    return ptr()->renderer();
}


WKCRect
ConstNodeAccessor::getRect() const
{
    WebCore::IntRect rect = ptr()->getRect();
    WKCRect result = { rect.x(), rect.y(), rect.width(), rect.height() };
    return result;
}


bool
ConstNodeAccessor::disabled() const
{
    return ptr()->disabled();
}


bool
ConstNodeAccessor::isElementNode() const
{
    return ptr()->isElementNode();
}

bool
ConstNodeAccessor::isContainerNode() const
{
    return ptr()->isContainerNode();
}

bool
ConstNodeAccessor::isTextNode() const
{
    return ptr()->isTextNode();
}

bool
ConstNodeAccessor::isHTMLElement() const
{
    return ptr()->isHTMLElement();
}

bool
ConstNodeAccessor::isSVGElement() const
{
    return ptr()->isSVGElement();
}

bool
ConstNodeAccessor::isMediaControlElement() const
{
    return ptr()->isMediaControlElement();
}

bool
ConstNodeAccessor::isMediaControls() const
{
    return ptr()->isMediaControls();
}

bool
ConstNodeAccessor::isStyledElement() const
{
    return ptr()->isStyledElement();
}

bool
ConstNodeAccessor::isFrameOwnerElement() const
{
    return ptr()->isFrameOwnerElement();
}

bool
ConstNodeAccessor::isAttributeNode() const
{
    return ptr()->isAttributeNode();
}

bool
ConstNodeAccessor::isCharacterDataNode() const
{
    return ptr()->isCharacterDataNode();
}

bool
ConstNodeAccessor::isDocumentNode() const
{
    return ptr()->isDocumentNode();
}

bool
ConstNodeAccessor::isShadowRoot() const
{
    return ptr()->isShadowRoot();
}

bool
ConstNodeAccessor::isBlockFlow() const
{
    return ptr()->isBlockFlow();
}

bool
ConstNodeAccessor::isBlockFlowOrBlockTable() const
{
    return ptr()->isBlockFlowOrBlockTable();
}

bool
ConstNodeAccessor::isSameNode(WebCore::Node* other) const
{
    return ptr()->isSameNode(other);
}

bool
ConstNodeAccessor::isEqualNode(WebCore::Node* node) const
{
    return ptr()->isEqualNode(node);
}

bool
ConstNodeAccessor::isLink() const
{
    return ptr()->isLink();
}

bool
ConstNodeAccessor::isFocusable() const
{
    return ptr()->isFocusable();
}

bool
ConstNodeAccessor::isKeyboardFocusable(WebCore::KeyboardEvent* event) const
{
    return ptr()->isKeyboardFocusable(event);
}

bool
ConstNodeAccessor::isMouseFocusable() const
{
    return ptr()->isMouseFocusable();
}

bool
ConstNodeAccessor::inDocument() const
{
    return ptr()->inDocument();
}

bool
ConstNodeAccessor::isReadOnlyNode() const
{
    return ptr()->isReadOnlyNode();
}


WKCFloatPoint
ConstNodeAccessor::convertToPage(const WKCFloatPoint& point) const
{
    WebCore::FloatPoint wcPoint(point.fX, point.fY);
    WebCore::FloatPoint wcResult = ptr()->convertToPage(wcPoint);
    WKCFloatPoint result = { wcResult.x(), wcResult.y() };
    return result;
}

WKCFloatPoint
ConstNodeAccessor::convertFromPage(const WKCFloatPoint& point) const
{
    WebCore::FloatPoint wcPoint(point.fX, point.fY);
    WebCore::FloatPoint wcResult = ptr()->convertFromPage(wcPoint);
    WKCFloatPoint result = { wcResult.x(), wcResult.y() };
    return result;
}


ConstNodeAccessor::NodeType
ConstNodeAccessor::nodeType() const
{
    return static_cast<ConstNodeAccessor::NodeType>( ptr()->nodeType() );
}


bool
ConstNodeAccessor::isDescendantOf(const WebCore::Node* node) const
{
    return ptr()->isDescendantOf(node);
}

bool
ConstNodeAccessor::contains(const WebCore::Node* node) const
{
    return ptr()->contains(node);
}


WebCore::Node*
ConstNodeAccessor::lastDescendant() const
{
    return ptr()->lastDescendant();
}

WebCore::Node*
ConstNodeAccessor::firstDescendant() const
{
    return ptr()->firstDescendant();
}


WebCore::Node*
ConstNodeAccessor::nextLeafNode() const
{
    return ptr()->nextLeafNode();
}

WebCore::Node*
ConstNodeAccessor::previousLeafNode() const
{
    return ptr()->previousLeafNode();
}


bool
ConstNodeAccessor::hasID() const
{
    return ptr()->hasID();
}

bool
ConstNodeAccessor::hasClass() const
{
    return ptr()->hasClass();
}

bool
ConstNodeAccessor::active() const
{
    return ptr()->active();
}

bool
ConstNodeAccessor::inActiveChain() const
{
    return ptr()->inActiveChain();
}

bool
ConstNodeAccessor::inDetach() const
{
    return ptr()->inDetach();
}

bool
ConstNodeAccessor::hovered() const
{
    return ptr()->hovered();
}

bool
ConstNodeAccessor::focused() const
{
    return ptr()->focused();
}

bool
ConstNodeAccessor::attached() const
{
    return ptr()->attached();
}


short
ConstNodeAccessor::tabIndex() const
{
    return ptr()->tabIndex();
}

bool
ConstNodeAccessor::supportsFocus() const
{
    return ptr()->supportsFocus();
}

bool
ConstNodeAccessor::rendererIsEditable() const
{
    return ptr()->rendererIsEditable();
}

bool
ConstNodeAccessor::rendererIsRichlyEditable() const
{
    return ptr()->rendererIsRichlyEditable();
}

bool
ConstNodeAccessor::hasNonEmptyBoundingBox() const
{
    return ptr()->hasNonEmptyBoundingBox();
}

unsigned
ConstNodeAccessor::nodeIndex() const
{
    return ptr()->nodeIndex();
}



} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
