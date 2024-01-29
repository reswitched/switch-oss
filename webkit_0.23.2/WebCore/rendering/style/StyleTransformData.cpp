/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "StyleTransformData.h"

#include "RenderStyle.h"

namespace WebCore {

StyleTransformData::StyleTransformData()
    : m_operations(RenderStyle::initialTransform())
    , m_x(RenderStyle::initialTransformOriginX())
    , m_y(RenderStyle::initialTransformOriginY())
    , m_z(RenderStyle::initialTransformOriginZ())
{
}

inline StyleTransformData::StyleTransformData(const StyleTransformData& o)
    : RefCounted<StyleTransformData>()
    , m_operations(o.m_operations)
    , m_x(o.m_x)
    , m_y(o.m_y)
    , m_z(o.m_z)
{
}

Ref<StyleTransformData> StyleTransformData::copy() const
{
    return adoptRef(*new StyleTransformData(*this));
}

bool StyleTransformData::operator==(const StyleTransformData& o) const
{
    return m_x == o.m_x && m_y == o.m_y && m_z == o.m_z && m_operations == o.m_operations;
}

} // namespace WebCore
