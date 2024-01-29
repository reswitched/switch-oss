/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
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

#ifndef SVGPathSegLinetoVerticalAbs_h
#define SVGPathSegLinetoVerticalAbs_h

#include "SVGPathSegLinetoVertical.h"

namespace WebCore {

class SVGPathSegLinetoVerticalAbs : public SVGPathSegLinetoVertical {
public:
    static Ref<SVGPathSegLinetoVerticalAbs> create(SVGPathElement* element, SVGPathSegRole role, float y)
    {
        return adoptRef(*new SVGPathSegLinetoVerticalAbs(element, role, y));
    }

private:
    SVGPathSegLinetoVerticalAbs(SVGPathElement* element, SVGPathSegRole role, float y)
        : SVGPathSegLinetoVertical(element, role, y)
    {
    }

    virtual unsigned short pathSegType() const override { return PATHSEG_LINETO_VERTICAL_ABS; }
    virtual String pathSegTypeAsLetter() const override { return "V"; }
};

} // namespace WebCore

#endif
