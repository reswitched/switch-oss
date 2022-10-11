/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SVGAnimatedBoolean.h"

#include "SVGAnimateElementBase.h"

namespace WebCore {

SVGAnimatedBooleanAnimator::SVGAnimatedBooleanAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedBoolean, animationElement, contextElement)
{
}

std::unique_ptr<SVGAnimatedType> SVGAnimatedBooleanAnimator::constructFromString(const String& string)
{
#if !PLATFORM(WKC)
    auto animatedType = SVGAnimatedType::createBoolean(std::make_unique<bool>());
#else
    void* p = fastMalloc(sizeof(bool));
    auto animatedType = SVGAnimatedType::createBoolean(std::unique_ptr<bool>(new (p) bool()));
#endif
    animatedType->boolean() = (string == "true"); // wat?
    return animatedType;
}

std::unique_ptr<SVGAnimatedType> SVGAnimatedBooleanAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    return SVGAnimatedType::createBoolean(constructFromBaseValue<SVGAnimatedBoolean>(animatedTypes));
}

void SVGAnimatedBooleanAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedBoolean>(animatedTypes);
}

void SVGAnimatedBooleanAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    resetFromBaseValue<SVGAnimatedBoolean>(animatedTypes, type, &SVGAnimatedType::boolean);
}

void SVGAnimatedBooleanAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedBoolean>(animatedTypes);
}

void SVGAnimatedBooleanAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedBoolean>(animatedTypes);
}

void SVGAnimatedBooleanAnimator::addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*)
{
    ASSERT_NOT_REACHED();
}

void SVGAnimatedBooleanAnimator::calculateAnimatedValue(float percentage, unsigned, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType*, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    bool fromBoolean = m_animationElement->animationMode() == ToAnimation ? animated->boolean() : from->boolean();
    bool toBoolean = to->boolean();
    bool& animatedBoolean = animated->boolean();

    m_animationElement->animateDiscreteType<bool>(percentage, fromBoolean, toBoolean, animatedBoolean);
}

float SVGAnimatedBooleanAnimator::calculateDistance(const String&, const String&)
{
    // No paced animations for boolean.
    return -1;
}

}
