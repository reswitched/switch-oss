/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGGElement.h"

#include "RenderSVGHiddenContainer.h"
#include "RenderSVGResource.h"
#include "RenderSVGTransformableContainer.h"
#include "SVGNames.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGGElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGGElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

SVGGElement::SVGGElement(const QualifiedName& tagName, Document& document)
    : SVGGraphicsElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::gTag));
    registerAnimatedPropertiesForSVGGElement();
}

Ref<SVGGElement> SVGGElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGGElement(tagName, document));
}

Ref<SVGGElement> SVGGElement::create(Document& document)
{
    return create(SVGNames::gTag, document);
}

bool SVGGElement::isSupportedAttribute(const QualifiedName& attrName)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<QualifiedName>> supportedAttributes;
    if (supportedAttributes.get().isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
    }
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
#else
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, supportedAttributes, 0);
    if (!supportedAttributes)
        supportedAttributes = new HashSet<QualifiedName>();
    if (supportedAttributes->isEmpty()) {
        SVGLangSpace::addSupportedAttributes(*supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(*supportedAttributes);
    }
    return supportedAttributes->contains<SVGAttributeHashTranslator>(attrName);
#endif
}

void SVGGElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGGraphicsElement::parseAttribute(name, value);
    SVGExternalResourcesRequired::parseAttribute(name, value);
}

void SVGGElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);

    if (auto renderer = this->renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(*renderer);
}

RenderPtr<RenderElement> SVGGElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    // SVG 1.1 testsuite explicitely uses constructs like <g display="none"><linearGradient>
    // We still have to create renderers for the <g> & <linearGradient> element, though the
    // subtree may be hidden - we only want the resource renderers to exist so they can be
    // referenced from somewhere else.
    if (style.get().display() == NONE)
        return createRenderer<RenderSVGHiddenContainer>(*this, WTF::move(style));

    return createRenderer<RenderSVGTransformableContainer>(*this, WTF::move(style));
}

bool SVGGElement::rendererIsNeeded(const RenderStyle&)
{
    // Unlike SVGElement::rendererIsNeeded(), we still create renderers, even if
    // display is set to 'none' - which is special to SVG <g> container elements.
    return parentOrShadowHostElement() && parentOrShadowHostElement()->isSVGElement();
}

}
