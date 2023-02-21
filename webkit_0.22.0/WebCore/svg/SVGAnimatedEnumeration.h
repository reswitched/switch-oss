/*
 * Copyright (C) Research In Motion Limited 2010, 2012. All rights reserved.
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

#ifndef SVGAnimatedEnumeration_h
#define SVGAnimatedEnumeration_h

#include "SVGAnimatedEnumerationPropertyTearOff.h"
#include "SVGAnimatedPropertyMacros.h"
#include "SVGAnimatedTypeAnimator.h"

namespace WebCore {

typedef SVGAnimatedStaticPropertyTearOff<unsigned> SVGAnimatedEnumeration;
#if PLATFORM(WKC)
template<> inline
std::unique_ptr<SVGAnimatedEnumeration::ContentType> SVGAnimatedTypeAnimator::constructFromBaseValue<SVGAnimatedEnumeration>(const SVGElementAnimatedPropertyList& animatedTypes)
{
    ASSERT(animatedTypes[0].properties.size() == 1);
    const unsigned& animatedType = castAnimatedPropertyToActualType<SVGAnimatedEnumeration>(animatedTypes[0].properties[0].get())->currentBaseValue();

    void* p = fastMalloc(sizeof(unsigned));
    auto copy = std::unique_ptr<SVGAnimatedEnumeration::ContentType>(new (p) unsigned(animatedType));
    executeAction<SVGAnimatedEnumeration>(StartAnimationAction, animatedTypes, 0, copy.get());
    return copy;
}
#endif

// Helper macros to declare/define a SVGAnimatedEnumeration object
#define DECLARE_ANIMATED_ENUMERATION(UpperProperty, LowerProperty, EnumType) \
DECLARE_ANIMATED_PROPERTY(SVGAnimatedEnumerationPropertyTearOff<EnumType>, EnumType, UpperProperty, LowerProperty, )

#define DEFINE_ANIMATED_ENUMERATION(OwnerType, DOMAttribute, UpperProperty, LowerProperty, EnumType) \
DEFINE_ANIMATED_PROPERTY(AnimatedEnumeration, OwnerType, DOMAttribute, DOMAttribute.localName(), UpperProperty, LowerProperty)

class SVGAnimatedEnumerationAnimator final : public SVGAnimatedTypeAnimator {
public:
    SVGAnimatedEnumerationAnimator(SVGAnimationElement*, SVGElement*);

    virtual std::unique_ptr<SVGAnimatedType> constructFromString(const String&) override;
    virtual std::unique_ptr<SVGAnimatedType> startAnimValAnimation(const SVGElementAnimatedPropertyList&) override;
    virtual void stopAnimValAnimation(const SVGElementAnimatedPropertyList&) override;
    virtual void resetAnimValToBaseVal(const SVGElementAnimatedPropertyList&, SVGAnimatedType*) override;
    virtual void animValWillChange(const SVGElementAnimatedPropertyList&) override;
    virtual void animValDidChange(const SVGElementAnimatedPropertyList&) override;

    virtual void addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*) override;
    virtual void calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType*, SVGAnimatedType*, SVGAnimatedType*, SVGAnimatedType*) override;
    virtual float calculateDistance(const String& fromString, const String& toString) override;
};

} // namespace WebCore

#endif
