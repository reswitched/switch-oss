/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "SVGForeignObjectElement.h"

#include "CSSPropertyNames.h"
#include "RenderSVGForeignObject.h"
#include "RenderSVGResource.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "XLinkNames.h"
#include <wtf/Assertions.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGForeignObjectElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGForeignObjectElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_LENGTH(SVGForeignObjectElement, SVGNames::widthAttr, Width, width)
DEFINE_ANIMATED_LENGTH(SVGForeignObjectElement, SVGNames::heightAttr, Height, height)
DEFINE_ANIMATED_STRING(SVGForeignObjectElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGForeignObjectElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGForeignObjectElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y)
    REGISTER_LOCAL_ANIMATED_PROPERTY(width)
    REGISTER_LOCAL_ANIMATED_PROPERTY(height)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGForeignObjectElement::SVGForeignObjectElement(const QualifiedName& tagName, Document& document)
    : SVGGraphicsElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth)
    , m_height(LengthModeHeight)
{
    ASSERT(hasTagName(SVGNames::foreignObjectTag));
    registerAnimatedPropertiesForSVGForeignObjectElement();
}

Ref<SVGForeignObjectElement> SVGForeignObjectElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGForeignObjectElement(tagName, document));
}

bool SVGForeignObjectElement::isSupportedAttribute(const QualifiedName& attrName)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<QualifiedName>> supportedAttributes;
    if (supportedAttributes.get().isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.get().add(SVGNames::xAttr);
        supportedAttributes.get().add(SVGNames::yAttr);
        supportedAttributes.get().add(SVGNames::widthAttr);
        supportedAttributes.get().add(SVGNames::heightAttr);
    }
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
#else
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, supportedAttributes, 0);
    if (!supportedAttributes)
        supportedAttributes = new HashSet<QualifiedName>();
    if (supportedAttributes->isEmpty()) {
        SVGLangSpace::addSupportedAttributes(*supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(*supportedAttributes);
        supportedAttributes->add(SVGNames::xAttr);
        supportedAttributes->add(SVGNames::yAttr);
        supportedAttributes->add(SVGNames::widthAttr);
        supportedAttributes->add(SVGNames::heightAttr);
    }
    return supportedAttributes->contains<SVGAttributeHashTranslator>(attrName);
#endif
}

void SVGForeignObjectElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (name == SVGNames::xAttr)
        setXBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::yAttr)
        setYBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));

    reportAttributeParsingError(parseError, name, value);

    SVGGraphicsElement::parseAttribute(name, value);
    SVGExternalResourcesRequired::parseAttribute(name, value);
}

void SVGForeignObjectElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);

    if (attrName == SVGNames::widthAttr
        || attrName == SVGNames::heightAttr) {
        invalidateSVGPresentationAttributeStyle();
        return;
    }

    if (attrName == SVGNames::xAttr || attrName == SVGNames::yAttr)
        updateRelativeLengthsInformation();

    if (auto renderer = this->renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(*renderer);
}

RenderPtr<RenderElement> SVGForeignObjectElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderSVGForeignObject>(*this, WTF::move(style));
}

bool SVGForeignObjectElement::childShouldCreateRenderer(const Node& child) const
{
    // Disallow arbitary SVG content. Only allow proper <svg xmlns="svgNS"> subdocuments.
    if (child.isSVGElement())
        return child.hasTagName(SVGNames::svgTag);

    // Skip over SVG rules which disallow non-SVG kids
    return StyledElement::childShouldCreateRenderer(child);
}

bool SVGForeignObjectElement::rendererIsNeeded(const RenderStyle& style)
{
    // Suppress foreignObject renderers in SVG hidden containers.
    // (https://bugs.webkit.org/show_bug.cgi?id=87297)
    // Note that we currently do not support foreignObject instantiation via <use>, hence it is safe
    // to use parentElement() here. If that changes, this method should be updated to use
    // parentOrShadowHostElement() instead.
    Element* ancestor = parentElement();
    while (ancestor && ancestor->isSVGElement()) {
        if (ancestor->renderer() && ancestor->renderer()->isSVGHiddenContainer())
            return false;

        ancestor = ancestor->parentElement();
    }

    return SVGGraphicsElement::rendererIsNeeded(style);
}

}
