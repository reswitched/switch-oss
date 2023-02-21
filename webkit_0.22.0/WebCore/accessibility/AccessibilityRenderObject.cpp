/*
* Copyright (C) 2008 Apple Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1.  Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
* 2.  Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
* 3.  Neither the name of Apple Inc. ("Apple") nor the names of
*     its contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "AccessibilityRenderObject.h"

#include "AXObjectCache.h"
#include "AccessibilityImageMapLink.h"
#include "AccessibilityListBox.h"
#include "AccessibilitySVGRoot.h"
#include "AccessibilitySpinButton.h"
#include "AccessibilityTable.h"
#include "CachedImage.h"
#include "Chrome.h"
#include "ElementIterator.h"
#include "EventNames.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "HTMLAreaElement.h"
#include "HTMLAudioElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLLabelElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"
#include "HTMLSelectElement.h"
#include "HTMLTableElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLVideoElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "Image.h"
#include "LocalizedStrings.h"
#include "MathMLNames.h"
#include "NodeList.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "RenderButton.h"
#include "RenderFieldset.h"
#include "RenderFileUploadControl.h"
#include "RenderHTMLCanvas.h"
#include "RenderImage.h"
#include "RenderInline.h"
#include "RenderIterator.h"
#include "RenderLayer.h"
#include "RenderLineBreak.h"
#include "RenderListBox.h"
#include "RenderListItem.h"
#include "RenderListMarker.h"
#include "RenderMathMLBlock.h"
#include "RenderMathMLFraction.h"
#include "RenderMathMLOperator.h"
#include "RenderMathMLRoot.h"
#include "RenderMenuList.h"
#include "RenderSVGRoot.h"
#include "RenderSVGShape.h"
#include "RenderText.h"
#include "RenderTextControl.h"
#include "RenderTextControlSingleLine.h"
#include "RenderTextFragment.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "RenderedPosition.h"
#include "SVGDocument.h"
#include "SVGImage.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"
#include "Text.h"
#include "TextControlInnerElements.h"
#include "TextIterator.h"
#include "VisibleUnits.h"
#include "htmlediting.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

using namespace HTMLNames;

AccessibilityRenderObject::AccessibilityRenderObject(RenderObject* renderer)
    : AccessibilityNodeObject(renderer->node())
    , m_renderer(renderer)
    , m_weakPtrFactory(this)
{
#ifndef NDEBUG
    m_renderer->setHasAXObject(true);
#endif
}

AccessibilityRenderObject::~AccessibilityRenderObject()
{
    ASSERT(isDetached());
}

void AccessibilityRenderObject::init()
{
    AccessibilityNodeObject::init();
}

Ref<AccessibilityRenderObject> AccessibilityRenderObject::create(RenderObject* renderer)
{
    return adoptRef(*new AccessibilityRenderObject(renderer));
}

void AccessibilityRenderObject::detach(AccessibilityDetachmentType detachmentType, AXObjectCache* cache)
{
    AccessibilityNodeObject::detach(detachmentType, cache);
    
    detachRemoteSVGRoot();
    
#ifndef NDEBUG
    if (m_renderer)
        m_renderer->setHasAXObject(false);
#endif
    m_renderer = nullptr;
}

RenderBoxModelObject* AccessibilityRenderObject::renderBoxModelObject() const
{
    if (!is<RenderBoxModelObject>(m_renderer))
        return nullptr;
    return downcast<RenderBoxModelObject>(m_renderer);
}

void AccessibilityRenderObject::setRenderer(RenderObject* renderer)
{
    m_renderer = renderer;
    setNode(renderer->node());
}

static inline bool isInlineWithContinuation(RenderObject& object)
{
    return is<RenderInline>(object) && downcast<RenderInline>(object).continuation();
}

static inline RenderObject* firstChildInContinuation(RenderInline& renderer)
{
    auto continuation = renderer.continuation();

    while (continuation) {
        if (is<RenderBlock>(*continuation))
            return continuation;
        if (RenderObject* child = continuation->firstChild())
            return child;
        continuation = downcast<RenderInline>(*continuation).continuation();
    }

    return nullptr;
}

static inline RenderObject* firstChildConsideringContinuation(RenderObject& renderer)
{
    RenderObject* firstChild = renderer.firstChildSlow();

    if (!firstChild && isInlineWithContinuation(renderer))
        firstChild = firstChildInContinuation(downcast<RenderInline>(renderer));

    return firstChild;
}


static inline RenderObject* lastChildConsideringContinuation(RenderObject& renderer)
{
    if (!is<RenderInline>(renderer) && !is<RenderBlock>(renderer))
        return &renderer;

    RenderObject* lastChild = downcast<RenderBoxModelObject>(renderer).lastChild();
    RenderBoxModelObject* previous;
    for (auto* current = &downcast<RenderBoxModelObject>(renderer); current; ) {
        previous = current;

        if (RenderObject* newLastChild = current->lastChild())
            lastChild = newLastChild;

        if (is<RenderInline>(*current)) {
            current = downcast<RenderInline>(*current).inlineElementContinuation();
            ASSERT_UNUSED(previous, current || !downcast<RenderInline>(*previous).continuation());
        } else
            current = downcast<RenderBlock>(*current).inlineElementContinuation();
    }

    return lastChild;
}

AccessibilityObject* AccessibilityRenderObject::firstChild() const
{
    if (!m_renderer)
        return nullptr;
    
    RenderObject* firstChild = firstChildConsideringContinuation(*m_renderer);

    // If an object can't have children, then it is using this method to help
    // calculate some internal property (like its description).
    // In this case, it should check the Node level for children in case they're
    // not rendered (like a <meter> element).
    if (!firstChild && !canHaveChildren())
        return AccessibilityNodeObject::firstChild();

    return axObjectCache()->getOrCreate(firstChild);
}

AccessibilityObject* AccessibilityRenderObject::lastChild() const
{
    if (!m_renderer)
        return nullptr;

    RenderObject* lastChild = lastChildConsideringContinuation(*m_renderer);

    if (!lastChild && !canHaveChildren())
        return AccessibilityNodeObject::lastChild();

    return axObjectCache()->getOrCreate(lastChild);
}

static inline RenderInline* startOfContinuations(RenderObject& renderer)
{
    if (renderer.isInlineElementContinuation() && is<RenderInline>(renderer.node()->renderer()))
        return downcast<RenderInline>(renderer.node()->renderer());

    // Blocks with a previous continuation always have a next continuation
    if (is<RenderBlock>(renderer) && downcast<RenderBlock>(renderer).inlineElementContinuation())
        return downcast<RenderInline>(downcast<RenderBlock>(renderer).inlineElementContinuation()->element()->renderer());

    return nullptr;
}

static inline RenderObject* endOfContinuations(RenderObject& renderer)
{
    if (!is<RenderInline>(renderer) && !is<RenderBlock>(renderer))
        return &renderer;

    auto* previous = &downcast<RenderBoxModelObject>(renderer);
    for (auto* current = previous; current; ) {
        previous = current;
        if (is<RenderInline>(*current)) {
            current = downcast<RenderInline>(*current).inlineElementContinuation();
            ASSERT(current || !downcast<RenderInline>(*previous).continuation());
        } else 
            current = downcast<RenderBlock>(*current).inlineElementContinuation();
    }

    return previous;
}


static inline RenderObject* childBeforeConsideringContinuations(RenderInline* renderer, RenderObject* child)
{
    RenderObject* previous = nullptr;
    for (RenderBoxModelObject* currentContainer = renderer; currentContainer; ) {
        if (is<RenderInline>(*currentContainer)) {
            auto* current = currentContainer->firstChild();
            while (current) {
                if (current == child)
                    return previous;
                previous = current;
                current = current->nextSibling();
            }

            currentContainer = downcast<RenderInline>(*currentContainer).continuation();
        } else if (is<RenderBlock>(*currentContainer)) {
            if (currentContainer == child)
                return previous;

            previous = currentContainer;
            currentContainer = downcast<RenderBlock>(*currentContainer).inlineElementContinuation();
        }
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

static inline bool firstChildIsInlineContinuation(RenderElement& renderer)
{
    RenderObject* child = renderer.firstChild();
    return child && child->isInlineElementContinuation();
}

AccessibilityObject* AccessibilityRenderObject::previousSibling() const
{
    if (!m_renderer)
        return nullptr;

    RenderObject* previousSibling = nullptr;

    // Case 1: The node is a block and is an inline's continuation. In that case, the inline's
    // last child is our previous sibling (or further back in the continuation chain)
    RenderInline* startOfConts;
    if (is<RenderBox>(*m_renderer) && (startOfConts = startOfContinuations(*m_renderer)))
        previousSibling = childBeforeConsideringContinuations(startOfConts, m_renderer);

    // Case 2: Anonymous block parent of the end of a continuation - skip all the way to before
    // the parent of the start, since everything in between will be linked up via the continuation.
    else if (m_renderer->isAnonymousBlock() && firstChildIsInlineContinuation(downcast<RenderBlock>(*m_renderer))) {
        RenderBlock& renderBlock = downcast<RenderBlock>(*m_renderer);
        auto* firstParent = startOfContinuations(*renderBlock.firstChild())->parent();
        ASSERT(firstParent);
        while (firstChildIsInlineContinuation(*firstParent))
            firstParent = startOfContinuations(*firstParent->firstChild())->parent();
        previousSibling = firstParent->previousSibling();
    }

    // Case 3: The node has an actual previous sibling
    else if (RenderObject* ps = m_renderer->previousSibling())
        previousSibling = ps;

    // Case 4: This node has no previous siblings, but its parent is an inline,
    // and is another node's inline continutation. Follow the continuation chain.
    else if (is<RenderInline>(*m_renderer->parent()) && (startOfConts = startOfContinuations(*m_renderer->parent())))
        previousSibling = childBeforeConsideringContinuations(startOfConts, m_renderer->parent()->firstChild());

    if (!previousSibling)
        return nullptr;
    
    return axObjectCache()->getOrCreate(previousSibling);
}

static inline bool lastChildHasContinuation(RenderElement& renderer)
{
    RenderObject* child = renderer.lastChild();
    return child && isInlineWithContinuation(*child);
}

AccessibilityObject* AccessibilityRenderObject::nextSibling() const
{
    if (!m_renderer)
        return nullptr;

    RenderObject* nextSibling = nullptr;

    // Case 1: node is a block and has an inline continuation. Next sibling is the inline continuation's
    // first child.
    RenderInline* inlineContinuation;
    if (is<RenderBlock>(*m_renderer) && (inlineContinuation = downcast<RenderBlock>(*m_renderer).inlineElementContinuation()))
        nextSibling = firstChildConsideringContinuation(*inlineContinuation);

    // Case 2: Anonymous block parent of the start of a continuation - skip all the way to
    // after the parent of the end, since everything in between will be linked up via the continuation.
    else if (m_renderer->isAnonymousBlock() && lastChildHasContinuation(downcast<RenderBlock>(*m_renderer))) {
        RenderElement* lastParent = endOfContinuations(*downcast<RenderBlock>(*m_renderer).lastChild())->parent();
        ASSERT(lastParent);
        while (lastChildHasContinuation(*lastParent))
            lastParent = endOfContinuations(*lastParent->lastChild())->parent();
        nextSibling = lastParent->nextSibling();
    }

    // Case 3: node has an actual next sibling
    else if (RenderObject* ns = m_renderer->nextSibling())
        nextSibling = ns;

    // Case 4: node is an inline with a continuation. Next sibling is the next sibling of the end 
    // of the continuation chain.
    else if (isInlineWithContinuation(*m_renderer))
        nextSibling = endOfContinuations(*m_renderer)->nextSibling();

    // Case 5: node has no next sibling, and its parent is an inline with a continuation.
    else if (isInlineWithContinuation(*m_renderer->parent())) {
        auto& continuation = *downcast<RenderInline>(*m_renderer->parent()).continuation();
        
        // Case 5a: continuation is a block - in this case the block itself is the next sibling.
        if (is<RenderBlock>(continuation))
            nextSibling = &continuation;
        // Case 5b: continuation is an inline - in this case the inline's first child is the next sibling
        else
            nextSibling = firstChildConsideringContinuation(continuation);
    }

    if (!nextSibling)
        return nullptr;
    
    return axObjectCache()->getOrCreate(nextSibling);
}

static RenderBoxModelObject* nextContinuation(RenderObject& renderer)
{
    if (is<RenderInline>(renderer) && !renderer.isReplaced())
        return downcast<RenderInline>(renderer).continuation();
    if (is<RenderBlock>(renderer))
        return downcast<RenderBlock>(renderer).inlineElementContinuation();
    return nullptr;
}
    
RenderObject* AccessibilityRenderObject::renderParentObject() const
{
    if (!m_renderer)
        return nullptr;

    RenderElement* parent = m_renderer->parent();

    // Case 1: node is a block and is an inline's continuation. Parent
    // is the start of the continuation chain.
    RenderInline* startOfConts = nullptr;
    RenderObject* firstChild = nullptr;
    if (is<RenderBlock>(*m_renderer) && (startOfConts = startOfContinuations(*m_renderer)))
        parent = startOfConts;

    // Case 2: node's parent is an inline which is some node's continuation; parent is 
    // the earliest node in the continuation chain.
    else if (is<RenderInline>(parent) && (startOfConts = startOfContinuations(*parent)))
        parent = startOfConts;
    
    // Case 3: The first sibling is the beginning of a continuation chain. Find the origin of that continuation.
    else if (parent && (firstChild = parent->firstChild()) && firstChild->node()) {
        // Get the node's renderer and follow that continuation chain until the first child is found
        RenderObject* nodeRenderFirstChild = firstChild->node()->renderer();
        while (nodeRenderFirstChild != firstChild) {
            for (RenderObject* contsTest = nodeRenderFirstChild; contsTest; contsTest = nextContinuation(*contsTest)) {
                if (contsTest == firstChild) {
                    parent = nodeRenderFirstChild->parent();
                    break;
                }
            }
            RenderObject* parentFirstChild = parent->firstChild();
            if (firstChild == parentFirstChild)
                break;
            firstChild = parentFirstChild;
            if (!firstChild->node())
                break;
            nodeRenderFirstChild = firstChild->node()->renderer();
        }
    }
        
    return parent;
}
    
AccessibilityObject* AccessibilityRenderObject::parentObjectIfExists() const
{
    AXObjectCache* cache = axObjectCache();
    if (!cache)
        return nullptr;
    
    // WebArea's parent should be the scroll view containing it.
    if (isWebArea())
        return cache->get(&m_renderer->view().frameView());

    return cache->get(renderParentObject());
}
    
AccessibilityObject* AccessibilityRenderObject::parentObject() const
{
    if (!m_renderer)
        return nullptr;
    
    if (ariaRoleAttribute() == MenuBarRole)
        return axObjectCache()->getOrCreate(m_renderer->parent());

    // menuButton and its corresponding menu are DOM siblings, but Accessibility needs them to be parent/child
    if (ariaRoleAttribute() == MenuRole) {
        AccessibilityObject* parent = menuButtonForMenu();
        if (parent)
            return parent;
    }
    
    AXObjectCache* cache = axObjectCache();
    if (!cache)
        return nullptr;
    
    RenderObject* parentObj = renderParentObject();
    if (parentObj)
        return cache->getOrCreate(parentObj);
    
    // WebArea's parent should be the scroll view containing it.
    if (isWebArea())
        return cache->getOrCreate(&m_renderer->view().frameView());
    
    return nullptr;
}
    
bool AccessibilityRenderObject::isAttachment() const
{
    RenderBoxModelObject* renderer = renderBoxModelObject();
    if (!renderer)
        return false;
    // Widgets are the replaced elements that we represent to AX as attachments
    bool isWidget = renderer->isWidget();

    return isWidget && ariaRoleAttribute() == UnknownRole;
}

bool AccessibilityRenderObject::isFileUploadButton() const
{
    if (m_renderer && is<HTMLInputElement>(m_renderer->node())) {
        HTMLInputElement& input = downcast<HTMLInputElement>(*m_renderer->node());
        return input.isFileUpload();
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isReadOnly() const
{
    ASSERT(m_renderer);
    
    if (isWebArea()) {
        if (HTMLElement* body = m_renderer->document().bodyOrFrameset()) {
            if (body->hasEditableStyle())
                return false;
        }

        return !m_renderer->document().hasEditableStyle();
    }

    return AccessibilityNodeObject::isReadOnly();
}

bool AccessibilityRenderObject::isOffScreen() const
{
    ASSERT(m_renderer);
    IntRect contentRect = snappedIntRect(m_renderer->absoluteClippedOverflowRect());
    // FIXME: unclear if we need LegacyIOSDocumentVisibleRect.
    IntRect viewRect = m_renderer->view().frameView().visibleContentRect(ScrollableArea::LegacyIOSDocumentVisibleRect);
    viewRect.intersect(contentRect);
    return viewRect.isEmpty();
}

Element* AccessibilityRenderObject::anchorElement() const
{
    if (!m_renderer)
        return nullptr;
    
    AXObjectCache* cache = axObjectCache();
    if (!cache)
        return nullptr;
    
    RenderObject* currentRenderer;
    
    // Search up the render tree for a RenderObject with a DOM node.  Defer to an earlier continuation, though.
    for (currentRenderer = m_renderer; currentRenderer && !currentRenderer->node(); currentRenderer = currentRenderer->parent()) {
        if (currentRenderer->isAnonymousBlock()) {
            if (RenderObject* continuation = downcast<RenderBlock>(*currentRenderer).continuation())
                return cache->getOrCreate(continuation)->anchorElement();
        }
    }
    
    // bail if none found
    if (!currentRenderer)
        return nullptr;
    
    // search up the DOM tree for an anchor element
    // NOTE: this assumes that any non-image with an anchor is an HTMLAnchorElement
    for (Node* node = currentRenderer->node(); node; node = node->parentNode()) {
        if (is<HTMLAnchorElement>(*node) || (node->renderer() && cache->getOrCreate(node->renderer())->isLink()))
            return downcast<Element>(node);
    }
    
    return nullptr;
}

String AccessibilityRenderObject::helpText() const
{
    if (!m_renderer)
        return String();
    
    const AtomicString& ariaHelp = getAttribute(aria_helpAttr);
    if (!ariaHelp.isEmpty())
        return ariaHelp;
    
    String describedBy = ariaDescribedByAttribute();
    if (!describedBy.isEmpty())
        return describedBy;
    
    String description = accessibilityDescription();
    for (RenderObject* ancestor = m_renderer; ancestor; ancestor = ancestor->parent()) {
        if (is<HTMLElement>(ancestor->node())) {
            HTMLElement& element = downcast<HTMLElement>(*ancestor->node());
            const AtomicString& summary = element.getAttribute(summaryAttr);
            if (!summary.isEmpty())
                return summary;
            
            // The title attribute should be used as help text unless it is already being used as descriptive text.
            const AtomicString& title = element.getAttribute(titleAttr);
            if (!title.isEmpty() && description != title)
                return title;
        }
        
        // Only take help text from an ancestor element if its a group or an unknown role. If help was 
        // added to those kinds of elements, it is likely it was meant for a child element.
        AccessibilityObject* axObj = axObjectCache()->getOrCreate(ancestor);
        if (axObj) {
            AccessibilityRole role = axObj->roleValue();
            if (role != GroupRole && role != UnknownRole)
                break;
        }
    }
    
    return String();
}

String AccessibilityRenderObject::textUnderElement(AccessibilityTextUnderElementMode mode) const
{
    if (!m_renderer)
        return String();

    if (is<RenderFileUploadControl>(*m_renderer))
        return downcast<RenderFileUploadControl>(*m_renderer).buttonValue();
    
    // Reflect when a content author has explicitly marked a line break.
    if (m_renderer->isBR())
        return ASCIILiteral("\n");

#if ENABLE(MATHML)
    // Math operators create RenderText nodes on the fly that are not tied into the DOM in a reasonable way,
    // so rangeOfContents does not work for them (nor does regular text selection).
    if (is<RenderText>(*m_renderer) && m_renderer->isAnonymous() && ancestorsOfType<RenderMathMLOperator>(*m_renderer).first())
        return downcast<RenderText>(*m_renderer).text();
    if (is<RenderMathMLOperator>(*m_renderer) && !m_renderer->isAnonymous())
        return downcast<RenderMathMLOperator>(*m_renderer).element().textContent();
#endif

    // rangeOfContents does not include CSS-generated content.
    if (m_renderer->isBeforeOrAfterContent())
        return AccessibilityNodeObject::textUnderElement(mode);

    // We use a text iterator for text objects AND for those cases where we are
    // explicitly asking for the full text under a given element.
    if (is<RenderText>(*m_renderer) || mode.childrenInclusion == AccessibilityTextUnderElementMode::TextUnderElementModeIncludeAllChildren) {
        // If possible, use a text iterator to get the text, so that whitespace
        // is handled consistently.
        Document* nodeDocument = nullptr;
        RefPtr<Range> textRange;
        if (Node* node = m_renderer->node()) {
            // rangeOfContents does not include CSS-generated content.
            Node* firstChild = node->pseudoAwareFirstChild();
            Node* lastChild = node->pseudoAwareLastChild();
            if ((firstChild && firstChild->isPseudoElement()) || (lastChild && lastChild->isPseudoElement()))
                return AccessibilityNodeObject::textUnderElement(mode);

            nodeDocument = &node->document();
            textRange = rangeOfContents(*node);
        } else {
            // For anonymous blocks, we work around not having a direct node to create a range from
            // defining one based in the two external positions defining the boundaries of the subtree.
            RenderObject* firstChildRenderer = m_renderer->firstChildSlow();
            RenderObject* lastChildRenderer = m_renderer->lastChildSlow();
            if (firstChildRenderer && lastChildRenderer) {
                ASSERT(firstChildRenderer->node());
                ASSERT(lastChildRenderer->node());

                // We define the start and end positions for the range as the ones right before and after
                // the first and the last nodes in the DOM tree that is wrapped inside the anonymous block.
                Node* firstNodeInBlock = firstChildRenderer->node();
                Position startPosition = positionInParentBeforeNode(firstNodeInBlock);
                Position endPosition = positionInParentAfterNode(lastChildRenderer->node());

                nodeDocument = &firstNodeInBlock->document();
                textRange = Range::create(*nodeDocument, startPosition, endPosition);
            }
        }

        if (nodeDocument && textRange) {
            if (Frame* frame = nodeDocument->frame()) {
                // catch stale WebCoreAXObject (see <rdar://problem/3960196>)
                if (frame->document() != nodeDocument)
                    return String();
                return plainText(textRange.get(), textIteratorBehaviorForTextRange());
            }
        }
    
        // Sometimes text fragments don't have Nodes associated with them (like when
        // CSS content is used to insert text or when a RenderCounter is used.)
        if (is<RenderText>(*m_renderer)) {
            RenderText& renderTextObject = downcast<RenderText>(*m_renderer);
            if (is<RenderTextFragment>(renderTextObject)) {
                RenderTextFragment& renderTextFragment = downcast<RenderTextFragment>(renderTextObject);
                // The alt attribute may be set on a text fragment through CSS, which should be honored.
                const String& altText = renderTextFragment.altText();
                if (!altText.isEmpty())
                    return altText;
                return renderTextFragment.contentString();
            }

            return renderTextObject.text();
        }
    }
    
    return AccessibilityNodeObject::textUnderElement(mode);
}

Node* AccessibilityRenderObject::node() const
{
    if (!m_renderer)
        return nullptr;
    if (m_renderer->isRenderView())
        return &m_renderer->document();
    return m_renderer->node();
}    

String AccessibilityRenderObject::stringValue() const
{
    if (!m_renderer)
        return String();

    if (isPasswordField())
        return passwordFieldValue();

    RenderBoxModelObject* cssBox = renderBoxModelObject();

    if (ariaRoleAttribute() == StaticTextRole) {
        String staticText = text();
        if (!staticText.length())
            staticText = textUnderElement();
        return staticText;
    }
        
    if (is<RenderText>(*m_renderer))
        return textUnderElement();
    
    if (is<RenderMenuList>(cssBox)) {
        // RenderMenuList will go straight to the text() of its selected item.
        // This has to be overridden in the case where the selected item has an ARIA label.
        HTMLSelectElement& selectElement = downcast<HTMLSelectElement>(*m_renderer->node());
        int selectedIndex = selectElement.selectedIndex();
        const Vector<HTMLElement*>& listItems = selectElement.listItems();
        if (selectedIndex >= 0 && static_cast<size_t>(selectedIndex) < listItems.size()) {
            const AtomicString& overriddenDescription = listItems[selectedIndex]->fastGetAttribute(aria_labelAttr);
            if (!overriddenDescription.isNull())
                return overriddenDescription;
        }
        return downcast<RenderMenuList>(*m_renderer).text();
    }
    
    if (is<RenderListMarker>(*m_renderer))
        return downcast<RenderListMarker>(*m_renderer).text();
    
    if (isWebArea())
        return String();
    
    if (isTextControl())
        return text();
    
    if (is<RenderFileUploadControl>(*m_renderer))
        return downcast<RenderFileUploadControl>(*m_renderer).fileTextValue();
    
    // FIXME: We might need to implement a value here for more types
    // FIXME: It would be better not to advertise a value at all for the types for which we don't implement one;
    // this would require subclassing or making accessibilityAttributeNames do something other than return a
    // single static array.
    return String();
}

HTMLLabelElement* AccessibilityRenderObject::labelElementContainer() const
{
    if (!m_renderer)
        return nullptr;

    // the control element should not be considered part of the label
    if (isControl())
        return nullptr;
    
    // find if this has a parent that is a label
    for (Node* parentNode = m_renderer->node(); parentNode; parentNode = parentNode->parentNode()) {
        if (is<HTMLLabelElement>(*parentNode))
            return downcast<HTMLLabelElement>(parentNode);
    }
    
    return nullptr;
}

// The boundingBox for elements within the remote SVG element needs to be offset by its position
// within the parent page, otherwise they are in relative coordinates only.
void AccessibilityRenderObject::offsetBoundingBoxForRemoteSVGElement(LayoutRect& rect) const
{
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
        if (parent->isAccessibilitySVGRoot()) {
            rect.moveBy(parent->parentObject()->boundingBoxRect().location());
            break;
        }
    }
}
    
LayoutRect AccessibilityRenderObject::boundingBoxRect() const
{
    RenderObject* obj = m_renderer;
    
    if (!obj)
        return LayoutRect();
    
    if (obj->node()) // If we are a continuation, we want to make sure to use the primary renderer.
        obj = obj->node()->renderer();
    
    // absoluteFocusRingQuads will query the hierarchy below this element, which for large webpages can be very slow.
    // For a web area, which will have the most elements of any element, absoluteQuads should be used.
    // We should also use absoluteQuads for SVG elements, otherwise transforms won't be applied.
    Vector<FloatQuad> quads;
    bool isSVGRoot = false;

    if (obj->isSVGRoot())
        isSVGRoot = true;

    if (is<RenderText>(*obj))
        quads = downcast<RenderText>(*obj).absoluteQuadsClippedToEllipsis();
    else if (isWebArea() || isSVGRoot)
        obj->absoluteQuads(quads);
    else
        obj->absoluteFocusRingQuads(quads);
    
    LayoutRect result = boundingBoxForQuads(obj, quads);

    Document* document = this->document();
    if (document && document->isSVGDocument())
        offsetBoundingBoxForRemoteSVGElement(result);
    
    // The size of the web area should be the content size, not the clipped size.
    if (isWebArea())
        result.setSize(obj->view().frameView().contentsSize());
    
    return result;
}
    
LayoutRect AccessibilityRenderObject::checkboxOrRadioRect() const
{
    if (!m_renderer)
        return LayoutRect();
    
    HTMLLabelElement* label = labelForElement(downcast<Element>(m_renderer->node()));
    if (!label || !label->renderer())
        return boundingBoxRect();
    
    LayoutRect labelRect = axObjectCache()->getOrCreate(label)->elementRect();
    labelRect.unite(boundingBoxRect());
    return labelRect;
}

LayoutRect AccessibilityRenderObject::elementRect() const
{
    // a checkbox or radio button should encompass its label
    if (isCheckboxOrRadio())
        return checkboxOrRadioRect();
    
    return boundingBoxRect();
}
    
bool AccessibilityRenderObject::supportsPath() const
{
    return is<RenderSVGShape>(m_renderer);
}

Path AccessibilityRenderObject::elementPath() const
{
    if (is<RenderSVGShape>(m_renderer) && downcast<RenderSVGShape>(*m_renderer).hasPath()) {
        Path path = downcast<RenderSVGShape>(*m_renderer).path();
        
        // The SVG path is in terms of the parent's bounding box. The path needs to be offset to frame coordinates.
        if (auto svgRoot = ancestorsOfType<RenderSVGRoot>(*m_renderer).first()) {
            LayoutPoint parentOffset = axObjectCache()->getOrCreate(&*svgRoot)->elementRect().location();
            path.transform(AffineTransform().translate(parentOffset.x(), parentOffset.y()));
        }
        
        return path;
    }
    
    return Path();
}

IntPoint AccessibilityRenderObject::clickPoint()
{
    // Headings are usually much wider than their textual content. If the mid point is used, often it can be wrong.
    if (isHeading() && children().size() == 1)
        return children()[0]->clickPoint();

    // use the default position unless this is an editable web area, in which case we use the selection bounds.
    if (!isWebArea() || isReadOnly())
        return AccessibilityObject::clickPoint();
    
    VisibleSelection visSelection = selection();
    VisiblePositionRange range = VisiblePositionRange(visSelection.visibleStart(), visSelection.visibleEnd());
    IntRect bounds = boundsForVisiblePositionRange(range);
#if PLATFORM(COCOA)
    bounds.setLocation(m_renderer->view().frameView().screenToContents(bounds.location()));
#endif        
    return IntPoint(bounds.x() + (bounds.width() / 2), bounds.y() - (bounds.height() / 2));
}
    
AccessibilityObject* AccessibilityRenderObject::internalLinkElement() const
{
    Element* element = anchorElement();
    // Right now, we do not support ARIA links as internal link elements
    if (!is<HTMLAnchorElement>(element))
        return nullptr;
    HTMLAnchorElement& anchor = downcast<HTMLAnchorElement>(*element);
    
    URL linkURL = anchor.href();
    String fragmentIdentifier = linkURL.fragmentIdentifier();
    if (fragmentIdentifier.isEmpty())
        return nullptr;
    
    // check if URL is the same as current URL
    URL documentURL = m_renderer->document().url();
    if (!equalIgnoringFragmentIdentifier(documentURL, linkURL))
        return nullptr;
    
    Node* linkedNode = m_renderer->document().findAnchor(fragmentIdentifier);
    if (!linkedNode)
        return nullptr;
    
    // The element we find may not be accessible, so find the first accessible object.
    return firstAccessibleObjectFromNode(linkedNode);
}

ESpeak AccessibilityRenderObject::speakProperty() const
{
    if (!m_renderer)
        return AccessibilityObject::speakProperty();
    
    return m_renderer->style().speak();
}
    
void AccessibilityRenderObject::addRadioButtonGroupMembers(AccessibilityChildrenVector& linkedUIElements) const
{
    if (!m_renderer || roleValue() != RadioButtonRole)
        return;
    
    Node* node = m_renderer->node();
    if (!is<HTMLInputElement>(node))
        return;
    
    HTMLInputElement& input = downcast<HTMLInputElement>(*node);
    // if there's a form, then this is easy
    if (input.form()) {
        for (auto& associateElement : input.form()->namedElements(input.name())) {
            if (AccessibilityObject* object = axObjectCache()->getOrCreate(&associateElement.get()))
                linkedUIElements.append(object);        
        } 
    } else {
        RefPtr<NodeList> list = node->document().getElementsByTagName(inputTag.localName());
        unsigned length = list->length();
        for (unsigned i = 0; i < length; ++i) {
            Node* item = list->item(i);
            if (is<HTMLInputElement>(*item)) {
                HTMLInputElement& associateElement = downcast<HTMLInputElement>(*item);
                if (associateElement.isRadioButton() && associateElement.name() == input.name()) {
                    if (AccessibilityObject* object = axObjectCache()->getOrCreate(&associateElement))
                        linkedUIElements.append(object);
                }
            }
        }
    }
}
    
// linked ui elements could be all the related radio buttons in a group
// or an internal anchor connection
void AccessibilityRenderObject::linkedUIElements(AccessibilityChildrenVector& linkedUIElements) const
{
    ariaFlowToElements(linkedUIElements);

    if (isLink()) {
        AccessibilityObject* linkedAXElement = internalLinkElement();
        if (linkedAXElement)
            linkedUIElements.append(linkedAXElement);
    }

    if (roleValue() == RadioButtonRole)
        addRadioButtonGroupMembers(linkedUIElements);
}

bool AccessibilityRenderObject::hasTextAlternative() const
{
    // ARIA: section 2A, bullet #3 says if aria-labeledby or aria-label appears, it should
    // override the "label" element association.
    return ariaAccessibilityDescription().length();
}
    
bool AccessibilityRenderObject::ariaHasPopup() const
{
    return elementAttributeValue(aria_haspopupAttr);
}

void AccessibilityRenderObject::ariaElementsFromAttribute(AccessibilityChildrenVector& children, const QualifiedName& attributeName) const
{
    Vector<Element*> elements;
    elementsFromAttribute(elements, attributeName);
    AXObjectCache* cache = axObjectCache();
    for (const auto& element : elements) {
        if (AccessibilityObject* axObject = cache->getOrCreate(element))
            children.append(axObject);
    }
}

bool AccessibilityRenderObject::supportsARIAFlowTo() const
{
    return !getAttribute(aria_flowtoAttr).isEmpty();
}
    
void AccessibilityRenderObject::ariaFlowToElements(AccessibilityChildrenVector& flowTo) const
{
    ariaElementsFromAttribute(flowTo, aria_flowtoAttr);
}

bool AccessibilityRenderObject::supportsARIADescribedBy() const
{
    return !getAttribute(aria_describedbyAttr).isEmpty();
}

void AccessibilityRenderObject::ariaDescribedByElements(AccessibilityChildrenVector& ariaDescribedBy) const
{
    ariaElementsFromAttribute(ariaDescribedBy, aria_describedbyAttr);
}

bool AccessibilityRenderObject::supportsARIAControls() const
{
    return !getAttribute(aria_controlsAttr).isEmpty();
}

void AccessibilityRenderObject::ariaControlsElements(AccessibilityChildrenVector& ariaControls) const
{
    ariaElementsFromAttribute(ariaControls, aria_controlsAttr);
}

bool AccessibilityRenderObject::supportsARIADropping() const 
{
    const AtomicString& dropEffect = getAttribute(aria_dropeffectAttr);
    return !dropEffect.isEmpty();
}

bool AccessibilityRenderObject::supportsARIADragging() const
{
    const AtomicString& grabbed = getAttribute(aria_grabbedAttr);
    return equalIgnoringCase(grabbed, "true") || equalIgnoringCase(grabbed, "false");   
}

bool AccessibilityRenderObject::isARIAGrabbed()
{
    return elementAttributeValue(aria_grabbedAttr);
}

void AccessibilityRenderObject::determineARIADropEffects(Vector<String>& effects)
{
    const AtomicString& dropEffects = getAttribute(aria_dropeffectAttr);
    if (dropEffects.isEmpty()) {
        effects.clear();
        return;
    }
    
    String dropEffectsString = dropEffects.string();
    dropEffectsString.replace('\n', ' ');
    dropEffectsString.split(' ', effects);
}
    
bool AccessibilityRenderObject::exposesTitleUIElement() const
{
    if (!isControl())
        return false;

    // If this control is ignored (because it's invisible), 
    // then the label needs to be exposed so it can be visible to accessibility.
    if (accessibilityIsIgnored())
        return true;
    
    // When controls have their own descriptions, the title element should be ignored.
    if (hasTextAlternative())
        return false;
    
    return true;
}
    
AccessibilityObject* AccessibilityRenderObject::titleUIElement() const
{
    if (!m_renderer)
        return nullptr;
    
    // if isFieldset is true, the renderer is guaranteed to be a RenderFieldset
    if (isFieldset())
        return axObjectCache()->getOrCreate(downcast<RenderFieldset>(*m_renderer).findLegend(RenderFieldset::IncludeFloatingOrOutOfFlow));
    
    Node* node = m_renderer->node();
    if (!is<Element>(node))
        return nullptr;
    HTMLLabelElement* label = labelForElement(downcast<Element>(node));
    if (label && label->renderer())
        return axObjectCache()->getOrCreate(label);

    return nullptr;
}
    
bool AccessibilityRenderObject::isAllowedChildOfTree() const
{
    // Determine if this is in a tree. If so, we apply special behavior to make it work like an AXOutline.
    AccessibilityObject* axObj = parentObject();
    bool isInTree = false;
    bool isTreeItemDescendant = false;
    while (axObj) {
        if (axObj->roleValue() == TreeItemRole)
            isTreeItemDescendant = true;
        if (axObj->isTree()) {
            isInTree = true;
            break;
        }
        axObj = axObj->parentObject();
    }
    
    // If the object is in a tree, only tree items should be exposed (and the children of tree items).
    if (isInTree) {
        AccessibilityRole role = roleValue();
        if (role != TreeItemRole && role != StaticTextRole && !isTreeItemDescendant)
            return false;
    }
    return true;
}
    
static AccessibilityObjectInclusion objectInclusionFromAltText(const String& altText)
{
    // Don't ignore an image that has an alt tag.
    if (!altText.containsOnlyWhitespace())
        return IncludeObject;
    
    // The informal standard is to ignore images with zero-length alt strings.
    if (!altText.isNull())
        return IgnoreObject;
    
    return DefaultBehavior;
}

AccessibilityObjectInclusion AccessibilityRenderObject::defaultObjectInclusion() const
{
    // The following cases can apply to any element that's a subclass of AccessibilityRenderObject.
    
    if (!m_renderer)
        return IgnoreObject;

    if (m_renderer->style().visibility() != VISIBLE) {
        // aria-hidden is meant to override visibility as the determinant in AX hierarchy inclusion.
        if (equalIgnoringCase(getAttribute(aria_hiddenAttr), "false"))
            return DefaultBehavior;
        
        return IgnoreObject;
    }

    return AccessibilityObject::defaultObjectInclusion();
}

bool AccessibilityRenderObject::computeAccessibilityIsIgnored() const
{
#ifndef NDEBUG
    ASSERT(m_initialized);
#endif

    if (!m_renderer)
        return true;
    
    // Check first if any of the common reasons cause this element to be ignored.
    // Then process other use cases that need to be applied to all the various roles
    // that AccessibilityRenderObjects take on.
    AccessibilityObjectInclusion decision = defaultObjectInclusion();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    // If this element is within a parent that cannot have children, it should not be exposed.
    if (isDescendantOfBarrenParent())
        return true;    
    
    if (roleValue() == IgnoredRole)
        return true;
    
    if (roleValue() == PresentationalRole || inheritsPresentationalRole())
        return true;
    
    // An ARIA tree can only have tree items and static text as children.
    if (!isAllowedChildOfTree())
        return true;

    // Allow the platform to decide if the attachment is ignored or not.
    if (isAttachment())
        return accessibilityIgnoreAttachment();
    
    // ignore popup menu items because AppKit does
    if (m_renderer && ancestorsOfType<RenderMenuList>(*m_renderer).first())
        return true;

    // find out if this element is inside of a label element.
    // if so, it may be ignored because it's the label for a checkbox or radio button
    AccessibilityObject* controlObject = correspondingControlForLabelElement();
    if (controlObject && !controlObject->exposesTitleUIElement() && controlObject->isCheckboxOrRadio())
        return true;

    if (m_renderer->isBR())
        return true;

    if (is<RenderText>(*m_renderer)) {
        // static text beneath MenuItems and MenuButtons are just reported along with the menu item, so it's ignored on an individual level
        AccessibilityObject* parent = parentObjectUnignored();
        if (parent && (parent->isMenuItem() || parent->ariaRoleAttribute() == MenuButtonRole))
            return true;
        auto& renderText = downcast<RenderText>(*m_renderer);
        if (!renderText.hasRenderedText())
            return true;

        // static text beneath TextControls is reported along with the text control text so it's ignored.
        for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) { 
            if (parent->roleValue() == TextFieldRole)
                return true;
        }
        
        // The alt attribute may be set on a text fragment through CSS, which should be honored.
        if (is<RenderTextFragment>(renderText)) {
            AccessibilityObjectInclusion altTextInclusion = objectInclusionFromAltText(downcast<RenderTextFragment>(renderText).altText());
            if (altTextInclusion == IgnoreObject)
                return true;
            if (altTextInclusion == IncludeObject)
                return false;
        }

        // text elements that are just empty whitespace should not be returned
        return renderText.text()->containsOnlyWhitespace();
    }
    
    if (isHeading())
        return false;
    
    if (isLink())
        return false;
    
    if (isLandmark())
        return false;

    // all controls are accessible
    if (isControl())
        return false;

    switch (roleValue()) {
    case AudioRole:
    case DescriptionListTermRole:
    case DescriptionListDetailRole:
    case DetailsRole:
    case DocumentArticleRole:
    case DocumentRegionRole:
    case ListItemRole:
    case VideoRole:
        return false;
    default:
        break;
    }
    
    if (ariaRoleAttribute() != UnknownRole)
        return false;
    
    if (roleValue() == HorizontalRuleRole)
        return false;
    
    // don't ignore labels, because they serve as TitleUIElements
    Node* node = m_renderer->node();
    if (is<HTMLLabelElement>(node))
        return false;
    
    // Anything that is content editable should not be ignored.
    // However, one cannot just call node->hasEditableStyle() since that will ask if its parents
    // are also editable. Only the top level content editable region should be exposed.
    if (hasContentEditableAttributeSet())
        return false;
    
    
    // if this element has aria attributes on it, it should not be ignored.
    if (supportsARIAAttributes())
        return false;

#if ENABLE(MATHML)
    // First check if this is a special case within the math tree that needs to be ignored.
    if (isIgnoredElementWithinMathTree())
        return true;
    // Otherwise all other math elements are in the tree.
    if (isMathElement())
        return false;
#endif
    
    if (is<RenderBlockFlow>(*m_renderer) && m_renderer->childrenInline() && !canSetFocusAttribute())
        return !downcast<RenderBlockFlow>(*m_renderer).hasLines() && !mouseButtonListener();
    
    // ignore images seemingly used as spacers
    if (isImage()) {
        
        // If the image can take focus, it should not be ignored, lest the user not be able to interact with something important.
        if (canSetFocusAttribute())
            return false;
        
        // First check the RenderImage's altText (which can be set through a style sheet, or come from the Element).
        // However, if this is not a native image, fallback to the attribute on the Element.
        AccessibilityObjectInclusion altTextInclusion = DefaultBehavior;
        bool isRenderImage = is<RenderImage>(m_renderer);
        if (isRenderImage)
            altTextInclusion = objectInclusionFromAltText(downcast<RenderImage>(*m_renderer).altText());
        else
            altTextInclusion = objectInclusionFromAltText(getAttribute(altAttr).string());

        if (altTextInclusion == IgnoreObject)
            return true;
        if (altTextInclusion == IncludeObject)
            return false;
        
        // If an image has a title attribute on it, accessibility should be lenient and allow it to appear in the hierarchy (according to WAI-ARIA).
        if (!getAttribute(titleAttr).isEmpty())
            return false;
    
        if (isRenderImage) {
            // check for one-dimensional image
            RenderImage& image = downcast<RenderImage>(*m_renderer);
            if (image.height() <= 1 || image.width() <= 1)
                return true;
            
            // check whether rendered image was stretched from one-dimensional file image
            if (image.cachedImage()) {
                LayoutSize imageSize = image.cachedImage()->imageSizeForRenderer(&image, image.view().zoomFactor());
                return imageSize.height() <= 1 || imageSize.width() <= 1;
            }
        }
        return false;
    }

    if (isCanvas()) {
        if (canvasHasFallbackContent())
            return false;

        if (is<RenderBox>(*m_renderer)) {
            auto& canvasBox = downcast<RenderBox>(*m_renderer);
            if (canvasBox.height() <= 1 || canvasBox.width() <= 1)
                return true;
        }
        // Otherwise fall through; use presence of help text, title, or description to decide.
    }

    if (m_renderer->isListMarker()) {
        AccessibilityObject* parent = parentObjectUnignored();
        return parent && !parent->isListItem();
    }

    if (isWebArea())
        return false;
    
    // Using the presence of an accessible name to decide an element's visibility is not
    // as definitive as previous checks, so this should remain as one of the last.
    if (hasAttributesRequiredForInclusion())
        return false;

    // Don't ignore generic focusable elements like <div tabindex=0>
    // unless they're completely empty, with no children.
    if (isGenericFocusableElement() && node->firstChild())
        return false;

    // <span> tags are inline tags and not meant to convey information if they have no other aria
    // information on them. If we don't ignore them, they may emit signals expected to come from
    // their parent. In addition, because included spans are GroupRole objects, and GroupRole
    // objects are often containers with meaningful information, the inclusion of a span can have
    // the side effect of causing the immediate parent accessible to be ignored. This is especially
    // problematic for platforms which have distinct roles for textual block elements.
    if (node && node->hasTagName(spanTag))
        return true;

    // Other non-ignored host language elements
    if (node && node->hasTagName(dfnTag))
        return false;
    
    // Make sure that ruby containers are not ignored.
    if (m_renderer->isRubyRun() || m_renderer->isRubyBlock() || m_renderer->isRubyInline())
        return false;

    // By default, objects should be ignored so that the AX hierarchy is not
    // filled with unnecessary items.
    return true;
}

bool AccessibilityRenderObject::isLoaded() const
{
    return !m_renderer->document().parser();
}

double AccessibilityRenderObject::estimatedLoadingProgress() const
{
    if (!m_renderer)
        return 0;
    
    if (isLoaded())
        return 1.0;
    
    Page* page = m_renderer->document().page();
    if (!page)
        return 0;
    
    return page->progress().estimatedProgress();
}
    
int AccessibilityRenderObject::layoutCount() const
{
    if (!is<RenderView>(*m_renderer))
        return 0;
    return downcast<RenderView>(*m_renderer).frameView().layoutCount();
}

String AccessibilityRenderObject::text() const
{
    if (isPasswordField())
        return passwordFieldValue();

    return AccessibilityNodeObject::text();
}
    
int AccessibilityRenderObject::textLength() const
{
    ASSERT(isTextControl());
    
    if (isPasswordField())
        return passwordFieldValue().length();

    return text().length();
}

PlainTextRange AccessibilityRenderObject::documentBasedSelectedTextRange() const
{
    Node* node = m_renderer->node();
    if (!node)
        return PlainTextRange();
    
    VisibleSelection visibleSelection = selection();
    RefPtr<Range> currentSelectionRange = visibleSelection.toNormalizedRange();
    if (!currentSelectionRange || !currentSelectionRange->intersectsNode(node, IGNORE_EXCEPTION))
        return PlainTextRange();
    
    int start = indexForVisiblePosition(visibleSelection.start());
    int end = indexForVisiblePosition(visibleSelection.end());
    
    return PlainTextRange(start, end - start);
}

String AccessibilityRenderObject::selectedText() const
{
    ASSERT(isTextControl());
    
    if (isPasswordField())
        return String(); // need to return something distinct from empty string
    
    if (isNativeTextControl()) {
        HTMLTextFormControlElement& textControl = downcast<RenderTextControl>(*m_renderer).textFormControlElement();
        return textControl.selectedText();
    }
    
    return doAXStringForRange(documentBasedSelectedTextRange());
}

const AtomicString& AccessibilityRenderObject::accessKey() const
{
    Node* node = m_renderer->node();
    if (!is<Element>(node))
        return nullAtom;
    return downcast<Element>(*node).getAttribute(accesskeyAttr);
}

VisibleSelection AccessibilityRenderObject::selection() const
{
    return m_renderer->frame().selection().selection();
}

PlainTextRange AccessibilityRenderObject::selectedTextRange() const
{
    ASSERT(isTextControl());
    
    if (isPasswordField())
        return PlainTextRange();
    
    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (isNativeTextControl() && ariaRole == UnknownRole) {
        HTMLTextFormControlElement& textControl = downcast<RenderTextControl>(*m_renderer).textFormControlElement();
        return PlainTextRange(textControl.selectionStart(), textControl.selectionEnd() - textControl.selectionStart());
    }
    
    return documentBasedSelectedTextRange();
}

static void setTextSelectionIntent(AXObjectCache* cache, AXTextStateChangeType type)
{
    if (!cache)
        return;
    AXTextStateChangeIntent intent(type, AXTextSelection { AXTextSelectionDirectionDiscontiguous, AXTextSelectionGranularityUnknown, false });
    cache->setTextSelectionIntent(intent);
    cache->setIsSynchronizingSelection(true);
}

static void clearTextSelectionIntent(AXObjectCache* cache)
{
    if (!cache)
        return;
    cache->setTextSelectionIntent(AXTextStateChangeIntent());
    cache->setIsSynchronizingSelection(false);
}

void AccessibilityRenderObject::setSelectedTextRange(const PlainTextRange& range)
{
    if (isNativeTextControl()) {
        setTextSelectionIntent(axObjectCache(), range.length ? AXTextStateChangeTypeSelectionExtend : AXTextStateChangeTypeSelectionMove);
        HTMLTextFormControlElement& textControl = downcast<RenderTextControl>(*m_renderer).textFormControlElement();
        textControl.setSelectionRange(range.start, range.start + range.length);
        clearTextSelectionIntent(axObjectCache());
        return;
    }

    Node* node = m_renderer->node();
    VisibleSelection newSelection(Position(node, range.start, Position::PositionIsOffsetInAnchor), Position(node, range.start + range.length, Position::PositionIsOffsetInAnchor), DOWNSTREAM);
    setTextSelectionIntent(axObjectCache(), range.length ? AXTextStateChangeTypeSelectionExtend : AXTextStateChangeTypeSelectionMove);
    m_renderer->frame().selection().setSelection(newSelection, FrameSelection::defaultSetSelectionOptions());
    clearTextSelectionIntent(axObjectCache());
}

URL AccessibilityRenderObject::url() const
{
    if (isLink() && is<HTMLAnchorElement>(*m_renderer->node())) {
        if (HTMLAnchorElement* anchor = downcast<HTMLAnchorElement>(anchorElement()))
            return anchor->href();
    }
    
    if (isWebArea())
        return m_renderer->document().url();
    
    if (isImage() && is<HTMLImageElement>(m_renderer->node()))
        return downcast<HTMLImageElement>(*m_renderer->node()).src();
    
    if (isInputImage())
        return downcast<HTMLInputElement>(*m_renderer->node()).src();
    
    return URL();
}

bool AccessibilityRenderObject::isUnvisited() const
{
    // FIXME: Is it a privacy violation to expose unvisited information to accessibility APIs?
    return m_renderer->style().isLink() && m_renderer->style().insideLink() == InsideUnvisitedLink;
}

bool AccessibilityRenderObject::isVisited() const
{
    // FIXME: Is it a privacy violation to expose visited information to accessibility APIs?
    return m_renderer->style().isLink() && m_renderer->style().insideLink() == InsideVisitedLink;
}

void AccessibilityRenderObject::setElementAttributeValue(const QualifiedName& attributeName, bool value)
{
    if (!m_renderer)
        return;
    
    Node* node = m_renderer->node();
    if (!is<Element>(node))
        return;
    
    downcast<Element>(*node).setAttribute(attributeName, (value) ? "true" : "false");
}
    
bool AccessibilityRenderObject::elementAttributeValue(const QualifiedName& attributeName) const
{
    if (!m_renderer)
        return false;
    
    return equalIgnoringCase(getAttribute(attributeName), "true");
}
    
bool AccessibilityRenderObject::isSelected() const
{
    if (!m_renderer)
        return false;
    
    Node* node = m_renderer->node();
    if (!node)
        return false;
    
    const AtomicString& ariaSelected = getAttribute(aria_selectedAttr);
    if (equalIgnoringCase(ariaSelected, "true"))
        return true;    
    
    if (isTabItem() && isTabItemSelected())
        return true;

    return false;
}

bool AccessibilityRenderObject::isTabItemSelected() const
{
    if (!isTabItem() || !m_renderer)
        return false;
    
    Node* node = m_renderer->node();
    if (!node || !node->isElementNode())
        return false;
    
    // The ARIA spec says a tab item can also be selected if it is aria-labeled by a tabpanel
    // that has keyboard focus inside of it, or if a tabpanel in its aria-controls list has KB
    // focus inside of it.
    AccessibilityObject* focusedElement = focusedUIElement();
    if (!focusedElement)
        return false;
    
    Vector<Element*> elements;
    elementsFromAttribute(elements, aria_controlsAttr);
    
    AXObjectCache* cache = axObjectCache();
    if (!cache)
        return false;
    
    for (const auto& element : elements) {
        AccessibilityObject* tabPanel = cache->getOrCreate(element);

        // A tab item should only control tab panels.
        if (!tabPanel || tabPanel->roleValue() != TabPanelRole)
            continue;
        
        AccessibilityObject* checkFocusElement = focusedElement;
        // Check if the focused element is a descendant of the element controlled by the tab item.
        while (checkFocusElement) {
            if (tabPanel == checkFocusElement)
                return true;
            checkFocusElement = checkFocusElement->parentObject();
        }
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isFocused() const
{
    if (!m_renderer)
        return false;
    
    Document& document = m_renderer->document();

    Element* focusedElement = document.focusedElement();
    if (!focusedElement)
        return false;
    
    // A web area is represented by the Document node in the DOM tree, which isn't focusable.
    // Check instead if the frame's selection controller is focused
    if (focusedElement == m_renderer->node()
        || (roleValue() == WebAreaRole && document.frame()->selection().isFocusedAndActive()))
        return true;
    
    return false;
}

void AccessibilityRenderObject::setFocused(bool on)
{
    if (!canSetFocusAttribute())
        return;
    
    Document* document = this->document();
    Node* node = this->node();

    if (!on || !is<Element>(node)) {
        document->setFocusedElement(nullptr);
        return;
    }

    // If this node is already the currently focused node, then calling focus() won't do anything.
    // That is a problem when focus is removed from the webpage to chrome, and then returns.
    // In these cases, we need to do what keyboard and mouse focus do, which is reset focus first.
    if (document->focusedElement() == node)
        document->setFocusedElement(nullptr);

    // When a node is told to set focus, that can cause it to be deallocated, which means that doing
    // anything else inside this object will crash. To fix this, we added a RefPtr to protect this object
    // long enough for duration. We can also locally cache the axObjectCache.
    RefPtr<AccessibilityObject> protect(this);
    AXObjectCache* cache = axObjectCache();
    
    cache->setIsSynchronizingSelection(true);
    downcast<Element>(*node).focus();
    cache->setIsSynchronizingSelection(false);
}

void AccessibilityRenderObject::setSelectedRows(AccessibilityChildrenVector& selectedRows)
{
    // Setting selected only makes sense in trees and tables (and tree-tables).
    AccessibilityRole role = roleValue();
    if (role != TreeRole && role != TreeGridRole && role != TableRole)
        return;
    
    bool isMulti = isMultiSelectable();
    unsigned count = selectedRows.size();
    if (count > 1 && !isMulti)
        count = 1;
    
    for (const auto& selectedRow : selectedRows)
        selectedRow->setSelected(true);
}
    
void AccessibilityRenderObject::setValue(const String& string)
{
    if (!m_renderer || !is<Element>(m_renderer->node()))
        return;
    Element& element = downcast<Element>(*m_renderer->node());

    if (!is<RenderBoxModelObject>(*m_renderer))
        return;
    RenderBoxModelObject& renderer = downcast<RenderBoxModelObject>(*m_renderer);

    // FIXME: Do we want to do anything here for ARIA textboxes?
    if (renderer.isTextField() && is<HTMLInputElement>(element))
        downcast<HTMLInputElement>(element).setValue(string);
    else if (renderer.isTextArea() && is<HTMLTextAreaElement>(element))
        downcast<HTMLTextAreaElement>(element).setValue(string);
}

void AccessibilityRenderObject::ariaOwnsElements(AccessibilityChildrenVector& axObjects) const
{
    ariaElementsFromAttribute(axObjects, aria_ownsAttr);
}

bool AccessibilityRenderObject::supportsARIAOwns() const
{
    if (!m_renderer)
        return false;
    const AtomicString& ariaOwns = getAttribute(aria_ownsAttr);

    return !ariaOwns.isEmpty();
}
    
RenderView* AccessibilityRenderObject::topRenderer() const
{
    Document* topDoc = topDocument();
    if (!topDoc)
        return nullptr;
    
    return topDoc->renderView();
}

Document* AccessibilityRenderObject::document() const
{
    if (!m_renderer)
        return nullptr;
    return &m_renderer->document();
}

Widget* AccessibilityRenderObject::widget() const
{
    if (!is<RenderWidget>(*m_renderer))
        return nullptr;
    return downcast<RenderWidget>(*m_renderer).widget();
}

AccessibilityObject* AccessibilityRenderObject::accessibilityParentForImageMap(HTMLMapElement* map) const
{
    // find an image that is using this map
    if (!map)
        return nullptr;

    HTMLImageElement* imageElement = map->imageElement();
    if (!imageElement)
        return nullptr;
    
    if (AXObjectCache* cache = axObjectCache())
        return cache->getOrCreate(imageElement);
    
    return nullptr;
}
    
void AccessibilityRenderObject::getDocumentLinks(AccessibilityChildrenVector& result)
{
    Document& document = m_renderer->document();
    Ref<HTMLCollection> links = document.links();
    for (unsigned i = 0; Node* curr = links->item(i); i++) {
        RenderObject* obj = curr->renderer();
        if (obj) {
            RefPtr<AccessibilityObject> axobj = document.axObjectCache()->getOrCreate(obj);
            ASSERT(axobj);
            if (!axobj->accessibilityIsIgnored() && axobj->isLink())
                result.append(axobj);
        } else {
            Node* parent = curr->parentNode();
            if (is<HTMLAreaElement>(*curr) && is<HTMLMapElement>(parent)) {
                auto& areaObject = downcast<AccessibilityImageMapLink>(*axObjectCache()->getOrCreate(ImageMapLinkRole));
                HTMLMapElement& map = downcast<HTMLMapElement>(*parent);
                areaObject.setHTMLAreaElement(downcast<HTMLAreaElement>(curr));
                areaObject.setHTMLMapElement(&map);
                areaObject.setParent(accessibilityParentForImageMap(&map));

                result.append(&areaObject);
            }
        }
    }
}

FrameView* AccessibilityRenderObject::documentFrameView() const 
{ 
    if (!m_renderer)
        return nullptr;

    // this is the RenderObject's Document's Frame's FrameView 
    return &m_renderer->view().frameView();
}

Widget* AccessibilityRenderObject::widgetForAttachmentView() const
{
    if (!isAttachment())
        return nullptr;
    return downcast<RenderWidget>(*m_renderer).widget();
}

// This function is like a cross-platform version of - (WebCoreTextMarkerRange*)textMarkerRange. It returns
// a Range that we can convert to a WebCoreTextMarkerRange in the Obj-C file
VisiblePositionRange AccessibilityRenderObject::visiblePositionRange() const
{
    if (!m_renderer)
        return VisiblePositionRange();
    
    // construct VisiblePositions for start and end
    Node* node = m_renderer->node();
    if (!node)
        return VisiblePositionRange();

    VisiblePosition startPos = firstPositionInOrBeforeNode(node);
    VisiblePosition endPos = lastPositionInOrAfterNode(node);

    // the VisiblePositions are equal for nodes like buttons, so adjust for that
    // FIXME: Really?  [button, 0] and [button, 1] are distinct (before and after the button)
    // I expect this code is only hit for things like empty divs?  In which case I don't think
    // the behavior is correct here -- eseidel
    if (startPos == endPos) {
        endPos = endPos.next();
        if (endPos.isNull())
            endPos = startPos;
    }

    return VisiblePositionRange(startPos, endPos);
}

VisiblePositionRange AccessibilityRenderObject::visiblePositionRangeForLine(unsigned lineCount) const
{
    if (!lineCount || !m_renderer)
        return VisiblePositionRange();
    
    // iterate over the lines
    // FIXME: this is wrong when lineNumber is lineCount+1,  because nextLinePosition takes you to the
    // last offset of the last line
    VisiblePosition visiblePos = m_renderer->view().positionForPoint(IntPoint(), nullptr);
    VisiblePosition savedVisiblePos;
    while (--lineCount) {
        savedVisiblePos = visiblePos;
        visiblePos = nextLinePosition(visiblePos, 0);
        if (visiblePos.isNull() || visiblePos == savedVisiblePos)
            return VisiblePositionRange();
    }
    
    // make a caret selection for the marker position, then extend it to the line
    // NOTE: ignores results of sel.modify because it returns false when
    // starting at an empty line.  The resulting selection in that case
    // will be a caret at visiblePos.
    FrameSelection selection;
    selection.setSelection(VisibleSelection(visiblePos));
    selection.modify(FrameSelection::AlterationExtend, DirectionRight, LineBoundary);
    
    return VisiblePositionRange(selection.selection().visibleStart(), selection.selection().visibleEnd());
}
    
VisiblePosition AccessibilityRenderObject::visiblePositionForIndex(int index) const
{
    if (!m_renderer)
        return VisiblePosition();

    if (isNativeTextControl())
        return downcast<RenderTextControl>(*m_renderer).textFormControlElement().visiblePositionForIndex(index);

    if (!allowsTextRanges() && !is<RenderText>(*m_renderer))
        return VisiblePosition();
    
    Node* node = m_renderer->node();
    if (!node)
        return VisiblePosition();

    return visiblePositionForIndexUsingCharacterIterator(node, index);
}
    
int AccessibilityRenderObject::indexForVisiblePosition(const VisiblePosition& pos) const
{
    if (isNativeTextControl())
        return downcast<RenderTextControl>(*m_renderer).textFormControlElement().indexForVisiblePosition(pos);

    if (!isTextControl())
        return 0;
    
    Node* node = m_renderer->node();
    if (!node)
        return 0;

    Position indexPosition = pos.deepEquivalent();
    if (indexPosition.isNull() || highestEditableRoot(indexPosition, HasEditableAXRole) != node)
        return 0;

#if PLATFORM(GTK)
    // We need to consider replaced elements for GTK, as they will be
    // presented with the 'object replacement character' (0xFFFC).
    bool forSelectionPreservation = true;
#else
    bool forSelectionPreservation = false;
#endif

    return WebCore::indexForVisiblePosition(node, pos, forSelectionPreservation);
}

Element* AccessibilityRenderObject::rootEditableElementForPosition(const Position& position) const
{
    // Find the root editable or pseudo-editable (i.e. having an editable ARIA role) element.
    Element* result = nullptr;
    
    Element* rootEditableElement = position.rootEditableElement();

    for (Element* e = position.element(); e && e != rootEditableElement; e = e->parentElement()) {
        if (nodeIsTextControl(e))
            result = e;
        if (e->hasTagName(bodyTag))
            break;
    }

    if (result)
        return result;

    return rootEditableElement;
}

bool AccessibilityRenderObject::nodeIsTextControl(const Node* node) const
{
    if (!node)
        return false;

    if (AXObjectCache* cache = axObjectCache()) {
        if (AccessibilityObject* axObjectForNode = cache->getOrCreate(const_cast<Node*>(node)))
            return axObjectForNode->isTextControl();
    }

    return false;
}

IntRect AccessibilityRenderObject::boundsForVisiblePositionRange(const VisiblePositionRange& visiblePositionRange) const
{
    if (visiblePositionRange.isNull())
        return IntRect();
    
    // Create a mutable VisiblePositionRange.
    VisiblePositionRange range(visiblePositionRange);
    LayoutRect rect1 = range.start.absoluteCaretBounds();
    LayoutRect rect2 = range.end.absoluteCaretBounds();
    
    // readjust for position at the edge of a line.  This is to exclude line rect that doesn't need to be accounted in the range bounds
    if (rect2.y() != rect1.y()) {
        VisiblePosition endOfFirstLine = endOfLine(range.start);
        if (range.start == endOfFirstLine) {
            range.start.setAffinity(DOWNSTREAM);
            rect1 = range.start.absoluteCaretBounds();
        }
        if (range.end == endOfFirstLine) {
            range.end.setAffinity(UPSTREAM);
            rect2 = range.end.absoluteCaretBounds();
        }
    }
    
    LayoutRect ourrect = rect1;
    ourrect.unite(rect2);
    
    // if the rectangle spans lines and contains multiple text chars, use the range's bounding box intead
    if (rect1.maxY() != rect2.maxY()) {
        RefPtr<Range> dataRange = makeRange(range.start, range.end);
        LayoutRect boundingBox = dataRange->boundingBox();
        String rangeString = plainText(dataRange.get());
        if (rangeString.length() > 1 && !boundingBox.isEmpty())
            ourrect = boundingBox;
    }
    
#if PLATFORM(MAC)
    return m_renderer->view().frameView().contentsToScreen(snappedIntRect(ourrect));
#else
    return snappedIntRect(ourrect);
#endif
}
    
void AccessibilityRenderObject::setSelectedVisiblePositionRange(const VisiblePositionRange& range) const
{
    if (range.start.isNull() || range.end.isNull())
        return;

    // make selection and tell the document to use it. if it's zero length, then move to that position
    if (range.start == range.end) {
        setTextSelectionIntent(axObjectCache(), AXTextStateChangeTypeSelectionMove);
        m_renderer->frame().selection().moveTo(range.start, UserTriggered);
        clearTextSelectionIntent(axObjectCache());
    }
    else {
        setTextSelectionIntent(axObjectCache(), AXTextStateChangeTypeSelectionExtend);
        VisibleSelection newSelection = VisibleSelection(range.start, range.end);
        m_renderer->frame().selection().setSelection(newSelection, FrameSelection::defaultSetSelectionOptions());
        clearTextSelectionIntent(axObjectCache());
    }
}

VisiblePosition AccessibilityRenderObject::visiblePositionForPoint(const IntPoint& point) const
{
    if (!m_renderer)
        return VisiblePosition();

    // convert absolute point to view coordinates
    RenderView* renderView = topRenderer();
    if (!renderView)
        return VisiblePosition();

#if PLATFORM(COCOA)
    FrameView* frameView = &renderView->frameView();
#endif

    Node* innerNode = nullptr;
    
    // locate the node containing the point
    LayoutPoint pointResult;
    while (1) {
        LayoutPoint ourpoint;
#if PLATFORM(MAC)
        ourpoint = frameView->screenToContents(point);
#else
        ourpoint = point;
#endif
        HitTestRequest request(HitTestRequest::ReadOnly |
                               HitTestRequest::Active);
        HitTestResult result(ourpoint);
        renderView->hitTest(request, result);
        innerNode = result.innerNode();
        if (!innerNode)
            return VisiblePosition();
        
        RenderObject* renderer = innerNode->renderer();
        if (!renderer)
            return VisiblePosition();
        
        pointResult = result.localPoint();

        // done if hit something other than a widget
        if (!is<RenderWidget>(*renderer))
            break;

        // descend into widget (FRAME, IFRAME, OBJECT...)
        Widget* widget = downcast<RenderWidget>(*renderer).widget();
        if (!is<FrameView>(widget))
            break;
        Frame& frame = downcast<FrameView>(*widget).frame();
        renderView = frame.document()->renderView();
#if PLATFORM(COCOA)
        frameView = downcast<FrameView>(widget);
#endif
    }
    
    return innerNode->renderer()->positionForPoint(pointResult, nullptr);
}

// NOTE: Consider providing this utility method as AX API
VisiblePosition AccessibilityRenderObject::visiblePositionForIndex(unsigned indexValue, bool lastIndexOK) const
{
    if (!isTextControl())
        return VisiblePosition();
    
    // lastIndexOK specifies whether the position after the last character is acceptable
    if (indexValue >= text().length()) {
        if (!lastIndexOK || indexValue > text().length())
            return VisiblePosition();
    }
    VisiblePosition position = visiblePositionForIndex(indexValue);
    position.setAffinity(DOWNSTREAM);
    return position;
}

// NOTE: Consider providing this utility method as AX API
int AccessibilityRenderObject::index(const VisiblePosition& position) const
{
    if (position.isNull() || !isTextControl())
        return -1;

    if (renderObjectContainsPosition(m_renderer, position.deepEquivalent()))
        return indexForVisiblePosition(position);
    
    return -1;
}

void AccessibilityRenderObject::lineBreaks(Vector<int>& lineBreaks) const
{
    if (!isTextControl())
        return;

    VisiblePosition visiblePos = visiblePositionForIndex(0);
    VisiblePosition savedVisiblePos = visiblePos;
    visiblePos = nextLinePosition(visiblePos, 0);
    while (!visiblePos.isNull() && visiblePos != savedVisiblePos) {
        lineBreaks.append(indexForVisiblePosition(visiblePos));
        savedVisiblePos = visiblePos;
        visiblePos = nextLinePosition(visiblePos, 0);
    }
}

// Given a line number, the range of characters of the text associated with this accessibility
// object that contains the line number.
PlainTextRange AccessibilityRenderObject::doAXRangeForLine(unsigned lineNumber) const
{
    if (!isTextControl())
        return PlainTextRange();
    
    // iterate to the specified line
    VisiblePosition visiblePos = visiblePositionForIndex(0);
    VisiblePosition savedVisiblePos;
    for (unsigned lineCount = lineNumber; lineCount; lineCount -= 1) {
        savedVisiblePos = visiblePos;
        visiblePos = nextLinePosition(visiblePos, 0);
        if (visiblePos.isNull() || visiblePos == savedVisiblePos)
            return PlainTextRange();
    }

    // Get the end of the line based on the starting position.
    VisiblePosition endPosition = endOfLine(visiblePos);

    int index1 = indexForVisiblePosition(visiblePos);
    int index2 = indexForVisiblePosition(endPosition);
    
    // add one to the end index for a line break not caused by soft line wrap (to match AppKit)
    if (endPosition.affinity() == DOWNSTREAM && endPosition.next().isNotNull())
        index2 += 1;
    
    // return nil rather than an zero-length range (to match AppKit)
    if (index1 == index2)
        return PlainTextRange();
    
    return PlainTextRange(index1, index2 - index1);
}

// The composed character range in the text associated with this accessibility object that
// is specified by the given index value. This parameterized attribute returns the complete
// range of characters (including surrogate pairs of multi-byte glyphs) at the given index.
PlainTextRange AccessibilityRenderObject::doAXRangeForIndex(unsigned index) const
{
    if (!isTextControl())
        return PlainTextRange();
    
    String elementText = text();
    if (!elementText.length() || index > elementText.length() - 1)
        return PlainTextRange();
    
    return PlainTextRange(index, 1);
}

// A substring of the text associated with this accessibility object that is
// specified by the given character range.
String AccessibilityRenderObject::doAXStringForRange(const PlainTextRange& range) const
{
    if (!range.length)
        return String();
    
    if (!isTextControl())
        return String();
    
    String elementText = isPasswordField() ? passwordFieldValue() : text();
    return elementText.substring(range.start, range.length);
}

// The bounding rectangle of the text associated with this accessibility object that is
// specified by the given range. This is the bounding rectangle a sighted user would see
// on the display screen, in pixels.
IntRect AccessibilityRenderObject::doAXBoundsForRange(const PlainTextRange& range) const
{
    if (allowsTextRanges())
        return boundsForVisiblePositionRange(visiblePositionRangeForRange(range));
    return IntRect();
}

AccessibilityObject* AccessibilityRenderObject::accessibilityImageMapHitTest(HTMLAreaElement* area, const IntPoint& point) const
{
    if (!area)
        return nullptr;

    AccessibilityObject* parent = nullptr;
    for (Element* mapParent = area->parentElement(); mapParent; mapParent = mapParent->parentElement()) {
        if (is<HTMLMapElement>(*mapParent)) {
            parent = accessibilityParentForImageMap(downcast<HTMLMapElement>(mapParent));
            break;
        }
    }
    if (!parent)
        return nullptr;
    
    for (const auto& child : parent->children()) {
        if (child->elementRect().contains(point))
            return child.get();
    }
    
    return nullptr;
}

AccessibilityObject* AccessibilityRenderObject::remoteSVGElementHitTest(const IntPoint& point) const
{
    AccessibilityObject* remote = remoteSVGRootElement();
    if (!remote)
        return nullptr;
    
    IntSize offset = point - roundedIntPoint(boundingBoxRect().location());
    return remote->accessibilityHitTest(IntPoint(offset));
}

AccessibilityObject* AccessibilityRenderObject::elementAccessibilityHitTest(const IntPoint& point) const
{
    if (isSVGImage())
        return remoteSVGElementHitTest(point);
    
    return AccessibilityObject::elementAccessibilityHitTest(point);
}
    
AccessibilityObject* AccessibilityRenderObject::accessibilityHitTest(const IntPoint& point) const
{
    if (!m_renderer || !m_renderer->hasLayer())
        return nullptr;
    
    m_renderer->document().updateLayout();

    RenderLayer* layer = downcast<RenderBox>(*m_renderer).layer();
     
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::AccessibilityHitTest);
    HitTestResult hitTestResult = HitTestResult(point);
    layer->hitTest(request, hitTestResult);
    if (!hitTestResult.innerNode())
        return nullptr;
    Node* node = hitTestResult.innerNode()->deprecatedShadowAncestorNode();
    ASSERT(node);

    if (is<HTMLAreaElement>(*node))
        return accessibilityImageMapHitTest(downcast<HTMLAreaElement>(node), point);
    
    if (is<HTMLOptionElement>(*node))
        node = downcast<HTMLOptionElement>(*node).ownerSelectElement();
    
    RenderObject* obj = node->renderer();
    if (!obj)
        return nullptr;
    
    AccessibilityObject* result = obj->document().axObjectCache()->getOrCreate(obj);
    result->updateChildrenIfNecessary();

    // Allow the element to perform any hit-testing it might need to do to reach non-render children.
    result = result->elementAccessibilityHitTest(point);
    
    if (result && result->accessibilityIsIgnored()) {
        // If this element is the label of a control, a hit test should return the control.
        AccessibilityObject* controlObject = result->correspondingControlForLabelElement();
        if (controlObject && !controlObject->exposesTitleUIElement())
            return controlObject;

        result = result->parentObjectUnignored();
    }

    return result;
}

bool AccessibilityRenderObject::shouldNotifyActiveDescendant() const
{
    // We want to notify that the combo box has changed its active descendant,
    // but we do not want to change the focus, because focus should remain with the combo box.
    if (isComboBox())
        return true;
    
    return shouldFocusActiveDescendant();
}

bool AccessibilityRenderObject::shouldFocusActiveDescendant() const
{
    switch (ariaRoleAttribute()) {
    case GroupRole:
    case ListBoxRole:
    case MenuRole:
    case MenuBarRole:
    case RadioGroupRole:
    case RowRole:
    case PopUpButtonRole:
    case ProgressIndicatorRole:
    case ToolbarRole:
    case OutlineRole:
    case TreeRole:
    case GridRole:
    /* FIXME: replace these with actual roles when they are added to AccessibilityRole
    composite
    alert
    alertdialog
    status
    timer
    */
        return true;
    default:
        return false;
    }
}

AccessibilityObject* AccessibilityRenderObject::activeDescendant() const
{
    if (!m_renderer)
        return nullptr;
    
    const AtomicString& activeDescendantAttrStr = getAttribute(aria_activedescendantAttr);
    if (activeDescendantAttrStr.isNull() || activeDescendantAttrStr.isEmpty())
        return nullptr;
    
    Element* element = this->element();
    if (!element)
        return nullptr;
    
    Element* target = element->treeScope().getElementById(activeDescendantAttrStr);
    if (!target)
        return nullptr;
    
    if (AXObjectCache* cache = axObjectCache()) {
        AccessibilityObject* obj = cache->getOrCreate(target);
        if (obj && obj->isAccessibilityRenderObject())
            // an activedescendant is only useful if it has a renderer, because that's what's needed to post the notification
            return obj;
    }
    
    return nullptr;
}

void AccessibilityRenderObject::handleAriaExpandedChanged()
{
    // This object might be deleted under the call to the parentObject() method.
    auto protectedThis = Ref<AccessibilityRenderObject>(*this);
    
    // Find if a parent of this object should handle aria-expanded changes.
    AccessibilityObject* containerParent = this->parentObject();
    while (containerParent) {
        bool foundParent = false;
        
        switch (containerParent->roleValue()) {
        case TreeRole:
        case TreeGridRole:
        case GridRole:
        case TableRole:
        case BrowserRole:
            foundParent = true;
            break;
        default:
            break;
        }
        
        if (foundParent)
            break;
        
        containerParent = containerParent->parentObject();
    }
    
    // Post that the row count changed.
    AXObjectCache* cache = axObjectCache();
    if (!cache)
        return;
    
    if (containerParent)
        cache->postNotification(containerParent, document(), AXObjectCache::AXRowCountChanged);

    // Post that the specific row either collapsed or expanded.
    if (roleValue() == RowRole || roleValue() == TreeItemRole)
        cache->postNotification(this, document(), isExpanded() ? AXObjectCache::AXRowExpanded : AXObjectCache::AXRowCollapsed);
    else
        cache->postNotification(this, document(), AXObjectCache::AXExpandedChanged);
}

void AccessibilityRenderObject::handleActiveDescendantChanged()
{
    Element* element = downcast<Element>(renderer()->node());
    if (!element)
        return;
    if (!renderer()->frame().selection().isFocusedAndActive() || renderer()->document().focusedElement() != element)
        return;

    if (activeDescendant() && shouldNotifyActiveDescendant())
        renderer()->document().axObjectCache()->postNotification(m_renderer, AXObjectCache::AXActiveDescendantChanged);
}

AccessibilityObject* AccessibilityRenderObject::correspondingControlForLabelElement() const
{
    HTMLLabelElement* labelElement = labelElementContainer();
    if (!labelElement)
        return nullptr;
    
    HTMLElement* correspondingControl = labelElement->control();
    if (!correspondingControl)
        return nullptr;

    // Make sure the corresponding control isn't a descendant of this label that's in the middle of being destroyed.
    if (correspondingControl->renderer() && !correspondingControl->renderer()->parent())
        return nullptr;
    
    return axObjectCache()->getOrCreate(correspondingControl);     
}

AccessibilityObject* AccessibilityRenderObject::correspondingLabelForControlElement() const
{
    if (!m_renderer)
        return nullptr;

    // ARIA: section 2A, bullet #3 says if aria-labeledby or aria-label appears, it should
    // override the "label" element association.
    if (hasTextAlternative())
        return nullptr;

    Node* node = m_renderer->node();
    if (is<HTMLElement>(node)) {
        if (HTMLLabelElement* label = labelForElement(downcast<HTMLElement>(node)))
            return axObjectCache()->getOrCreate(label);
    }

    return nullptr;
}

bool AccessibilityRenderObject::renderObjectIsObservable(RenderObject& renderer) const
{
    // AX clients will listen for AXValueChange on a text control.
    if (is<RenderTextControl>(renderer))
        return true;
    
    // AX clients will listen for AXSelectedChildrenChanged on listboxes.
    Node* node = renderer.node();
    if (!node)
        return false;
    
    if (nodeHasRole(node, "listbox") || (is<RenderBoxModelObject>(renderer) && downcast<RenderBoxModelObject>(renderer).isListBox()))
        return true;

    // Textboxes should send out notifications.
    if (nodeHasRole(node, "textbox") || (is<Element>(*node) && contentEditableAttributeIsEnabled(downcast<Element>(node))))
        return true;
    
    return false;
}
    
AccessibilityObject* AccessibilityRenderObject::observableObject() const
{
    // Find the object going up the parent chain that is used in accessibility to monitor certain notifications.
    for (RenderObject* renderer = m_renderer; renderer && renderer->node(); renderer = renderer->parent()) {
        if (renderObjectIsObservable(*renderer)) {
            if (AXObjectCache* cache = axObjectCache())
                return cache->getOrCreate(renderer);
        }
    }
    
    return nullptr;
}

bool AccessibilityRenderObject::isDescendantOfElementType(const QualifiedName& tagName) const
{
    for (auto& ancestor : ancestorsOfType<RenderElement>(*m_renderer)) {
        if (ancestor.element() && ancestor.element()->hasTagName(tagName))
            return true;
    }
    return false;
}
    
String AccessibilityRenderObject::expandedTextValue() const
{
    if (AccessibilityObject* parent = parentObject()) {
        if (parent->hasTagName(abbrTag) || parent->hasTagName(acronymTag))
            return parent->getAttribute(titleAttr);
    }
    
    return String();
}

bool AccessibilityRenderObject::supportsExpandedTextValue() const
{
    if (roleValue() == StaticTextRole) {
        if (AccessibilityObject* parent = parentObject())
            return parent->hasTagName(abbrTag) || parent->hasTagName(acronymTag);
    }
    
    return false;
}

AccessibilityRole AccessibilityRenderObject::determineAccessibilityRole()
{
    if (!m_renderer)
        return UnknownRole;

    if ((m_ariaRole = determineAriaRoleAttribute()) != UnknownRole)
        return m_ariaRole;
    
    Node* node = m_renderer->node();
    RenderBoxModelObject* cssBox = renderBoxModelObject();

    if (node && node->isLink())
        return WebCoreLinkRole;
    if (node && is<HTMLImageElement>(*node) && downcast<HTMLImageElement>(*node).fastHasAttribute(usemapAttr))
        return ImageMapRole;
    if ((cssBox && cssBox->isListItem()) || (node && node->hasTagName(liTag)))
        return ListItemRole;
    if (m_renderer->isListMarker())
        return ListMarkerRole;
    if (node && node->hasTagName(buttonTag))
        return buttonRoleType();
    if (node && node->hasTagName(legendTag))
        return LegendRole;
    if (m_renderer->isText())
        return StaticTextRole;
    if (cssBox && cssBox->isImage()) {
        if (is<HTMLInputElement>(node))
            return ariaHasPopup() ? PopUpButtonRole : ButtonRole;
        if (isSVGImage())
            return SVGRootRole;
        return ImageRole;
    }
    
    if (node && node->hasTagName(canvasTag))
        return CanvasRole;

    if (cssBox && cssBox->isRenderView())
        return WebAreaRole;
    
    if (cssBox && cssBox->isTextField()) {
        if (is<HTMLInputElement>(node))
            return downcast<HTMLInputElement>(*node).isSearchField() ? SearchFieldRole : TextFieldRole;
    }
    
    if (cssBox && cssBox->isTextArea())
        return TextAreaRole;

    if (is<HTMLInputElement>(node)) {
        HTMLInputElement& input = downcast<HTMLInputElement>(*node);
        if (input.isCheckbox())
            return CheckBoxRole;
        if (input.isRadioButton())
            return RadioButtonRole;
        if (input.isTextButton())
            return buttonRoleType();
        // On iOS, the date field is a popup button. On other platforms this is a text field.
#if PLATFORM(IOS)
        if (input.isDateField())
            return PopUpButtonRole;
#endif
        
#if ENABLE(INPUT_TYPE_COLOR)
        // FIXME: Shouldn't this use input.isColorControl()?
        const AtomicString& type = input.getAttribute(typeAttr);
        if (equalIgnoringCase(type, "color"))
            return ColorWellRole;
#endif
    }
    
    if (hasContentEditableAttributeSet())
        return TextAreaRole;
    
    if (isFileUploadButton())
        return ButtonRole;
    
    if (cssBox && cssBox->isMenuList())
        return PopUpButtonRole;
    
    if (headingLevel())
        return HeadingRole;
    
    if (m_renderer->isSVGImage())
        return ImageRole;
    if (m_renderer->isSVGRoot())
        return SVGRootRole;
    if (node && node->hasTagName(SVGNames::gTag))
        return GroupRole;

#if ENABLE(MATHML)
    if (node && node->hasTagName(MathMLNames::mathTag))
        return DocumentMathRole;
#endif
    // It's not clear which role a platform should choose for a math element.
    // Declaring a math element role should give flexibility to platforms to choose.
    if (isMathElement())
        return MathElementRole;
    
    if (node && node->hasTagName(ddTag))
        return DescriptionListDetailRole;
    
    if (node && node->hasTagName(dtTag))
        return DescriptionListTermRole;

    if (node && node->hasTagName(dlTag))
        return DescriptionListRole;

    // Check for Ruby elements
    if (m_renderer->isRubyText())
        return RubyTextRole;
    if (m_renderer->isRubyBase())
        return RubyBaseRole;
    if (m_renderer->isRubyRun())
        return RubyRunRole;
    if (m_renderer->isRubyBlock())
        return RubyBlockRole;
    if (m_renderer->isRubyInline())
        return RubyInlineRole;
    
    // This return value is what will be used if AccessibilityTableCell determines
    // the cell should not be treated as a cell (e.g. because it is a layout table.
    // In ATK, there is a distinction between generic text block elements and other
    // generic containers; AX API does not make this distinction.
    if (node && (node->hasTagName(tdTag) || node->hasTagName(thTag)))
#if PLATFORM(GTK) || PLATFORM(EFL)
        return DivRole;
#else
        return GroupRole;
#endif

    // Table sections should be ignored.
    if (m_renderer->isTableSection())
        return IgnoredRole;

    if (m_renderer->isHR())
        return HorizontalRuleRole;

    if (node && node->hasTagName(pTag))
        return ParagraphRole;

    if (is<HTMLLabelElement>(node))
        return LabelRole;

    if (node && node->hasTagName(dfnTag))
        return DefinitionRole;

    if (node && node->hasTagName(divTag))
        return DivRole;

    if (is<HTMLFormElement>(node))
        return FormRole;

    if (node && node->hasTagName(articleTag))
        return DocumentArticleRole;

    if (node && node->hasTagName(mainTag))
        return LandmarkMainRole;

    if (node && node->hasTagName(navTag))
        return LandmarkNavigationRole;

    if (node && node->hasTagName(asideTag))
        return LandmarkComplementaryRole;

    if (node && node->hasTagName(sectionTag))
        return DocumentRegionRole;

    if (node && node->hasTagName(addressTag))
        return LandmarkContentInfoRole;

    if (node && node->hasTagName(blockquoteTag))
        return BlockquoteRole;

    if (node && node->hasTagName(captionTag))
        return CaptionRole;

    if (node && node->hasTagName(preTag))
        return PreRole;

    if (is<HTMLDetailsElement>(node))
        return DetailsRole;
    if (is<HTMLSummaryElement>(node))
        return SummaryRole;

#if ENABLE(VIDEO)
    if (is<HTMLVideoElement>(node))
        return VideoRole;
    if (is<HTMLAudioElement>(node))
        return AudioRole;
#endif
    
    // The HTML element should not be exposed as an element. That's what the RenderView element does.
    if (node && node->hasTagName(htmlTag))
        return IgnoredRole;

    // There should only be one banner/contentInfo per page. If header/footer are being used within an article or section
    // then it should not be exposed as whole page's banner/contentInfo
    if (node && node->hasTagName(headerTag) && !isDescendantOfElementType(articleTag) && !isDescendantOfElementType(sectionTag))
        return LandmarkBannerRole;
    if (node && node->hasTagName(footerTag) && !isDescendantOfElementType(articleTag) && !isDescendantOfElementType(sectionTag))
        return FooterRole;

    if (m_renderer->isRenderBlockFlow())
        return GroupRole;
    
    // If the element does not have role, but it has ARIA attributes, or accepts tab focus, accessibility should fallback to exposing it as a group.
    if (supportsARIAAttributes() || canSetFocusAttribute())
        return GroupRole;
    
    // InlineRole is the final fallback before assigning UnknownRole to an object. It makes it
    // possible to distinguish truly unknown objects from non-focusable inline text elements
    // which have an event handler or attribute suggesting possible inclusion by the platform.
    if (is<RenderInline>(*m_renderer)
        && (hasAttributesRequiredForInclusion()
            || (node && node->hasEventListeners())
            || (supportsDatetimeAttribute() && !getAttribute(datetimeAttr).isEmpty())))
        return InlineRole;

    return UnknownRole;
}

AccessibilityOrientation AccessibilityRenderObject::orientation() const
{
    const AtomicString& ariaOrientation = getAttribute(aria_orientationAttr);
    if (equalIgnoringCase(ariaOrientation, "horizontal"))
        return AccessibilityOrientationHorizontal;
    if (equalIgnoringCase(ariaOrientation, "vertical"))
        return AccessibilityOrientationVertical;

    if (isScrollbar())
        return AccessibilityOrientationVertical;
    
    return AccessibilityObject::orientation();
}
    
bool AccessibilityRenderObject::inheritsPresentationalRole() const
{
    // ARIA states if an item can get focus, it should not be presentational.
    if (canSetFocusAttribute())
        return false;
    
    // ARIA spec says that when a parent object is presentational, and it has required child elements,
    // those child elements are also presentational. For example, <li> becomes presentational from <ul>.
    // http://www.w3.org/WAI/PF/aria/complete#presentation
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<QualifiedName>> listItemParents;
    static NeverDestroyed<HashSet<QualifiedName>> tableCellParents;
#else
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, listItemParents, 0);
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, tableCellParents, 0);
    if (!listItemParents)
        listItemParents = new HashSet<QualifiedName>();
    if (!tableCellParents)
        tableCellParents = new HashSet<QualifiedName>();
#endif

    HashSet<QualifiedName>* possibleParentTagNames = nullptr;
#if !PLATFORM(WKC)
    switch (roleValue()) {
    case ListItemRole:
    case ListMarkerRole:
        if (listItemParents.get().isEmpty()) {
            listItemParents.get().add(ulTag);
            listItemParents.get().add(olTag);
            listItemParents.get().add(dlTag);
        }
        possibleParentTagNames = &listItemParents.get();
        break;
    case CellRole:
        if (tableCellParents.get().isEmpty())
            tableCellParents.get().add(tableTag);
        possibleParentTagNames = &tableCellParents.get();
        break;
    default:
        break;
    }
#else
    switch (roleValue()) {
    case ListItemRole:
    case ListMarkerRole:
        if (listItemParents->isEmpty()) {
            listItemParents->add(ulTag);
            listItemParents->add(olTag);
            listItemParents->add(dlTag);
        }
        possibleParentTagNames = listItemParents;
        break;
    case CellRole:
        if (tableCellParents->isEmpty())
            tableCellParents->add(tableTag);
        possibleParentTagNames = tableCellParents;
        break;
    default:
        break;
    }
#endif
    
    // Not all elements need to check for this, only ones that are required children.
    if (!possibleParentTagNames)
        return false;
    
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) { 
        if (!is<AccessibilityRenderObject>(*parent))
            continue;
        
        Node* node = downcast<AccessibilityRenderObject>(*parent).node();
        if (!is<Element>(node))
            continue;
        
        // If native tag of the parent element matches an acceptable name, then return
        // based on its presentational status.
        if (possibleParentTagNames->contains(downcast<Element>(node)->tagQName()))
            return parent->roleValue() == PresentationalRole;
    }
    
    return false;
}
    
bool AccessibilityRenderObject::isPresentationalChildOfAriaRole() const
{
    // Walk the parent chain looking for a parent that has presentational children
    AccessibilityObject* parent;
    for (parent = parentObject(); parent && !parent->ariaRoleHasPresentationalChildren(); parent = parent->parentObject())
    { }
    
    return parent;
}
    
bool AccessibilityRenderObject::ariaRoleHasPresentationalChildren() const
{
    switch (m_ariaRole) {
    case ButtonRole:
    case SliderRole:
    case ImageRole:
    case ProgressIndicatorRole:
    case SpinButtonRole:
    // case SeparatorRole:
        return true;
    default:
        return false;
    }
}

bool AccessibilityRenderObject::canSetExpandedAttribute() const
{
    if (roleValue() == DetailsRole)
        return true;
    
    // An object can be expanded if it aria-expanded is true or false.
    const AtomicString& ariaExpanded = getAttribute(aria_expandedAttr);
    return equalIgnoringCase(ariaExpanded, "true") || equalIgnoringCase(ariaExpanded, "false");
}

bool AccessibilityRenderObject::canSetValueAttribute() const
{

    // In the event of a (Boolean)@readonly and (True/False/Undefined)@aria-readonly
    // value mismatch, the host language native attribute value wins.    
    if (isNativeTextControl())
        return !isReadOnly();

    if (isMeter())
        return false;

    if (equalIgnoringCase(getAttribute(aria_readonlyAttr), "true"))
        return false;
    
    if (equalIgnoringCase(getAttribute(aria_readonlyAttr), "false"))
        return true;

    if (isProgressIndicator() || isSlider())
        return true;

    if (isTextControl() && !isNativeTextControl())
        return true;

    // Any node could be contenteditable, so isReadOnly should be relied upon
    // for this information for all elements.
    return !isReadOnly();
}

bool AccessibilityRenderObject::canSetTextRangeAttributes() const
{
    return isTextControl();
}

void AccessibilityRenderObject::textChanged()
{
    // If this element supports ARIA live regions, or is part of a region with an ARIA editable role,
    // then notify the AT of changes.
    AXObjectCache* cache = axObjectCache();
    if (!cache)
        return;
    
    for (RenderObject* renderParent = m_renderer; renderParent; renderParent = renderParent->parent()) {
        AccessibilityObject* parent = cache->get(renderParent);
        if (!parent)
            continue;
        
        if (parent->supportsARIALiveRegion())
            cache->postNotification(renderParent, AXObjectCache::AXLiveRegionChanged);

        if ((parent->isARIATextControl() || parent->hasContentEditableAttributeSet()) && !parent->isNativeTextControl())
            cache->postNotification(renderParent, AXObjectCache::AXValueChanged);
    }
}

void AccessibilityRenderObject::clearChildren()
{
    AccessibilityObject::clearChildren();
    m_childrenDirty = false;
}

void AccessibilityRenderObject::addImageMapChildren()
{
    RenderBoxModelObject* cssBox = renderBoxModelObject();
    if (!is<RenderImage>(cssBox))
        return;
    
    HTMLMapElement* map = downcast<RenderImage>(*cssBox).imageMap();
    if (!map)
        return;

    for (auto& area : descendantsOfType<HTMLAreaElement>(*map)) {
        // add an <area> element for this child if it has a link
        if (!area.isLink())
            continue;
        auto& areaObject = downcast<AccessibilityImageMapLink>(*axObjectCache()->getOrCreate(ImageMapLinkRole));
        areaObject.setHTMLAreaElement(&area);
        areaObject.setHTMLMapElement(map);
        areaObject.setParent(this);
        if (!areaObject.accessibilityIsIgnored())
            m_children.append(&areaObject);
        else
            axObjectCache()->remove(areaObject.axObjectID());
    }
}

void AccessibilityRenderObject::updateChildrenIfNecessary()
{
    if (needsToUpdateChildren())
        clearChildren();        
    
    AccessibilityObject::updateChildrenIfNecessary();
}
    
void AccessibilityRenderObject::addTextFieldChildren()
{
    Node* node = this->node();
    if (!is<HTMLInputElement>(node))
        return;
    
    HTMLInputElement& input = downcast<HTMLInputElement>(*node);
    HTMLElement* spinButtonElement = input.innerSpinButtonElement();
    if (!is<SpinButtonElement>(spinButtonElement))
        return;

    auto& axSpinButton = downcast<AccessibilitySpinButton>(*axObjectCache()->getOrCreate(SpinButtonRole));
    axSpinButton.setSpinButtonElement(downcast<SpinButtonElement>(spinButtonElement));
    axSpinButton.setParent(this);
    m_children.append(&axSpinButton);
}
    
bool AccessibilityRenderObject::isSVGImage() const
{
    return remoteSVGRootElement();
}
    
void AccessibilityRenderObject::detachRemoteSVGRoot()
{
    if (AccessibilitySVGRoot* root = remoteSVGRootElement())
        root->setParent(nullptr);
}

AccessibilitySVGRoot* AccessibilityRenderObject::remoteSVGRootElement() const
{
    if (!is<RenderImage>(m_renderer))
        return nullptr;
    
    CachedImage* cachedImage = downcast<RenderImage>(*m_renderer).cachedImage();
    if (!cachedImage)
        return nullptr;
    
    Image* image = cachedImage->image();
    if (!is<SVGImage>(image))
        return nullptr;
    
    FrameView* frameView = downcast<SVGImage>(*image).frameView();
    if (!frameView)
        return nullptr;
    Frame& frame = frameView->frame();
    
    Document* document = frame.document();
    if (!is<SVGDocument>(document))
        return nullptr;
    
    SVGSVGElement* rootElement = downcast<SVGDocument>(*document).rootElement();
    if (!rootElement)
        return nullptr;
    RenderObject* rendererRoot = rootElement->renderer();
    if (!rendererRoot)
        return nullptr;
    
    AXObjectCache* cache = frame.document()->axObjectCache();
    if (!cache)
        return nullptr;
    AccessibilityObject* rootSVGObject = cache->getOrCreate(rendererRoot);

    // In order to connect the AX hierarchy from the SVG root element from the loaded resource
    // the parent must be set, because there's no other way to get back to who created the image.
    ASSERT(rootSVGObject);
    if (!is<AccessibilitySVGRoot>(*rootSVGObject))
        return nullptr;
    
    return downcast<AccessibilitySVGRoot>(rootSVGObject);
}
    
void AccessibilityRenderObject::addRemoteSVGChildren()
{
    AccessibilitySVGRoot* root = remoteSVGRootElement();
    if (!root)
        return;
    
    root->setParent(this);
    
    if (root->accessibilityIsIgnored()) {
        for (const auto& child : root->children())
            m_children.append(child);
    } else
        m_children.append(root);
}

void AccessibilityRenderObject::addCanvasChildren()
{
    // Add the unrendered canvas children as AX nodes, unless we're not using a canvas renderer
    // because JS is disabled for example.
    if (!node() || !node()->hasTagName(canvasTag) || (renderer() && !renderer()->isCanvas()))
        return;

    // If it's a canvas, it won't have rendered children, but it might have accessible fallback content.
    // Clear m_haveChildren because AccessibilityNodeObject::addChildren will expect it to be false.
    ASSERT(!m_children.size());
    m_haveChildren = false;
    AccessibilityNodeObject::addChildren();
}

void AccessibilityRenderObject::addAttachmentChildren()
{
    if (!isAttachment())
        return;

    // FrameView's need to be inserted into the AX hierarchy when encountered.
    Widget* widget = widgetForAttachmentView();
    if (!widget || !widget->isFrameView())
        return;
    
    AccessibilityObject* axWidget = axObjectCache()->getOrCreate(widget);
    if (!axWidget->accessibilityIsIgnored())
        m_children.append(axWidget);
}

#if PLATFORM(COCOA)
void AccessibilityRenderObject::updateAttachmentViewParents()
{
    // Only the unignored parent should set the attachment parent, because that's what is reflected in the AX 
    // hierarchy to the client.
    if (accessibilityIsIgnored())
        return;
    
    for (const auto& child : m_children) {
        if (child->isAttachment())
            child->overrideAttachmentParent(this);
    }
}
#endif

// Hidden children are those that are not rendered or visible, but are specifically marked as aria-hidden=false,
// meaning that they should be exposed to the AX hierarchy.
void AccessibilityRenderObject::addHiddenChildren()
{
    Node* node = this->node();
    if (!node)
        return;
    
    // First do a quick run through to determine if we have any hidden nodes (most often we will not).
    // If we do have hidden nodes, we need to determine where to insert them so they match DOM order as close as possible.
    bool shouldInsertHiddenNodes = false;
    for (Node* child = node->firstChild(); child; child = child->nextSibling()) {
        if (!child->renderer() && isNodeAriaVisible(child)) {
            shouldInsertHiddenNodes = true;
            break;
        }
    }
    
    if (!shouldInsertHiddenNodes)
        return;
    
    // Iterate through all of the children, including those that may have already been added, and
    // try to insert hidden nodes in the correct place in the DOM order.
    unsigned insertionIndex = 0;
    for (Node* child = node->firstChild(); child; child = child->nextSibling()) {
        if (child->renderer()) {
            // Find out where the last render sibling is located within m_children.
            AccessibilityObject* childObject = axObjectCache()->get(child->renderer());
            if (childObject && childObject->accessibilityIsIgnored()) {
                auto& children = childObject->children();
                if (children.size())
                    childObject = children.last().get();
                else
                    childObject = nullptr;
            }

            if (childObject)
                insertionIndex = m_children.find(childObject) + 1;
            continue;
        }

        if (!isNodeAriaVisible(child))
            continue;
        
        unsigned previousSize = m_children.size();
        if (insertionIndex > previousSize)
            insertionIndex = previousSize;
        
        insertChild(axObjectCache()->getOrCreate(child), insertionIndex);
        insertionIndex += (m_children.size() - previousSize);
    }
}
    
void AccessibilityRenderObject::updateRoleAfterChildrenCreation()
{
    // If a menu does not have valid menuitem children, it should not be exposed as a menu.
    if (roleValue() == MenuRole) {
        // Elements marked as menus must have at least one menu item child.
        size_t menuItemCount = 0;
        for (const auto& child : children()) {
            if (child->isMenuItem()) {
                menuItemCount++;
                break;
            }
        }

        if (!menuItemCount)
            m_role = GroupRole;
    }
}
    
void AccessibilityRenderObject::addChildren()
{
    // If the need to add more children in addition to existing children arises, 
    // childrenChanged should have been called, leaving the object with no children.
    ASSERT(!m_haveChildren); 

    m_haveChildren = true;
    
    if (!canHaveChildren())
        return;
    
    for (RefPtr<AccessibilityObject> obj = firstChild(); obj; obj = obj->nextSibling())
        addChild(obj.get());
    
    addHiddenChildren();
    addAttachmentChildren();
    addImageMapChildren();
    addTextFieldChildren();
    addCanvasChildren();
    addRemoteSVGChildren();
    
#if PLATFORM(COCOA)
    updateAttachmentViewParents();
#endif
    
    updateRoleAfterChildrenCreation();
}

bool AccessibilityRenderObject::canHaveChildren() const
{
    if (!m_renderer)
        return false;

    return AccessibilityNodeObject::canHaveChildren();
}

const String AccessibilityRenderObject::ariaLiveRegionStatus() const
{
    const AtomicString& liveRegionStatus = getAttribute(aria_liveAttr);
    // These roles have implicit live region status.
    if (liveRegionStatus.isEmpty())
        return defaultLiveRegionStatusForRole(roleValue());

    return liveRegionStatus;
}

const AtomicString& AccessibilityRenderObject::ariaLiveRegionRelevant() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const AtomicString> defaultLiveRegionRelevant("additions text", AtomicString::ConstructFromLiteral);
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, defaultLiveRegionRelevant_, 0);
    if (!defaultLiveRegionRelevant_)
        defaultLiveRegionRelevant_ = new AtomicString("additions text", AtomicString::ConstructFromLiteral);
    const AtomicString& defaultLiveRegionRelevant = *defaultLiveRegionRelevant_;
#endif
    const AtomicString& relevant = getAttribute(aria_relevantAttr);

    // Default aria-relevant = "additions text".
    if (relevant.isEmpty())
        return defaultLiveRegionRelevant;
    
    return relevant;
}

bool AccessibilityRenderObject::ariaLiveRegionAtomic() const
{
    const AtomicString& atomic = getAttribute(aria_atomicAttr);
    if (equalIgnoringCase(atomic, "true"))
        return true;
    if (equalIgnoringCase(atomic, "false"))
        return false;
    // WAI-ARIA "alert" and "status" roles have an implicit aria-atomic value of true.
    switch (roleValue()) {
    case ApplicationAlertRole:
    case ApplicationStatusRole:
        return true;
    default:
        return false;
    }
}

bool AccessibilityRenderObject::ariaLiveRegionBusy() const
{
    return elementAttributeValue(aria_busyAttr);    
}
    
void AccessibilityRenderObject::ariaSelectedRows(AccessibilityChildrenVector& result)
{
    // Determine which rows are selected.
    bool isMulti = isMultiSelectable();

    // Prefer active descendant over aria-selected.
    AccessibilityObject* activeDesc = activeDescendant();
    if (activeDesc && (activeDesc->isTreeItem() || activeDesc->isTableRow())) {
        result.append(activeDesc);    
        if (!isMulti)
            return;
    }

    // Get all the rows.
    auto rowsIteration = [&](const AccessibilityChildrenVector& rows) {
        for (auto& row : rows) {
            if (row->isSelected()) {
                result.append(row);
                if (!isMulti)
                    break;
            }
        }
    };
    if (isTree()) {
        AccessibilityChildrenVector allRows;
        ariaTreeRows(allRows);
        rowsIteration(allRows);
    } else if (is<AccessibilityTable>(*this)) {
        auto& thisTable = downcast<AccessibilityTable>(*this);
        if (thisTable.isExposableThroughAccessibility() && thisTable.supportsSelectedRows())
            rowsIteration(thisTable.rows());
    }
}
    
void AccessibilityRenderObject::ariaListboxSelectedChildren(AccessibilityChildrenVector& result)
{
    bool isMulti = isMultiSelectable();

    for (const auto& child : children()) {
        // Every child should have aria-role option, and if so, check for selected attribute/state.
        if (child->isSelected() && child->ariaRoleAttribute() == ListBoxOptionRole) {
            result.append(child);
            if (!isMulti)
                return;
        }
    }
}

void AccessibilityRenderObject::selectedChildren(AccessibilityChildrenVector& result)
{
    ASSERT(result.isEmpty());

    // only listboxes should be asked for their selected children. 
    AccessibilityRole role = roleValue();
    if (role == ListBoxRole) // native list boxes would be AccessibilityListBoxes, so only check for aria list boxes
        ariaListboxSelectedChildren(result);
    else if (role == TreeRole || role == TreeGridRole || role == TableRole)
        ariaSelectedRows(result);
}

void AccessibilityRenderObject::ariaListboxVisibleChildren(AccessibilityChildrenVector& result)      
{
    if (!hasChildren())
        addChildren();
    
    for (const auto& child : children()) {
        if (child->isOffScreen())
            result.append(child);
    }
}

void AccessibilityRenderObject::visibleChildren(AccessibilityChildrenVector& result)
{
    ASSERT(result.isEmpty());
        
    // only listboxes are asked for their visible children. 
    if (ariaRoleAttribute() != ListBoxRole) { // native list boxes would be AccessibilityListBoxes, so only check for aria list boxes
        ASSERT_NOT_REACHED();
        return;
    }
    return ariaListboxVisibleChildren(result);
}
 
void AccessibilityRenderObject::tabChildren(AccessibilityChildrenVector& result)
{
    ASSERT(roleValue() == TabListRole);
    
    for (const auto& child : children()) {
        if (child->isTabItem())
            result.append(child);
    }
}
    
const String& AccessibilityRenderObject::actionVerb() const
{
#if !PLATFORM(IOS) && !PLATFORM(WKC)
    // FIXME: Need to add verbs for select elements.
    static NeverDestroyed<const String> buttonAction(AXButtonActionVerb());
    static NeverDestroyed<const String> textFieldAction(AXTextFieldActionVerb());
    static NeverDestroyed<const String> radioButtonAction(AXRadioButtonActionVerb());
    static NeverDestroyed<const String> checkedCheckBoxAction(AXUncheckedCheckBoxActionVerb());
    static NeverDestroyed<const String> uncheckedCheckBoxAction(AXUncheckedCheckBoxActionVerb());
    static NeverDestroyed<const String> linkAction(AXLinkActionVerb());

    switch (roleValue()) {
    case ButtonRole:
    case ToggleButtonRole:
        return buttonAction;
    case TextFieldRole:
    case TextAreaRole:
        return textFieldAction;
    case RadioButtonRole:
        return radioButtonAction;
    case CheckBoxRole:
        return isChecked() ? checkedCheckBoxAction : uncheckedCheckBoxAction;
    case LinkRole:
    case WebCoreLinkRole:
        return linkAction;
    default:
        return nullAtom;
    }
#elif PLATFORM(WKC)
    // FIXME: Need to add verbs for select elements.
    WKC_DEFINE_STATIC_PTR(const String*, buttonAction, 0);
    WKC_DEFINE_STATIC_PTR(const String*, textFieldAction, 0);
    WKC_DEFINE_STATIC_PTR(const String*, radioButtonAction, 0);
    WKC_DEFINE_STATIC_PTR(const String*, checkedCheckBoxAction, 0);
    WKC_DEFINE_STATIC_PTR(const String*, uncheckedCheckBoxAction, 0);
    WKC_DEFINE_STATIC_PTR(const String*, linkAction, 0);

    if (!buttonAction) {
        buttonAction = new String(AXButtonActionVerb());
        textFieldAction = new String(AXTextFieldActionVerb());
        radioButtonAction = new String(AXRadioButtonActionVerb());
        checkedCheckBoxAction = new String(AXUncheckedCheckBoxActionVerb());
        uncheckedCheckBoxAction = new String(AXUncheckedCheckBoxActionVerb());
        linkAction = new String(AXLinkActionVerb());
    }

    switch (roleValue()) {
    case ButtonRole:
    case ToggleButtonRole:
        return *buttonAction;
    case TextFieldRole:
    case TextAreaRole:
        return *textFieldAction;
    case RadioButtonRole:
        return *radioButtonAction;
    case CheckBoxRole:
        return isChecked() ? *checkedCheckBoxAction : *uncheckedCheckBoxAction;
    case LinkRole:
    case WebCoreLinkRole:
        return *linkAction;
    default:
        return nullAtom;
    }
#else
    return nullAtom;
#endif
}
    
void AccessibilityRenderObject::setAccessibleName(const AtomicString& name)
{
    // Setting the accessible name can store the value in the DOM
    if (!m_renderer)
        return;

    Node* node = nullptr;
    // For web areas, set the aria-label on the HTML element.
    if (isWebArea())
        node = m_renderer->document().documentElement();
    else
        node = m_renderer->node();

    if (is<Element>(node))
        downcast<Element>(*node).setAttribute(aria_labelAttr, name);
}
    
static bool isLinkable(const AccessibilityRenderObject& object)
{
    if (!object.renderer())
        return false;

    // See https://wiki.mozilla.org/Accessibility/AT-Windows-API for the elements
    // Mozilla considers linkable.
    return object.isLink() || object.isImage() || object.renderer()->isText();
}

String AccessibilityRenderObject::stringValueForMSAA() const
{
    if (isLinkable(*this)) {
        Element* anchor = anchorElement();
        if (is<HTMLAnchorElement>(anchor))
            return downcast<HTMLAnchorElement>(*anchor).href();
    }

    return stringValue();
}

bool AccessibilityRenderObject::isLinked() const
{
    if (!isLinkable(*this))
        return false;

    Element* anchor = anchorElement();
    if (!is<HTMLAnchorElement>(anchor))
        return false;

    return !downcast<HTMLAnchorElement>(*anchor).href().isEmpty();
}

bool AccessibilityRenderObject::hasBoldFont() const
{
    if (!m_renderer)
        return false;
    
    return m_renderer->style().fontDescription().weight() >= FontWeightBold;
}

bool AccessibilityRenderObject::hasItalicFont() const
{
    if (!m_renderer)
        return false;
    
    return m_renderer->style().fontDescription().italic() == FontItalicOn;
}

bool AccessibilityRenderObject::hasPlainText() const
{
    if (!m_renderer)
        return false;
    
    const RenderStyle& style = m_renderer->style();
    
    return style.fontDescription().weight() == FontWeightNormal
        && style.fontDescription().italic() == FontItalicOff
        && style.textDecorationsInEffect() == TextDecorationNone;
}

bool AccessibilityRenderObject::hasSameFont(RenderObject* renderer) const
{
    if (!m_renderer || !renderer)
        return false;
    
    return m_renderer->style().fontDescription().families() == renderer->style().fontDescription().families();
}

bool AccessibilityRenderObject::hasSameFontColor(RenderObject* renderer) const
{
    if (!m_renderer || !renderer)
        return false;
    
    return m_renderer->style().visitedDependentColor(CSSPropertyColor) == renderer->style().visitedDependentColor(CSSPropertyColor);
}

bool AccessibilityRenderObject::hasSameStyle(RenderObject* renderer) const
{
    if (!m_renderer || !renderer)
        return false;
    
    return m_renderer->style() == renderer->style();
}

bool AccessibilityRenderObject::hasUnderline() const
{
    if (!m_renderer)
        return false;
    
    return m_renderer->style().textDecorationsInEffect() & TextDecorationUnderline;
}

String AccessibilityRenderObject::nameForMSAA() const
{
    if (m_renderer && m_renderer->isText())
        return textUnderElement();

    return title();
}

static bool shouldReturnTagNameAsRoleForMSAA(const Element& element)
{
    // See "document structure",
    // https://wiki.mozilla.org/Accessibility/AT-Windows-API
    // FIXME: Add the other tag names that should be returned as the role.
    return element.hasTagName(h1Tag) || element.hasTagName(h2Tag) 
        || element.hasTagName(h3Tag) || element.hasTagName(h4Tag)
        || element.hasTagName(h5Tag) || element.hasTagName(h6Tag);
}

String AccessibilityRenderObject::stringRoleForMSAA() const
{
    if (!m_renderer)
        return String();

    Node* node = m_renderer->node();
    if (!is<Element>(node))
        return String();

    Element& element = downcast<Element>(*node);
    if (!shouldReturnTagNameAsRoleForMSAA(element))
        return String();

    return element.tagName();
}

String AccessibilityRenderObject::positionalDescriptionForMSAA() const
{
    // See "positional descriptions",
    // https://wiki.mozilla.org/Accessibility/AT-Windows-API
    if (isHeading())
        return "L" + String::number(headingLevel());

    // FIXME: Add positional descriptions for other elements.
    return String();
}

String AccessibilityRenderObject::descriptionForMSAA() const
{
    String description = positionalDescriptionForMSAA();
    if (!description.isEmpty())
        return description;

    description = accessibilityDescription();
    if (!description.isEmpty()) {
        // From the Mozilla MSAA implementation:
        // "Signal to screen readers that this description is speakable and is not
        // a formatted positional information description. Don't localize the
        // 'Description: ' part of this string, it will be parsed out by assistive
        // technologies."
        return "Description: " + description;
    }

    return String();
}

static AccessibilityRole msaaRoleForRenderer(const RenderObject* renderer)
{
    if (!renderer)
        return UnknownRole;

    if (is<RenderText>(*renderer))
        return EditableTextRole;

    if (is<RenderListItem>(*renderer))
        return ListItemRole;

    return UnknownRole;
}

AccessibilityRole AccessibilityRenderObject::roleValueForMSAA() const
{
    if (m_roleForMSAA != UnknownRole)
        return m_roleForMSAA;

    m_roleForMSAA = msaaRoleForRenderer(m_renderer);

    if (m_roleForMSAA == UnknownRole)
        m_roleForMSAA = roleValue();

    return m_roleForMSAA;
}

String AccessibilityRenderObject::passwordFieldValue() const
{
    ASSERT(isPasswordField());

    // Look for the RenderText object in the RenderObject tree for this input field.
    RenderObject* renderer = node()->renderer();
    while (renderer && !is<RenderText>(renderer))
        renderer = downcast<RenderElement>(*renderer).firstChild();

    if (!is<RenderText>(renderer))
        return String();

    // Return the text that is actually being rendered in the input field.
    return downcast<RenderText>(*renderer).textWithoutConvertingBackslashToYenSymbol();
}

ScrollableArea* AccessibilityRenderObject::getScrollableAreaIfScrollable() const
{
    // If the parent is a scroll view, then this object isn't really scrollable, the parent ScrollView should handle the scrolling.
    if (parentObject() && parentObject()->isAccessibilityScrollView())
        return nullptr;

    if (!is<RenderBox>(m_renderer))
        return nullptr;

    auto& box = downcast<RenderBox>(*m_renderer);
    if (!box.canBeScrolledAndHasScrollableArea())
        return nullptr;

    return box.layer();
}

void AccessibilityRenderObject::scrollTo(const IntPoint& point) const
{
    if (!is<RenderBox>(m_renderer))
        return;

    auto& box = downcast<RenderBox>(*m_renderer);
    if (!box.canBeScrolledAndHasScrollableArea())
        return;

    box.layer()->scrollToOffset(toIntSize(point), RenderLayer::ScrollOffsetClamped);
}

#if ENABLE(MATHML)
bool AccessibilityRenderObject::isMathElement() const
{
    if (!m_renderer)
        return false;
    
    return is<MathMLElement>(node());
}

bool AccessibilityRenderObject::isMathFraction() const
{
    return m_renderer && m_renderer->isRenderMathMLFraction();
}

bool AccessibilityRenderObject::isMathFenced() const
{
    return m_renderer && m_renderer->isRenderMathMLFenced();
}

bool AccessibilityRenderObject::isMathSubscriptSuperscript() const
{
    return m_renderer && m_renderer->isRenderMathMLScripts() && !isMathMultiscript();
}

bool AccessibilityRenderObject::isMathRow() const
{
    return m_renderer && m_renderer->isRenderMathMLRow();
}

bool AccessibilityRenderObject::isMathUnderOver() const
{
    return m_renderer && m_renderer->isRenderMathMLUnderOver();
}

bool AccessibilityRenderObject::isMathSquareRoot() const
{
    return m_renderer && m_renderer->isRenderMathMLSquareRoot();
}
    
bool AccessibilityRenderObject::isMathToken() const
{
    return m_renderer && m_renderer->isRenderMathMLToken();
}

bool AccessibilityRenderObject::isMathRoot() const
{
    return m_renderer && m_renderer->isRenderMathMLRoot();
}

bool AccessibilityRenderObject::isMathOperator() const
{
    if (!m_renderer || !m_renderer->isRenderMathMLOperator())
        return false;

    return true;
}

bool AccessibilityRenderObject::isMathFenceOperator() const
{
    if (!is<RenderMathMLOperator>(m_renderer))
        return false;

    return downcast<RenderMathMLOperator>(*m_renderer).hasOperatorFlag(MathMLOperatorDictionary::Fence);
}

bool AccessibilityRenderObject::isMathSeparatorOperator() const
{
    if (!is<RenderMathMLOperator>(m_renderer))
        return false;

    return downcast<RenderMathMLOperator>(*m_renderer).hasOperatorFlag(MathMLOperatorDictionary::Separator);
}
    
bool AccessibilityRenderObject::isMathText() const
{
    return node() && (node()->hasTagName(MathMLNames::mtextTag) || hasTagName(MathMLNames::msTag));
}

bool AccessibilityRenderObject::isMathNumber() const
{
    return node() && node()->hasTagName(MathMLNames::mnTag);
}

bool AccessibilityRenderObject::isMathIdentifier() const
{
    return node() && node()->hasTagName(MathMLNames::miTag);
}

bool AccessibilityRenderObject::isMathMultiscript() const
{
    return node() && node()->hasTagName(MathMLNames::mmultiscriptsTag);
}
    
bool AccessibilityRenderObject::isMathTable() const
{
    return node() && node()->hasTagName(MathMLNames::mtableTag);
}

bool AccessibilityRenderObject::isMathTableRow() const
{
    return node() && (node()->hasTagName(MathMLNames::mtrTag) || hasTagName(MathMLNames::mlabeledtrTag));
}

bool AccessibilityRenderObject::isMathTableCell() const
{
    return node() && node()->hasTagName(MathMLNames::mtdTag);
}

bool AccessibilityRenderObject::isMathScriptObject(AccessibilityMathScriptObjectType type) const
{
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent)
        return false;

    return type == Subscript ? this == parent->mathSubscriptObject() : this == parent->mathSuperscriptObject();
}

bool AccessibilityRenderObject::isMathMultiscriptObject(AccessibilityMathMultiscriptObjectType type) const
{
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent || !parent->isMathMultiscript())
        return false;

    // The scripts in a MathML <mmultiscripts> element consist of one or more
    // subscript, superscript pairs. In order to determine if this object is
    // a scripted token, we need to examine each set of pairs to see if the
    // this token is present and in the position corresponding with the type.

    AccessibilityMathMultiscriptPairs pairs;
    if (type == PreSubscript || type == PreSuperscript)
        parent->mathPrescripts(pairs);
    else
        parent->mathPostscripts(pairs);

    for (const auto& pair : pairs) {
        if (this == pair.first)
            return (type == PreSubscript || type == PostSubscript);
        if (this == pair.second)
            return (type == PreSuperscript || type == PostSuperscript);
    }

    return false;
}
    
bool AccessibilityRenderObject::isIgnoredElementWithinMathTree() const
{
    if (!m_renderer)
        return true;
    
    // We ignore anonymous renderers inside math blocks.
    // However, we do not exclude anonymous RenderMathMLOperator nodes created by the mfenced element nor RenderText nodes created by math operators so that the text can be exposed by AccessibilityRenderObject::textUnderElement.
    if (m_renderer->isAnonymous()) {
        if (m_renderer->isRenderMathMLOperator())
            return false;
        for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
            if (parent->isMathElement())
                return !(m_renderer->isText() && ancestorsOfType<RenderMathMLOperator>(*m_renderer).first());
        }
    }

    // Only math elements that we explicitly recognize should be included
    // We don't want things like <mstyle> to appear in the tree.
    if (isMathElement()) {
        if (isMathFraction() || isMathFenced() || isMathSubscriptSuperscript() || isMathRow()
            || isMathUnderOver() || isMathRoot() || isMathText() || isMathNumber()
            || isMathOperator() || isMathFenceOperator() || isMathSeparatorOperator()
            || isMathIdentifier() || isMathTable() || isMathTableRow() || isMathTableCell() || isMathMultiscript())
            return false;
        return true;
    }

    return false;
}

AccessibilityObject* AccessibilityRenderObject::mathRadicandObject()
{
    if (!isMathRoot())
        return nullptr;
    RenderMathMLRoot* root = &downcast<RenderMathMLRoot>(*m_renderer);
    AccessibilityObject* rootRadicandWrapper = axObjectCache()->getOrCreate(root->baseWrapper());
    if (!rootRadicandWrapper)
        return nullptr;
    AccessibilityObject* rootRadicand = rootRadicandWrapper->children().first().get();
    ASSERT(rootRadicand && children().contains(rootRadicand));
    return rootRadicand;
}

AccessibilityObject* AccessibilityRenderObject::mathRootIndexObject()
{
    if (!isMathRoot())
        return nullptr;
    RenderMathMLRoot* root = &downcast<RenderMathMLRoot>(*m_renderer);
    AccessibilityObject* rootIndexWrapper = axObjectCache()->getOrCreate(root->indexWrapper());
    if (!rootIndexWrapper)
        return nullptr;
    AccessibilityObject* rootIndex = rootIndexWrapper->children().first().get();
    ASSERT(rootIndex && children().contains(rootIndex));
    return rootIndex;
}

AccessibilityObject* AccessibilityRenderObject::mathNumeratorObject()
{
    if (!isMathFraction())
        return nullptr;
    
    const auto& children = this->children();
    if (children.size() != 2)
        return nullptr;
    
    return children[0].get();
}
    
AccessibilityObject* AccessibilityRenderObject::mathDenominatorObject()
{
    if (!isMathFraction())
        return nullptr;

    const auto& children = this->children();
    if (children.size() != 2)
        return nullptr;
    
    return children[1].get();
}
    
AccessibilityObject* AccessibilityRenderObject::mathUnderObject()
{
    if (!isMathUnderOver() || !node())
        return nullptr;
    
    const auto& children = this->children();
    if (children.size() < 2)
        return nullptr;
    
    if (node()->hasTagName(MathMLNames::munderTag) || node()->hasTagName(MathMLNames::munderoverTag))
        return children[1].get();
    
    return nullptr;
}

AccessibilityObject* AccessibilityRenderObject::mathOverObject()
{
    if (!isMathUnderOver() || !node())
        return nullptr;
    
    const auto& children = this->children();
    if (children.size() < 2)
        return nullptr;
    
    if (node()->hasTagName(MathMLNames::moverTag))
        return children[1].get();
    if (node()->hasTagName(MathMLNames::munderoverTag))
        return children[2].get();

    return nullptr;
}

AccessibilityObject* AccessibilityRenderObject::mathBaseObject()
{
    if (!isMathSubscriptSuperscript() && !isMathUnderOver() && !isMathMultiscript())
        return nullptr;
    
    const auto& children = this->children();
    // The base object in question is always the first child.
    if (children.size() > 0)
        return children[0].get();

    return nullptr;
}

AccessibilityObject* AccessibilityRenderObject::mathSubscriptObject()
{
    if (!isMathSubscriptSuperscript() || !node())
        return nullptr;
    
    const auto& children = this->children();
    if (children.size() < 2)
        return nullptr;

    if (node()->hasTagName(MathMLNames::msubTag) || node()->hasTagName(MathMLNames::msubsupTag))
        return children[1].get();
    
    return nullptr;
}

AccessibilityObject* AccessibilityRenderObject::mathSuperscriptObject()
{
    if (!isMathSubscriptSuperscript() || !node())
        return nullptr;
    
    const auto& children = this->children();
    unsigned count = children.size();

    if (count >= 2 && node()->hasTagName(MathMLNames::msupTag))
        return children[1].get();

    if (count >= 3 && node()->hasTagName(MathMLNames::msubsupTag))
        return children[2].get();
    
    return nullptr;
}
    
String AccessibilityRenderObject::mathFencedOpenString() const
{
    if (!isMathFenced())
        return String();
    
    return getAttribute(MathMLNames::openAttr);
}

String AccessibilityRenderObject::mathFencedCloseString() const
{
    if (!isMathFenced())
        return String();
    
    return getAttribute(MathMLNames::closeAttr);
}
    
void AccessibilityRenderObject::mathPrescripts(AccessibilityMathMultiscriptPairs& prescripts)
{
    if (!isMathMultiscript() || !node())
        return;
    
    bool foundPrescript = false;
    std::pair<AccessibilityObject*, AccessibilityObject*> prescriptPair;
    for (Node* child = node()->firstChild(); child; child = child->nextSibling()) {
        if (foundPrescript) {
            AccessibilityObject* axChild = axObjectCache()->getOrCreate(child);
            if (axChild && axChild->isMathElement()) {
                if (!prescriptPair.first)
                    prescriptPair.first = axChild;
                else {
                    prescriptPair.second = axChild;
                    prescripts.append(prescriptPair);
                    prescriptPair.first = nullptr;
                    prescriptPair.second = nullptr;
                }
            }
        } else if (child->hasTagName(MathMLNames::mprescriptsTag))
            foundPrescript = true;
    }
    
    // Handle the odd number of pre scripts case.
    if (prescriptPair.first)
        prescripts.append(prescriptPair);
}

void AccessibilityRenderObject::mathPostscripts(AccessibilityMathMultiscriptPairs& postscripts)
{
    if (!isMathMultiscript() || !node())
        return;

    // In Multiscripts, the post-script elements start after the first element (which is the base)
    // and continue until a <mprescripts> tag is found
    std::pair<AccessibilityObject*, AccessibilityObject*> postscriptPair;
    bool foundBaseElement = false;
    for (Node* child = node()->firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(MathMLNames::mprescriptsTag))
            break;

        AccessibilityObject* axChild = axObjectCache()->getOrCreate(child);
        if (axChild && axChild->isMathElement()) {
            if (!foundBaseElement)
                foundBaseElement = true;
            else if (!postscriptPair.first)
                postscriptPair.first = axChild;
            else {
                postscriptPair.second = axChild;
                postscripts.append(postscriptPair);
                postscriptPair.first = nullptr;
                postscriptPair.second = nullptr;
            }
        }
    }

    // Handle the odd number of post scripts case.
    if (postscriptPair.first)
        postscripts.append(postscriptPair);
}

int AccessibilityRenderObject::mathLineThickness() const
{
    if (!is<RenderMathMLFraction>(m_renderer))
        return -1;
    
    return downcast<RenderMathMLFraction>(*m_renderer).lineThickness();
}

#endif
    
} // namespace WebCore
