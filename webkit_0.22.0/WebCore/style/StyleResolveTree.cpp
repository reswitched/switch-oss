/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2004-2010, 2012-2014 Apple Inc. All rights reserved.
 *           (C) 2007 Eric Seidel (eric@webkit.org)
 * Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"
#include "StyleResolveTree.h"

#include "AXObjectCache.h"
#include "AnimationController.h"
#include "CSSFontSelector.h"
#include "ElementIterator.h"
#include "ElementRareData.h"
#include "FlowThreadController.h"
#include "InsertionPoint.h"
#include "InspectorInstrumentation.h"
#include "LoaderStrategy.h"
#include "MainFrame.h"
#include "NodeRenderStyle.h"
#include "NodeRenderingTraversal.h"
#include "NodeTraversal.h"
#include "PlatformStrategies.h"
#include "RenderFullScreen.h"
#include "RenderNamedFlowThread.h"
#include "RenderText.h"
#include "RenderTreePosition.h"
#include "RenderWidget.h"
#include "ResourceLoadScheduler.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleResolveForDocument.h"
#include "StyleResolver.h"
#include "Text.h"

#if PLATFORM(IOS)
#include "CSSFontSelector.h"
#include "WKContentObservation.h"
#endif

namespace WebCore {

namespace Style {

enum DetachType { NormalDetach, ReattachDetach };

static void attachRenderTree(Element&, RenderStyle& inheritedStyle, RenderTreePosition&, PassRefPtr<RenderStyle>);
static void attachTextRenderer(Text&, RenderTreePosition&);
static void detachRenderTree(Element&, DetachType);
static void resolveTextNode(Text&, RenderTreePosition&);
static void resolveTree(Element&, RenderStyle& inheritedStyle, RenderTreePosition&, Change);

Change determineChange(const RenderStyle& s1, const RenderStyle& s2)
{
    if (s1.display() != s2.display())
        return Detach;
    if (s1.hasPseudoStyle(FIRST_LETTER) != s2.hasPseudoStyle(FIRST_LETTER))
        return Detach;
    // We just detach if a renderer acquires or loses a column-span, since spanning elements
    // typically won't contain much content.
    if (s1.columnSpan() != s2.columnSpan())
        return Detach;
    if (!s1.contentDataEquivalent(&s2))
        return Detach;
    // When text-combine property has been changed, we need to prepare a separate renderer object.
    // When text-combine is on, we use RenderCombineText, otherwise RenderText.
    // https://bugs.webkit.org/show_bug.cgi?id=55069
    if (s1.hasTextCombine() != s2.hasTextCombine())
        return Detach;
    // We need to reattach the node, so that it is moved to the correct RenderFlowThread.
    if (s1.flowThread() != s2.flowThread())
        return Detach;
    // When the region thread has changed, we need to prepare a separate render region object.
    if (s1.regionThread() != s2.regionThread())
        return Detach;
    // FIXME: Multicolumn regions not yet supported (http://dev.w3.org/csswg/css-regions/#multi-column-regions)
    // When the node has region style and changed its multicol style, we have to prepare
    // a separate render region object.
    if (s1.hasFlowFrom() && (s1.specifiesColumns() != s2.specifiesColumns()))
        return Detach;
    if (s1.alignItems() != s2.alignItems())
        return Detach;

    if (s1 != s2) {
        if (s1.inheritedNotEqual(&s2))
            return Inherit;
        if (s1.hasExplicitlyInheritedProperties() || s2.hasExplicitlyInheritedProperties())
            return Inherit;

        return NoInherit;
    }
    // If the pseudoStyles have changed, we want any StyleChange that is not NoChange
    // because setStyle will do the right thing with anything else.
    if (s1.hasAnyPublicPseudoStyles()) {
        for (PseudoId pseudoId = FIRST_PUBLIC_PSEUDOID; pseudoId < FIRST_INTERNAL_PSEUDOID; pseudoId = static_cast<PseudoId>(pseudoId + 1)) {
            if (s1.hasPseudoStyle(pseudoId)) {
                RenderStyle* ps2 = s2.getCachedPseudoStyle(pseudoId);
                if (!ps2)
                    return NoInherit;
                RenderStyle* ps1 = s1.getCachedPseudoStyle(pseudoId);
                if (!ps1 || *ps1 != *ps2)
                    return NoInherit;
            }
        }
    }

    return NoChange;
}

static bool shouldCreateRenderer(const Element& element, const RenderElement& parentRenderer)
{
    if (!element.document().shouldCreateRenderers())
        return false;
    if (!parentRenderer.canHaveChildren() && !(element.isPseudoElement() && parentRenderer.canHaveGeneratedChildren()))
        return false;
    if (parentRenderer.element() && !parentRenderer.element()->childShouldCreateRenderer(element))
        return false;
    return true;
}

static Ref<RenderStyle> styleForElement(Element& element, RenderStyle& inheritedStyle)
{
    if (element.hasCustomStyleResolveCallbacks()) {
        if (RefPtr<RenderStyle> style = element.customStyleForRenderer(inheritedStyle))
            return style.releaseNonNull();
    }
    return element.document().ensureStyleResolver().styleForElement(&element, &inheritedStyle);
}

#if ENABLE(CSS_REGIONS)
static RenderNamedFlowThread* moveToFlowThreadIfNeeded(Element& element, const RenderStyle& style)
{
    if (!element.shouldMoveToFlowThread(style))
        return 0;

    FlowThreadController& flowThreadController = element.document().renderView()->flowThreadController();
    RenderNamedFlowThread& parentFlowRenderer = flowThreadController.ensureRenderFlowThreadWithName(style.flowThread());
    flowThreadController.registerNamedFlowContentElement(element, parentFlowRenderer);
    return &parentFlowRenderer;
}
#endif

static void createRendererIfNeeded(Element& element, RenderStyle& inheritedStyle, RenderTreePosition& renderTreePosition, PassRefPtr<RenderStyle> resolvedStyle)
{
    ASSERT(!element.renderer());

    RefPtr<RenderStyle> style = resolvedStyle;

    if (!shouldCreateRenderer(element, renderTreePosition.parent()))
        return;

    if (!style)
        style = styleForElement(element, inheritedStyle);

    RenderNamedFlowThread* parentFlowRenderer = 0;
#if ENABLE(CSS_REGIONS)
    parentFlowRenderer = moveToFlowThreadIfNeeded(element, *style);
#endif

    if (!element.rendererIsNeeded(*style))
        return;

    renderTreePosition.computeNextSibling(element);

    RenderTreePosition insertionPosition = parentFlowRenderer
        ? RenderTreePosition(*parentFlowRenderer, parentFlowRenderer->nextRendererForElement(element))
        : renderTreePosition;

    RenderElement* newRenderer = element.createElementRenderer(style.releaseNonNull(), insertionPosition).leakPtr();
    if (!newRenderer)
        return;
    if (!insertionPosition.canInsert(*newRenderer)) {
        newRenderer->destroy();
        return;
    }

    // Make sure the RenderObject already knows it is going to be added to a RenderFlowThread before we set the style
    // for the first time. Otherwise code using inRenderFlowThread() in the styleWillChange and styleDidChange will fail.
    newRenderer->setFlowThreadState(insertionPosition.parent().flowThreadState());

    // Code below updateAnimations() can depend on Element::renderer() already being set.
    element.setRenderer(newRenderer);

    // FIXME: There's probably a better way to factor this.
    // This just does what setAnimatedStyle() does, except with setStyleInternal() instead of setStyle().
    Ref<RenderStyle> animatedStyle = newRenderer->style();
    newRenderer->animation().updateAnimations(*newRenderer, animatedStyle, animatedStyle);
    newRenderer->setStyleInternal(WTF::move(animatedStyle));

    newRenderer->initializeStyle();

#if ENABLE(FULLSCREEN_API)
    Document& document = element.document();
    if (document.webkitIsFullScreen() && document.webkitCurrentFullScreenElement() == &element) {
        newRenderer = RenderFullScreen::wrapRenderer(newRenderer, &insertionPosition.parent(), document);
        if (!newRenderer)
            return;
    }
#endif
    // Note: Adding newRenderer instead of renderer(). renderer() may be a child of newRenderer.
    insertionPosition.insert(*newRenderer);
}

static void invalidateWhitespaceOnlyTextSiblingsAfterAttachIfNeeded(Node& current)
{
    if (is<InsertionPoint>(current))
        return;
    // This function finds sibling text renderers where the results of textRendererIsNeeded may have changed as a result of
    // the current node gaining or losing the renderer. This can only affect white space text nodes.
    for (Node* sibling = current.nextSibling(); sibling; sibling = sibling->nextSibling()) {
        if (sibling->needsStyleRecalc())
            return;
        if (is<Element>(*sibling)) {
            // Text renderers beyond rendered elements can't be affected.
            if (!sibling->renderer() || RenderTreePosition::isRendererReparented(*sibling->renderer()))
                continue;
            return;
        }
        if (!is<Text>(*sibling))
            continue;
        Text& textSibling = downcast<Text>(*sibling);
        if (!textSibling.containsOnlyWhitespace())
            continue;
        textSibling.setNeedsStyleRecalc();
    }
}

static bool textRendererIsNeeded(const Text& textNode, const RenderTreePosition& renderTreePosition)
{
    const RenderElement& parentRenderer = renderTreePosition.parent();
    if (!parentRenderer.canHaveChildren())
        return false;
    if (parentRenderer.element() && !parentRenderer.element()->childShouldCreateRenderer(textNode))
        return false;
    if (textNode.isEditingText())
        return true;
    if (!textNode.length())
        return false;
    if (!textNode.containsOnlyWhitespace())
        return true;
    // This text node has nothing but white space. We may still need a renderer in some cases.
    if (parentRenderer.isTable() || parentRenderer.isTableRow() || parentRenderer.isTableSection() || parentRenderer.isRenderTableCol() || parentRenderer.isFrameSet())
        return false;
    if (parentRenderer.style().preserveNewline()) // pre/pre-wrap/pre-line always make renderers.
        return true;

    RenderObject* previousRenderer = RenderTreePosition::previousSiblingRenderer(textNode);
    if (previousRenderer && previousRenderer->isBR()) // <span><br/> <br/></span>
        return false;
        
    if (parentRenderer.isRenderInline()) {
        // <span><div/> <div/></span>
        if (previousRenderer && !previousRenderer->isInline())
            return false;
    } else {
        if (parentRenderer.isRenderBlock() && !parentRenderer.childrenInline() && (!previousRenderer || !previousRenderer->isInline()))
            return false;
        
        RenderObject* first = parentRenderer.firstChild();
        while (first && first->isFloatingOrOutOfFlowPositioned())
            first = first->nextSibling();
        RenderObject* nextRenderer = RenderTreePosition::nextSiblingRenderer(textNode, parentRenderer);
        if (!first || nextRenderer == first) {
            // Whitespace at the start of a block just goes away. Don't even make a render object for this text.
            return false;
        }
    }
    return true;
}

static void createTextRendererIfNeeded(Text& textNode, RenderTreePosition& renderTreePosition)
{
    ASSERT(!textNode.renderer());

    if (!textRendererIsNeeded(textNode, renderTreePosition))
        return;

    auto newRenderer = textNode.createTextRenderer(renderTreePosition.parent().style());
    ASSERT(newRenderer);

    renderTreePosition.computeNextSibling(textNode);

    if (!renderTreePosition.canInsert(*newRenderer))
        return;

    // Make sure the RenderObject already knows it is going to be added to a RenderFlowThread before we set the style
    // for the first time. Otherwise code using inRenderFlowThread() in the styleWillChange and styleDidChange will fail.
    newRenderer->setFlowThreadState(renderTreePosition.parent().flowThreadState());

    textNode.setRenderer(newRenderer.get());
    // Parent takes care of the animations, no need to call setAnimatableStyle.
    renderTreePosition.insert(*newRenderer.leakPtr());
}

void attachTextRenderer(Text& textNode, RenderTreePosition& renderTreePosition)
{
    createTextRendererIfNeeded(textNode, renderTreePosition);

    textNode.clearNeedsStyleRecalc();
}

void detachTextRenderer(Text& textNode)
{
    if (textNode.renderer())
        textNode.renderer()->destroyAndCleanupAnonymousWrappers();
    textNode.setRenderer(0);
}

void updateTextRendererAfterContentChange(Text& textNode, unsigned offsetOfReplacedData, unsigned lengthOfReplacedData)
{
    ContainerNode* renderingParentNode = NodeRenderingTraversal::parent(&textNode);
    if (!renderingParentNode || !renderingParentNode->renderer())
        return;

    bool hadRenderer = textNode.renderer();

    RenderTreePosition renderTreePosition(*renderingParentNode->renderer());
    resolveTextNode(textNode, renderTreePosition);

    if (hadRenderer && textNode.renderer())
        textNode.renderer()->setTextWithOffset(textNode.data(), offsetOfReplacedData, lengthOfReplacedData);
}

#if PLATFORM(WKC)
static unsigned int renderTreeDepth(RenderTreePosition& position)
{
    unsigned int depth = 0;
    RenderElement& firstelement = position.parent();

    if (firstelement.isRoot())
        return depth;

    RenderElement* element = firstelement.parent();
    depth += 1;

    while (element && !element->isRoot()) {
        depth += 1;
        element = element->parent();
    }

    return depth;
}
#endif

static void attachChildren(ContainerNode& current, RenderStyle& inheritedStyle, RenderTreePosition& renderTreePosition)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    for (Node* child = current.firstChild(); child; child = child->nextSibling()) {
        ASSERT((!child->renderer() || child->isNamedFlowContentNode()) || current.shadowRoot());
#if PLATFORM(WKC)
        if (renderTreeDepth(renderTreePosition) > Settings::defaultMaximumRenderTreeDepth) {
            current.clearNeedsStyleRecalc();
            current.clearChildNeedsStyleRecalc();
            return;
        }
#endif
        if (child->renderer()) {
            renderTreePosition.invalidateNextSibling(*child->renderer());
            continue;
        }
        if (is<Text>(*child)) {
            attachTextRenderer(downcast<Text>(*child), renderTreePosition);
            continue;
        }
        if (is<Element>(*child))
            attachRenderTree(downcast<Element>(*child), inheritedStyle, renderTreePosition, nullptr);
    }
}

static void attachDistributedChildren(InsertionPoint& insertionPoint, RenderStyle& inheritedStyle, RenderTreePosition& renderTreePosition)
{
    if (ShadowRoot* shadowRoot = insertionPoint.containingShadowRoot())
        ContentDistributor::ensureDistribution(shadowRoot);

    for (Node* current = insertionPoint.firstDistributed(); current; current = insertionPoint.nextDistributedTo(current)) {
        if (current->renderer())
            renderTreePosition.invalidateNextSibling(*current->renderer());
        if (is<Text>(*current)) {
            if (current->renderer())
                continue;
            attachTextRenderer(downcast<Text>(*current), renderTreePosition);
            continue;
        }
        if (is<Element>(*current)) {
            Element& currentElement = downcast<Element>(*current);
            if (currentElement.renderer())
                detachRenderTree(currentElement);
            attachRenderTree(currentElement, inheritedStyle, renderTreePosition, nullptr);
        }
    }
    // Use actual children as fallback content.
    if (!insertionPoint.hasDistribution())
        attachChildren(insertionPoint, inheritedStyle, renderTreePosition);
}

static void attachShadowRoot(ShadowRoot& shadowRoot)
{
    ASSERT(shadowRoot.hostElement());
    ASSERT(shadowRoot.hostElement()->renderer());

    auto& renderer = *shadowRoot.hostElement()->renderer();
    RenderTreePosition renderTreePosition(renderer);
    attachChildren(shadowRoot, renderer.style(), renderTreePosition);

    shadowRoot.clearNeedsStyleRecalc();
    shadowRoot.clearChildNeedsStyleRecalc();
}

static PseudoElement* beforeOrAfterPseudoElement(Element& current, PseudoId pseudoId)
{
    ASSERT(pseudoId == BEFORE || pseudoId == AFTER);
    if (pseudoId == BEFORE)
        return current.beforePseudoElement();
    return current.afterPseudoElement();
}

static void setBeforeOrAfterPseudoElement(Element& current, Ref<PseudoElement>&& pseudoElement, PseudoId pseudoId)
{
    ASSERT(pseudoId == BEFORE || pseudoId == AFTER);
    if (pseudoId == BEFORE) {
        current.setBeforePseudoElement(WTF::move(pseudoElement));
        return;
    }
    current.setAfterPseudoElement(WTF::move(pseudoElement));
}

static void clearBeforeOrAfterPseudoElement(Element& current, PseudoId pseudoId)
{
    ASSERT(pseudoId == BEFORE || pseudoId == AFTER);
    if (pseudoId == BEFORE) {
        current.clearBeforePseudoElement();
        return;
    }
    current.clearAfterPseudoElement();
}

static void resetStyleForNonRenderedDescendants(Element& current)
{
    ASSERT(!current.renderer());
    bool elementNeedingStyleRecalcAffectsNextSiblingElementStyle = false;
    for (auto& child : childrenOfType<Element>(current)) {
        ASSERT(!child.renderer());
        if (elementNeedingStyleRecalcAffectsNextSiblingElementStyle) {
            if (child.styleIsAffectedByPreviousSibling())
                child.setNeedsStyleRecalc();
            elementNeedingStyleRecalcAffectsNextSiblingElementStyle = child.affectsNextSiblingElementStyle();
        }

        if (child.needsStyleRecalc()) {
            child.resetComputedStyle();
            child.clearNeedsStyleRecalc();
            elementNeedingStyleRecalcAffectsNextSiblingElementStyle = child.affectsNextSiblingElementStyle();
        }

        if (child.childNeedsStyleRecalc()) {
            resetStyleForNonRenderedDescendants(child);
            child.clearChildNeedsStyleRecalc();
        }
    }
}

static bool needsPseudoElement(Element& current, PseudoId pseudoId)
{
    if (!current.renderer() || !current.renderer()->canHaveGeneratedChildren())
        return false;
    if (current.isPseudoElement())
        return false;
    if (!pseudoElementRendererIsNeeded(current.renderer()->getCachedPseudoStyle(pseudoId)))
        return false;
    return true;
}

static void attachBeforeOrAfterPseudoElementIfNeeded(Element& current, PseudoId pseudoId, RenderTreePosition& renderTreePosition)
{
    if (!needsPseudoElement(current, pseudoId))
        return;
    Ref<PseudoElement> pseudoElement = PseudoElement::create(current, pseudoId);
    InspectorInstrumentation::pseudoElementCreated(pseudoElement->document().page(), pseudoElement.get());
    setBeforeOrAfterPseudoElement(current, pseudoElement.copyRef(), pseudoId);
    attachRenderTree(pseudoElement.get(), *current.renderStyle(), renderTreePosition, nullptr);
}

static void attachRenderTree(Element& current, RenderStyle& inheritedStyle, RenderTreePosition& renderTreePosition, PassRefPtr<RenderStyle> resolvedStyle)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    PostResolutionCallbackDisabler callbackDisabler(current.document());
    WidgetHierarchyUpdatesSuspensionScope suspendWidgetHierarchyUpdates;

    if (is<InsertionPoint>(current)) {
        attachDistributedChildren(downcast<InsertionPoint>(current), inheritedStyle, renderTreePosition);
        current.clearNeedsStyleRecalc();
        current.clearChildNeedsStyleRecalc();
        return;
    }

    if (current.hasCustomStyleResolveCallbacks())
        current.willAttachRenderers();

    createRendererIfNeeded(current, inheritedStyle, renderTreePosition, resolvedStyle);

    if (auto* renderer = current.renderer()) {
        StyleResolverParentPusher parentPusher(&current);

        RenderTreePosition childRenderTreePosition(*renderer);
        attachBeforeOrAfterPseudoElementIfNeeded(current, BEFORE, childRenderTreePosition);

        if (ShadowRoot* shadowRoot = current.shadowRoot()) {
            parentPusher.push();
            attachShadowRoot(*shadowRoot);
        } else if (current.firstChild())
            parentPusher.push();

        attachChildren(current, renderer->style(), childRenderTreePosition);

        if (AXObjectCache* cache = current.document().axObjectCache())
            cache->updateCacheAfterNodeIsAttached(&current);

        attachBeforeOrAfterPseudoElementIfNeeded(current, AFTER, childRenderTreePosition);

        current.updateFocusAppearanceAfterAttachIfNeeded();
    } else
        resetStyleForNonRenderedDescendants(current);

    current.clearNeedsStyleRecalc();
    current.clearChildNeedsStyleRecalc();

    if (current.hasCustomStyleResolveCallbacks())
        current.didAttachRenderers();
}

static void detachDistributedChildren(InsertionPoint& insertionPoint)
{
    for (Node* current = insertionPoint.firstDistributed(); current; current = insertionPoint.nextDistributedTo(current)) {
        if (is<Text>(*current)) {
            detachTextRenderer(downcast<Text>(*current));
            continue;
        }
        if (is<Element>(*current))
            detachRenderTree(downcast<Element>(*current));
    }
}

static void detachChildren(ContainerNode& current, DetachType detachType)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    if (is<InsertionPoint>(current))
        detachDistributedChildren(downcast<InsertionPoint>(current));

    for (Node* child = current.firstChild(); child; child = child->nextSibling()) {
        if (is<Text>(*child)) {
            Style::detachTextRenderer(downcast<Text>(*child));
            continue;
        }
        if (is<Element>(*child))
            detachRenderTree(downcast<Element>(*child), detachType);
    }
    current.clearChildNeedsStyleRecalc();
}

static void detachShadowRoot(ShadowRoot& shadowRoot, DetachType detachType)
{
    detachChildren(shadowRoot, detachType);
}

static void detachRenderTree(Element& current, DetachType detachType)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    WidgetHierarchyUpdatesSuspensionScope suspendWidgetHierarchyUpdates;

    if (current.hasCustomStyleResolveCallbacks())
        current.willDetachRenderers();

    current.clearStyleDerivedDataBeforeDetachingRenderer();

    // Do not remove the element's hovered and active status
    // if performing a reattach.
    if (detachType != ReattachDetach)
        current.clearHoverAndActiveStatusBeforeDetachingRenderer();

    if (ShadowRoot* shadowRoot = current.shadowRoot())
        detachShadowRoot(*shadowRoot, detachType);

    detachChildren(current, detachType);

    if (current.renderer())
        current.renderer()->destroyAndCleanupAnonymousWrappers();
    current.setRenderer(0);

    if (current.hasCustomStyleResolveCallbacks())
        current.didDetachRenderers();
}

static bool pseudoStyleCacheIsInvalid(RenderElement* renderer, RenderStyle* newStyle)
{
    const RenderStyle& currentStyle = renderer->style();

    const PseudoStyleCache* pseudoStyleCache = currentStyle.cachedPseudoStyles();
    if (!pseudoStyleCache)
        return false;

    for (auto& cache : *pseudoStyleCache) {
        RefPtr<RenderStyle> newPseudoStyle;
        PseudoId pseudoId = cache->styleType();
        if (pseudoId == FIRST_LINE || pseudoId == FIRST_LINE_INHERITED)
            newPseudoStyle = renderer->uncachedFirstLineStyle(newStyle);
        else
            newPseudoStyle = renderer->getUncachedPseudoStyle(PseudoStyleRequest(pseudoId), newStyle, newStyle);
        if (!newPseudoStyle)
            return true;
        if (*newPseudoStyle != *cache) {
            if (pseudoId < FIRST_INTERNAL_PSEUDOID)
                newStyle->setHasPseudoStyle(pseudoId);
            newStyle->addCachedPseudoStyle(newPseudoStyle);
            if (pseudoId == FIRST_LINE || pseudoId == FIRST_LINE_INHERITED) {
                // FIXME: We should do an actual diff to determine whether a repaint vs. layout
                // is needed, but for now just assume a layout will be required. The diff code
                // in RenderObject::setStyle would need to be factored out so that it could be reused.
                renderer->setNeedsLayoutAndPrefWidthsRecalc();
            }
            return true;
        }
    }
    return false;
}

static Change resolveLocal(Element& current, RenderStyle& inheritedStyle, RenderTreePosition& renderTreePosition, Change inheritedChange)
{
    Change localChange = Detach;
    RefPtr<RenderStyle> newStyle;
    RefPtr<RenderStyle> currentStyle = current.renderStyle();

    Document& document = current.document();
    if (currentStyle && current.styleChangeType() != ReconstructRenderTree) {
        Ref<RenderStyle> style(styleForElement(current, inheritedStyle));
        newStyle = style.ptr();
        localChange = determineChange(*currentStyle, style);
    }
    if (localChange == Detach) {
        if (current.renderer() || current.isNamedFlowContentNode())
            detachRenderTree(current, ReattachDetach);
        attachRenderTree(current, inheritedStyle, renderTreePosition, newStyle.release());
        invalidateWhitespaceOnlyTextSiblingsAfterAttachIfNeeded(current);

        return Detach;
    }

    if (RenderElement* renderer = current.renderer()) {
        if (localChange != NoChange || pseudoStyleCacheIsInvalid(renderer, newStyle.get()) || (inheritedChange == Force && renderer->requiresForcedStyleRecalcPropagation()) || current.styleChangeType() == SyntheticStyleChange)
            renderer->setAnimatableStyle(*newStyle, current.styleChangeType() == SyntheticStyleChange ? StyleDifferenceRecompositeLayer : StyleDifferenceEqual);
        else if (current.needsStyleRecalc()) {
            // Although no change occurred, we use the new style so that the cousin style sharing code won't get
            // fooled into believing this style is the same.
            renderer->setStyleInternal(*newStyle);
        }
    }

    // If "rem" units are used anywhere in the document, and if the document element's font size changes, then force font updating
    // all the way down the tree. This is simpler than having to maintain a cache of objects (and such font size changes should be rare anyway).
    if (document.styleSheetCollection().usesRemUnits() && document.documentElement() == &current && localChange != NoChange && currentStyle && newStyle && currentStyle->fontSize() != newStyle->fontSize()) {
        // Cached RenderStyles may depend on the re units.
        if (StyleResolver* styleResolver = document.styleResolverIfExists())
            styleResolver->invalidateMatchedPropertiesCache();
        return Force;
    }
    if (inheritedChange == Force)
        return Force;
    if (current.styleChangeType() >= FullStyleChange)
        return Force;

    return localChange;
}

void resolveTextNode(Text& text, RenderTreePosition& renderTreePosition)
{
    text.clearNeedsStyleRecalc();

    bool hasRenderer = text.renderer();
    bool needsRenderer = textRendererIsNeeded(text, renderTreePosition);
    if (hasRenderer) {
        if (needsRenderer)
            return;
        detachTextRenderer(text);
        invalidateWhitespaceOnlyTextSiblingsAfterAttachIfNeeded(text);
        return;
    }
    if (!needsRenderer)
        return;
    attachTextRenderer(text, renderTreePosition);
    invalidateWhitespaceOnlyTextSiblingsAfterAttachIfNeeded(text);
}

static void resolveShadowTree(ShadowRoot& shadowRoot, Element& host, Style::Change change)
{
    ASSERT(shadowRoot.hostElement() == &host);
    ASSERT(host.renderer());
    RenderTreePosition renderTreePosition(*host.renderer());
    for (Node* child = shadowRoot.firstChild(); child; child = child->nextSibling()) {
        if (child->renderer())
            renderTreePosition.invalidateNextSibling(*child->renderer());
        if (is<Text>(*child) && child->needsStyleRecalc()) {
            resolveTextNode(downcast<Text>(*child), renderTreePosition);
            continue;
        }
        if (is<Element>(*child))
            resolveTree(downcast<Element>(*child), host.renderer()->style(), renderTreePosition, change);
    }

    shadowRoot.clearNeedsStyleRecalc();
    shadowRoot.clearChildNeedsStyleRecalc();
}

static void updateBeforeOrAfterPseudoElement(Element& current, Change change, PseudoId pseudoId, RenderTreePosition& renderTreePosition)
{
    ASSERT(current.renderer());
    if (PseudoElement* existingPseudoElement = beforeOrAfterPseudoElement(current, pseudoId)) {
        if (existingPseudoElement->renderer())
            renderTreePosition.invalidateNextSibling(*existingPseudoElement->renderer());

        if (needsPseudoElement(current, pseudoId))
            resolveTree(*existingPseudoElement, current.renderer()->style(), renderTreePosition, current.needsStyleRecalc() ? Force : change);
        else
            clearBeforeOrAfterPseudoElement(current, pseudoId);
        return;
    }
    attachBeforeOrAfterPseudoElementIfNeeded(current, pseudoId, renderTreePosition);
}

#if PLATFORM(IOS)
static EVisibility elementImplicitVisibility(const Element* element)
{
    RenderObject* renderer = element->renderer();
    if (!renderer)
        return VISIBLE;

    RenderStyle& style = renderer->style();

    Length width(style.width());
    Length height(style.height());
    if ((width.isFixed() && width.value() <= 0) || (height.isFixed() && height.value() <= 0))
        return HIDDEN;

    Length top(style.top());
    Length left(style.left());
    if (left.isFixed() && width.isFixed() && -left.value() >= width.value())
        return HIDDEN;

    if (top.isFixed() && height.isFixed() && -top.value() >= height.value())
        return HIDDEN;
    return VISIBLE;
}

class CheckForVisibilityChangeOnRecalcStyle {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    CheckForVisibilityChangeOnRecalcStyle(Element* element, RenderStyle* currentStyle)
        : m_element(element)
        , m_previousDisplay(currentStyle ? currentStyle->display() : NONE)
        , m_previousVisibility(currentStyle ? currentStyle->visibility() : HIDDEN)
        , m_previousImplicitVisibility(WKObservingContentChanges() && WKContentChange() != WKContentVisibilityChange ? elementImplicitVisibility(element) : VISIBLE)
    {
    }
    ~CheckForVisibilityChangeOnRecalcStyle()
    {
        if (!WKObservingContentChanges())
            return;
        RenderStyle* style = m_element->renderStyle();
        if (!style)
            return;
        if ((m_previousDisplay == NONE && style->display() != NONE) || (m_previousVisibility == HIDDEN && style->visibility() != HIDDEN)
            || (m_previousImplicitVisibility == HIDDEN && elementImplicitVisibility(m_element.get()) == VISIBLE))
            WKSetObservedContentChange(WKContentVisibilityChange);
    }
private:
    RefPtr<Element> m_element;
    EDisplay m_previousDisplay;
    EVisibility m_previousVisibility;
    EVisibility m_previousImplicitVisibility;
};
#endif // PLATFORM(IOS)

void resolveTree(Element& current, RenderStyle& inheritedStyle, RenderTreePosition& renderTreePosition, Change change)
{
    ASSERT(change != Detach);

    if (is<InsertionPoint>(current)) {
        current.clearNeedsStyleRecalc();
        current.clearChildNeedsStyleRecalc();
        return;
    }

    if (current.hasCustomStyleResolveCallbacks()) {
        if (!current.willRecalcStyle(change))
            return;
    }

#if PLATFORM(IOS)
    CheckForVisibilityChangeOnRecalcStyle checkForVisibilityChange(&current, current.renderStyle());
#endif

    if (change > NoChange || current.needsStyleRecalc())
        current.resetComputedStyle();

    if (change >= Inherit || current.needsStyleRecalc())
        change = resolveLocal(current, inheritedStyle, renderTreePosition, change);

    auto* renderer = current.renderer();

    if (change != Detach && renderer) {
        StyleResolverParentPusher parentPusher(&current);

        if (ShadowRoot* shadowRoot = current.shadowRoot()) {
            if (change >= Inherit || shadowRoot->childNeedsStyleRecalc() || shadowRoot->needsStyleRecalc()) {
                parentPusher.push();
                resolveShadowTree(*shadowRoot, current, change);
            }
        }

        RenderTreePosition childRenderTreePosition(*renderer);
        updateBeforeOrAfterPseudoElement(current, change, BEFORE, childRenderTreePosition);

        bool elementNeedingStyleRecalcAffectsNextSiblingElementStyle = false;
        for (Node* child = current.firstChild(); child; child = child->nextSibling()) {
            if (RenderObject* childRenderer = child->renderer())
                childRenderTreePosition.invalidateNextSibling(*childRenderer);
            if (is<Text>(*child) && child->needsStyleRecalc()) {
                resolveTextNode(downcast<Text>(*child), childRenderTreePosition);
                continue;
            }
            if (!is<Element>(*child))
                continue;

            Element& childElement = downcast<Element>(*child);
            if (elementNeedingStyleRecalcAffectsNextSiblingElementStyle) {
                if (childElement.styleIsAffectedByPreviousSibling())
                    childElement.setNeedsStyleRecalc();
                elementNeedingStyleRecalcAffectsNextSiblingElementStyle = childElement.affectsNextSiblingElementStyle();
            } else if (childElement.styleChangeType() >= FullStyleChange)
                elementNeedingStyleRecalcAffectsNextSiblingElementStyle = childElement.affectsNextSiblingElementStyle();
            if (change >= Inherit || childElement.childNeedsStyleRecalc() || childElement.needsStyleRecalc()) {
                parentPusher.push();
                resolveTree(childElement, renderer->style(), childRenderTreePosition, change);
            }
        }

        updateBeforeOrAfterPseudoElement(current, change, AFTER, childRenderTreePosition);
    }
    if (change != Detach && !renderer)
        resetStyleForNonRenderedDescendants(current);

    current.clearNeedsStyleRecalc();
    current.clearChildNeedsStyleRecalc();
    
    if (current.hasCustomStyleResolveCallbacks())
        current.didRecalcStyle(change);
}

void resolveTree(Document& document, Change change)
{
    if (change == Force) {
        auto documentStyle = resolveForDocument(document);

        // Inserting the pictograph font at the end of the font fallback list is done by the
        // font selector, so set a font selector if needed.
        if (Settings* settings = document.settings()) {
            if (settings->fontFallbackPrefersPictographs())
                documentStyle.get().fontCascade().update(&document.fontSelector());
        }

        Style::Change documentChange = determineChange(documentStyle.get(), document.renderView()->style());
        if (documentChange != NoChange)
            document.renderView()->setStyle(WTF::move(documentStyle));
    }

    Element* documentElement = document.documentElement();
    if (!documentElement)
        return;
    if (change < Inherit && !documentElement->childNeedsStyleRecalc() && !documentElement->needsStyleRecalc())
        return;
    RenderTreePosition renderTreePosition(*document.renderView());
    resolveTree(*documentElement, *document.renderStyle(), renderTreePosition, change);
}

void detachRenderTree(Element& element)
{
    detachRenderTree(element, NormalDetach);
}

static Vector<std::function<void ()>>& postResolutionCallbackQueue()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<Vector<std::function<void ()>>> vector;
    return vector;
#else
    WKC_DEFINE_STATIC_PTR(Vector<std::function<void ()>>*, vector, 0);
    if (!vector)
        vector = new Vector<std::function<void ()>>();
    return *vector;
#endif
}

void queuePostResolutionCallback(std::function<void ()> callback)
{
    postResolutionCallbackQueue().append(callback);
}

static void suspendMemoryCacheClientCalls(Document& document)
{
    Page* page = document.page();
    if (!page || !page->areMemoryCacheClientCallsEnabled())
        return;

    page->setMemoryCacheClientCallsEnabled(false);

    RefPtr<MainFrame> protectedMainFrame = &page->mainFrame();
#if !PLATFORM(WKC)
    postResolutionCallbackQueue().append([protectedMainFrame]{
        if (Page* page = protectedMainFrame->page())
            page->setMemoryCacheClientCallsEnabled(true);
    });
#else
    std::function<void()> p(std::allocator_arg, WTF::voidFuncAllocator(), [protectedMainFrame] {
        if (Page* page = protectedMainFrame->page())
            page->setMemoryCacheClientCallsEnabled(true);
    });
    postResolutionCallbackQueue().append(WTF::move(p));
#endif
}

#if !PLATFORM(WKC)
static unsigned resolutionNestingDepth;
#else
WKC_DEFINE_GLOBAL_UINT(resolutionNestingDepth, 0);
#endif

PostResolutionCallbackDisabler::PostResolutionCallbackDisabler(Document& document)
{
    ++resolutionNestingDepth;

    if (resolutionNestingDepth == 1)
        platformStrategies()->loaderStrategy()->resourceLoadScheduler()->suspendPendingRequests();

    // FIXME: It's strange to build this into the disabler.
    suspendMemoryCacheClientCalls(document);
}

PostResolutionCallbackDisabler::~PostResolutionCallbackDisabler()
{
    if (resolutionNestingDepth == 1) {
        // Get size each time through the loop because a callback can add more callbacks to the end of the queue.
        auto& queue = postResolutionCallbackQueue();
        for (size_t i = 0; i < queue.size(); ++i)
            queue[i]();
        queue.clear();

        platformStrategies()->loaderStrategy()->resourceLoadScheduler()->resumePendingRequests();
    }

    --resolutionNestingDepth;
}

bool postResolutionCallbacksAreSuspended()
{
    return resolutionNestingDepth;
}

}
}
