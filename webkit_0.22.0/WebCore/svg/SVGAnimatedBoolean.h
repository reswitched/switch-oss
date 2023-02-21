/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGAnimatedBoolean_h
#define SVGAnimatedBoolean_h

#include "SVGAnimatedStaticPropertyTearOff.h"
#include "SVGAnimatedTypeAnimator.h"

namespace WebCore {

typedef SVGAnimatedStaticPropertyTearOff<bool> SVGAnimatedBoolean;
#if PLATFORM(WKC)
template<> inline
std::unique_ptr<SVGAnimatedBoolean::ContentType> SVGAnimatedTypeAnimator::constructFromBaseValue<SVGAnimatedBoolean>(const SVGElementAnimatedPropertyList& animatedTypes)
{
    ASSERT(animatedTypes[0].properties.size() == 1);
    const bool& animatedType = castAnimatedPropertyToActualType<SVGAnimatedBoolean>(animatedTypes[0].properties[0].get())->currentBaseValue();

    void* p = fastMalloc(sizeof(bool));
    auto copy = std::unique_ptr<SVGAnimatedBoolean::ContentType>(new (p) bool(animatedType));
    executeAction<SVGAnimatedBoolean>(StartAnimationAction, animatedTypes, 0, copy.get());
    return copy;
}
#endif

// Helper macros to declare/define a SVGAnimatedBoolean object
#define DECLARE_ANIMATED_BOOLEAN(UpperProperty, LowerProperty) \
DECLARE_ANIMATED_PROPERTY(SVGAnimatedBoolean, bool, UpperProperty, LowerProperty, )

#define DECLARE_ANIMATED_BOOLEAN_OVERRIDE(UpperProperty, LowerProperty) \
DECLARE_ANIMATED_PROPERTY(SVGAnimatedBoolean, bool, UpperProperty, LowerProperty, override)

#define DEFINE_ANIMATED_BOOLEAN(OwnerType, DOMAttribute, UpperProperty, LowerProperty) \
DEFINE_ANIMATED_PROPERTY(AnimatedBoolean, OwnerType, DOMAttribute, DOMAttribute.localName(), UpperProperty, LowerProperty)

class SVGAnimationElement;

class SVGAnimatedBooleanAnimator final : public SVGAnimatedTypeAnimator {
public:
    SVGAnimatedBooleanAnimator(SVGAnimationElement*, SVGElement*);

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
