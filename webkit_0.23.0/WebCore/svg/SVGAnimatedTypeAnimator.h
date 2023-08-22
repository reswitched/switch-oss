/*
 * Copyright (C) Research In Motion Limited 2011-2012. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef SVGAnimatedTypeAnimator_h
#define SVGAnimatedTypeAnimator_h

#include "SVGAnimatedProperty.h"
#include "SVGAnimatedType.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

struct SVGElementAnimatedProperties {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    SVGElement* element;
    Vector<RefPtr<SVGAnimatedProperty>> properties;
};
typedef Vector<SVGElementAnimatedProperties> SVGElementAnimatedPropertyList;

class SVGAnimationElement;

class SVGAnimatedTypeAnimator {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~SVGAnimatedTypeAnimator();
    virtual std::unique_ptr<SVGAnimatedType> constructFromString(const String&) = 0;

    virtual std::unique_ptr<SVGAnimatedType> startAnimValAnimation(const SVGElementAnimatedPropertyList&) = 0;
    virtual void stopAnimValAnimation(const SVGElementAnimatedPropertyList&) = 0;
    virtual void resetAnimValToBaseVal(const SVGElementAnimatedPropertyList&, SVGAnimatedType*) = 0;
    virtual void animValWillChange(const SVGElementAnimatedPropertyList&) = 0;
    virtual void animValDidChange(const SVGElementAnimatedPropertyList&) = 0;
    virtual void addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*) = 0;

    virtual void calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType*, SVGAnimatedType*, SVGAnimatedType*, SVGAnimatedType*) = 0;
    virtual float calculateDistance(const String& fromString, const String& toString) = 0;

    void calculateFromAndToValues(std::unique_ptr<SVGAnimatedType>&, std::unique_ptr<SVGAnimatedType>&, const String& fromString, const String& toString);
    void calculateFromAndByValues(std::unique_ptr<SVGAnimatedType>&, std::unique_ptr<SVGAnimatedType>&, const String& fromString, const String& byString);

    void setContextElement(SVGElement* contextElement) { m_contextElement = contextElement; }
    AnimatedPropertyType type() const { return m_type; }

    SVGElementAnimatedPropertyList findAnimatedPropertiesForAttributeName(SVGElement&, const QualifiedName&);

protected:
    SVGAnimatedTypeAnimator(AnimatedPropertyType, SVGAnimationElement*, SVGElement*);

    // Helpers for animators that operate on single types, eg. just one SVGAnimatedInteger.
    template<typename AnimValType>
    std::unique_ptr<typename AnimValType::ContentType> constructFromBaseValue(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 1);
        const typename AnimValType::ContentType& animatedType = castAnimatedPropertyToActualType<AnimValType>(animatedTypes[0].properties[0].get())->currentBaseValue();

        auto copy = std::make_unique<typename AnimValType::ContentType>(animatedType);
        executeAction<AnimValType>(StartAnimationAction, animatedTypes, 0, copy.get());
        return copy;
    }

    template<typename AnimValType>
    void resetFromBaseValue(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type, typename AnimValType::ContentType& (SVGAnimatedType::*getter)())
    {
        ASSERT(animatedTypes[0].properties.size() == 1);
        ASSERT(type);
        ASSERT(type->type() == m_type);
        auto* property = castAnimatedPropertyToActualType<AnimValType>(animatedTypes[0].properties[0].get());
        property->synchronizeWrappersIfNeeded();

        typename AnimValType::ContentType& animatedTypeValue = (type->*getter)();
        animatedTypeValue = property->currentBaseValue();

        executeAction<AnimValType>(StartAnimationAction, animatedTypes, 0, &animatedTypeValue);
    }

    template<typename AnimValType>
    void stopAnimValAnimationForType(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 1);
        executeAction<AnimValType>(StopAnimationAction, animatedTypes, 0);
    }

    template<typename AnimValType>
    void animValDidChangeForType(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 1);
        executeAction<AnimValType>(AnimValDidChangeAction, animatedTypes, 0);
    }

    template<typename AnimValType>
    void animValWillChangeForType(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 1);
        executeAction<AnimValType>(AnimValWillChangeAction, animatedTypes, 0);
    }

    // Helpers for animators that operate on pair types, eg. a pair of SVGAnimatedIntegers.
    template<typename AnimValType1, typename AnimValType2>
    std::unique_ptr<std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>> constructFromBaseValues(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 2);
        const typename AnimValType1::ContentType& firstType = castAnimatedPropertyToActualType<AnimValType1>(animatedTypes[0].properties[0].get())->currentBaseValue();
        const typename AnimValType2::ContentType& secondType = castAnimatedPropertyToActualType<AnimValType2>(animatedTypes[0].properties[1].get())->currentBaseValue();

#if !PLATFORM(WKC)
        auto copy = std::make_unique<std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>>(firstType, secondType);
#else
        void* p = fastMalloc(sizeof(std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>));
        auto copy = std::unique_ptr<std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>>(new (p) std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>(firstType, secondType));
#endif
        executeAction<AnimValType1>(StartAnimationAction, animatedTypes, 0, &copy->first);
        executeAction<AnimValType2>(StartAnimationAction, animatedTypes, 1, &copy->second);
        return copy;
    }

    template<typename AnimValType1, typename AnimValType2>
    void resetFromBaseValues(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type, std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>& (SVGAnimatedType::*getter)())
    {
        ASSERT(animatedTypes[0].properties.size() == 2);
        ASSERT(type);
        ASSERT(type->type() == m_type);
        auto* firstProperty = castAnimatedPropertyToActualType<AnimValType1>(animatedTypes[0].properties[0].get());
        auto* secondProperty = castAnimatedPropertyToActualType<AnimValType2>(animatedTypes[0].properties[1].get());
        firstProperty->synchronizeWrappersIfNeeded();
        secondProperty->synchronizeWrappersIfNeeded();

        std::pair<typename AnimValType1::ContentType, typename AnimValType2::ContentType>& animatedTypeValue = (type->*getter)();
        animatedTypeValue.first = firstProperty->currentBaseValue();
        animatedTypeValue.second = secondProperty->currentBaseValue();

        executeAction<AnimValType1>(StartAnimationAction, animatedTypes, 0, &animatedTypeValue.first);
        executeAction<AnimValType2>(StartAnimationAction, animatedTypes, 1, &animatedTypeValue.second);
    }

    template<typename AnimValType1, typename AnimValType2>
    void stopAnimValAnimationForTypes(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 2);
        executeAction<AnimValType1>(StopAnimationAction, animatedTypes, 0);
        executeAction<AnimValType2>(StopAnimationAction, animatedTypes, 1);
    }

    template<typename AnimValType1, typename AnimValType2>
    void animValDidChangeForTypes(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 2);
        executeAction<AnimValType1>(AnimValDidChangeAction, animatedTypes, 0);
        executeAction<AnimValType2>(AnimValDidChangeAction, animatedTypes, 1);
    }

    template<typename AnimValType1, typename AnimValType2>
    void animValWillChangeForTypes(const SVGElementAnimatedPropertyList& animatedTypes)
    {
        ASSERT(animatedTypes[0].properties.size() == 2);
        executeAction<AnimValType1>(AnimValWillChangeAction, animatedTypes, 0);
        executeAction<AnimValType2>(AnimValWillChangeAction, animatedTypes, 1);
    }

    template<typename AnimValType>
    AnimValType* castAnimatedPropertyToActualType(SVGAnimatedProperty* property)
    {
        ASSERT(property);
        ASSERT(property->contextElement());
        // We can't assert property->animatedPropertyType() == m_type, as there's an exception for SVGMarkerElements orient attribute.
        if (property->animatedPropertyType() != m_type) {
            ASSERT(m_type == AnimatedAngle);
            ASSERT(property->animatedPropertyType() == AnimatedEnumeration);
        }
        return static_cast<AnimValType*>(property);
    }

    AnimatedPropertyType m_type;
    SVGAnimationElement* m_animationElement;
    SVGElement* m_contextElement;

private:
    enum AnimationAction {
        StartAnimationAction,
        StopAnimationAction,
        AnimValWillChangeAction,
        AnimValDidChangeAction
    };

    template<typename AnimValType>
    void executeAction(AnimationAction action, const SVGElementAnimatedPropertyList& animatedTypes, unsigned whichProperty, typename AnimValType::ContentType* type = 0)
    {
        // FIXME: Can't use SVGElement::InstanceUpdateBlocker because of circular header dependency. Would be nice to untangle this.
        setInstanceUpdatesBlocked(*animatedTypes[0].element, true);

        for (auto& animatedType : animatedTypes) {
            ASSERT_WITH_SECURITY_IMPLICATION(whichProperty < animatedType.properties.size());
            AnimValType* property = castAnimatedPropertyToActualType<AnimValType>(animatedType.properties[whichProperty].get());

            switch (action) {
            case StartAnimationAction:
                ASSERT(type);
                if (!property->isAnimating())
                    property->animationStarted(type);
                break;
            case StopAnimationAction:
                ASSERT(!type);
                if (property->isAnimating())
                    property->animationEnded();
                break;
            case AnimValWillChangeAction:
                ASSERT(!type);
                property->animValWillChange();
                break;
            case AnimValDidChangeAction:
                ASSERT(!type);
                property->animValDidChange();
                break;
            }
        }

        setInstanceUpdatesBlocked(*animatedTypes[0].element, false);
    }

    static void setInstanceUpdatesBlocked(SVGElement&, bool);
};

} // namespace WebCore

#endif // SVGAnimatedTypeAnimator_h
