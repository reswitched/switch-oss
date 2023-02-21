/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2008, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 * Copyright (C) 2014 Adobe Systems Incorporated. All rights reserved.
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
#include "SVGElement.h"

#include "CSSCursorImageValue.h"
#include "CSSParser.h"
#include "DOMImplementation.h"
#include "Document.h"
#include "ElementIterator.h"
#include "Event.h"
#include "EventNames.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMasker.h"
#include "SVGCursorElement.h"
#include "SVGDocumentExtensions.h"
#include "SVGElementRareData.h"
#include "SVGGraphicsElement.h"
#include "SVGImageElement.h"
#include "SVGNames.h"
#include "SVGRenderStyle.h"
#include "SVGRenderSupport.h"
#include "SVGSVGElement.h"
#include "SVGTitleElement.h"
#include "SVGUseElement.h"
#include "ShadowRoot.h"
#include "XLinkNames.h"
#include "XMLNames.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>


namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGElement, HTMLNames::classAttr, ClassName, className)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(className)
END_REGISTER_ANIMATED_PROPERTIES

static NEVER_INLINE void populateAttributeNameToCSSPropertyIDMap(HashMap<AtomicStringImpl*, CSSPropertyID>& map)
{
    using namespace HTMLNames;
    using namespace SVGNames;

    // This list should include all base CSS and SVG CSS properties which are exposed as SVG XML attributes.
    static const QualifiedName* const attributeNames[] = {
        &alignment_baselineAttr,
        &baseline_shiftAttr,
        &buffered_renderingAttr,
        &clipAttr,
        &clip_pathAttr,
        &clip_ruleAttr,
        &SVGNames::colorAttr,
        &color_interpolationAttr,
        &color_interpolation_filtersAttr,
        &color_profileAttr,
        &color_renderingAttr,
        &cursorAttr,
        &cxAttr,
        &cyAttr,
        &SVGNames::directionAttr,
        &displayAttr,
        &dominant_baselineAttr,
        &enable_backgroundAttr,
        &fillAttr,
        &fill_opacityAttr,
        &fill_ruleAttr,
        &filterAttr,
        &flood_colorAttr,
        &flood_opacityAttr,
        &font_familyAttr,
        &font_sizeAttr,
        &font_stretchAttr,
        &font_styleAttr,
        &font_variantAttr,
        &font_weightAttr,
        &glyph_orientation_horizontalAttr,
        &glyph_orientation_verticalAttr,
        &image_renderingAttr,
        &SVGNames::heightAttr,
        &kerningAttr,
        &letter_spacingAttr,
        &lighting_colorAttr,
        &marker_endAttr,
        &marker_midAttr,
        &marker_startAttr,
        &maskAttr,
        &mask_typeAttr,
        &opacityAttr,
        &overflowAttr,
        &paint_orderAttr,
        &pointer_eventsAttr,
        &rAttr,
        &rxAttr,
        &ryAttr,
        &shape_renderingAttr,
        &stop_colorAttr,
        &stop_opacityAttr,
        &strokeAttr,
        &stroke_dasharrayAttr,
        &stroke_dashoffsetAttr,
        &stroke_linecapAttr,
        &stroke_linejoinAttr,
        &stroke_miterlimitAttr,
        &stroke_opacityAttr,
        &stroke_widthAttr,
        &text_anchorAttr,
        &text_decorationAttr,
        &text_renderingAttr,
        &unicode_bidiAttr,
        &vector_effectAttr,
        &visibilityAttr,
        &SVGNames::widthAttr,
        &word_spacingAttr,
        &writing_modeAttr,
        &xAttr,
        &yAttr,
    };

    for (auto& name : attributeNames) {
        const AtomicString& localName = name->localName();
        map.add(localName.impl(), cssPropertyID(localName));
    }

    // FIXME: When CSS supports "transform-origin" this special case can be removed,
    // and we can add transform_originAttr to the table above instead.
    map.add(transform_originAttr.localName().impl(), CSSPropertyTransformOrigin);
}

static NEVER_INLINE void populateAttributeNameToAnimatedPropertyTypeMap(HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>& map)
{
    using namespace HTMLNames;
    using namespace SVGNames;

    struct TableEntry {
        const QualifiedName& attributeName;
        AnimatedPropertyType type;
    };

    static const TableEntry table[] = {
        { alignment_baselineAttr, AnimatedString },
        { baseline_shiftAttr, AnimatedString },
        { buffered_renderingAttr, AnimatedString },
        { clipAttr, AnimatedRect },
        { clip_pathAttr, AnimatedString },
        { clip_ruleAttr, AnimatedString },
        { SVGNames::colorAttr, AnimatedColor },
        { color_interpolationAttr, AnimatedString },
        { color_interpolation_filtersAttr, AnimatedString },
        { color_profileAttr, AnimatedString },
        { color_renderingAttr, AnimatedString },
        { cursorAttr, AnimatedString },
        { displayAttr, AnimatedString },
        { dominant_baselineAttr, AnimatedString },
        { fillAttr, AnimatedColor },
        { fill_opacityAttr, AnimatedNumber },
        { fill_ruleAttr, AnimatedString },
        { filterAttr, AnimatedString },
        { flood_colorAttr, AnimatedColor },
        { flood_opacityAttr, AnimatedNumber },
        { font_familyAttr, AnimatedString },
        { font_sizeAttr, AnimatedLength },
        { font_stretchAttr, AnimatedString },
        { font_styleAttr, AnimatedString },
        { font_variantAttr, AnimatedString },
        { font_weightAttr, AnimatedString },
        { image_renderingAttr, AnimatedString },
        { kerningAttr, AnimatedLength },
        { letter_spacingAttr, AnimatedLength },
        { lighting_colorAttr, AnimatedColor },
        { marker_endAttr, AnimatedString },
        { marker_midAttr, AnimatedString },
        { marker_startAttr, AnimatedString },
        { maskAttr, AnimatedString },
        { mask_typeAttr, AnimatedString },
        { opacityAttr, AnimatedNumber },
        { overflowAttr, AnimatedString },
        { paint_orderAttr, AnimatedString },
        { pointer_eventsAttr, AnimatedString },
        { shape_renderingAttr, AnimatedString },
        { stop_colorAttr, AnimatedColor },
        { stop_opacityAttr, AnimatedNumber },
        { strokeAttr, AnimatedColor },
        { stroke_dasharrayAttr, AnimatedLengthList },
        { stroke_dashoffsetAttr, AnimatedLength },
        { stroke_linecapAttr, AnimatedString },
        { stroke_linejoinAttr, AnimatedString },
        { stroke_miterlimitAttr, AnimatedNumber },
        { stroke_opacityAttr, AnimatedNumber },
        { stroke_widthAttr, AnimatedLength },
        { text_anchorAttr, AnimatedString },
        { text_decorationAttr, AnimatedString },
        { text_renderingAttr, AnimatedString },
        { vector_effectAttr, AnimatedString },
        { visibilityAttr, AnimatedString },
        { word_spacingAttr, AnimatedLength },
    };

    for (auto& entry : table)
        map.add(entry.attributeName.impl(), entry.type);
}

static inline HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>& attributeNameToAnimatedPropertyTypeMap()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>> map;
    if (map.get().isEmpty())
        populateAttributeNameToAnimatedPropertyTypeMap(map);
    return map;
#else
    WKC_DEFINE_STATIC_HASHMAPPTR(QualifiedName::QualifiedNameImpl*, AnimatedPropertyType, map, 0);
    if (!map)
        map = new HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>();
    if (map->isEmpty())
        populateAttributeNameToAnimatedPropertyTypeMap(*map);
    return *map;
#endif
}

static NEVER_INLINE void populateCSSPropertyWithSVGDOMNameToAnimatedPropertyTypeMap(HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>& map)
{
    using namespace HTMLNames;
    using namespace SVGNames;

    struct TableEntry {
        const QualifiedName& attributeName;
        AnimatedPropertyType type;
    };

    static const TableEntry table[] = {
        { cxAttr, AnimatedLength },
        { cyAttr, AnimatedLength },
        { rAttr, AnimatedLength },
        { rxAttr, AnimatedLength },
        { ryAttr, AnimatedLength },
        { SVGNames::heightAttr, AnimatedLength },
        { SVGNames::widthAttr, AnimatedLength },
        { xAttr, AnimatedLength },
        { yAttr, AnimatedLength },
    };

    for (auto& entry : table)
        map.add(entry.attributeName.impl(), entry.type);
}

static inline HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>& cssPropertyWithSVGDOMNameToAnimatedPropertyTypeMap()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>> map;
    if (map.get().isEmpty())
        populateCSSPropertyWithSVGDOMNameToAnimatedPropertyTypeMap(map);
    return map;
#else
    WKC_DEFINE_STATIC_HASHMAPPTR(QualifiedName::QualifiedNameImpl*, AnimatedPropertyType, map, 0);
    if (!map)
        map = new HashMap<QualifiedName::QualifiedNameImpl*, AnimatedPropertyType>();
    if (map->isEmpty())
        populateCSSPropertyWithSVGDOMNameToAnimatedPropertyTypeMap(*map);
    return *map;
#endif
}

SVGElement::SVGElement(const QualifiedName& tagName, Document& document)
    : StyledElement(tagName, document, CreateSVGElement)
{
    registerAnimatedPropertiesForSVGElement();
}

SVGElement::~SVGElement()
{
    if (m_svgRareData) {
        for (SVGElement* instance : m_svgRareData->instances())
            instance->m_svgRareData->setCorrespondingElement(nullptr);
        if (SVGCursorElement* cursorElement = m_svgRareData->cursorElement())
            cursorElement->removeClient(this);
        if (CSSCursorImageValue* cursorImageValue = m_svgRareData->cursorImageValue())
            cursorImageValue->removeReferencedElement(this);
        if (SVGElement* correspondingElement = m_svgRareData->correspondingElement())
            correspondingElement->m_svgRareData->instances().remove(this);

        m_svgRareData = nullptr;
    }
    document().accessSVGExtensions().rebuildAllElementReferencesForTarget(*this);
    document().accessSVGExtensions().removeAllElementReferencesForTarget(this);
}

short SVGElement::tabIndex() const
{
    if (supportsFocus())
        return Element::tabIndex();
    return -1;
}

bool SVGElement::willRecalcStyle(Style::Change change)
{
    if (!m_svgRareData || styleChangeType() == SyntheticStyleChange)
        return true;
    // If the style changes because of a regular property change (not induced by SMIL animations themselves)
    // reset the "computed style without SMIL style properties", so the base value change gets reflected.
    if (change > Style::NoChange || needsStyleRecalc())
        m_svgRareData->setNeedsOverrideComputedStyleUpdate();
    return true;
}

SVGElementRareData& SVGElement::ensureSVGRareData()
{
    if (!m_svgRareData)
        m_svgRareData = std::make_unique<SVGElementRareData>();
    return *m_svgRareData;
}

bool SVGElement::isOutermostSVGSVGElement() const
{
    if (!is<SVGSVGElement>(*this))
        return false;

    // If we're living in a shadow tree, we're a <svg> element that got created as replacement
    // for a <symbol> element or a cloned <svg> element in the referenced tree. In that case
    // we're always an inner <svg> element.
    if (isInShadowTree() && parentOrShadowHostElement() && parentOrShadowHostElement()->isSVGElement())
        return false;

    // Element may not be in the document, pretend we're outermost for viewport(), getCTM(), etc.
    if (!parentNode())
        return true;

    // We act like an outermost SVG element, if we're a direct child of a <foreignObject> element.
    if (parentNode()->hasTagName(SVGNames::foreignObjectTag))
        return true;

    // This is true whenever this is the outermost SVG, even if there are HTML elements outside it
    return !parentNode()->isSVGElement();
}

void SVGElement::reportAttributeParsingError(SVGParsingError error, const QualifiedName& name, const AtomicString& value)
{
    if (error == NoError)
        return;

    String errorString = "<" + tagName() + "> attribute " + name.toString() + "=\"" + value + "\"";
    SVGDocumentExtensions& extensions = document().accessSVGExtensions();

    if (error == NegativeValueForbiddenError) {
        extensions.reportError("Invalid negative value for " + errorString);
        return;
    }

    if (error == ParsingAttributeFailedError) {
        extensions.reportError("Invalid value for " + errorString);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGElement::isSupported(StringImpl* feature, StringImpl* version) const
{
    return DOMImplementation::hasFeature(feature, version);
}

String SVGElement::xmlbase() const
{
    return fastGetAttribute(XMLNames::baseAttr);
}

void SVGElement::setXmlbase(const String& value, ExceptionCode&)
{
    setAttribute(XMLNames::baseAttr, value);
}

void SVGElement::removedFrom(ContainerNode& rootParent)
{
    bool wasInDocument = rootParent.inDocument();
    if (wasInDocument)
        updateRelativeLengthsInformation(false, this);

    StyledElement::removedFrom(rootParent);

    if (wasInDocument) {
        document().accessSVGExtensions().clearTargetDependencies(*this);
        document().accessSVGExtensions().removeAllElementReferencesForTarget(this);
    }
    invalidateInstances();
}

SVGSVGElement* SVGElement::ownerSVGElement() const
{
    ContainerNode* node = parentOrShadowHostNode();
    while (node) {
        if (is<SVGSVGElement>(*node))
            return downcast<SVGSVGElement>(node);

        node = node->parentOrShadowHostNode();
    }

    return nullptr;
}

SVGElement* SVGElement::viewportElement() const
{
    // This function needs shadow tree support - as RenderSVGContainer uses this function
    // to determine the "overflow" property. <use> on <symbol> wouldn't work otherwhise.
    ContainerNode* node = parentOrShadowHostNode();
    while (node) {
        if (is<SVGSVGElement>(*node) || is<SVGImageElement>(*node) || node->hasTagName(SVGNames::symbolTag))
            return downcast<SVGElement>(node);

        node = node->parentOrShadowHostNode();
    }

    return nullptr;
}
 
const HashSet<SVGElement*>& SVGElement::instances() const
{
    if (!m_svgRareData) {
#if !PLATFORM(WKC)
        static NeverDestroyed<HashSet<SVGElement*>> emptyInstances;
        return emptyInstances;
#else
        WKC_DEFINE_STATIC_PTR(HashSet<SVGElement*>*, emptyInstances, 0);
        if (!emptyInstances)
            emptyInstances = new HashSet<SVGElement*>();
        return *emptyInstances;
#endif
    }
    return m_svgRareData->instances();
}

bool SVGElement::getBoundingBox(FloatRect& rect, SVGLocatable::StyleUpdateStrategy styleUpdateStrategy)
{
    if (is<SVGGraphicsElement>(*this)) {
        rect = downcast<SVGGraphicsElement>(*this).getBBox(styleUpdateStrategy);
        return true;
    }
    return false;
}

void SVGElement::setCursorElement(SVGCursorElement* cursorElement)
{
    SVGElementRareData& rareData = ensureSVGRareData();
    if (SVGCursorElement* oldCursorElement = rareData.cursorElement()) {
        if (cursorElement == oldCursorElement)
            return;
        oldCursorElement->removeReferencedElement(this);
    }
    rareData.setCursorElement(cursorElement);
}

void SVGElement::cursorElementRemoved() 
{
    ASSERT(m_svgRareData);
    m_svgRareData->setCursorElement(nullptr);
}

void SVGElement::setCursorImageValue(CSSCursorImageValue* cursorImageValue)
{
    SVGElementRareData& rareData = ensureSVGRareData();
    if (CSSCursorImageValue* oldCursorImageValue = rareData.cursorImageValue()) {
        if (cursorImageValue == oldCursorImageValue)
            return;
        oldCursorImageValue->removeReferencedElement(this);
    }
    rareData.setCursorImageValue(cursorImageValue);
}

void SVGElement::cursorImageValueRemoved()
{
    ASSERT(m_svgRareData);
    m_svgRareData->setCursorImageValue(nullptr);
}

SVGElement* SVGElement::correspondingElement() const
{
    return m_svgRareData ? m_svgRareData->correspondingElement() : nullptr;
}

SVGUseElement* SVGElement::correspondingUseElement() const
{
    auto* root = containingShadowRoot();
    if (!root)
        return nullptr;
    if (root->type() != ShadowRoot::UserAgentShadowRoot)
        return nullptr;
    auto* host = root->hostElement();
    if (!is<SVGUseElement>(host))
        return nullptr;
    return &downcast<SVGUseElement>(*host);
}

void SVGElement::setCorrespondingElement(SVGElement* correspondingElement)
{
    if (m_svgRareData) {
        if (SVGElement* oldCorrespondingElement = m_svgRareData->correspondingElement())
            oldCorrespondingElement->m_svgRareData->instances().remove(this);
    }
    if (m_svgRareData || correspondingElement)
        ensureSVGRareData().setCorrespondingElement(correspondingElement);
    if (correspondingElement)
        correspondingElement->ensureSVGRareData().instances().add(this);
}

void SVGElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == HTMLNames::classAttr) {
        setClassNameBaseValue(value);
        return;
    }

    if (name == HTMLNames::tabindexAttr) {
        int tabindex = 0;
        if (value.isEmpty())
            clearTabIndexExplicitlyIfNeeded();
        else if (parseHTMLInteger(value, tabindex)) {
            // Clamp tabindex to the range of 'short' to match Firefox's behavior.
            setTabIndexExplicitly(std::max(static_cast<int>(std::numeric_limits<short>::min()), std::min(tabindex, static_cast<int>(std::numeric_limits<short>::max()))));
        }
        return;
    }

    auto& eventName = HTMLElement::eventNameForEventHandlerAttribute(name);
    if (!eventName.isNull()) {
        setAttributeEventListener(eventName, name, value);
        return;
    }

    SVGLangSpace::parseAttribute(name, value);
}

Vector<AnimatedPropertyType> SVGElement::animatedPropertyTypesForAttribute(const QualifiedName& attributeName)
{
    auto types = localAttributeToPropertyMap().types(attributeName);
    if (!types.isEmpty())
        return types;

    {
        auto& map = attributeNameToAnimatedPropertyTypeMap();
        auto it = map.find(attributeName.impl());
        if (it != map.end()) {
            types.append(it->value);
            return types;
        }
    }

    {
        auto& map = cssPropertyWithSVGDOMNameToAnimatedPropertyTypeMap();
        auto it = map.find(attributeName.impl());
        if (it != map.end()) {
            types.append(it->value);
            return types;
        }
    }

    return types;
}

bool SVGElement::haveLoadedRequiredResources()
{
    for (auto& child : childrenOfType<SVGElement>(*this)) {
        if (!child.haveLoadedRequiredResources())
            return false;
    }
    return true;
}

bool SVGElement::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> prpListener, bool useCapture)
{
    RefPtr<EventListener> listener = prpListener;
    
    // Add event listener to regular DOM element
    if (!Node::addEventListener(eventType, listener, useCapture))
        return false;

    if (containingShadowRoot())
        return true;

    // Add event listener to all shadow tree DOM element instances
    ASSERT(!instanceUpdatesBlocked());
    for (auto* instance : instances()) {
        ASSERT(instance->correspondingElement() == this);
        bool result = instance->Node::addEventListener(eventType, listener, useCapture);
        ASSERT_UNUSED(result, result);
    }

    return true;
}

bool SVGElement::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    if (containingShadowRoot())
        return Node::removeEventListener(eventType, listener, useCapture);

    // EventTarget::removeEventListener creates a PassRefPtr around the given EventListener
    // object when creating a temporary RegisteredEventListener object used to look up the
    // event listener in a cache. If we want to be able to call removeEventListener() multiple
    // times on different nodes, we have to delay its immediate destruction, which would happen
    // after the first call below.
    RefPtr<EventListener> protector(listener);

    // Remove event listener from regular DOM element
    if (!Node::removeEventListener(eventType, listener, useCapture))
        return false;

    // Remove event listener from all shadow tree DOM element instances
    ASSERT(!instanceUpdatesBlocked());
    for (auto& instance : instances()) {
        ASSERT(instance->correspondingElement() == this);

        if (instance->Node::removeEventListener(eventType, listener, useCapture))
            continue;

        // This case can only be hit for event listeners created from markup
        ASSERT(listener->wasCreatedFromMarkup());

        // If the event listener 'listener' has been created from markup and has been fired before
        // then JSLazyEventListener::parseCode() has been called and m_jsFunction of that listener
        // has been created (read: it's not 0 anymore). During shadow tree creation, the event
        // listener DOM attribute has been cloned, and another event listener has been setup in
        // the shadow tree. If that event listener has not been used yet, m_jsFunction is still 0,
        // and tryRemoveEventListener() above will fail. Work around that very rare problem.
        ASSERT(instance->eventTargetData());
        instance->eventTargetData()->eventListenerMap.removeFirstEventListenerCreatedFromMarkup(eventType);
    }

    return true;
}

static bool hasLoadListener(Element* element)
{
    if (element->hasEventListeners(eventNames().loadEvent))
        return true;

    for (element = element->parentOrShadowHostElement(); element; element = element->parentOrShadowHostElement()) {
        const EventListenerVector& entry = element->getEventListeners(eventNames().loadEvent);
        for (auto& listener : entry) {
            if (listener.useCapture)
                return true;
        }
    }

    return false;
}

#if ENABLE(CSS_REGIONS)
bool SVGElement::shouldMoveToFlowThread(const RenderStyle& styleToUse) const
{
    // Allow only svg root elements to be directly collected by a render flow thread.
    return parentNode() && !parentNode()->isSVGElement() && hasTagName(SVGNames::svgTag) && Element::shouldMoveToFlowThread(styleToUse);
}
#endif

void SVGElement::sendSVGLoadEventIfPossible(bool sendParentLoadEvents)
{
    if (!inDocument() || !document().frame())
        return;

    RefPtr<SVGElement> currentTarget = this;
    while (currentTarget && currentTarget->haveLoadedRequiredResources()) {
        RefPtr<Element> parent;
        if (sendParentLoadEvents)
            parent = currentTarget->parentOrShadowHostElement(); // save the next parent to dispatch too incase dispatching the event changes the tree
        if (hasLoadListener(currentTarget.get()))
            currentTarget->dispatchEvent(Event::create(eventNames().loadEvent, false, false));
        currentTarget = (parent && parent->isSVGElement()) ? static_pointer_cast<SVGElement>(parent) : RefPtr<SVGElement>();
        SVGElement* element = currentTarget.get();
        if (!element || !element->isOutermostSVGSVGElement())
            continue;

        // Consider <svg onload="foo()"><image xlink:href="foo.png" externalResourcesRequired="true"/></svg>.
        // If foo.png is not yet loaded, the first SVGLoad event will go to the <svg> element, sent through
        // Document::implicitClose(). Then the SVGLoad event will fire for <image>, once its loaded.
        ASSERT(sendParentLoadEvents);

        // If the load event was not sent yet by Document::implicitClose(), but the <image> from the example
        // above, just appeared, don't send the SVGLoad event to the outermost <svg>, but wait for the document
        // to be "ready to render", first.
        if (!document().loadEventFinished())
            break;
    }
}

void SVGElement::sendSVGLoadEventIfPossibleAsynchronously()
{
    svgLoadEventTimer()->startOneShot(0);
}

void SVGElement::svgLoadEventTimerFired()
{
    sendSVGLoadEventIfPossible();
}

Timer* SVGElement::svgLoadEventTimer()
{
    ASSERT_NOT_REACHED();
    return 0;
}

void SVGElement::finishParsingChildren()
{
    StyledElement::finishParsingChildren();

    // The outermost SVGSVGElement SVGLoad event is fired through Document::dispatchWindowLoadEvent.
    if (isOutermostSVGSVGElement())
        return;

    // finishParsingChildren() is called when the close tag is reached for an element (e.g. </svg>)
    // we send SVGLoad events here if we can, otherwise they'll be sent when any required loads finish
    sendSVGLoadEventIfPossible();

    // Notify all the elements which have references to this element to rebuild their shadow and render
    // trees, e.g. a <use> element references a target element before this target element is defined.
    invalidateInstances();
}

bool SVGElement::childShouldCreateRenderer(const Node& child) const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<QualifiedName>> invalidTextContent;

    if (invalidTextContent.get().isEmpty()) {
        invalidTextContent.get().add(SVGNames::textPathTag);
#if ENABLE(SVG_FONTS)
        invalidTextContent.get().add(SVGNames::altGlyphTag);
#endif
        invalidTextContent.get().add(SVGNames::trefTag);
        invalidTextContent.get().add(SVGNames::tspanTag);
    }
    if (child.isSVGElement()) {
        const SVGElement& svgChild = downcast<SVGElement>(child);
        if (invalidTextContent.get().contains(svgChild.tagQName()))
            return false;

        return svgChild.isValid();
    }
    return false;
#else
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, invalidTextContent, 0);
    if (!invalidTextContent)
        invalidTextContent = new HashSet<QualifiedName>();

    if (invalidTextContent->isEmpty()) {
        invalidTextContent->add(SVGNames::textPathTag);
#if ENABLE(SVG_FONTS)
        invalidTextContent->add(SVGNames::altGlyphTag);
#endif
        invalidTextContent->add(SVGNames::trefTag);
        invalidTextContent->add(SVGNames::tspanTag);
    }
    if (child.isSVGElement()) {
        const SVGElement& svgChild = downcast<SVGElement>(child);
        if (invalidTextContent->contains(svgChild.tagQName()))
            return false;

        return svgChild.isValid();
    }
    return false;
#endif
}

void SVGElement::attributeChanged(const QualifiedName& name, const AtomicString& oldValue, const AtomicString& newValue, AttributeModificationReason)
{
    StyledElement::attributeChanged(name, oldValue, newValue);

    if (name == HTMLNames::idAttr)
        document().accessSVGExtensions().rebuildAllElementReferencesForTarget(*this);

    // Changes to the style attribute are processed lazily (see Element::getAttribute() and related methods),
    // so we don't want changes to the style attribute to result in extra work here.
    if (name != HTMLNames::styleAttr)
        svgAttributeChanged(name);
}

void SVGElement::synchronizeAllAnimatedSVGAttribute(SVGElement* svgElement)
{
    ASSERT(svgElement->elementData());
    ASSERT(svgElement->elementData()->animatedSVGAttributesAreDirty());

    svgElement->localAttributeToPropertyMap().synchronizeProperties(*svgElement);
    svgElement->elementData()->setAnimatedSVGAttributesAreDirty(false);
}

void SVGElement::synchronizeAnimatedSVGAttribute(const QualifiedName& name) const
{
    if (!elementData() || !elementData()->animatedSVGAttributesAreDirty())
        return;

    SVGElement* nonConstThis = const_cast<SVGElement*>(this);
    if (name == anyQName())
        synchronizeAllAnimatedSVGAttribute(nonConstThis);
    else
        nonConstThis->localAttributeToPropertyMap().synchronizeProperty(*nonConstThis, name);
}

void SVGElement::synchronizeRequiredFeatures(SVGElement* contextElement)
{
    ASSERT(contextElement);
    contextElement->synchronizeRequiredFeatures();
}

void SVGElement::synchronizeRequiredExtensions(SVGElement* contextElement)
{
    ASSERT(contextElement);
    contextElement->synchronizeRequiredExtensions();
}

void SVGElement::synchronizeSystemLanguage(SVGElement* contextElement)
{
    ASSERT(contextElement);
    contextElement->synchronizeSystemLanguage();
}

RefPtr<RenderStyle> SVGElement::customStyleForRenderer(RenderStyle& parentStyle)
{
    if (!correspondingElement())
        return document().ensureStyleResolver().styleForElement(this, &parentStyle);

    return document().ensureStyleResolver().styleForElement(correspondingElement(), &parentStyle, DisallowStyleSharing);
}

MutableStyleProperties* SVGElement::animatedSMILStyleProperties() const
{
    if (m_svgRareData)
        return m_svgRareData->animatedSMILStyleProperties();
    return 0;
}

MutableStyleProperties& SVGElement::ensureAnimatedSMILStyleProperties()
{
    return ensureSVGRareData().ensureAnimatedSMILStyleProperties();
}

void SVGElement::setUseOverrideComputedStyle(bool value)
{
    if (m_svgRareData)
        m_svgRareData->setUseOverrideComputedStyle(value);
}

RenderStyle* SVGElement::computedStyle(PseudoId pseudoElementSpecifier)
{
    if (!m_svgRareData || !m_svgRareData->useOverrideComputedStyle())
        return Element::computedStyle(pseudoElementSpecifier);

    RenderStyle* parentStyle = nullptr;
    if (Element* parent = parentOrShadowHostElement()) {
        if (auto renderer = parent->renderer())
            parentStyle = &renderer->style();
    }

    return m_svgRareData->overrideComputedStyle(this, parentStyle);
}

static void addQualifiedName(HashMap<AtomicString, QualifiedName>& map, const QualifiedName& name)
{
    HashMap<AtomicString, QualifiedName>::AddResult addResult = map.add(name.localName(), name);
    ASSERT_UNUSED(addResult, addResult.isNewEntry);
}

QualifiedName SVGElement::animatableAttributeForName(const AtomicString& localName)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashMap<AtomicString, QualifiedName>> neverDestroyedAnimatableAttributes;
    HashMap<AtomicString, QualifiedName>& animatableAttributes = neverDestroyedAnimatableAttributes;
#else
    WKC_DEFINE_STATIC_HASHMAPPTR(AtomicString, QualifiedName, neverDestroyedAnimatableAttributes, 0);
    if (!neverDestroyedAnimatableAttributes)
        neverDestroyedAnimatableAttributes = new HashMap<AtomicString, QualifiedName>();
    HashMap<AtomicString, QualifiedName>& animatableAttributes = *neverDestroyedAnimatableAttributes;
#endif

    if (animatableAttributes.isEmpty()) {
        addQualifiedName(animatableAttributes, HTMLNames::classAttr);
        addQualifiedName(animatableAttributes, SVGNames::amplitudeAttr);
        addQualifiedName(animatableAttributes, SVGNames::azimuthAttr);
        addQualifiedName(animatableAttributes, SVGNames::baseFrequencyAttr);
        addQualifiedName(animatableAttributes, SVGNames::biasAttr);
        addQualifiedName(animatableAttributes, SVGNames::clipPathUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::cxAttr);
        addQualifiedName(animatableAttributes, SVGNames::cyAttr);
        addQualifiedName(animatableAttributes, SVGNames::diffuseConstantAttr);
        addQualifiedName(animatableAttributes, SVGNames::divisorAttr);
        addQualifiedName(animatableAttributes, SVGNames::dxAttr);
        addQualifiedName(animatableAttributes, SVGNames::dyAttr);
        addQualifiedName(animatableAttributes, SVGNames::edgeModeAttr);
        addQualifiedName(animatableAttributes, SVGNames::elevationAttr);
        addQualifiedName(animatableAttributes, SVGNames::exponentAttr);
        addQualifiedName(animatableAttributes, SVGNames::externalResourcesRequiredAttr);
        addQualifiedName(animatableAttributes, SVGNames::filterResAttr);
        addQualifiedName(animatableAttributes, SVGNames::filterUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::fxAttr);
        addQualifiedName(animatableAttributes, SVGNames::fyAttr);
        addQualifiedName(animatableAttributes, SVGNames::gradientTransformAttr);
        addQualifiedName(animatableAttributes, SVGNames::gradientUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::heightAttr);
        addQualifiedName(animatableAttributes, SVGNames::in2Attr);
        addQualifiedName(animatableAttributes, SVGNames::inAttr);
        addQualifiedName(animatableAttributes, SVGNames::interceptAttr);
        addQualifiedName(animatableAttributes, SVGNames::k1Attr);
        addQualifiedName(animatableAttributes, SVGNames::k2Attr);
        addQualifiedName(animatableAttributes, SVGNames::k3Attr);
        addQualifiedName(animatableAttributes, SVGNames::k4Attr);
        addQualifiedName(animatableAttributes, SVGNames::kernelMatrixAttr);
        addQualifiedName(animatableAttributes, SVGNames::kernelUnitLengthAttr);
        addQualifiedName(animatableAttributes, SVGNames::lengthAdjustAttr);
        addQualifiedName(animatableAttributes, SVGNames::limitingConeAngleAttr);
        addQualifiedName(animatableAttributes, SVGNames::markerHeightAttr);
        addQualifiedName(animatableAttributes, SVGNames::markerUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::markerWidthAttr);
        addQualifiedName(animatableAttributes, SVGNames::maskContentUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::maskUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::methodAttr);
        addQualifiedName(animatableAttributes, SVGNames::modeAttr);
        addQualifiedName(animatableAttributes, SVGNames::numOctavesAttr);
        addQualifiedName(animatableAttributes, SVGNames::offsetAttr);
        addQualifiedName(animatableAttributes, SVGNames::operatorAttr);
        addQualifiedName(animatableAttributes, SVGNames::orderAttr);
        addQualifiedName(animatableAttributes, SVGNames::orientAttr);
        addQualifiedName(animatableAttributes, SVGNames::pathLengthAttr);
        addQualifiedName(animatableAttributes, SVGNames::patternContentUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::patternTransformAttr);
        addQualifiedName(animatableAttributes, SVGNames::patternUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::pointsAtXAttr);
        addQualifiedName(animatableAttributes, SVGNames::pointsAtYAttr);
        addQualifiedName(animatableAttributes, SVGNames::pointsAtZAttr);
        addQualifiedName(animatableAttributes, SVGNames::preserveAlphaAttr);
        addQualifiedName(animatableAttributes, SVGNames::preserveAspectRatioAttr);
        addQualifiedName(animatableAttributes, SVGNames::primitiveUnitsAttr);
        addQualifiedName(animatableAttributes, SVGNames::radiusAttr);
        addQualifiedName(animatableAttributes, SVGNames::rAttr);
        addQualifiedName(animatableAttributes, SVGNames::refXAttr);
        addQualifiedName(animatableAttributes, SVGNames::refYAttr);
        addQualifiedName(animatableAttributes, SVGNames::resultAttr);
        addQualifiedName(animatableAttributes, SVGNames::rotateAttr);
        addQualifiedName(animatableAttributes, SVGNames::rxAttr);
        addQualifiedName(animatableAttributes, SVGNames::ryAttr);
        addQualifiedName(animatableAttributes, SVGNames::scaleAttr);
        addQualifiedName(animatableAttributes, SVGNames::seedAttr);
        addQualifiedName(animatableAttributes, SVGNames::slopeAttr);
        addQualifiedName(animatableAttributes, SVGNames::spacingAttr);
        addQualifiedName(animatableAttributes, SVGNames::specularConstantAttr);
        addQualifiedName(animatableAttributes, SVGNames::specularExponentAttr);
        addQualifiedName(animatableAttributes, SVGNames::spreadMethodAttr);
        addQualifiedName(animatableAttributes, SVGNames::startOffsetAttr);
        addQualifiedName(animatableAttributes, SVGNames::stdDeviationAttr);
        addQualifiedName(animatableAttributes, SVGNames::stitchTilesAttr);
        addQualifiedName(animatableAttributes, SVGNames::surfaceScaleAttr);
        addQualifiedName(animatableAttributes, SVGNames::tableValuesAttr);
        addQualifiedName(animatableAttributes, SVGNames::targetAttr);
        addQualifiedName(animatableAttributes, SVGNames::targetXAttr);
        addQualifiedName(animatableAttributes, SVGNames::targetYAttr);
        addQualifiedName(animatableAttributes, SVGNames::transformAttr);
        addQualifiedName(animatableAttributes, SVGNames::typeAttr);
        addQualifiedName(animatableAttributes, SVGNames::valuesAttr);
        addQualifiedName(animatableAttributes, SVGNames::viewBoxAttr);
        addQualifiedName(animatableAttributes, SVGNames::widthAttr);
        addQualifiedName(animatableAttributes, SVGNames::x1Attr);
        addQualifiedName(animatableAttributes, SVGNames::x2Attr);
        addQualifiedName(animatableAttributes, SVGNames::xAttr);
        addQualifiedName(animatableAttributes, SVGNames::xChannelSelectorAttr);
        addQualifiedName(animatableAttributes, SVGNames::y1Attr);
        addQualifiedName(animatableAttributes, SVGNames::y2Attr);
        addQualifiedName(animatableAttributes, SVGNames::yAttr);
        addQualifiedName(animatableAttributes, SVGNames::yChannelSelectorAttr);
        addQualifiedName(animatableAttributes, SVGNames::zAttr);
        addQualifiedName(animatableAttributes, XLinkNames::hrefAttr);
    }
    return animatableAttributes.get(localName);
}

#ifndef NDEBUG
bool SVGElement::isAnimatableAttribute(const QualifiedName& name) const
{
    if (SVGElement::animatableAttributeForName(name.localName()) == name)
        return !filterOutAnimatableAttribute(name);
    return false;
}

bool SVGElement::filterOutAnimatableAttribute(const QualifiedName&) const
{
    return false;
}
#endif

String SVGElement::title() const
{
    // According to spec, for stand-alone SVG documents we should not return a title when
    // hovering over the rootmost SVG element (the first <title> element is the title of
    // the document, not a tooltip) so we instantly return.
    if (isOutermostSVGSVGElement() && document().topDocument().isSVGDocument())
        return String();
    auto firstTitle = childrenOfType<SVGTitleElement>(*this).first();
    return firstTitle ? const_cast<SVGTitleElement*>(firstTitle)->innerText() : String();
}

bool SVGElement::rendererIsNeeded(const RenderStyle& style)
{
    // http://www.w3.org/TR/SVG/extend.html#PrivateData
    // Prevent anything other than SVG renderers from appearing in our render tree
    // Spec: SVG allows inclusion of elements from foreign namespaces anywhere
    // with the SVG content. In general, the SVG user agent will include the unknown
    // elements in the DOM but will otherwise ignore unknown elements.
    if (!parentOrShadowHostElement() || parentOrShadowHostElement()->isSVGElement())
        return StyledElement::rendererIsNeeded(style);

    return false;
}

CSSPropertyID SVGElement::cssPropertyIdForSVGAttributeName(const QualifiedName& attrName)
{
    if (!attrName.namespaceURI().isNull())
        return CSSPropertyInvalid;

#if !PLATFORM(WKC)
    static NeverDestroyed<HashMap<AtomicStringImpl*, CSSPropertyID>> properties;
    if (properties.get().isEmpty())
        populateAttributeNameToCSSPropertyIDMap(properties.get());

    return properties.get().get(attrName.localName().impl());
#else
    WKC_DEFINE_STATIC_HASHMAPPTR(AtomicStringImpl*, CSSPropertyID, properties, 0);
    if (!properties)
        properties = new HashMap<AtomicStringImpl*, CSSPropertyID>();
    if (properties->isEmpty())
        populateAttributeNameToCSSPropertyIDMap(*properties);

    return properties->get(attrName.localName().impl());
#endif
}

bool SVGElement::isAnimatableCSSProperty(const QualifiedName& attributeName)
{
    return attributeNameToAnimatedPropertyTypeMap().contains(attributeName.impl())
        || cssPropertyWithSVGDOMNameToAnimatedPropertyTypeMap().contains(attributeName.impl());
}

bool SVGElement::isPresentationAttributeWithSVGDOM(const QualifiedName& attributeName)
{
    return !localAttributeToPropertyMap().types(attributeName).isEmpty();
}

bool SVGElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (cssPropertyIdForSVGAttributeName(name) > 0)
        return true;
    return StyledElement::isPresentationAttribute(name);
}

void SVGElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStyleProperties& style)
{
    CSSPropertyID propertyID = cssPropertyIdForSVGAttributeName(name);
    if (propertyID > 0)
        addPropertyToPresentationAttributeStyle(style, propertyID, value);
}

bool SVGElement::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName == HTMLNames::idAttr;
}

void SVGElement::svgAttributeChanged(const QualifiedName& attrName)
{
    CSSPropertyID propId = cssPropertyIdForSVGAttributeName(attrName);
    if (propId > 0) {
        invalidateInstances();
        return;
    }

    if (attrName == HTMLNames::classAttr) {
        classAttributeChanged(className());
        invalidateInstances();
        return;
    }

    if (attrName == HTMLNames::idAttr) {
        auto renderer = this->renderer();
        // Notify resources about id changes, this is important as we cache resources by id in SVGDocumentExtensions
        if (is<RenderSVGResourceContainer>(renderer))
            downcast<RenderSVGResourceContainer>(*renderer).idChanged();
        if (inDocument())
            buildPendingResourcesIfNeeded();
        invalidateInstances();
        return;
    }
}

Node::InsertionNotificationRequest SVGElement::insertedInto(ContainerNode& rootParent)
{
    StyledElement::insertedInto(rootParent);
    updateRelativeLengthsInformation();
    buildPendingResourcesIfNeeded();
    return InsertionDone;
}

void SVGElement::buildPendingResourcesIfNeeded()
{
    if (!needsPendingResourceHandling() || !inDocument() || isInShadowTree())
        return;

    SVGDocumentExtensions& extensions = document().accessSVGExtensions();
    String resourceId = getIdAttribute();
    if (!extensions.isIdOfPendingResource(resourceId))
        return;

    // Mark pending resources as pending for removal.
    extensions.markPendingResourcesForRemoval(resourceId);

    // Rebuild pending resources for each client of a pending resource that is being removed.
    while (Element* clientElement = extensions.removeElementFromPendingResourcesForRemovalMap(resourceId)) {
        ASSERT(clientElement->hasPendingResources());
        if (clientElement->hasPendingResources()) {
            clientElement->buildPendingResource();
            extensions.clearHasPendingResourcesIfPossible(clientElement);
        }
    }
}

void SVGElement::childrenChanged(const ChildChange& change)
{
    StyledElement::childrenChanged(change);

    if (change.source == ChildChangeSourceParser)
        return;
    invalidateInstances();
}

RefPtr<CSSValue> SVGElement::getPresentationAttribute(const String& name)
{
    if (!hasAttributesWithoutUpdate())
        return 0;

    QualifiedName attributeName(nullAtom, name, nullAtom);
    const Attribute* attribute = findAttributeByName(attributeName);
    if (!attribute)
        return 0;

    RefPtr<MutableStyleProperties> style = MutableStyleProperties::create(SVGAttributeMode);
    CSSPropertyID propertyID = cssPropertyIdForSVGAttributeName(attribute->name());
    style->setProperty(propertyID, attribute->value());
    RefPtr<CSSValue> cssValue = style->getPropertyCSSValue(propertyID);
    return cssValue ? cssValue->cloneForCSSOM() : nullptr;
}

bool SVGElement::instanceUpdatesBlocked() const
{
    return m_svgRareData && m_svgRareData->instanceUpdatesBlocked();
}

void SVGElement::setInstanceUpdatesBlocked(bool value)
{
    // Catch any callers that calls setInstanceUpdatesBlocked(true) twice in a row.
    // That probably indicates nested use of InstanceUpdateBlocker and a bug.
    ASSERT(!value || !instanceUpdatesBlocked());

    if (m_svgRareData)
        m_svgRareData->setInstanceUpdatesBlocked(value);
}

AffineTransform SVGElement::localCoordinateSpaceTransform(SVGLocatable::CTMScope) const
{
    // To be overriden by SVGGraphicsElement (or as special case SVGTextElement and SVGPatternElement)
    return AffineTransform();
}

void SVGElement::updateRelativeLengthsInformation(bool hasRelativeLengths, SVGElement* element)
{
    // If we're not yet in a document, this function will be called again from insertedInto(). Do nothing now.
    if (!inDocument())
        return;

    // An element wants to notify us that its own relative lengths state changed.
    // Register it in the relative length map, and register us in the parent relative length map.
    // Register the parent in the grandparents map, etc. Repeat procedure until the root of the SVG tree.

    if (hasRelativeLengths)
        m_elementsWithRelativeLengths.add(element);
    else {
        if (!m_elementsWithRelativeLengths.contains(element)) {
            // We were never registered. Do nothing.
            return;
        }

        m_elementsWithRelativeLengths.remove(element);
    }

    if (!element->isSVGGraphicsElement())
        return;

    // Find first styled parent node, and notify it that we've changed our relative length state.
    ContainerNode* node = parentNode();
    while (node) {
        if (!node->isSVGElement())
            break;

        // Register us in the parent element map.
        downcast<SVGElement>(*node).updateRelativeLengthsInformation(hasRelativeLengths, this);
        break;
    }
}

bool SVGElement::hasFocusEventListeners() const
{
    Element* eventTarget = const_cast<SVGElement*>(this);
    return eventTarget->hasEventListeners(eventNames().focusinEvent)
        || eventTarget->hasEventListeners(eventNames().focusoutEvent)
        || eventTarget->hasEventListeners(eventNames().focusEvent)
        || eventTarget->hasEventListeners(eventNames().blurEvent);
}

bool SVGElement::isMouseFocusable() const
{
    if (!isFocusable())
        return false;
    Element* eventTarget = const_cast<SVGElement*>(this);
    return hasFocusEventListeners()
        || eventTarget->hasEventListeners(eventNames().keydownEvent)
        || eventTarget->hasEventListeners(eventNames().keyupEvent)
        || eventTarget->hasEventListeners(eventNames().keypressEvent);
}
    
void SVGElement::accessKeyAction(bool sendMouseEvents)
{
    dispatchSimulatedClick(0, sendMouseEvents ? SendMouseUpDownEvents : SendNoEvents);
}

void SVGElement::invalidateInstances()
{
    if (instanceUpdatesBlocked())
        return;

    auto& instances = this->instances();
    while (!instances.isEmpty()) {
        SVGElement* instance = *instances.begin();
        if (SVGUseElement* useElement = instance->correspondingUseElement())
            useElement->invalidateShadowTree();
        instance->setCorrespondingElement(nullptr);
    } while (!instances.isEmpty());
}

}
