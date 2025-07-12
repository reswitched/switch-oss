/*
 * Copyright (c) 2012 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "Color.h"
#include "helpers/WKCColor.h"

#define PARENT() ((WebCore::Color *)m_parent)

namespace WKC {

Color::Color(ColorPrivate* parent)
    : m_parent((ColorPrivate *)new WebCore::Color())
{
    // copy struct
    *PARENT() = *(WebCore::Color *)parent;
}

Color::Color(const Color& color)
    : m_parent((ColorPrivate *)new WebCore::Color())
{
    // copy struct
    *PARENT() = *(WebCore::Color *)color.parent();
}

Color::Color()
    : m_parent((ColorPrivate *)new WebCore::Color())
{
}

Color::~Color()
{
    delete (WebCore::Color *)m_parent;
}

Color&
Color::operator=(const Color& orig)
{
    if (this!=&orig) {
        // copy struct
        *PARENT() = *(WebCore::Color *)orig.parent();
    }
    return *this;
}

bool
Color::isValid() const
{
    return PARENT()->isValid();
}

bool
Color::hasAlpha() const
{
    return PARENT()->hasAlpha();
}

int
Color::red() const
{
    return PARENT()->red();
}

int
Color::green() const
{
    return PARENT()->green();
}

int
Color::blue() const
{
    return PARENT()->blue();
}

int
Color::alpha() const
{
    return PARENT()->alpha();
}

RGBA32
Color::rgb() const
{
    return PARENT()->rgb();
}

} // namespace
