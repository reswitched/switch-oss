/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2010 Dirk Schulze <krit@webkit.org>
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
#include "SVGFEImageElement.h"

#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "ColorSpace.h"
#include "Document.h"
#include "Image.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include "XLinkNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGFEImageElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)
DEFINE_ANIMATED_STRING(SVGFEImageElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGFEImageElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGFEImageElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGFilterPrimitiveStandardAttributes)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGFEImageElement::SVGFEImageElement(const QualifiedName& tagName, Document& document)
    : SVGFilterPrimitiveStandardAttributes(tagName, document)
{
    ASSERT(hasTagName(SVGNames::feImageTag));
    registerAnimatedPropertiesForSVGFEImageElement();
}

Ref<SVGFEImageElement> SVGFEImageElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGFEImageElement(tagName, document));
}

SVGFEImageElement::~SVGFEImageElement()
{
    clearResourceReferences();
}

void SVGFEImageElement::clearResourceReferences()
{
    if (m_cachedImage) {
        m_cachedImage->removeClient(this);
        m_cachedImage = nullptr;
    }

    document().accessSVGExtensions().removeAllTargetReferencesForElement(this);
}

void SVGFEImageElement::requestImageResource()
{
    ResourceLoaderOptions options = CachedResourceLoader::defaultCachedResourceOptions();
    options.setContentSecurityPolicyImposition(isInUserAgentShadowTree() ? ContentSecurityPolicyImposition::SkipPolicyCheck : ContentSecurityPolicyImposition::DoPolicyCheck);

    CachedResourceRequest request(ResourceRequest(document().completeURL(href())), options);
    request.setInitiator(this);
    m_cachedImage = document().cachedResourceLoader().requestImage(request);

    if (m_cachedImage)
        m_cachedImage->addClient(this);
}

void SVGFEImageElement::buildPendingResource()
{
    clearResourceReferences();
    if (!inDocument())
        return;

    String id;
    Element* target = SVGURIReference::targetElementFromIRIString(href(), document(), &id);
    if (!target) {
        if (id.isEmpty())
            requestImageResource();
        else {
            document().accessSVGExtensions().addPendingResource(id, this);
            ASSERT(hasPendingResources());
        }
    } else if (target->isSVGElement()) {
        // Register us with the target in the dependencies map. Any change of hrefElement
        // that leads to relayout/repainting now informs us, so we can react to it.
        document().accessSVGExtensions().addElementReferencingTarget(this, downcast<SVGElement>(target));
    }

    invalidate();
}

void SVGFEImageElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == SVGNames::preserveAspectRatioAttr) {
        SVGPreserveAspectRatio preserveAspectRatio;
        preserveAspectRatio.parse(value);
        setPreserveAspectRatioBaseValue(preserveAspectRatio);
        return;
    }

    SVGFilterPrimitiveStandardAttributes::parseAttribute(name, value);
    SVGURIReference::parseAttribute(name, value);
    SVGExternalResourcesRequired::parseAttribute(name, value);
}

void SVGFEImageElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (attrName == SVGNames::preserveAspectRatioAttr) {
        InstanceInvalidationGuard guard(*this);
        invalidate();
        return;
    }

    if (SVGURIReference::isKnownAttribute(attrName)) {
        InstanceInvalidationGuard guard(*this);
        buildPendingResource();
        return;
    }

    SVGFilterPrimitiveStandardAttributes::svgAttributeChanged(attrName);
}

Node::InsertionNotificationRequest SVGFEImageElement::insertedInto(ContainerNode& rootParent)
{
    SVGFilterPrimitiveStandardAttributes::insertedInto(rootParent);
    return InsertionShouldCallFinishedInsertingSubtree;
}

void SVGFEImageElement::finishedInsertingSubtree()
{
    buildPendingResource();
}

void SVGFEImageElement::removedFrom(ContainerNode& rootParent)
{
    SVGFilterPrimitiveStandardAttributes::removedFrom(rootParent);
    if (rootParent.inDocument())
        clearResourceReferences();
}

void SVGFEImageElement::notifyFinished(CachedResource*)
{
    if (!inDocument())
        return;

    Element* parent = parentElement();

    if (!parent || !parent->hasTagName(SVGNames::filterTag))
        return;

    RenderElement* parentRenderer = parent->renderer();
    if (!parentRenderer)
        return;

    RenderSVGResource::markForLayoutAndParentResourceInvalidation(*parentRenderer);
}

RefPtr<FilterEffect> SVGFEImageElement::build(SVGFilterBuilder*, Filter& filter)
{
    if (m_cachedImage)
        return FEImage::createWithImage(filter, m_cachedImage->imageForRenderer(renderer()), preserveAspectRatio());
    return FEImage::createWithIRIReference(filter, document(), href(), preserveAspectRatio());
}

void SVGFEImageElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    SVGFilterPrimitiveStandardAttributes::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document().completeURL(href()));
}

}
