/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGPathSegClosePath_h
#define SVGPathSegClosePath_h

#include "SVGPathSegWithContext.h"

namespace WebCore {

class SVGPathSegClosePath : public SVGPathSegWithContext {
public:
    static Ref<SVGPathSegClosePath> create(SVGPathElement* element, SVGPathSegRole role)
    {
        return adoptRef(*new SVGPathSegClosePath(element, role));
    }

private:
    SVGPathSegClosePath(SVGPathElement* element, SVGPathSegRole role)
        : SVGPathSegWithContext(element, role)
    {
    }

    virtual unsigned short pathSegType() const override { return PATHSEG_CLOSEPATH; }
    virtual String pathSegTypeAsLetter() const override { return "Z"; }
};

} // namespace WebCore

#endif
