/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#ifndef SVGPathSegListPropertyTearOff_h
#define SVGPathSegListPropertyTearOff_h

#include "SVGAnimatedListPropertyTearOff.h"
#include "SVGPathSegList.h"

namespace WebCore {

class SVGPathElement;

class SVGPathSegListPropertyTearOff : public SVGListProperty<SVGPathSegList> {
public:
    typedef SVGListProperty<SVGPathSegList> Base;
    typedef SVGAnimatedListPropertyTearOff<SVGPathSegList> AnimatedListPropertyTearOff;
    typedef SVGPropertyTraits<SVGPathSegList>::ListItemType ListItemType;
    typedef RefPtr<SVGPathSeg> PtrListItemType;

    static Ref<SVGPathSegListPropertyTearOff> create(AnimatedListPropertyTearOff* animatedProperty, SVGPropertyRole role, SVGPathSegRole pathSegRole, SVGPathSegList& values, ListWrapperCache& wrappers)
    {
        ASSERT(animatedProperty);
        return adoptRef(*new SVGPathSegListPropertyTearOff(animatedProperty, role, pathSegRole, values, wrappers));
    }

    int findItem(const ListItemType& item) const
    {
        ASSERT(m_values);

        unsigned size = m_values->size();
        for (size_t i = 0; i < size; ++i) {
            if (item == m_values->at(i))
                return i;
        }

        return -1;
    }

    void removeItemFromList(size_t itemIndex, bool shouldSynchronizeWrappers)
    {
        ASSERT(m_values);
        ASSERT_WITH_SECURITY_IMPLICATION(itemIndex < m_values->size());

        m_values->remove(itemIndex);

        if (shouldSynchronizeWrappers)
            commitChange();
    }

    // SVGList API
    void clear(ExceptionCode&);

    PtrListItemType initialize(PtrListItemType newItem, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!newItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return nullptr;
        }

        return Base::initializeValues(newItem, ec);
    }

    PtrListItemType getItem(unsigned index, ExceptionCode&);

    PtrListItemType insertItemBefore(PtrListItemType newItem, unsigned index, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!newItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return 0;
        }

        return Base::insertItemBeforeValues(newItem, index, ec);
    }

    PtrListItemType replaceItem(PtrListItemType, unsigned index, ExceptionCode&);

    PtrListItemType removeItem(unsigned index, ExceptionCode&);

    PtrListItemType appendItem(PtrListItemType newItem, ExceptionCode& ec)
    {
        // Not specified, but FF/Opera do it this way, and it's just sane.
        if (!newItem) {
            ec = SVGException::SVG_WRONG_TYPE_ERR;
            return nullptr;
        }

        return Base::appendItemValues(newItem, ec);
    }

private:
    SVGPathSegListPropertyTearOff(AnimatedListPropertyTearOff* animatedProperty, SVGPropertyRole role, SVGPathSegRole pathSegRole, SVGPathSegList& values, ListWrapperCache& wrappers)
        : SVGListProperty<SVGPathSegList>(role, values, &wrappers)
        , m_animatedProperty(animatedProperty)
        , m_pathSegRole(pathSegRole)
    {
    }

    SVGPathElement* contextElement() const;

    using Base::m_role;

    virtual bool isReadOnly() const override
    {
        if (m_role == AnimValRole)
            return true;
        if (m_animatedProperty && m_animatedProperty->isReadOnly())
            return true;
        return false;
    }

    virtual void commitChange() override
    {
        ASSERT(m_values);
        m_values->commitChange(m_animatedProperty->contextElement(), ListModificationUnknown);
    }

    virtual void commitChange(ListModification listModification) override
    {
        ASSERT(m_values);
        m_values->commitChange(m_animatedProperty->contextElement(), listModification);
    }

    virtual bool processIncomingListItemValue(const ListItemType& newItem, unsigned* indexToModify) override;
    virtual bool processIncomingListItemWrapper(RefPtr<ListItemTearOff>&, unsigned*) override
    {
        ASSERT_NOT_REACHED();
        return true;
    }

private:
    RefPtr<AnimatedListPropertyTearOff> m_animatedProperty;
    SVGPathSegRole m_pathSegRole;
};

}

#endif // SVGListPropertyTearOff_h
