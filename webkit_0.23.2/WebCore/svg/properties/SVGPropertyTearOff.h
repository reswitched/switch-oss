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

#ifndef SVGPropertyTearOff_h
#define SVGPropertyTearOff_h

#include "SVGAnimatedProperty.h"
#include "SVGElement.h"
#include "SVGProperty.h"
#include <wtf/WeakPtr.h>

namespace WebCore {

class SVGPropertyTearOffBase : public SVGProperty {
public:
    virtual void detachWrapper() = 0;
};

template<typename PropertyType>
class SVGPropertyTearOff : public SVGPropertyTearOffBase {
public:
    typedef SVGPropertyTearOff<PropertyType> Self;

    // Used for child types (baseVal/animVal) of a SVGAnimated* property (for example: SVGAnimatedLength::baseVal()).
    // Also used for list tear offs (for example: text.x.baseVal.getItem(0)).
    static Ref<Self> create(SVGAnimatedProperty* animatedProperty, SVGPropertyRole role, PropertyType& value)
    {
        ASSERT(animatedProperty);
        return adoptRef(*new Self(animatedProperty, role, value));
    }

    // Used for non-animated POD types (for example: SVGSVGElement::createSVGLength()).
    static Ref<Self> create(const PropertyType& initialValue)
    {
        return adoptRef(*new Self(initialValue));
    }

    static Ref<Self> create(const PropertyType* initialValue)
    {
        return adoptRef(*new Self(initialValue));
    }

    virtual PropertyType& propertyReference() { return *m_value; }
    SVGAnimatedProperty* animatedProperty() const { return m_animatedProperty; }

    virtual void setValue(PropertyType& value)
    {
        if (m_valueIsCopy) {
            detachChildren();
#if !PLATFORM(WKC)
            delete m_value;
#else
            WTF::fastFree(m_value);
#endif
        }
        m_valueIsCopy = false;
        m_value = &value;
    }

    void setAnimatedProperty(SVGAnimatedProperty* animatedProperty)
    {
        m_animatedProperty = animatedProperty;

        if (m_animatedProperty)
            m_contextElement = m_animatedProperty->contextElement();
    }

    SVGElement* contextElement() const
    {
        if (!m_animatedProperty || m_valueIsCopy)
            return 0;
        return m_contextElement.get();
    }

    void addChild(WeakPtr<SVGPropertyTearOffBase> child)
    {
        m_childTearOffs.append(child);
    }

    virtual void detachWrapper() override
    {
        if (m_valueIsCopy)
            return;

        detachChildren();

        // Switch from a live value, to a non-live value.
        // For example: <text x="50"/>
        // var item = text.x.baseVal.getItem(0);
        // text.setAttribute("x", "100");
        // item.value still has to report '50' and it has to be possible to modify 'item'
        // w/o changing the "new item" (with x=100) in the text element.
        // Whenever the XML DOM modifies the "x" attribute, all existing wrappers are detached, using this function.
#if !PLATFORM(WKC)
        m_value = new PropertyType(*m_value);
#else
        void* p = WTF::fastMalloc(sizeof(PropertyType));
        m_value = new (p) PropertyType(*m_value);
#endif
        m_valueIsCopy = true;
        m_animatedProperty = nullptr;
    }

    virtual void commitChange() override
    {
        if (!m_animatedProperty || m_valueIsCopy)
            return;
        m_animatedProperty->commitChange();
    }

    virtual bool isReadOnly() const override
    {
        if (m_role == AnimValRole)
            return true;
        if (m_animatedProperty && m_animatedProperty->isReadOnly())
            return true;
        return false;
    }

protected:
    SVGPropertyTearOff(SVGAnimatedProperty* animatedProperty, SVGPropertyRole role, PropertyType& value)
        : m_animatedProperty(animatedProperty)
        , m_role(role)
        , m_value(&value)
        , m_valueIsCopy(false)
    {
        // Using operator & is completely fine, as SVGAnimatedProperty owns this reference,
        // and we're guaranteed to live as long as SVGAnimatedProperty does.

        if (m_animatedProperty)
            m_contextElement = m_animatedProperty->contextElement();
    }

    SVGPropertyTearOff(const PropertyType& initialValue)
        : SVGPropertyTearOff(&initialValue)
    {
    }

    SVGPropertyTearOff(const PropertyType* initialValue)
        : m_animatedProperty(nullptr)
        , m_role(UndefinedRole)
#if !PLATFORM(WKC)
        , m_value(initialValue ? new PropertyType(*initialValue) : nullptr)
#else
        , m_value(nullptr)
#endif
        , m_valueIsCopy(true)
    {
#if PLATFORM(WKC)
        if (initialValue) {
            void* p = WTF::fastMalloc(sizeof(PropertyType));
            m_value = new (p) PropertyType(*initialValue);
        }
#endif
    }

    virtual ~SVGPropertyTearOff()
    {
        if (m_valueIsCopy) {
            detachChildren();
#if !PLATFORM(WKC)
            delete m_value;
#else
            fastFree(m_value);
#endif
        }
    }

    void detachChildren()
    {
        for (const auto& childTearOff : m_childTearOffs) {
            if (childTearOff.get())
                childTearOff.get()->detachWrapper();
        }
        m_childTearOffs.clear();
    }

    RefPtr<SVGElement> m_contextElement;
    SVGAnimatedProperty* m_animatedProperty;
    SVGPropertyRole m_role;
    PropertyType* m_value;
    Vector<WeakPtr<SVGPropertyTearOffBase>> m_childTearOffs;
    bool m_valueIsCopy : 1;
};

}

#endif // SVGPropertyTearOff_h
