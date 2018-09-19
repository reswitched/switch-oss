/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#ifndef SVGPathSegList_h
#define SVGPathSegList_h

#include "SVGListProperty.h"
#include "SVGPathSeg.h"
#include "SVGPropertyTraits.h"

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class SVGElement;

class SVGPathSegList : public Vector<RefPtr<SVGPathSeg>> {
public:
    using Base = Vector<RefPtr<SVGPathSeg>>;

    explicit SVGPathSegList(SVGPathSegRole role)
        : m_role(role)
    {
    }


    SVGPathSegList(const SVGPathSegList&) = default;
    SVGPathSegList(SVGPathSegList&&) = default;

    SVGPathSegList& operator=(const SVGPathSegList& other)
    {
        clearContextAndRoles();
        return static_cast<SVGPathSegList&>(Base::operator=(other));
    }

    SVGPathSegList& operator=(SVGPathSegList&& other)
    {
        clearContextAndRoles();
        return static_cast<SVGPathSegList&>(Base::operator=(WTF::move(other)));
    }

    void clear()
    {
        clearContextAndRoles();
        Base::clear();
    }

    String valueAsString() const;

    // Only used by SVGPathSegListPropertyTearOff.
    void commitChange(SVGElement* contextElement, ListModification);
    void clearItemContextAndRole(unsigned index);

private:
    void clearContextAndRoles();

    SVGPathSegRole m_role;
};

template<>
struct SVGPropertyTraits<SVGPathSegList> {
    static SVGPathSegList initialValue() { return SVGPathSegList(PathSegUndefinedRole); }
    typedef RefPtr<SVGPathSeg> ListItemType;
};

} // namespace WebCore

#endif
