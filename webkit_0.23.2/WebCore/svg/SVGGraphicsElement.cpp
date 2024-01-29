/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "SVGGraphicsElement.h"

#include "AffineTransform.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGNames.h"
#include "SVGPathData.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_TRANSFORM_LIST(SVGGraphicsElement, SVGNames::transformAttr, Transform, transform)
    
BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGGraphicsElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(transform)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGElement)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTests)
END_REGISTER_ANIMATED_PROPERTIES

SVGGraphicsElement::SVGGraphicsElement(const QualifiedName& tagName, Document& document)
    : SVGElement(tagName, document)
    , m_shouldIsolateBlending(false)
{
    registerAnimatedPropertiesForSVGGraphicsElement();
}

SVGGraphicsElement::~SVGGraphicsElement()
{
}

AffineTransform SVGGraphicsElement::getCTM(StyleUpdateStrategy styleUpdateStrategy)
{
    return SVGLocatable::computeCTM(this, SVGLocatable::NearestViewportScope, styleUpdateStrategy);
}

AffineTransform SVGGraphicsElement::getScreenCTM(StyleUpdateStrategy styleUpdateStrategy)
{
    return SVGLocatable::computeCTM(this, SVGLocatable::ScreenScope, styleUpdateStrategy);
}

AffineTransform SVGGraphicsElement::animatedLocalTransform() const
{
    AffineTransform matrix;
    RenderStyle* style = renderer() ? &renderer()->style() : nullptr;

    // If CSS property was set, use that, otherwise fallback to attribute (if set).
    if (style && style->hasTransform()) {
        // Note: objectBoundingBox is an emptyRect for elements like pattern or clipPath.
        // See the "Object bounding box units" section of http://dev.w3.org/csswg/css3-transforms/
        TransformationMatrix transform;
        style->applyTransform(transform, renderer()->objectBoundingBox());

        // Flatten any 3D transform.
        matrix = transform.toAffineTransform();
        // CSS bakes the zoom factor into lengths, including translation components.
        // In order to align CSS & SVG transforms, we need to invert this operation.
        float zoom = style->effectiveZoom();
        if (zoom != 1) {
            matrix.setE(matrix.e() / zoom);
            matrix.setF(matrix.f() / zoom);
        }

    } else
        transform().concatenate(matrix);

    if (m_supplementalTransform)
        return *m_supplementalTransform * matrix;
    return matrix;
}

AffineTransform* SVGGraphicsElement::supplementalTransform()
{
    if (!m_supplementalTransform)
        m_supplementalTransform = std::make_unique<AffineTransform>();
    return m_supplementalTransform.get();
}

bool SVGGraphicsElement::isSupportedAttribute(const QualifiedName& attrName)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<HashSet<QualifiedName>> supportedAttributes;
    if (supportedAttributes.get().isEmpty()) {
        SVGTests::addSupportedAttributes(supportedAttributes);
        supportedAttributes.get().add(SVGNames::transformAttr);
    }
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
#else
    WKC_DEFINE_STATIC_PTR(HashSet<QualifiedName>*, supportedAttributes, 0);
    if (!supportedAttributes)
        supportedAttributes = new HashSet<QualifiedName>();
    if (supportedAttributes->isEmpty()) {
        SVGTests::addSupportedAttributes(*supportedAttributes);
        supportedAttributes->add(SVGNames::transformAttr);
    }
    return supportedAttributes->contains<SVGAttributeHashTranslator>(attrName);
#endif
}

void SVGGraphicsElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == SVGNames::transformAttr) {
        SVGTransformList newList;
        newList.parse(value);
        detachAnimatedTransformListWrappers(newList.size());
        setTransformBaseValue(newList);
        return;
    }

    SVGElement::parseAttribute(name, value);
    SVGTests::parseAttribute(name, value);
}

void SVGGraphicsElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);

    if (SVGTests::handleAttributeChange(this, attrName))
        return;

    auto renderer = this->renderer();
    if (!renderer)
        return;

    if (attrName == SVGNames::transformAttr) {
        renderer->setNeedsTransformUpdate();
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(*renderer);
        return;
    }

    ASSERT_NOT_REACHED();
}

SVGElement* SVGGraphicsElement::nearestViewportElement() const
{
    return SVGTransformable::nearestViewportElement(this);
}

SVGElement* SVGGraphicsElement::farthestViewportElement() const
{
    return SVGTransformable::farthestViewportElement(this);
}

FloatRect SVGGraphicsElement::getBBox(StyleUpdateStrategy styleUpdateStrategy)
{
    return SVGTransformable::getBBox(this, styleUpdateStrategy);
}

RenderPtr<RenderElement> SVGGraphicsElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    // By default, any subclass is expected to do path-based drawing
    return createRenderer<RenderSVGPath>(*this, WTF::move(style));
}

void SVGGraphicsElement::toClipPath(Path& path)
{
    updatePathFromGraphicsElement(this, path);
    // FIXME: How do we know the element has done a layout?
    path.transform(animatedLocalTransform());
}

}
