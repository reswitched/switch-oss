/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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
#include "SVGCursorElement.h"

#include "Document.h"
#include "SVGNames.h"
#include "XLinkNames.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGCursorElement, SVGNames::xAttr, X, x)
DEFINE_ANIMATED_LENGTH(SVGCursorElement, SVGNames::yAttr, Y, y)
DEFINE_ANIMATED_STRING(SVGCursorElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGCursorElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGCursorElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTests)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGCursorElement::SVGCursorElement(const QualifiedName& tagName, Document& document)
    : SVGElement(tagName, document)
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
{
    ASSERT(hasTagName(SVGNames::cursorTag));
    registerAnimatedPropertiesForSVGCursorElement();
}

Ref<SVGCursorElement> SVGCursorElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGCursorElement(tagName, document));
}

SVGCursorElement::~SVGCursorElement()
{
    for (auto& client : m_clients)
        client->cursorElementRemoved();
}

bool SVGCursorElement::isSupportedAttribute(const QualifiedName& attrName)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<QualifiedName>> supportedAttributes;
    if (supportedAttributes.get().isEmpty()) {
        SVGTests::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        supportedAttributes.get().add(SVGNames::xAttr);
        supportedAttributes.get().add(SVGNames::yAttr);
    }
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
#else
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, supportedAttributes, 0);
    if (!supportedAttributes)
        supportedAttributes = new HashSet<QualifiedName>();
    if (supportedAttributes->isEmpty()) {
        SVGTests::addSupportedAttributes(*supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(*supportedAttributes);
        SVGURIReference::addSupportedAttributes(*supportedAttributes);
        supportedAttributes->add(SVGNames::xAttr);
        supportedAttributes->add(SVGNames::yAttr);
    }
    return supportedAttributes->contains<SVGAttributeHashTranslator>(attrName);
#endif
}

void SVGCursorElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (name == SVGNames::xAttr)
        setXBaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::yAttr)
        setYBaseValue(SVGLength::construct(LengthModeHeight, value, parseError));

    reportAttributeParsingError(parseError, name, value);

    SVGElement::parseAttribute(name, value);
    SVGTests::parseAttribute(name, value);
    SVGExternalResourcesRequired::parseAttribute(name, value);
    SVGURIReference::parseAttribute(name, value);
}

void SVGCursorElement::addClient(SVGElement* element)
{
    m_clients.add(element);
    element->setCursorElement(this);
}

void SVGCursorElement::removeClient(SVGElement* element)
{
    if (m_clients.remove(element))
        element->cursorElementRemoved();
}

void SVGCursorElement::removeReferencedElement(SVGElement* element)
{
    m_clients.remove(element);
}

void SVGCursorElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);
    for (auto& client : m_clients)
        client->setNeedsStyleRecalc();
}

void SVGCursorElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    SVGElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document().completeURL(href()));
}

}
