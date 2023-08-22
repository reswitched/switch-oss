/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Inc.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef CSSProperty_h
#define CSSProperty_h

#include "CSSPropertyNames.h"
#include "CSSValue.h"
#include "RenderStyleConstants.h"
#include "WritingMode.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

struct StylePropertyMetadata {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    StylePropertyMetadata(CSSPropertyID propertyID, bool isSetFromShorthand, int indexInShorthandsVector, bool important, bool implicit, bool inherited)
        : m_propertyID(propertyID)
        , m_isSetFromShorthand(isSetFromShorthand)
        , m_indexInShorthandsVector(indexInShorthandsVector)
        , m_important(important)
        , m_implicit(implicit)
        , m_inherited(inherited)
    {
    }

    CSSPropertyID shorthandID() const;
    
    bool operator==(const StylePropertyMetadata& other) const
    {
        return m_propertyID == other.m_propertyID
            && m_isSetFromShorthand == other.m_isSetFromShorthand
            && m_indexInShorthandsVector == other.m_indexInShorthandsVector
            && m_important == other.m_important
            && m_implicit == other.m_implicit
            && m_inherited == other.m_inherited;
    }

    uint16_t m_propertyID : 10;
    uint16_t m_isSetFromShorthand : 1;
    uint16_t m_indexInShorthandsVector : 2; // If this property was set as part of an ambiguous shorthand, gives the index in the shorthands vector.
    uint16_t m_important : 1;
    uint16_t m_implicit : 1; // Whether or not the property was set implicitly as the result of a shorthand.
    uint16_t m_inherited : 1;
};

class CSSProperty {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    CSSProperty(CSSPropertyID propertyID, PassRefPtr<CSSValue> value, bool important = false, bool isSetFromShorthand = false, int indexInShorthandsVector = 0, bool implicit = false)
        : m_metadata(propertyID, isSetFromShorthand, indexInShorthandsVector, important, implicit, isInheritedProperty(propertyID))
        , m_value(value)
    {
    }

    CSSPropertyID id() const { return static_cast<CSSPropertyID>(m_metadata.m_propertyID); }
    bool isSetFromShorthand() const { return m_metadata.m_isSetFromShorthand; };
    CSSPropertyID shorthandID() const { return m_metadata.shorthandID(); };
    bool isImportant() const { return m_metadata.m_important; }

    CSSValue* value() const { return m_value.get(); }

    void wrapValueInCommaSeparatedList();

    static CSSPropertyID resolveDirectionAwareProperty(CSSPropertyID, TextDirection, WritingMode);
    static bool isInheritedProperty(CSSPropertyID);
    static bool isDirectionAwareProperty(CSSPropertyID);

    const StylePropertyMetadata& metadata() const { return m_metadata; }

    bool operator==(const CSSProperty& other) const
    {
        if (!(m_metadata == other.m_metadata))
            return false;

        if (!m_value && !other.m_value)
            return true;

        if (!m_value || !other.m_value)
            return false;
        
        return m_value->equals(*other.m_value);
    }

private:
    StylePropertyMetadata m_metadata;
    RefPtr<CSSValue> m_value;
};

inline CSSPropertyID prefixingVariantForPropertyId(CSSPropertyID propertyID)
{
    ASSERT(propertyID != CSSPropertyInvalid);

    switch (propertyID) {
    case CSSPropertyAnimation:
        return CSSPropertyWebkitAnimation;
    case CSSPropertyAnimationDelay:
        return CSSPropertyWebkitAnimationDelay;
    case CSSPropertyAnimationDirection:
        return CSSPropertyWebkitAnimationDirection;
    case CSSPropertyAnimationDuration:
        return CSSPropertyWebkitAnimationDuration;
    case CSSPropertyAnimationFillMode:
        return CSSPropertyWebkitAnimationFillMode;
    case CSSPropertyAnimationName:
        return CSSPropertyWebkitAnimationName;
    case CSSPropertyAnimationPlayState:
        return CSSPropertyWebkitAnimationPlayState;
    case CSSPropertyAnimationIterationCount:
        return CSSPropertyWebkitAnimationIterationCount;
    case CSSPropertyAnimationTimingFunction:
        return CSSPropertyWebkitAnimationTimingFunction;
    case CSSPropertyWebkitAnimation:
        return CSSPropertyAnimation;
    case CSSPropertyWebkitAnimationDelay:
        return CSSPropertyAnimationDelay;
    case CSSPropertyWebkitAnimationDirection:
        return CSSPropertyAnimationDirection;
    case CSSPropertyWebkitAnimationDuration:
        return CSSPropertyAnimationDuration;
    case CSSPropertyWebkitAnimationFillMode:
        return CSSPropertyAnimationFillMode;
    case CSSPropertyWebkitAnimationName:
        return CSSPropertyAnimationName;
    case CSSPropertyWebkitAnimationPlayState:
        return CSSPropertyAnimationPlayState;
    case CSSPropertyWebkitAnimationIterationCount:
        return CSSPropertyAnimationIterationCount;
    case CSSPropertyWebkitAnimationTimingFunction:
        return CSSPropertyAnimationTimingFunction;
    case CSSPropertyTransitionDelay:
        return CSSPropertyWebkitTransitionDelay;
    case CSSPropertyTransitionDuration:
        return CSSPropertyWebkitTransitionDuration;
    case CSSPropertyTransitionProperty:
        return CSSPropertyWebkitTransitionProperty;
    case CSSPropertyTransitionTimingFunction:
        return CSSPropertyWebkitTransitionTimingFunction;
    case CSSPropertyTransition:
        return CSSPropertyWebkitTransition;
    case CSSPropertyWebkitTransitionDelay:
        return CSSPropertyTransitionDelay;
    case CSSPropertyWebkitTransitionDuration:
        return CSSPropertyTransitionDuration;
    case CSSPropertyWebkitTransitionProperty:
        return CSSPropertyTransitionProperty;
    case CSSPropertyWebkitTransitionTimingFunction:
        return CSSPropertyTransitionTimingFunction;
    case CSSPropertyWebkitTransition:
        return CSSPropertyTransition;
    default:
        return propertyID;
    }
}

} // namespace WebCore

namespace WTF {
template <> struct VectorTraits<WebCore::CSSProperty> : VectorTraitsBase<false, WebCore::CSSProperty> {
    static const bool canInitializeWithMemset = true;
    static const bool canMoveWithMemcpy = true;
};
}

#endif // CSSProperty_h
