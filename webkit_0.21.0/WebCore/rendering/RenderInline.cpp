/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "RenderInline.h"

#include "Chrome.h"
#include "FloatQuad.h"
#include "FrameSelection.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "InlineElementBox.h"
#include "InlineTextBox.h"
#include "Page.h"
#include "RenderBlock.h"
#include "RenderFullScreen.h"
#include "RenderGeometryMap.h"
#include "RenderIterator.h"
#include "RenderLayer.h"
#include "RenderLineBreak.h"
#include "RenderListMarker.h"
#include "RenderNamedFlowThread.h"
#include "RenderTable.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Settings.h"
#include "StyleInheritedData.h"
#include "TransformState.h"
#include "VisiblePosition.h"

#if ENABLE(DASHBOARD_SUPPORT)
#include "Frame.h"
#endif

namespace WebCore {

RenderInline::RenderInline(Element& element, Ref<RenderStyle>&& style)
    : RenderBoxModelObject(element, WTF::move(style), RenderInlineFlag)
{
    setChildrenInline(true);
}

RenderInline::RenderInline(Document& document, Ref<RenderStyle>&& style)
    : RenderBoxModelObject(document, WTF::move(style), RenderInlineFlag)
{
    setChildrenInline(true);
}

void RenderInline::willBeDestroyed()
{
#if !ASSERT_DISABLED
    // Make sure we do not retain "this" in the continuation outline table map of our containing blocks.
    if (parent() && style().visibility() == VISIBLE && hasOutline()) {
        bool containingBlockPaintsContinuationOutline = continuation() || isInlineElementContinuation();
        if (containingBlockPaintsContinuationOutline) {
            if (RenderBlock* cb = containingBlock()) {
                if (RenderBlock* cbCb = cb->containingBlock())
                    ASSERT(!cbCb->paintsContinuationOutline(this));
            }
        }
    }
#endif

    // Make sure to destroy anonymous children first while they are still connected to the rest of the tree, so that they will
    // properly dirty line boxes that they are removed from.  Effects that do :before/:after only on hover could crash otherwise.
    destroyLeftoverChildren();
    
    if (!documentBeingDestroyed()) {
        if (firstLineBox()) {
            // We can't wait for RenderBoxModelObject::destroy to clear the selection,
            // because by then we will have nuked the line boxes.
            if (isSelectionBorder())
                frame().selection().setNeedsSelectionUpdate();

            // If line boxes are contained inside a root, that means we're an inline.
            // In that case, we need to remove all the line boxes so that the parent
            // lines aren't pointing to deleted children. If the first line box does
            // not have a parent that means they are either already disconnected or
            // root lines that can just be destroyed without disconnecting.
            if (firstLineBox()->parent()) {
                for (auto box = firstLineBox(); box; box = box->nextLineBox())
                    box->removeFromParent();
            }
        } else if (parent())
            parent()->dirtyLinesFromChangedChild(*this);
    }

    m_lineBoxes.deleteLineBoxes();

    RenderBoxModelObject::willBeDestroyed();
}

RenderInline* RenderInline::inlineElementContinuation() const
{
    RenderBoxModelObject* continuation = this->continuation();
    if (!continuation)
        return nullptr;

    if (is<RenderInline>(*continuation))
        return downcast<RenderInline>(continuation);

    return is<RenderBlock>(*continuation) ? downcast<RenderBlock>(*continuation).inlineElementContinuation() : nullptr;
}

void RenderInline::updateFromStyle()
{
    RenderBoxModelObject::updateFromStyle();

    // FIXME: Support transforms and reflections on inline flows someday.
    setHasTransformRelatedProperty(false);
    setHasReflection(false);    
}

static RenderElement* inFlowPositionedInlineAncestor(RenderElement* p)
{
    while (p && p->isRenderInline()) {
        if (p->isInFlowPositioned())
            return p;
        p = p->parent();
    }
    return nullptr;
}

static void updateStyleOfAnonymousBlockContinuations(const RenderBlock& block, const RenderStyle* newStyle, const RenderStyle* oldStyle)
{
    // If any descendant blocks exist then they will be in the next anonymous block and its siblings.
    for (RenderBox* box = block.nextSiblingBox(); box && box->isAnonymousBlock(); box = box->nextSiblingBox()) {
        if (box->style().position() == newStyle->position())
            continue;
        
        if (!is<RenderBlock>(*box))
            continue;

        RenderBlock& block = downcast<RenderBlock>(*box);
        if (!block.isAnonymousBlockContinuation())
            continue;
        
        // If we are no longer in-flow positioned but our descendant block(s) still have an in-flow positioned ancestor then
        // their containing anonymous block should keep its in-flow positioning. 
        RenderInline* continuation = block.inlineElementContinuation();
        if (oldStyle->hasInFlowPosition() && inFlowPositionedInlineAncestor(continuation))
            continue;
        auto blockStyle = RenderStyle::createAnonymousStyleWithDisplay(&block.style(), BLOCK);
        blockStyle.get().setPosition(newStyle->position());
        block.setStyle(WTF::move(blockStyle));
    }
}

void RenderInline::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBoxModelObject::styleDidChange(diff, oldStyle);

    // Ensure that all of the split inlines pick up the new style. We
    // only do this if we're an inline, since we don't want to propagate
    // a block's style to the other inlines.
    // e.g., <font>foo <h4>goo</h4> moo</font>.  The <font> inlines before
    // and after the block share the same style, but the block doesn't
    // need to pass its style on to anyone else.
    RenderStyle& newStyle = style();
    RenderInline* continuation = inlineElementContinuation();
    if (continuation) {
        for (RenderInline* currCont = continuation; currCont; currCont = currCont->inlineElementContinuation()) {
            RenderBoxModelObject* nextCont = currCont->continuation();
            currCont->setContinuation(nullptr);
            currCont->setStyle(newStyle);
            currCont->setContinuation(nextCont);
        }
        // If an inline's in-flow positioning has changed and it is part of an active continuation as a descendant of an anonymous containing block,
        // then any descendant blocks will need to change their in-flow positioning accordingly.
        // Do this by updating the position of the descendant blocks' containing anonymous blocks - there may be more than one.
        if (containingBlock()->isAnonymousBlock() && oldStyle && newStyle.position() != oldStyle->position() && (newStyle.hasInFlowPosition() || oldStyle->hasInFlowPosition()))
            updateStyleOfAnonymousBlockContinuations(*containingBlock(), &newStyle, oldStyle);
    }

    if (!alwaysCreateLineBoxes()) {
        bool alwaysCreateLineBoxes = hasSelfPaintingLayer() || hasBoxDecorations() || newStyle.hasPadding() || newStyle.hasMargin() || hasOutline();
        if (oldStyle && alwaysCreateLineBoxes) {
            dirtyLineBoxes(false);
            setNeedsLayout();
        }
        setRenderInlineAlwaysCreatesLineBoxes(alwaysCreateLineBoxes);
    }
}

void RenderInline::updateAlwaysCreateLineBoxes(bool fullLayout)
{
    // Once we have been tainted once, just assume it will happen again. This way effects like hover highlighting that change the
    // background color will only cause a layout on the first rollover.
    if (alwaysCreateLineBoxes())
        return;

    RenderStyle* parentStyle = &parent()->style();
    RenderInline* parentRenderInline = is<RenderInline>(*parent()) ? downcast<RenderInline>(parent()) : nullptr;
    bool checkFonts = document().inNoQuirksMode();
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    bool alwaysCreateLineBoxes = (parentRenderInline && parentRenderInline->alwaysCreateLineBoxes())
        || (parentRenderInline && parentStyle->verticalAlign() != BASELINE)
        || style().verticalAlign() != BASELINE
        || style().textEmphasisMark() != TextEmphasisMarkNone
        || (checkFonts && (!parentStyle->fontCascade().fontMetrics().hasIdenticalAscentDescentAndLineGap(style().fontCascade().fontMetrics())
        || parentStyle->lineHeight() != style().lineHeight()))
        || (flowThread && flowThread->isRenderNamedFlowThread()); // FIXME: Enable the optimization once we make overflow computation for culled inlines in regions.

    if (!alwaysCreateLineBoxes && checkFonts && document().styleSheetCollection().usesFirstLineRules()) {
        // Have to check the first line style as well.
        parentStyle = &parent()->firstLineStyle();
        RenderStyle& childStyle = firstLineStyle();
        alwaysCreateLineBoxes = !parentStyle->fontCascade().fontMetrics().hasIdenticalAscentDescentAndLineGap(childStyle.fontCascade().fontMetrics())
            || childStyle.verticalAlign() != BASELINE
            || parentStyle->lineHeight() != childStyle.lineHeight();
    }

    if (alwaysCreateLineBoxes) {
        if (!fullLayout)
            dirtyLineBoxes(false);
        setAlwaysCreateLineBoxes();
    }
}

LayoutRect RenderInline::localCaretRect(InlineBox* inlineBox, int, LayoutUnit* extraWidthToEndOfLine)
{
    if (firstChild()) {
        // This condition is possible if the RenderInline is at an editing boundary,
        // i.e. the VisiblePosition is:
        //   <RenderInline editingBoundary=true>|<RenderText> </RenderText></RenderInline>
        // FIXME: need to figure out how to make this return a valid rect, note that
        // there are no line boxes created in the above case.
        return LayoutRect();
    }

    ASSERT_UNUSED(inlineBox, !inlineBox);

    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = 0;

    LayoutRect caretRect = localCaretRectForEmptyElement(horizontalBorderAndPaddingExtent(), 0);

    if (InlineBox* firstBox = firstLineBox())
        caretRect.moveBy(LayoutPoint(firstBox->topLeft()));

    return caretRect;
}

void RenderInline::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    auto* beforeChildOrPlaceholder = beforeChild;
    if (auto* flowThread = flowThreadContainingBlock())
        beforeChildOrPlaceholder = flowThread->resolveMovedChild(beforeChild);
    if (continuation())
        return addChildToContinuation(newChild, beforeChildOrPlaceholder);
    return addChildIgnoringContinuation(newChild, beforeChildOrPlaceholder);
}

static RenderBoxModelObject* nextContinuation(RenderObject* renderer)
{
    if (is<RenderInline>(*renderer) && !renderer->isReplaced())
        return downcast<RenderInline>(*renderer).continuation();
    return downcast<RenderBlock>(*renderer).inlineElementContinuation();
}

RenderBoxModelObject* RenderInline::continuationBefore(RenderObject* beforeChild)
{
    if (beforeChild && beforeChild->parent() == this)
        return this;

    RenderBoxModelObject* curr = nextContinuation(this);
    RenderBoxModelObject* nextToLast = this;
    RenderBoxModelObject* last = this;
    while (curr) {
        if (beforeChild && beforeChild->parent() == curr) {
            if (curr->firstChild() == beforeChild)
                return last;
            return curr;
        }

        nextToLast = last;
        last = curr;
        curr = nextContinuation(curr);
    }

    if (!beforeChild && !last->firstChild())
        return nextToLast;
    return last;
}

void RenderInline::addChildIgnoringContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    // Make sure we don't append things after :after-generated content if we have it.
    if (!beforeChild && isAfterContent(lastChild()))
        beforeChild = lastChild();
    
    bool useNewBlockInsideInlineModel = document().settings()->newBlockInsideInlineModelEnabled();
    
    // This code is for the old block-inside-inline model that uses continuations.
    if (!useNewBlockInsideInlineModel && !newChild->isInline() && !newChild->isFloatingOrOutOfFlowPositioned()) {
        // We are placing a block inside an inline. We have to perform a split of this
        // inline into continuations.  This involves creating an anonymous block box to hold
        // |newChild|.  We then make that block box a continuation of this inline.  We take all of
        // the children after |beforeChild| and put them in a clone of this object.
        auto newStyle = RenderStyle::createAnonymousStyleWithDisplay(&style(), BLOCK);
        
        // If inside an inline affected by in-flow positioning the block needs to be affected by it too.
        // Giving the block a layer like this allows it to collect the x/y offsets from inline parents later.
        if (auto positionedAncestor = inFlowPositionedInlineAncestor(this))
            newStyle.get().setPosition(positionedAncestor->style().position());

        RenderBlock* newBox = new RenderBlockFlow(document(), WTF::move(newStyle));
        newBox->initializeStyle();
        RenderBoxModelObject* oldContinuation = continuation();
        setContinuation(newBox);

        splitFlow(beforeChild, newBox, newChild, oldContinuation);
        return;
    }
    
    if (!useNewBlockInsideInlineModel) {
        RenderBoxModelObject::addChild(newChild, beforeChild);
        newChild->setNeedsLayoutAndPrefWidthsRecalc();
        return;
    }

    // This code is for the new block-inside-inline model that uses anonymous inline blocks.
    // If the requested beforeChild is not one of our children, then this is most likely because
    // there is an anonymous inline-block box within this object that contains the beforeChild.
    // Insert the child into the anonymous inline-block box instead of here.
    // A second possibility is that the beforeChild is an anonymous block inside the anonymous inline block.
    // This can happen if inlines are inserted in between two of the anonymous inline block's block-level
    // children after it has been created.
    if (beforeChild && beforeChild->parent() != this) {
        ASSERT(beforeChild->parent());
        ASSERT(beforeChild->parent()->isAnonymousInlineBlock() || beforeChild->parent()->isAnonymousBlock());
        if (beforeChild->parent()->isAnonymousInlineBlock()) {
            if (!newChild->isInline() || (newChild->isInline() && beforeChild->parent()->firstChild() != beforeChild))
                beforeChild->parent()->addChild(newChild, beforeChild);
            else
                addChild(newChild, beforeChild->parent());
        } else if (beforeChild->parent()->isAnonymousBlock()) {
            ASSERT(!beforeChild->parent()->parent() || beforeChild->parent()->parent()->isAnonymousInlineBlock());
            ASSERT(beforeChild->isInline());
            if (newChild->isInline() || (!newChild->isInline() && beforeChild->parent()->firstChild() != beforeChild))
                beforeChild->parent()->addChild(newChild, beforeChild);
            else
                addChild(newChild, beforeChild->parent());
        }
        return;
    }

    if (!newChild->isInline()) {
        // We are placing a block inside an inline. We have to place the block inside an anonymous inline-block.
        // This inline-block can house a sequence of contiguous block-level children, and they will all sit on the
        // same "line" together. We try to reuse an existing inline-block if possible.
        if (beforeChild) {
            if (beforeChild->previousSibling() && beforeChild->previousSibling()->isAnonymousInlineBlock()) {
                downcast<RenderBlockFlow>(beforeChild->previousSibling())->addChild(newChild);
                return;
            }
        } else {
            if (lastChild() && lastChild()->isAnonymousInlineBlock()) {
                downcast<RenderBlockFlow>(lastChild())->addChild(newChild);
                return;
            }
        }
 
        if (!newChild->isFloatingOrOutOfFlowPositioned()) {
            // There was no suitable existing anonymous inline-block. Create a new one.
            RenderBlockFlow* anonymousInlineBlock = new RenderBlockFlow(document(), RenderStyle::createAnonymousStyleWithDisplay(&style(), INLINE_BLOCK));
            anonymousInlineBlock->initializeStyle();
    
            RenderBoxModelObject::addChild(anonymousInlineBlock, beforeChild);
            anonymousInlineBlock->addChild(newChild);
            return;
        }
    }

    RenderBoxModelObject::addChild(newChild, beforeChild);

    newChild->setNeedsLayoutAndPrefWidthsRecalc();
}

RenderPtr<RenderInline> RenderInline::clone() const
{
    RenderPtr<RenderInline> cloneInline = createRenderer<RenderInline>(*element(), style());
    cloneInline->initializeStyle();
    cloneInline->setFlowThreadState(flowThreadState());
    return cloneInline;
}

void RenderInline::splitInlines(RenderBlock* fromBlock, RenderBlock* toBlock,
                                RenderBlock* middleBlock,
                                RenderObject* beforeChild, RenderBoxModelObject* oldCont)
{
    // Create a clone of this inline.
    RenderPtr<RenderInline> cloneInline = clone();
#if ENABLE(FULLSCREEN_API)
    // If we're splitting the inline containing the fullscreened element,
    // |beforeChild| may be the renderer for the fullscreened element. However,
    // that renderer is wrapped in a RenderFullScreen, so |this| is not its
    // parent. Since the splitting logic expects |this| to be the parent, set
    // |beforeChild| to be the RenderFullScreen.
    const Element* fullScreenElement = document().webkitCurrentFullScreenElement();
    if (fullScreenElement && beforeChild && beforeChild->node() == fullScreenElement)
        beforeChild = document().fullScreenRenderer();
#endif
    // Now take all of the children from beforeChild to the end and remove
    // them from |this| and place them in the clone.
    for (RenderObject* rendererToMove = beforeChild; rendererToMove;) {
        RenderObject* nextSibling = rendererToMove->nextSibling();
        // When anonymous wrapper is present, we might need to move the whole subtree instead.
        if (rendererToMove->parent() != this) {
            auto* anonymousParent = rendererToMove->parent();
            while (anonymousParent && anonymousParent->parent() != this) {
                ASSERT(anonymousParent->isAnonymous());
                anonymousParent = anonymousParent->parent();
            }
            if (!anonymousParent) {
                ASSERT_NOT_REACHED();
                break;
            }
            // If beforeChild is the first child in the subtree, we could just move the whole subtree.
            if (!rendererToMove->previousSibling()) {
                // Reparent the whole anonymous wrapper tree.
                rendererToMove = anonymousParent;
                // Skip to the next sibling that is not in this subtree.
                nextSibling = anonymousParent->nextSibling();
            } else if (!rendererToMove->nextSibling()) {
                // This is the last renderer in the subtree. We need to jump out of the wrapper subtree, so that
                // the siblings are getting reparented too.
                nextSibling = anonymousParent->nextSibling();
            }
            // Otherwise just move the renderer to the inline clone. Should the renderer need an anon
            // wrapper, the addChild() will generate one for it.
            // FIXME: When the anonymous wrapper has multiple children, we end up traversing up to the topmost wrapper
            // every time, which is a bit wasteful.
        }
        rendererToMove->parent()->removeChildInternal(*rendererToMove, NotifyChildren);
        cloneInline->addChildIgnoringContinuation(rendererToMove);
        rendererToMove->setNeedsLayoutAndPrefWidthsRecalc();
        rendererToMove = nextSibling;
    }
    cloneInline->setContinuation(oldCont);
    // Hook |clone| up as the continuation of the middle block.
    middleBlock->setContinuation(cloneInline.get());

    // We have been reparented and are now under the fromBlock.  We need
    // to walk up our inline parent chain until we hit the containing block.
    // Once we hit the containing block we're done.
    RenderBoxModelObject* current = downcast<RenderBoxModelObject>(parent());
    RenderBoxModelObject* currentChild = this;
    
    // FIXME: Because splitting is O(n^2) as tags nest pathologically, we cap the depth at which we're willing to clone.
    // There will eventually be a better approach to this problem that will let us nest to a much
    // greater depth (see bugzilla bug 13430) but for now we have a limit.  This *will* result in
    // incorrect rendering, but the alternative is to hang forever.
    unsigned splitDepth = 1;
    const unsigned cMaxSplitDepth = 200; 
    while (current && current != fromBlock) {
        if (splitDepth < cMaxSplitDepth) {
            // Create a new clone.
            RenderPtr<RenderInline> cloneChild = WTF::move(cloneInline);
            cloneInline = downcast<RenderInline>(*current).clone();

            // Insert our child clone as the first child.
            cloneInline->addChildIgnoringContinuation(cloneChild.leakPtr());

            // Hook the clone up as a continuation of |curr|.
            RenderInline& currentInline = downcast<RenderInline>(*current);
            oldCont = currentInline.continuation();
            currentInline.setContinuation(cloneInline.get());
            cloneInline->setContinuation(oldCont);

            // Now we need to take all of the children starting from the first child
            // *after* currentChild and append them all to the clone.
            for (auto* current = currentChild->nextSibling(); current;) {
                auto* next = current->nextSibling();
                currentInline.removeChildInternal(*current, NotifyChildren);
                cloneInline->addChildIgnoringContinuation(current);
                current->setNeedsLayoutAndPrefWidthsRecalc();
                current = next;
            }
        }
        
        // Keep walking up the chain.
        currentChild = current;
        current = downcast<RenderBoxModelObject>(current->parent());
        ++splitDepth;
    }

    // Clear the flow thread containing blocks cached during the detached state insertions.
    cloneInline->invalidateFlowThreadContainingBlockIncludingDescendants();

    // Now we are at the block level. We need to put the clone into the toBlock.
    toBlock->insertChildInternal(cloneInline.leakPtr(), nullptr, NotifyChildren);

    // Now take all the children after currentChild and remove them from the fromBlock
    // and put them in the toBlock.
    for (auto* current = currentChild->nextSibling(); current;) {
        auto* next = current->nextSibling();
        fromBlock->removeChildInternal(*current, NotifyChildren);
        toBlock->insertChildInternal(current, nullptr, NotifyChildren);
        current = next;
    }
}

void RenderInline::splitFlow(RenderObject* beforeChild, RenderBlock* newBlockBox,
                             RenderObject* newChild, RenderBoxModelObject* oldCont)
{
    RenderBlock* pre = nullptr;
    RenderBlock* block = containingBlock();
    
    // Delete our line boxes before we do the inline split into continuations.
    block->deleteLines();
    
    bool madeNewBeforeBlock = false;
    if (block->isAnonymousBlock() && (!block->parent() || !block->parent()->createsAnonymousWrapper())) {
        // We can reuse this block and make it the preBlock of the next continuation.
        pre = block;
        pre->removePositionedObjects(nullptr);
        // FIXME-BLOCKFLOW: The enclosing method should likely be switched over
        // to only work on RenderBlockFlow, in which case this conversion can be
        // removed.
        if (is<RenderBlockFlow>(*pre))
            downcast<RenderBlockFlow>(*pre).removeFloatingObjects();
        block = block->containingBlock();
    } else {
        // No anonymous block available for use.  Make one.
        pre = block->createAnonymousBlock();
        madeNewBeforeBlock = true;
    }

    RenderBlock& post = downcast<RenderBlock>(*pre->createAnonymousBoxWithSameTypeAs(block));

    RenderObject* boxFirst = madeNewBeforeBlock ? block->firstChild() : pre->nextSibling();
    if (madeNewBeforeBlock)
        block->insertChildInternal(pre, boxFirst, NotifyChildren);
    block->insertChildInternal(newBlockBox, boxFirst, NotifyChildren);
    block->insertChildInternal(&post, boxFirst, NotifyChildren);
    block->setChildrenInline(false);
    
    if (madeNewBeforeBlock) {
        RenderObject* o = boxFirst;
        while (o) {
            RenderObject* no = o;
            o = no->nextSibling();
            block->removeChildInternal(*no, NotifyChildren);
            pre->insertChildInternal(no, nullptr, NotifyChildren);
            no->setNeedsLayoutAndPrefWidthsRecalc();
        }
    }

    splitInlines(pre, &post, newBlockBox, beforeChild, oldCont);

    // We already know the newBlockBox isn't going to contain inline kids, so avoid wasting
    // time in makeChildrenNonInline by just setting this explicitly up front.
    newBlockBox->setChildrenInline(false);

    // We delayed adding the newChild until now so that the |newBlockBox| would be fully
    // connected, thus allowing newChild access to a renderArena should it need
    // to wrap itself in additional boxes (e.g., table construction).
    newBlockBox->addChild(newChild);

    // Always just do a full layout in order to ensure that line boxes (especially wrappers for images)
    // get deleted properly.  Because objects moves from the pre block into the post block, we want to
    // make new line boxes instead of leaving the old line boxes around.
    pre->setNeedsLayoutAndPrefWidthsRecalc();
    block->setNeedsLayoutAndPrefWidthsRecalc();
    post.setNeedsLayoutAndPrefWidthsRecalc();
}

static bool canUseAsParentForContinuation(const RenderObject* renderer)
{
    if (!renderer)
        return false;
    if (!is<RenderBlock>(renderer) && renderer->isAnonymous())
        return false;
    if (is<RenderTable>(renderer))
        return false;
    return true;
}

void RenderInline::addChildToContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    auto* flow = continuationBefore(beforeChild);
    // It may or may not be the direct parent of the beforeChild.
    RenderBoxModelObject* beforeChildAncestor = nullptr;
    if (!beforeChild) {
        auto* continuation = nextContinuation(flow);
        beforeChildAncestor = continuation ? continuation : flow;
    }
    else if (canUseAsParentForContinuation(beforeChild->parent()))
        beforeChildAncestor = downcast<RenderBoxModelObject>(beforeChild->parent());
    else if (beforeChild->parent()) {
        // In case of anonymous wrappers, the parent of the beforeChild is mostly irrelevant. What we need is the topmost wrapper.
        auto* parent = beforeChild->parent();
        while (parent && parent->parent() && parent->parent()->isAnonymous()) {
            // The ancestor candidate needs to be inside the continuation.
            if (parent->hasContinuation())
                break;
            parent = parent->parent();
        }
        ASSERT(parent && parent->parent());
        beforeChildAncestor = downcast<RenderBoxModelObject>(parent->parent());
    }
    else
        ASSERT_NOT_REACHED();

    if (newChild->isFloatingOrOutOfFlowPositioned())
        return beforeChildAncestor->addChildIgnoringContinuation(newChild, beforeChild);

    if (flow == beforeChildAncestor)
        return flow->addChildIgnoringContinuation(newChild, beforeChild);
    // A continuation always consists of two potential candidates: an inline or an anonymous
    // block box holding block children.
    bool childInline = newChild->isInline();
    // The goal here is to match up if we can, so that we can coalesce and create the
    // minimal # of continuations needed for the inline.
    if (childInline == beforeChildAncestor->isInline())
        return beforeChildAncestor->addChildIgnoringContinuation(newChild, beforeChild);
    if (flow->isInline() == childInline)
        return flow->addChildIgnoringContinuation(newChild); // Just treat like an append.
    return beforeChildAncestor->addChildIgnoringContinuation(newChild, beforeChild);
}

void RenderInline::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    m_lineBoxes.paint(this, paintInfo, paintOffset);
}

template<typename GeneratorContext>
void RenderInline::generateLineBoxRects(GeneratorContext& context) const
{
    if (!alwaysCreateLineBoxes())
        generateCulledLineBoxRects(context, this);
    else if (InlineFlowBox* curr = firstLineBox()) {
        for (; curr; curr = curr->nextLineBox())
            context.addRect(FloatRect(curr->topLeft(), curr->size()));
    } else
        context.addRect(FloatRect());
}

template<typename GeneratorContext>
void RenderInline::generateCulledLineBoxRects(GeneratorContext& context, const RenderInline* container) const
{
    if (!culledInlineFirstLineBox()) {
        context.addRect(FloatRect());
        return;
    }

    bool isHorizontal = style().isHorizontalWritingMode();

    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (current->isFloatingOrOutOfFlowPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (is<RenderBox>(*current)) {
            RenderBox& renderBox = downcast<RenderBox>(*current);
            if (renderBox.inlineBoxWrapper()) {
                const RootInlineBox& rootBox = renderBox.inlineBoxWrapper()->root();
                const RenderStyle& containerStyle = rootBox.isFirstLine() ? container->firstLineStyle() : container->style();
                int logicalTop = rootBox.logicalTop() + (rootBox.lineStyle().fontCascade().fontMetrics().ascent() - containerStyle.fontCascade().fontMetrics().ascent());
                int logicalHeight = containerStyle.fontCascade().fontMetrics().height();
                if (isHorizontal)
                    context.addRect(FloatRect(renderBox.inlineBoxWrapper()->x() - renderBox.marginLeft(), logicalTop, renderBox.width() + renderBox.horizontalMarginExtent(), logicalHeight));
                else
                    context.addRect(FloatRect(logicalTop, renderBox.inlineBoxWrapper()->y() - renderBox.marginTop(), logicalHeight, renderBox.height() + renderBox.verticalMarginExtent()));
            }
        } else if (is<RenderInline>(*current)) {
            // If the child doesn't need line boxes either, then we can recur.
            RenderInline& renderInline = downcast<RenderInline>(*current);
            if (!renderInline.alwaysCreateLineBoxes())
                renderInline.generateCulledLineBoxRects(context, container);
            else {
                for (InlineFlowBox* childLine = renderInline.firstLineBox(); childLine; childLine = childLine->nextLineBox()) {
                    const RootInlineBox& rootBox = childLine->root();
                    const RenderStyle& containerStyle = rootBox.isFirstLine() ? container->firstLineStyle() : container->style();
                    int logicalTop = rootBox.logicalTop() + (rootBox.lineStyle().fontCascade().fontMetrics().ascent() - containerStyle.fontCascade().fontMetrics().ascent());
                    int logicalHeight = containerStyle.fontMetrics().height();
                    if (isHorizontal) {
                        context.addRect(FloatRect(childLine->x() - childLine->marginLogicalLeft(),
                            logicalTop,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight(),
                            logicalHeight));
                    } else {
                        context.addRect(FloatRect(logicalTop,
                            childLine->y() - childLine->marginLogicalLeft(),
                            logicalHeight,
                            childLine->logicalWidth() + childLine->marginLogicalLeft() + childLine->marginLogicalRight()));
                    }
                }
            }
        } else if (is<RenderText>(*current)) {
            RenderText& currText = downcast<RenderText>(*current);
            for (InlineTextBox* childText = currText.firstTextBox(); childText; childText = childText->nextTextBox()) {
                const RootInlineBox& rootBox = childText->root();
                const RenderStyle& containerStyle = rootBox.isFirstLine() ? container->firstLineStyle() : container->style();
                int logicalTop = rootBox.logicalTop() + (rootBox.lineStyle().fontCascade().fontMetrics().ascent() - containerStyle.fontCascade().fontMetrics().ascent());
                int logicalHeight = containerStyle.fontCascade().fontMetrics().height();
                if (isHorizontal)
                    context.addRect(FloatRect(childText->x(), logicalTop, childText->logicalWidth(), logicalHeight));
                else
                    context.addRect(FloatRect(logicalTop, childText->y(), logicalHeight, childText->logicalWidth()));
            }
        } else if (is<RenderLineBreak>(*current)) {
            if (InlineBox* inlineBox = downcast<RenderLineBreak>(*current).inlineBoxWrapper()) {
                // FIXME: This could use a helper to share these with text path.
                const RootInlineBox& rootBox = inlineBox->root();
                const RenderStyle& containerStyle = rootBox.isFirstLine() ? container->firstLineStyle() : container->style();
                int logicalTop = rootBox.logicalTop() + (rootBox.lineStyle().fontCascade().fontMetrics().ascent() - containerStyle.fontCascade().fontMetrics().ascent());
                int logicalHeight = containerStyle.fontMetrics().height();
                if (isHorizontal)
                    context.addRect(FloatRect(inlineBox->x(), logicalTop, inlineBox->logicalWidth(), logicalHeight));
                else
                    context.addRect(FloatRect(logicalTop, inlineBox->y(), logicalHeight, inlineBox->logicalWidth()));
            }
        }
    }
}

namespace {

class AbsoluteRectsGeneratorContext {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    AbsoluteRectsGeneratorContext(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset)
        : m_rects(rects)
        , m_accumulatedOffset(accumulatedOffset) { }

    void addRect(const FloatRect& rect)
    {
        IntRect intRect = enclosingIntRect(rect);
        intRect.move(m_accumulatedOffset.x(), m_accumulatedOffset.y());
        m_rects.append(intRect);
    }
private:
    Vector<IntRect>& m_rects;
    const LayoutPoint& m_accumulatedOffset;
};

} // unnamed namespace

void RenderInline::absoluteRects(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset) const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    AbsoluteRectsGeneratorContext context(rects, accumulatedOffset);
    generateLineBoxRects(context);

    if (RenderBoxModelObject* continuation = this->continuation()) {
        if (is<RenderBox>(*continuation)) {
            auto& box = downcast<RenderBox>(*continuation);
            continuation->absoluteRects(rects, toLayoutPoint(accumulatedOffset - containingBlock()->location() + box.locationOffset()));
        } else
            continuation->absoluteRects(rects, toLayoutPoint(accumulatedOffset - containingBlock()->location()));
    }
}


namespace {

class AbsoluteQuadsGeneratorContext {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    AbsoluteQuadsGeneratorContext(const RenderInline* renderer, Vector<FloatQuad>& quads)
        : m_quads(quads)
        , m_geometryMap()
    {
        m_geometryMap.pushMappingsToAncestor(renderer, nullptr);
    }

    void addRect(const FloatRect& rect)
    {
        m_quads.append(m_geometryMap.absoluteRect(rect));
    }
private:
    Vector<FloatQuad>& m_quads;
    RenderGeometryMap m_geometryMap;
};

} // unnamed namespace

void RenderInline::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    AbsoluteQuadsGeneratorContext context(this, quads);
    generateLineBoxRects(context);

    if (RenderBoxModelObject* continuation = this->continuation())
        continuation->absoluteQuads(quads, wasFixed);
}

#if PLATFORM(IOS)
void RenderInline::absoluteQuadsForSelection(Vector<FloatQuad>& quads) const
{
    AbsoluteQuadsGeneratorContext context(this, quads);
    generateLineBoxRects(context);
}
#endif

LayoutUnit RenderInline::offsetLeft() const
{
    LayoutPoint topLeft;
    if (InlineBox* firstBox = firstLineBoxIncludingCulling())
        topLeft = flooredLayoutPoint(firstBox->topLeft());
    return adjustedPositionRelativeToOffsetParent(topLeft).x();
}

LayoutUnit RenderInline::offsetTop() const
{
    LayoutPoint topLeft;
    if (InlineBox* firstBox = firstLineBoxIncludingCulling())
        topLeft = flooredLayoutPoint(firstBox->topLeft());
    return adjustedPositionRelativeToOffsetParent(topLeft).y();
}

static LayoutUnit computeMargin(const RenderInline* renderer, const Length& margin)
{
    if (margin.isAuto())
        return 0;
    if (margin.isFixed())
        return margin.value();
    if (margin.isPercentOrCalculated())
        return minimumValueForLength(margin, std::max<LayoutUnit>(0, renderer->containingBlock()->availableLogicalWidth()));
    return 0;
}

LayoutUnit RenderInline::marginLeft() const
{
    return computeMargin(this, style().marginLeft());
}

LayoutUnit RenderInline::marginRight() const
{
    return computeMargin(this, style().marginRight());
}

LayoutUnit RenderInline::marginTop() const
{
    return computeMargin(this, style().marginTop());
}

LayoutUnit RenderInline::marginBottom() const
{
    return computeMargin(this, style().marginBottom());
}

LayoutUnit RenderInline::marginStart(const RenderStyle* otherStyle) const
{
    return computeMargin(this, style().marginStartUsing(otherStyle ? otherStyle : &style()));
}

LayoutUnit RenderInline::marginEnd(const RenderStyle* otherStyle) const
{
    return computeMargin(this, style().marginEndUsing(otherStyle ? otherStyle : &style()));
}

LayoutUnit RenderInline::marginBefore(const RenderStyle* otherStyle) const
{
    return computeMargin(this, style().marginBeforeUsing(otherStyle ? otherStyle : &style()));
}

LayoutUnit RenderInline::marginAfter(const RenderStyle* otherStyle) const
{
    return computeMargin(this, style().marginAfterUsing(otherStyle ? otherStyle : &style()));
}

const char* RenderInline::renderName() const
{
    if (isRelPositioned())
        return "RenderInline (relative positioned)";
    if (isStickyPositioned())
        return "RenderInline (sticky positioned)";
    // FIXME: Temporary hack while the new generated content system is being implemented.
    if (isPseudoElement())
        return "RenderInline (generated)";
    if (isAnonymous())
        return "RenderInline (generated)";
    return "RenderInline";
}

bool RenderInline::nodeAtPoint(const HitTestRequest& request, HitTestResult& result,
                                const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    return m_lineBoxes.hitTest(this, request, result, locationInContainer, accumulatedOffset, hitTestAction);
}

namespace {

class HitTestCulledInlinesGeneratorContext {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    HitTestCulledInlinesGeneratorContext(Region& region, const HitTestLocation& location)
        : m_intersected(false)
        , m_region(region)
        , m_location(location)
    { }

    void addRect(const FloatRect& rect)
    {
        m_intersected = m_intersected || m_location.intersects(rect);
        m_region.unite(enclosingIntRect(rect));
    }

    bool intersected() const { return m_intersected; }

private:
    bool m_intersected;
    Region& m_region;
    const HitTestLocation& m_location;
};

} // unnamed namespace

bool RenderInline::hitTestCulledInline(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset)
{
    ASSERT(result.isRectBasedTest() && !alwaysCreateLineBoxes());
    if (!visibleToHitTesting())
        return false;

    HitTestLocation tmpLocation(locationInContainer, -toLayoutSize(accumulatedOffset));

    Region regionResult;
    HitTestCulledInlinesGeneratorContext context(regionResult, tmpLocation);
    generateCulledLineBoxRects(context, this);

    if (context.intersected()) {
        updateHitTestResult(result, tmpLocation.point());
        // We can not use addNodeToRectBasedTestResult to determine if we fully enclose the hit-test area
        // because it can only handle rectangular targets.
        result.addNodeToRectBasedTestResult(element(), request, locationInContainer);
        return regionResult.contains(tmpLocation.boundingBox());
    }
    return false;
}

VisiblePosition RenderInline::positionForPoint(const LayoutPoint& point, const RenderRegion* region)
{
    // FIXME: Does not deal with relative or sticky positioned inlines (should it?)
    RenderBlock& containingBlock = *this->containingBlock();
    if (firstLineBox()) {
        // This inline actually has a line box.  We must have clicked in the border/padding of one of these boxes.  We
        // should try to find a result by asking our containing block.
        return containingBlock.positionForPoint(point, region);
    }

    // Translate the coords from the pre-anonymous block to the post-anonymous block.
    LayoutPoint parentBlockPoint = containingBlock.location() + point;
    RenderBoxModelObject* continuation = this->continuation();
    while (continuation) {
        RenderBlock* currentBlock = continuation->isInline() ? continuation->containingBlock() : downcast<RenderBlock>(continuation);
        if (continuation->isInline() || continuation->firstChild())
            return continuation->positionForPoint(parentBlockPoint - currentBlock->locationOffset(), region);
        continuation = downcast<RenderBlock>(*continuation).inlineElementContinuation();
    }
    
    return RenderBoxModelObject::positionForPoint(point, region);
}

namespace {

class LinesBoundingBoxGeneratorContext {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    LinesBoundingBoxGeneratorContext(FloatRect& rect) : m_rect(rect) { }

    void addRect(const FloatRect& rect)
    {
        m_rect.uniteIfNonZero(rect);
    }
private:
    FloatRect& m_rect;
};

} // unnamed namespace

IntRect RenderInline::linesBoundingBox() const
{
    if (!alwaysCreateLineBoxes()) {
        ASSERT(!firstLineBox());
        FloatRect floatResult;
        LinesBoundingBoxGeneratorContext context(floatResult);
        generateCulledLineBoxRects(context, this);
        return enclosingIntRect(floatResult);
    }

    IntRect result;
    
    // See <rdar://problem/5289721>, for an unknown reason the linked list here is sometimes inconsistent, first is non-zero and last is zero.  We have been
    // unable to reproduce this at all (and consequently unable to figure ot why this is happening).  The assert will hopefully catch the problem in debug
    // builds and help us someday figure out why.  We also put in a redundant check of lastLineBox() to avoid the crash for now.
    ASSERT(!firstLineBox() == !lastLineBox());  // Either both are null or both exist.
    if (firstLineBox() && lastLineBox()) {
        // Return the width of the minimal left side and the maximal right side.
        float logicalLeftSide = 0;
        float logicalRightSide = 0;
        for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
            if (curr == firstLineBox() || curr->logicalLeft() < logicalLeftSide)
                logicalLeftSide = curr->logicalLeft();
            if (curr == firstLineBox() || curr->logicalRight() > logicalRightSide)
                logicalRightSide = curr->logicalRight();
        }
        
        bool isHorizontal = style().isHorizontalWritingMode();
        
        float x = isHorizontal ? logicalLeftSide : firstLineBox()->x();
        float y = isHorizontal ? firstLineBox()->y() : logicalLeftSide;
        float width = isHorizontal ? logicalRightSide - logicalLeftSide : lastLineBox()->logicalBottom() - x;
        float height = isHorizontal ? lastLineBox()->logicalBottom() - y : logicalRightSide - logicalLeftSide;
        result = enclosingIntRect(FloatRect(x, y, width, height));
    }

    return result;
}

InlineBox* RenderInline::culledInlineFirstLineBox() const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (current->isFloatingOrOutOfFlowPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (is<RenderBox>(*current)) {
            const auto& renderBox = downcast<RenderBox>(*current);
            if (renderBox.inlineBoxWrapper())
                return renderBox.inlineBoxWrapper();
        } else if (is<RenderLineBreak>(*current)) {
            RenderLineBreak& renderBR = downcast<RenderLineBreak>(*current);
            if (renderBR.inlineBoxWrapper())
                return renderBR.inlineBoxWrapper();
        } else if (is<RenderInline>(*current)) {
            RenderInline& renderInline = downcast<RenderInline>(*current);
            if (InlineBox* result = renderInline.firstLineBoxIncludingCulling())
                return result;
        } else if (is<RenderText>(*current)) {
            RenderText& renderText = downcast<RenderText>(*current);
            if (renderText.firstTextBox())
                return renderText.firstTextBox();
        }
    }
    return nullptr;
}

InlineBox* RenderInline::culledInlineLastLineBox() const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    for (RenderObject* current = lastChild(); current; current = current->previousSibling()) {
        if (current->isFloatingOrOutOfFlowPositioned())
            continue;
            
        // We want to get the margin box in the inline direction, and then use our font ascent/descent in the block
        // direction (aligned to the root box's baseline).
        if (is<RenderBox>(*current)) {
            const auto& renderBox = downcast<RenderBox>(*current);
            if (renderBox.inlineBoxWrapper())
                return renderBox.inlineBoxWrapper();
        } else if (is<RenderLineBreak>(*current)) {
            RenderLineBreak& renderBR = downcast<RenderLineBreak>(*current);
            if (renderBR.inlineBoxWrapper())
                return renderBR.inlineBoxWrapper();
        } else if (is<RenderInline>(*current)) {
            RenderInline& renderInline = downcast<RenderInline>(*current);
            if (InlineBox* result = renderInline.lastLineBoxIncludingCulling())
                return result;
        } else if (is<RenderText>(*current)) {
            RenderText& renderText = downcast<RenderText>(*current);
            if (renderText.lastTextBox())
                return renderText.lastTextBox();
        }
    }
    return nullptr;
}

LayoutRect RenderInline::culledInlineVisualOverflowBoundingBox() const
{
    FloatRect floatResult;
    LinesBoundingBoxGeneratorContext context(floatResult);
    generateCulledLineBoxRects(context, this);
    LayoutRect result(enclosingLayoutRect(floatResult));
    bool isHorizontal = style().isHorizontalWritingMode();
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (current->isFloatingOrOutOfFlowPositioned())
            continue;
            
        // For overflow we just have to propagate by hand and recompute it all.
        if (is<RenderBox>(*current)) {
            RenderBox& renderBox = downcast<RenderBox>(*current);
            if (!renderBox.hasSelfPaintingLayer() && renderBox.inlineBoxWrapper()) {
                LayoutRect logicalRect = renderBox.logicalVisualOverflowRectForPropagation(&style());
                if (isHorizontal) {
                    logicalRect.moveBy(renderBox.location());
                    result.uniteIfNonZero(logicalRect);
                } else {
                    logicalRect.moveBy(renderBox.location());
                    result.uniteIfNonZero(logicalRect.transposedRect());
                }
            }
        } else if (is<RenderInline>(*current)) {
            // If the child doesn't need line boxes either, then we can recur.
            RenderInline& renderInline = downcast<RenderInline>(*current);
            if (!renderInline.alwaysCreateLineBoxes())
                result.uniteIfNonZero(renderInline.culledInlineVisualOverflowBoundingBox());
            else if (!renderInline.hasSelfPaintingLayer())
                result.uniteIfNonZero(renderInline.linesVisualOverflowBoundingBox());
        } else if (is<RenderText>(*current)) {
            // FIXME; Overflow from text boxes is lost. We will need to cache this information in
            // InlineTextBoxes.
            RenderText& renderText = downcast<RenderText>(*current);
            result.uniteIfNonZero(renderText.linesVisualOverflowBoundingBox());
        }
    }
    return result;
}

LayoutRect RenderInline::linesVisualOverflowBoundingBox() const
{
    if (!alwaysCreateLineBoxes())
        return culledInlineVisualOverflowBoundingBox();

    if (!firstLineBox() || !lastLineBox())
        return LayoutRect();

    // Return the width of the minimal left side and the maximal right side.
    LayoutUnit logicalLeftSide = LayoutUnit::max();
    LayoutUnit logicalRightSide = LayoutUnit::min();
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        logicalLeftSide = std::min(logicalLeftSide, curr->logicalLeftVisualOverflow());
        logicalRightSide = std::max(logicalRightSide, curr->logicalRightVisualOverflow());
    }

    const RootInlineBox& firstRootBox = firstLineBox()->root();
    const RootInlineBox& lastRootBox = lastLineBox()->root();
    
    LayoutUnit logicalTop = firstLineBox()->logicalTopVisualOverflow(firstRootBox.lineTop());
    LayoutUnit logicalWidth = logicalRightSide - logicalLeftSide;
    LayoutUnit logicalHeight = lastLineBox()->logicalBottomVisualOverflow(lastRootBox.lineBottom()) - logicalTop;
    
    LayoutRect rect(logicalLeftSide, logicalTop, logicalWidth, logicalHeight);
    if (!style().isHorizontalWritingMode())
        rect = rect.transposedRect();
    return rect;
}

LayoutRect RenderInline::linesVisualOverflowBoundingBoxInRegion(const RenderRegion* region) const
{
    ASSERT(alwaysCreateLineBoxes());
    ASSERT(region);

    if (!firstLineBox() || !lastLineBox())
        return LayoutRect();

    // Return the width of the minimal left side and the maximal right side.
    LayoutUnit logicalLeftSide = LayoutUnit::max();
    LayoutUnit logicalRightSide = LayoutUnit::min();
    LayoutUnit logicalTop;
    LayoutUnit logicalHeight;
    InlineFlowBox* lastInlineInRegion = 0;
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        const RootInlineBox& root = curr->root();
        if (root.containingRegion() != region) {
            if (lastInlineInRegion)
                break;
            continue;
        }

        if (!lastInlineInRegion)
            logicalTop = curr->logicalTopVisualOverflow(root.lineTop());

        lastInlineInRegion = curr;

        logicalLeftSide = std::min(logicalLeftSide, curr->logicalLeftVisualOverflow());
        logicalRightSide = std::max(logicalRightSide, curr->logicalRightVisualOverflow());
    }

    if (!lastInlineInRegion)
        return LayoutRect();

    logicalHeight = lastInlineInRegion->logicalBottomVisualOverflow(lastInlineInRegion->root().lineBottom()) - logicalTop;
    
    LayoutUnit logicalWidth = logicalRightSide - logicalLeftSide;
    
    LayoutRect rect(logicalLeftSide, logicalTop, logicalWidth, logicalHeight);
    if (!style().isHorizontalWritingMode())
        rect = rect.transposedRect();
    return rect;
}

LayoutRect RenderInline::clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const
{
    // Only first-letter renderers are allowed in here during layout. They mutate the tree triggering repaints.
    ASSERT(!view().layoutStateEnabled() || style().styleType() == FIRST_LETTER);

    if (!firstLineBoxIncludingCulling() && !continuation())
        return LayoutRect();

    LayoutRect repaintRect(linesVisualOverflowBoundingBox());
    bool hitRepaintContainer = false;

    // We need to add in the in-flow position offsets of any inlines (including us) up to our
    // containing block.
    RenderBlock* containingBlock = this->containingBlock();
    for (const RenderElement* inlineFlow = this; is<RenderInline>(inlineFlow) && inlineFlow != containingBlock;
         inlineFlow = inlineFlow->parent()) {
         if (inlineFlow == repaintContainer) {
            hitRepaintContainer = true;
            break;
        }
        if (inlineFlow->style().hasInFlowPosition() && inlineFlow->hasLayer())
            repaintRect.move(downcast<RenderInline>(*inlineFlow).layer()->offsetForInFlowPosition());
    }

    LayoutUnit outlineSize = style().outlineSize();
    repaintRect.inflate(outlineSize);

    if (hitRepaintContainer || !containingBlock)
        return repaintRect;

    if (containingBlock->hasOverflowClip())
        containingBlock->applyCachedClipAndScrollOffsetForRepaint(repaintRect);

    containingBlock->computeRectForRepaint(repaintContainer, repaintRect);

    if (outlineSize) {
        for (auto& child : childrenOfType<RenderElement>(*this))
            repaintRect.unite(child.rectWithOutlineForRepaint(repaintContainer, outlineSize));

        if (RenderBoxModelObject* continuation = this->continuation()) {
            if (!continuation->isInline() && continuation->parent())
                repaintRect.unite(continuation->rectWithOutlineForRepaint(repaintContainer, outlineSize));
        }
    }

    return repaintRect;
}

LayoutRect RenderInline::rectWithOutlineForRepaint(const RenderLayerModelObject* repaintContainer, LayoutUnit outlineWidth) const
{
    LayoutRect r(RenderBoxModelObject::rectWithOutlineForRepaint(repaintContainer, outlineWidth));
    for (auto& child : childrenOfType<RenderElement>(*this))
        r.unite(child.rectWithOutlineForRepaint(repaintContainer, outlineWidth));
    return r;
}

void RenderInline::computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect& rect, bool fixed) const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    // LayoutState is only valid for root-relative repainting
    if (view().layoutStateEnabled() && !repaintContainer) {
        LayoutState* layoutState = view().layoutState();
        if (style().hasInFlowPosition() && layer())
            rect.move(layer()->offsetForInFlowPosition());
        rect.move(layoutState->m_paintOffset);
        if (layoutState->m_clipped)
            rect.intersect(layoutState->m_clipRect);
        return;
    }

    if (repaintContainer == this)
        return;

    bool containerSkipped;
    RenderElement* container = this->container(repaintContainer, &containerSkipped);
    if (!container)
        return;

    LayoutPoint topLeft = rect.location();

    if (style().hasInFlowPosition() && layer()) {
        // Apply the in-flow position offset when invalidating a rectangle. The layer
        // is translated, but the render box isn't, so we need to do this to get the
        // right dirty rect. Since this is called from RenderObject::setStyle, the relative or sticky position
        // flag on the RenderObject has been cleared, so use the one on the style().
        topLeft += layer()->offsetForInFlowPosition();
    }
    
    // FIXME: We ignore the lightweight clipping rect that controls use, since if |o| is in mid-layout,
    // its controlClipRect will be wrong. For overflow clip we use the values cached by the layer.
    rect.setLocation(topLeft);
    if (container->hasOverflowClip()) {
        downcast<RenderBox>(*container).applyCachedClipAndScrollOffsetForRepaint(rect);
        if (rect.isEmpty())
            return;
    }

    if (containerSkipped) {
        // If the repaintContainer is below o, then we need to map the rect into repaintContainer's coordinates.
        LayoutSize containerOffset = repaintContainer->offsetFromAncestorContainer(*container);
        rect.move(-containerOffset);
        return;
    }
    
    container->computeRectForRepaint(repaintContainer, rect, fixed);
}

LayoutSize RenderInline::offsetFromContainer(RenderElement& container, const LayoutPoint&, bool* offsetDependsOnPoint) const
{
    ASSERT(&container == this->container());
    
    LayoutSize offset;    
    if (isInFlowPositioned())
        offset += offsetForInFlowPosition();

    if (is<RenderBox>(container))
        offset -= downcast<RenderBox>(container).scrolledContentOffset();

    if (offsetDependsOnPoint)
        *offsetDependsOnPoint = (is<RenderBox>(container) && container.style().isFlippedBlocksWritingMode()) || is<RenderFlowThread>(container);

    return offset;
}

void RenderInline::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    if (repaintContainer == this)
        return;

    if (view().layoutStateEnabled() && !repaintContainer) {
        LayoutState* layoutState = view().layoutState();
        LayoutSize offset = layoutState->m_paintOffset;
        if (style().hasInFlowPosition() && layer())
            offset += layer()->offsetForInFlowPosition();
        transformState.move(offset);
        return;
    }

    bool containerSkipped;
    RenderElement* container = this->container(repaintContainer, &containerSkipped);
    if (!container)
        return;

    if (mode & ApplyContainerFlip && is<RenderBox>(*container)) {
        if (container->style().isFlippedBlocksWritingMode()) {
            LayoutPoint centerPoint(transformState.mappedPoint());
            transformState.move(downcast<RenderBox>(*container).flipForWritingMode(centerPoint) - centerPoint);
        }
        mode &= ~ApplyContainerFlip;
    }

    LayoutSize containerOffset = offsetFromContainer(*container, LayoutPoint(transformState.mappedPoint()));

    bool preserve3D = mode & UseTransforms && (container->style().preserves3D() || style().preserves3D());
    if (mode & UseTransforms && shouldUseTransformFromContainer(container)) {
        TransformationMatrix t;
        getTransformFromContainer(container, containerOffset, t);
        transformState.applyTransform(t, preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
    } else
        transformState.move(containerOffset.width(), containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);

    if (containerSkipped) {
        // There can't be a transform between repaintContainer and o, because transforms create containers, so it should be safe
        // to just subtract the delta between the repaintContainer and o.
        LayoutSize containerOffset = repaintContainer->offsetFromAncestorContainer(*container);
        transformState.move(-containerOffset.width(), -containerOffset.height(), preserve3D ? TransformState::AccumulateTransform : TransformState::FlattenTransform);
        return;
    }

    container->mapLocalToContainer(repaintContainer, transformState, mode, wasFixed);
}

const RenderObject* RenderInline::pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    ASSERT(ancestorToStopAt != this);

    bool ancestorSkipped;
    RenderElement* container = this->container(ancestorToStopAt, &ancestorSkipped);
    if (!container)
        return nullptr;

    LayoutSize adjustmentForSkippedAncestor;
    if (ancestorSkipped) {
        // There can't be a transform between repaintContainer and o, because transforms create containers, so it should be safe
        // to just subtract the delta between the ancestor and o.
        adjustmentForSkippedAncestor = -ancestorToStopAt->offsetFromAncestorContainer(*container);
    }

    bool offsetDependsOnPoint = false;
    LayoutSize containerOffset = offsetFromContainer(*container, LayoutPoint(), &offsetDependsOnPoint);

    bool preserve3D = container->style().preserves3D() || style().preserves3D();
    if (shouldUseTransformFromContainer(container)) {
        TransformationMatrix t;
        getTransformFromContainer(container, containerOffset, t);
        t.translateRight(adjustmentForSkippedAncestor.width(), adjustmentForSkippedAncestor.height()); // FIXME: right?
        geometryMap.push(this, t, preserve3D, offsetDependsOnPoint);
    } else {
        containerOffset += adjustmentForSkippedAncestor;
        geometryMap.push(this, containerOffset, preserve3D, offsetDependsOnPoint);
    }
    
    return ancestorSkipped ? ancestorToStopAt : container;
}

void RenderInline::updateDragState(bool dragOn)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    RenderBoxModelObject::updateDragState(dragOn);
    if (RenderBoxModelObject* continuation = this->continuation())
        continuation->updateDragState(dragOn);
}

void RenderInline::childBecameNonInline(RenderElement& child)
{
    // We have to split the parent flow.
    RenderBlock* newBox = containingBlock()->createAnonymousBlock();
    RenderBoxModelObject* oldContinuation = continuation();
    setContinuation(newBox);
    RenderObject* beforeChild = child.nextSibling();
    removeChildInternal(child, NotifyChildren);
    splitFlow(beforeChild, newBox, &child, oldContinuation);
}

void RenderInline::updateHitTestResult(HitTestResult& result, const LayoutPoint& point)
{
    if (result.innerNode())
        return;

    LayoutPoint localPoint(point);
    if (Element* element = this->element()) {
        if (isInlineElementContinuation()) {
            // We're in the continuation of a split inline.  Adjust our local point to be in the coordinate space
            // of the principal renderer's containing block.  This will end up being the innerNonSharedNode.
            RenderBlock* firstBlock = element->renderer()->containingBlock();
            
            // Get our containing block.
            RenderBox* block = containingBlock();
            localPoint.moveBy(block->location() - firstBlock->locationOffset());
        }

        result.setInnerNode(element);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(element);
        result.setLocalPoint(localPoint);
    }
}

void RenderInline::dirtyLineBoxes(bool fullLayout)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    if (fullLayout) {
        m_lineBoxes.deleteLineBoxes();
        return;
    }

    if (!alwaysCreateLineBoxes()) {
        // We have to grovel into our children in order to dirty the appropriate lines.
        for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
            if (current->isFloatingOrOutOfFlowPositioned())
                continue;
            if (is<RenderBox>(*current) && !current->needsLayout()) {
                RenderBox& renderBox = downcast<RenderBox>(*current);
                if (renderBox.inlineBoxWrapper())
                    renderBox.inlineBoxWrapper()->root().markDirty();
            } else if (!current->selfNeedsLayout()) {
                if (is<RenderInline>(*current)) {
                    RenderInline& renderInline = downcast<RenderInline>(*current);
                    for (InlineFlowBox* childLine = renderInline.firstLineBox(); childLine; childLine = childLine->nextLineBox())
                        childLine->root().markDirty();
                } else if (is<RenderText>(*current)) {
                    RenderText& renderText = downcast<RenderText>(*current);
                    for (InlineTextBox* childText = renderText.firstTextBox(); childText; childText = childText->nextTextBox())
                        childText->root().markDirty();
                } else if (is<RenderLineBreak>(*current)) {
                    RenderLineBreak& renderBR = downcast<RenderLineBreak>(*current);
                    if (renderBR.inlineBoxWrapper())
                        renderBR.inlineBoxWrapper()->root().markDirty();
                }
            }
        }
    } else
        m_lineBoxes.dirtyLineBoxes();
}

void RenderInline::deleteLines()
{
    m_lineBoxes.deleteLineBoxTree();
}

std::unique_ptr<InlineFlowBox> RenderInline::createInlineFlowBox()
{
    return std::make_unique<InlineFlowBox>(*this);
}

InlineFlowBox* RenderInline::createAndAppendInlineFlowBox()
{
    setAlwaysCreateLineBoxes();
    auto newFlowBox = createInlineFlowBox();
    auto flowBox = newFlowBox.get();
    m_lineBoxes.appendLineBox(WTF::move(newFlowBox));
    return flowBox;
}

LayoutUnit RenderInline::lineHeight(bool firstLine, LineDirectionMode /*direction*/, LinePositionMode /*linePositionMode*/) const
{
    if (firstLine && document().styleSheetCollection().usesFirstLineRules()) {
        const RenderStyle& firstLineStyle = this->firstLineStyle();
        if (&firstLineStyle != &style())
            return firstLineStyle.computedLineHeight();
    }

    return style().computedLineHeight();
}

int RenderInline::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    const RenderStyle& style = firstLine ? firstLineStyle() : this->style();
    const FontMetrics& fontMetrics = style.fontMetrics();
    return fontMetrics.ascent(baselineType) + (lineHeight(firstLine, direction, linePositionMode) - fontMetrics.height()) / 2;
}

LayoutSize RenderInline::offsetForInFlowPositionedInline(const RenderBox* child) const
{
    // FIXME: This function isn't right with mixed writing modes.

    ASSERT(isInFlowPositioned());
    if (!isInFlowPositioned())
        return LayoutSize();

    // When we have an enclosing relpositioned inline, we need to add in the offset of the first line
    // box from the rest of the content, but only in the cases where we know we're positioned
    // relative to the inline itself.

    LayoutSize logicalOffset;
    LayoutUnit inlinePosition;
    LayoutUnit blockPosition;
    if (firstLineBox()) {
        inlinePosition = LayoutUnit::fromFloatRound(firstLineBox()->logicalLeft());
        blockPosition = firstLineBox()->logicalTop();
    } else {
        inlinePosition = layer()->staticInlinePosition();
        blockPosition = layer()->staticBlockPosition();
    }

    if (!child->style().hasStaticInlinePosition(style().isHorizontalWritingMode()))
        logicalOffset.setWidth(inlinePosition);

    // This is not terribly intuitive, but we have to match other browsers.  Despite being a block display type inside
    // an inline, we still keep our x locked to the left of the relative positioned inline.  Arguably the correct
    // behavior would be to go flush left to the block that contains the inline, but that isn't what other browsers
    // do.
    else if (!child->style().isOriginalDisplayInlineType())
        // Avoid adding in the left border/padding of the containing block twice.  Subtract it out.
        logicalOffset.setWidth(inlinePosition - child->containingBlock()->borderAndPaddingLogicalLeft());

    if (!child->style().hasStaticBlockPosition(style().isHorizontalWritingMode()))
        logicalOffset.setHeight(blockPosition);

    return style().isHorizontalWritingMode() ? logicalOffset : logicalOffset.transposedSize();
}

void RenderInline::imageChanged(WrappedImagePtr, const IntRect*)
{
    if (!parent())
        return;
        
    // FIXME: We can do better.
    repaint();
}

void RenderInline::addFocusRingRects(Vector<IntRect>& rects, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);

    {
        Vector<IntRect> tmpRects;
        LayoutPoint point;
        AbsoluteRectsGeneratorContext context(tmpRects, point);
        generateLineBoxRects(context);
        const size_t size = tmpRects.size();
        // Add transformed rects.
        for (size_t i = 0; i < size; i++)
            rects.append(roundedIntRect(localToContainerQuad(FloatRect(tmpRects[i]), paintContainer).boundingBox()));
    }
#else
    AbsoluteRectsGeneratorContext context(rects, additionalOffset);
    generateLineBoxRects(context);
#endif

    for (auto& child : childrenOfType<RenderElement>(*this)) {
        if (is<RenderListMarker>(child))
            continue;
        FloatPoint pos(additionalOffset);
#if PLATFORM(WKC) // get transformed position always.
        pos = child.localToContainerPoint(FloatPoint(), paintContainer);
#else
        // FIXME: This doesn't work correctly with transforms.
        if (child.hasLayer())
            pos = child.localToContainerPoint(FloatPoint(), paintContainer);
        else if (is<RenderBox>(child))
            pos.move(downcast<RenderBox>(child).locationOffset());
#endif
        child.addFocusRingRects(rects, flooredIntPoint(pos), paintContainer);
    }

    if (RenderBoxModelObject* continuation = this->continuation()) {
        if (continuation->isInline())
            continuation->addFocusRingRects(rects, flooredLayoutPoint(LayoutPoint(additionalOffset + continuation->containingBlock()->location() - containingBlock()->location())), paintContainer);
        else
            continuation->addFocusRingRects(rects, flooredLayoutPoint(LayoutPoint(additionalOffset + downcast<RenderBox>(*continuation).location() - containingBlock()->location())), paintContainer);
    }
}

void RenderInline::paintOutline(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    if (!hasOutline())
        return;

    RenderStyle& styleToUse = style();
    // Only paint the focus ring by hand if the theme isn't able to draw it.
    if (styleToUse.outlineStyleIsAuto() && !theme().supportsFocusRing(styleToUse))
        paintFocusRing(paintInfo, paintOffset, &styleToUse);

    if (hasOutlineAnnotation() && !styleToUse.outlineStyleIsAuto() && !theme().supportsFocusRing(styleToUse))
        addPDFURLRect(paintInfo, paintOffset);

    GraphicsContext* graphicsContext = paintInfo.context;
    if (graphicsContext->paintingDisabled())
        return;

    if (styleToUse.outlineStyleIsAuto() || styleToUse.outlineStyle() == BNONE)
        return;

    Vector<LayoutRect> rects;

    rects.append(LayoutRect());
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        const RootInlineBox& rootBox = curr->root();
        LayoutUnit top = std::max<LayoutUnit>(rootBox.lineTop(), curr->logicalTop());
        LayoutUnit bottom = std::min<LayoutUnit>(rootBox.lineBottom(), curr->logicalBottom());
        rects.append(LayoutRect(curr->x(), top, curr->logicalWidth(), bottom - top));
    }
    rects.append(LayoutRect());

    Color outlineColor = styleToUse.visitedDependentColor(CSSPropertyOutlineColor);
    bool useTransparencyLayer = outlineColor.hasAlpha();
    if (useTransparencyLayer) {
        graphicsContext->beginTransparencyLayer(static_cast<float>(outlineColor.alpha()) / 255);
        outlineColor = Color(outlineColor.red(), outlineColor.green(), outlineColor.blue());
    }

    for (unsigned i = 1; i < rects.size() - 1; i++)
        paintOutlineForLine(graphicsContext, paintOffset, rects.at(i - 1), rects.at(i), rects.at(i + 1), outlineColor);

    if (useTransparencyLayer)
        graphicsContext->endTransparencyLayer();
}

void RenderInline::paintOutlineForLine(GraphicsContext* graphicsContext, const LayoutPoint& paintOffset,
                                       const LayoutRect& lastline, const LayoutRect& thisline, const LayoutRect& nextline,
                                       const Color outlineColor)
{
    const RenderStyle& styleToUse = style();
    int outlineWidth = styleToUse.outlineWidth();
    EBorderStyle outlineStyle = styleToUse.outlineStyle();

    bool antialias = shouldAntialiasLines(graphicsContext);

    int offset = style().outlineOffset();

    LayoutRect box(LayoutPoint(paintOffset.x() + thisline.x() - offset, paintOffset.y() + thisline.y() - offset),
        LayoutSize(thisline.width() + offset, thisline.height() + offset));

    IntRect pixelSnappedBox = snappedIntRect(box);
    if (pixelSnappedBox.isEmpty())
        return;
    IntRect pixelSnappedLastLine = snappedIntRect(paintOffset.x() + lastline.x(), 0, lastline.width(), 0);
    IntRect pixelSnappedNextLine = snappedIntRect(paintOffset.x() + nextline.x(), 0, nextline.width(), 0);
    
    // left edge
    drawLineForBoxSide(graphicsContext,
        pixelSnappedBox.x() - outlineWidth,
        pixelSnappedBox.y() - (lastline.isEmpty() || thisline.x() < lastline.x() || (lastline.maxX() - 1) <= thisline.x() ? outlineWidth : 0),
        pixelSnappedBox.x(),
        pixelSnappedBox.maxY() + (nextline.isEmpty() || thisline.x() <= nextline.x() || (nextline.maxX() - 1) <= thisline.x() ? outlineWidth : 0),
        BSLeft,
        outlineColor, outlineStyle,
        (lastline.isEmpty() || thisline.x() < lastline.x() || (lastline.maxX() - 1) <= thisline.x() ? outlineWidth : -outlineWidth),
        (nextline.isEmpty() || thisline.x() <= nextline.x() || (nextline.maxX() - 1) <= thisline.x() ? outlineWidth : -outlineWidth),
        antialias);
    
    // right edge
    drawLineForBoxSide(graphicsContext,
        pixelSnappedBox.maxX(),
        pixelSnappedBox.y() - (lastline.isEmpty() || lastline.maxX() < thisline.maxX() || (thisline.maxX() - 1) <= lastline.x() ? outlineWidth : 0),
        pixelSnappedBox.maxX() + outlineWidth,
        pixelSnappedBox.maxY() + (nextline.isEmpty() || nextline.maxX() <= thisline.maxX() || (thisline.maxX() - 1) <= nextline.x() ? outlineWidth : 0),
        BSRight,
        outlineColor, outlineStyle,
        (lastline.isEmpty() || lastline.maxX() < thisline.maxX() || (thisline.maxX() - 1) <= lastline.x() ? outlineWidth : -outlineWidth),
        (nextline.isEmpty() || nextline.maxX() <= thisline.maxX() || (thisline.maxX() - 1) <= nextline.x() ? outlineWidth : -outlineWidth),
        antialias);
    // upper edge
    if (thisline.x() < lastline.x())
        drawLineForBoxSide(graphicsContext,
            pixelSnappedBox.x() - outlineWidth,
            pixelSnappedBox.y() - outlineWidth,
            std::min(pixelSnappedBox.maxX() + outlineWidth, (lastline.isEmpty() ? 1000000 : pixelSnappedLastLine.x())),
            pixelSnappedBox.y(),
            BSTop, outlineColor, outlineStyle,
            outlineWidth,
            (!lastline.isEmpty() && paintOffset.x() + lastline.x() + 1 < pixelSnappedBox.maxX() + outlineWidth) ? -outlineWidth : outlineWidth,
            antialias);
    
    if (lastline.maxX() < thisline.maxX())
        drawLineForBoxSide(graphicsContext,
            std::max(lastline.isEmpty() ? -1000000 : pixelSnappedLastLine.maxX(), pixelSnappedBox.x() - outlineWidth),
            pixelSnappedBox.y() - outlineWidth,
            pixelSnappedBox.maxX() + outlineWidth,
            pixelSnappedBox.y(),
            BSTop, outlineColor, outlineStyle,
            (!lastline.isEmpty() && pixelSnappedBox.x() - outlineWidth < paintOffset.x() + lastline.maxX()) ? -outlineWidth : outlineWidth,
            outlineWidth, antialias);

    if (thisline.x() == thisline.maxX())
          drawLineForBoxSide(graphicsContext,
            pixelSnappedBox.x() - outlineWidth,
            pixelSnappedBox.y() - outlineWidth,
            pixelSnappedBox.maxX() + outlineWidth,
            pixelSnappedBox.y(),
            BSTop, outlineColor, outlineStyle,
            outlineWidth,
            outlineWidth,
            antialias);

    // lower edge
    if (thisline.x() < nextline.x())
        drawLineForBoxSide(graphicsContext,
            pixelSnappedBox.x() - outlineWidth,
            pixelSnappedBox.maxY(),
            std::min(pixelSnappedBox.maxX() + outlineWidth, !nextline.isEmpty() ? pixelSnappedNextLine.x() + 1 : 1000000),
            pixelSnappedBox.maxY() + outlineWidth,
            BSBottom, outlineColor, outlineStyle,
            outlineWidth,
            (!nextline.isEmpty() && paintOffset.x() + nextline.x() + 1 < pixelSnappedBox.maxX() + outlineWidth) ? -outlineWidth : outlineWidth,
            antialias);
    
    if (nextline.maxX() < thisline.maxX())
        drawLineForBoxSide(graphicsContext,
            std::max(!nextline.isEmpty() ? pixelSnappedNextLine.maxX() : -1000000, pixelSnappedBox.x() - outlineWidth),
            pixelSnappedBox.maxY(),
            pixelSnappedBox.maxX() + outlineWidth,
            pixelSnappedBox.maxY() + outlineWidth,
            BSBottom, outlineColor, outlineStyle,
            (!nextline.isEmpty() && pixelSnappedBox.x() - outlineWidth < paintOffset.x() + nextline.maxX()) ? -outlineWidth : outlineWidth,
            outlineWidth, antialias);

    if (thisline.x() == thisline.maxX())
          drawLineForBoxSide(graphicsContext,
            pixelSnappedBox.x() - outlineWidth,
            pixelSnappedBox.maxY(),
            pixelSnappedBox.maxX() + outlineWidth,
            pixelSnappedBox.maxY() + outlineWidth,
            BSBottom, outlineColor, outlineStyle,
            outlineWidth,
            outlineWidth,
            antialias);
}

#if ENABLE(DASHBOARD_SUPPORT)
void RenderInline::addAnnotatedRegions(Vector<AnnotatedRegionValue>& regions)
{
    // Convert the style regions to absolute coordinates.
    if (style().visibility() != VISIBLE)
        return;

    const Vector<StyleDashboardRegion>& styleRegions = style().dashboardRegions();
    unsigned i, count = styleRegions.size();
    for (i = 0; i < count; i++) {
        StyleDashboardRegion styleRegion = styleRegions[i];

        LayoutRect linesBoundingBox = this->linesBoundingBox();
        LayoutUnit w = linesBoundingBox.width();
        LayoutUnit h = linesBoundingBox.height();

        AnnotatedRegionValue region;
        region.label = styleRegion.label;
        region.bounds = LayoutRect(linesBoundingBox.x() + styleRegion.offset.left().value(),
                                linesBoundingBox.y() + styleRegion.offset.top().value(),
                                w - styleRegion.offset.left().value() - styleRegion.offset.right().value(),
                                h - styleRegion.offset.top().value() - styleRegion.offset.bottom().value());
        region.type = styleRegion.type;

        RenderObject* container = containingBlock();
        if (!container)
            container = this;

        region.clip = region.bounds;
        container->computeAbsoluteRepaintRect(region.clip);
        if (region.clip.height() < 0) {
            region.clip.setHeight(0);
            region.clip.setWidth(0);
        }

        FloatPoint absPos = container->localToAbsolute();
        region.bounds.setX(absPos.x() + region.bounds.x());
        region.bounds.setY(absPos.y() + region.bounds.y());

        regions.append(region);
    }
}
#endif

} // namespace WebCore
