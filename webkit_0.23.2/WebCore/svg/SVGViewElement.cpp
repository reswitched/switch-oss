/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
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
#include "SVGViewElement.h"

#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGViewElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)
DEFINE_ANIMATED_RECT(SVGViewElement, SVGNames::viewBoxAttr, ViewBox, viewBox)
DEFINE_ANIMATED_PRESERVEASPECTRATIO(SVGViewElement, SVGNames::preserveAspectRatioAttr, PreserveAspectRatio, preserveAspectRatio)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGViewElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_LOCAL_ANIMATED_PROPERTY(viewBox)
    REGISTER_LOCAL_ANIMATED_PROPERTY(preserveAspectRatio)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGViewElement::SVGViewElement(const QualifiedName& tagName, Document& document)
    : SVGElement(tagName, document)
    , m_zoomAndPan(SVGZoomAndPanMagnify)
    , m_viewTarget(SVGNames::viewTargetAttr)
{
    ASSERT(hasTagName(SVGNames::viewTag));
    registerAnimatedPropertiesForSVGViewElement();
}

Ref<SVGViewElement> SVGViewElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGViewElement(tagName, document));
}

void SVGViewElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == SVGNames::viewTargetAttr)
        viewTarget().reset(value);

    SVGExternalResourcesRequired::parseAttribute(name, value);
    SVGFitToViewBox::parseAttribute(this, name, value);
    SVGZoomAndPan::parseAttribute(*this, name, value);
    SVGElement::parseAttribute(name, value);
}

}
