/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Andrew Wellington (proton@wiretapped.net)
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
 *
 */

#include "config.h"
#include "RenderListItem.h"

#include "ElementTraversal.h"
#include "HTMLNames.h"
#include "HTMLOListElement.h"
#include "HTMLUListElement.h"
#include "InlineElementBox.h"
#include "PseudoElement.h"
#include "RenderInline.h"
#include "RenderListMarker.h"
#include "RenderMultiColumnFlowThread.h"
#include "RenderTable.h"
#include "RenderView.h"
#include "StyleInheritedData.h"
#include <wtf/StackStats.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

RenderListItem::RenderListItem(Element& element, Ref<RenderStyle>&& style)
    : RenderBlockFlow(element, WTF::move(style))
    , m_marker(nullptr)
    , m_hasExplicitValue(false)
    , m_isValueUpToDate(false)
    , m_notInList(false)
#if PLATFORM(WKC)
    , m_weakFactory(this)
#endif
{
    setInline(false);
}

RenderListItem::~RenderListItem()
{
    ASSERT(!m_marker || !m_marker->parent());
    if (m_marker) {
        m_marker->destroy();
        ASSERT(!m_marker);
    }
}

void RenderListItem::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlockFlow::styleDidChange(diff, oldStyle);

    if (style().listStyleType() == NoneListStyle && (!style().listStyleImage() || style().listStyleImage()->errorOccurred())) {
        if (m_marker) {
            m_marker->destroy();
            ASSERT(!m_marker);
        }
        return;
    }

    auto newStyle = RenderStyle::create();
    // The marker always inherits from the list item, regardless of where it might end
    // up (e.g., in some deeply nested line box). See CSS3 spec.
    newStyle.get().inheritFrom(&style());
    if (!m_marker) {
        m_marker = createRenderer<RenderListMarker>(*this, WTF::move(newStyle)).leakPtr();
        m_marker->initializeStyle();
    } else {
        // Do not alter our marker's style unless our style has actually changed.
        if (diff != StyleDifferenceEqual)
            m_marker->setStyle(WTF::move(newStyle));
    }
}

void RenderListItem::insertedIntoTree()
{
    RenderBlockFlow::insertedIntoTree();

    updateListMarkerNumbers();
}

void RenderListItem::willBeRemovedFromTree()
{
    RenderBlockFlow::willBeRemovedFromTree();

    updateListMarkerNumbers();
}

static inline bool isHTMLListElement(const Node& node)
{
    return is<HTMLUListElement>(node) || is<HTMLOListElement>(node);
}

// Returns the enclosing list with respect to the DOM order.
static Element* enclosingList(const RenderListItem& listItem)
{
    Element& listItemElement = listItem.element();
    Element* parent = is<PseudoElement>(listItemElement) ? downcast<PseudoElement>(listItemElement).hostElement() : listItemElement.parentElement();
    Element* firstNode = parent;
    // We use parentNode because the enclosing list could be a ShadowRoot that's not Element.
    for (; parent; parent = parent->parentElement()) {
        if (isHTMLListElement(*parent))
            return parent;
    }

    // If there's no actual <ul> or <ol> list element, then the first found
    // node acts as our list for purposes of determining what other list items
    // should be numbered as part of the same list.
    return firstNode;
}

// Returns the next list item with respect to the DOM order.
static RenderListItem* nextListItem(const Element& listNode, const Element& element)
{
    for (const Element* next = ElementTraversal::nextIncludingPseudo(element, &listNode); next; ) {
        auto* renderer = next->renderer();
        if (!renderer || isHTMLListElement(*next)) {
            // We've found a nested, independent list or an unrendered Element : nothing to do here.
            next = ElementTraversal::nextIncludingPseudoSkippingChildren(*next, &listNode);
            continue;
        }

        if (is<RenderListItem>(*renderer))
            return downcast<RenderListItem>(renderer);

        next = ElementTraversal::nextIncludingPseudo(*next, &listNode);
    }

    return nullptr;
}

static inline RenderListItem* nextListItem(const Element& listNode, const RenderListItem& item)
{
    return nextListItem(listNode, item.element());
}

static inline RenderListItem* nextListItem(const Element& listNode)
{
    return nextListItem(listNode, listNode);
}

// Returns the previous list item with respect to the DOM order.
static RenderListItem* previousListItem(const Element* listNode, const RenderListItem& item)
{
    for (const Element* current = ElementTraversal::previousIncludingPseudo(item.element(), listNode); current; current = ElementTraversal::previousIncludingPseudo(*current, listNode)) {
        RenderElement* renderer = current->renderer();
        if (!is<RenderListItem>(renderer))
            continue;
        Element* otherList = enclosingList(downcast<RenderListItem>(*renderer));
        // This item is part of our current list, so it's what we're looking for.
        if (listNode == otherList)
            return downcast<RenderListItem>(renderer);
        // We found ourself inside another list; lets skip the rest of it.
        // Use nextIncludingPseudo() here because the other list itself may actually
        // be a list item itself. We need to examine it, so we do this to counteract
        // the previousIncludingPseudo() that will be done by the loop.
        if (otherList)
            current = ElementTraversal::nextIncludingPseudo(*otherList);
    }
    return nullptr;
}

void RenderListItem::updateItemValuesForOrderedList(const HTMLOListElement& listNode)
{
    for (RenderListItem* listItem = nextListItem(listNode); listItem; listItem = nextListItem(listNode, *listItem))
        listItem->updateValue();
}

unsigned RenderListItem::itemCountForOrderedList(const HTMLOListElement& listNode)
{
    unsigned itemCount = 0;
    for (RenderListItem* listItem = nextListItem(listNode); listItem; listItem = nextListItem(listNode, *listItem))
        ++itemCount;
    return itemCount;
}

inline int RenderListItem::calcValue() const
{
    if (m_hasExplicitValue)
        return m_explicitValue;

    Element* list = enclosingList(*this);
    HTMLOListElement* oListElement = is<HTMLOListElement>(list) ? downcast<HTMLOListElement>(list) : nullptr;
    int valueStep = 1;
    if (oListElement && oListElement->isReversed())
        valueStep = -1;

    // FIXME: This recurses to a possible depth of the length of the list.
    // That's not good -- we need to change this to an iterative algorithm.
    if (RenderListItem* previousItem = previousListItem(list, *this))
        return previousItem->value() + valueStep;

    if (oListElement)
        return oListElement->start();

    return 1;
}

void RenderListItem::updateValueNow() const
{
    m_value = calcValue();
    m_isValueUpToDate = true;
}

bool RenderListItem::isEmpty() const
{
    return lastChild() == m_marker;
}

static RenderBlock* getParentOfFirstLineBox(RenderBlock& current, RenderObject& marker)
{
    bool inQuirksMode = current.document().inQuirksMode();
    for (RenderObject* child = current.firstChild(); child; child = child->nextSibling()) {
        if (child == &marker)
            continue;

        if (child->isInline() && (!is<RenderInline>(*child) || current.generatesLineBoxesForInlineChild(child)))
            return &current;

        if (child->isFloating() || child->isOutOfFlowPositioned())
            continue;

        if (is<RenderTable>(*child) || !is<RenderBlock>(*child) || (is<RenderBox>(*child) && downcast<RenderBox>(*child).isWritingModeRoot()))
            break;

        if (is<RenderListItem>(current) && inQuirksMode && child->node() && isHTMLListElement(*child->node()))
            break;

        if (RenderBlock* lineBox = getParentOfFirstLineBox(downcast<RenderBlock>(*child), marker))
            return lineBox;
    }

    return nullptr;
}

void RenderListItem::updateValue()
{
    if (!m_hasExplicitValue) {
        m_isValueUpToDate = false;
        if (m_marker)
            m_marker->setNeedsLayoutAndPrefWidthsRecalc();
    }
}

static RenderObject* firstNonMarkerChild(RenderBlock& parent)
{
    RenderObject* child = parent.firstChild();
    while (is<RenderListMarker>(child))
        child = child->nextSibling();
    return child;
}

void RenderListItem::insertOrMoveMarkerRendererIfNeeded()
{
    // Sanity check the location of our marker.
    if (!m_marker)
        return;

    // FIXME: Do not even try to reposition the marker when we are not in layout
    // until after we fixed webkit.org/b/163789.
    if (!view().frameView().isInRenderTreeLayout())
        return;

    RenderElement* currentParent = m_marker->parent();
    RenderBlock* newParent = getParentOfFirstLineBox(*this, *m_marker);
    if (!newParent) {
        // If the marker is currently contained inside an anonymous box,
        // then we are the only item in that anonymous box (since no line box
        // parent was found). It's ok to just leave the marker where it is
        // in this case.
        if (currentParent && currentParent->isAnonymousBlock())
            return;
        if (multiColumnFlowThread())
            newParent = multiColumnFlowThread();
        else
            newParent = this;
    }

    if (newParent != currentParent) {
        // Removing and adding the marker can trigger repainting in
        // containers other than ourselves, so we need to disable LayoutState.
        LayoutStateDisabler layoutStateDisabler(&view());
        m_marker->removeFromParent();
        newParent->addChild(m_marker, firstNonMarkerChild(*newParent));
        m_marker->updateMarginsAndContent();
        // If current parent is an anonymous block that has lost all its children, destroy it.
        if (currentParent && currentParent->isAnonymousBlock() && !currentParent->firstChild() && !downcast<RenderBlock>(*currentParent).continuation())
            currentParent->destroy();
    }

}

void RenderListItem::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout()); 

    insertOrMoveMarkerRendererIfNeeded();
    RenderBlockFlow::layout();
}

void RenderListItem::addOverflowFromChildren()
{
    positionListMarker();
    RenderBlockFlow::addOverflowFromChildren();
}

void RenderListItem::computePreferredLogicalWidths()
{
#ifndef NDEBUG
    // FIXME: We shouldn't be modifying the tree in computePreferredLogicalWidths.
    // Instead, we should insert the marker soon after the tree construction.
    // This is similar case to RenderCounter::computePreferredLogicalWidths()
    // See https://bugs.webkit.org/show_bug.cgi?id=104829
    SetLayoutNeededForbiddenScope layoutForbiddenScope(this, false);
#endif
    insertOrMoveMarkerRendererIfNeeded();
    RenderBlockFlow::computePreferredLogicalWidths();
}

void RenderListItem::positionListMarker()
{
    if (m_marker && m_marker->parent()->isBox() && !m_marker->isInside() && m_marker->inlineBoxWrapper()) {
        LayoutUnit markerOldLogicalLeft = m_marker->logicalLeft();
        LayoutUnit blockOffset = 0;
        LayoutUnit lineOffset = 0;
        for (RenderBox* o = m_marker->parentBox(); o != this; o = o->parentBox()) {
            blockOffset += o->logicalTop();
            lineOffset += o->logicalLeft();
        }

        bool adjustOverflow = false;
        LayoutUnit markerLogicalLeft;
        bool hitSelfPaintingLayer = false;
        
        const RootInlineBox& rootBox = m_marker->inlineBoxWrapper()->root();
        LayoutUnit lineTop = rootBox.lineTop();
        LayoutUnit lineBottom = rootBox.lineBottom();

        // FIXME: Need to account for relative positioning in the layout overflow.
        if (style().isLeftToRightDirection()) {
            LayoutUnit leftLineOffset = logicalLeftOffsetForLine(blockOffset, logicalLeftOffsetForLine(blockOffset, false), false);
            markerLogicalLeft = leftLineOffset - lineOffset - paddingStart() - borderStart() + m_marker->marginStart();
            m_marker->inlineBoxWrapper()->adjustLineDirectionPosition(markerLogicalLeft - markerOldLogicalLeft);
            for (InlineFlowBox* box = m_marker->inlineBoxWrapper()->parent(); box; box = box->parent()) {
                LayoutRect newLogicalVisualOverflowRect = box->logicalVisualOverflowRect(lineTop, lineBottom);
                LayoutRect newLogicalLayoutOverflowRect = box->logicalLayoutOverflowRect(lineTop, lineBottom);
                if (markerLogicalLeft < newLogicalVisualOverflowRect.x() && !hitSelfPaintingLayer) {
                    newLogicalVisualOverflowRect.setWidth(newLogicalVisualOverflowRect.maxX() - markerLogicalLeft);
                    newLogicalVisualOverflowRect.setX(markerLogicalLeft);
                    if (box == &rootBox)
                        adjustOverflow = true;
                }
                if (markerLogicalLeft < newLogicalLayoutOverflowRect.x()) {
                    newLogicalLayoutOverflowRect.setWidth(newLogicalLayoutOverflowRect.maxX() - markerLogicalLeft);
                    newLogicalLayoutOverflowRect.setX(markerLogicalLeft);
                    if (box == &rootBox)
                        adjustOverflow = true;
                }
                box->setOverflowFromLogicalRects(newLogicalLayoutOverflowRect, newLogicalVisualOverflowRect, lineTop, lineBottom);
                if (box->renderer().hasSelfPaintingLayer())
                    hitSelfPaintingLayer = true;
            }
        } else {
            LayoutUnit rightLineOffset = logicalRightOffsetForLine(blockOffset, logicalRightOffsetForLine(blockOffset, false), false);
            markerLogicalLeft = rightLineOffset - lineOffset + paddingStart() + borderStart() + m_marker->marginEnd();
            m_marker->inlineBoxWrapper()->adjustLineDirectionPosition(markerLogicalLeft - markerOldLogicalLeft);
            for (InlineFlowBox* box = m_marker->inlineBoxWrapper()->parent(); box; box = box->parent()) {
                LayoutRect newLogicalVisualOverflowRect = box->logicalVisualOverflowRect(lineTop, lineBottom);
                LayoutRect newLogicalLayoutOverflowRect = box->logicalLayoutOverflowRect(lineTop, lineBottom);
                if (markerLogicalLeft + m_marker->logicalWidth() > newLogicalVisualOverflowRect.maxX() && !hitSelfPaintingLayer) {
                    newLogicalVisualOverflowRect.setWidth(markerLogicalLeft + m_marker->logicalWidth() - newLogicalVisualOverflowRect.x());
                    if (box == &rootBox)
                        adjustOverflow = true;
                }
                if (markerLogicalLeft + m_marker->logicalWidth() > newLogicalLayoutOverflowRect.maxX()) {
                    newLogicalLayoutOverflowRect.setWidth(markerLogicalLeft + m_marker->logicalWidth() - newLogicalLayoutOverflowRect.x());
                    if (box == &rootBox)
                        adjustOverflow = true;
                }
                box->setOverflowFromLogicalRects(newLogicalLayoutOverflowRect, newLogicalVisualOverflowRect, lineTop, lineBottom);
                
                if (box->renderer().hasSelfPaintingLayer())
                    hitSelfPaintingLayer = true;
            }
        }

        if (adjustOverflow) {
            LayoutRect markerRect(markerLogicalLeft + lineOffset, blockOffset, m_marker->width(), m_marker->height());
            if (!style().isHorizontalWritingMode())
                markerRect = markerRect.transposedRect();
            RenderBox* o = m_marker;
            bool propagateVisualOverflow = true;
            bool propagateLayoutOverflow = true;
            do {
                o = o->parentBox();
                if (o->hasOverflowClip())
                    propagateVisualOverflow = false;
                if (is<RenderBlock>(*o)) {
                    if (propagateVisualOverflow)
                        downcast<RenderBlock>(*o).addVisualOverflow(markerRect);
                    if (propagateLayoutOverflow)
                        downcast<RenderBlock>(*o).addLayoutOverflow(markerRect);
                }
                if (o->hasOverflowClip())
                    propagateLayoutOverflow = false;
                if (o->hasSelfPaintingLayer())
                    propagateVisualOverflow = false;
                markerRect.moveBy(-o->location());
            } while (o != this && propagateVisualOverflow && propagateLayoutOverflow);
        }
    }
}

void RenderListItem::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    if (!logicalHeight() && hasOverflowClip())
        return;

    RenderBlockFlow::paint(paintInfo, paintOffset);
}

const String& RenderListItem::markerText() const
{
    if (m_marker)
        return m_marker->text();
    return nullAtom.string();
}

String RenderListItem::markerTextWithSuffix() const
{
    if (!m_marker)
        return String();

    // Append the suffix for the marker in the right place depending
    // on the direction of the text (right-to-left or left-to-right).
    if (m_marker->style().isLeftToRightDirection())
        return m_marker->text() + m_marker->suffix();
    return m_marker->suffix() + m_marker->text();
}

void RenderListItem::explicitValueChanged()
{
    if (m_marker)
        m_marker->setNeedsLayoutAndPrefWidthsRecalc();

    updateValue();
    Element* listNode = enclosingList(*this);
    if (!listNode)
        return;
    for (RenderListItem* item = nextListItem(*listNode, *this); item; item = nextListItem(*listNode, *item))
        item->updateValue();
}

void RenderListItem::setExplicitValue(int value)
{
    if (m_hasExplicitValue && m_explicitValue == value)
        return;
    m_explicitValue = value;
    m_value = value;
    m_hasExplicitValue = true;
    explicitValueChanged();
}

void RenderListItem::clearExplicitValue()
{
    if (!m_hasExplicitValue)
        return;
    m_hasExplicitValue = false;
    m_isValueUpToDate = false;
    explicitValueChanged();
}

static inline RenderListItem* previousOrNextItem(bool isListReversed, Element& list, RenderListItem& item)
{
    return isListReversed ? previousListItem(&list, item) : nextListItem(list, item);
}

void RenderListItem::updateListMarkerNumbers()
{
    Element* listNode = enclosingList(*this);
    // The list node can be the shadow root which has no renderer.
    if (!listNode)
        return;

    bool isListReversed = false;
    if (is<HTMLOListElement>(*listNode)) {
        HTMLOListElement& oListElement = downcast<HTMLOListElement>(*listNode);
        oListElement.itemCountChanged();
        isListReversed = oListElement.isReversed();
    }
    for (RenderListItem* item = previousOrNextItem(isListReversed, *listNode, *this); item; item = previousOrNextItem(isListReversed, *listNode, *item)) {
        if (!item->m_isValueUpToDate) {
            // If an item has been marked for update before, we can safely
            // assume that all the following ones have too.
            // This gives us the opportunity to stop here and avoid
            // marking the same nodes again.
            break;
        }
        item->updateValue();
    }
}

} // namespace WebCore
