/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#include <wkc/wkcbase.h>

#include "config.h"

#include "IntPoint.h"

namespace WebCore {

IntPoint::IntPoint(const WKCPoint& pos)
    : m_x(pos.fX)
    , m_y(pos.fY)
{
}

IntPoint::operator WKCPoint() const
{
    WKCPoint p = { m_x, m_y };
    return p;
}

}


#include "IntSize.h"

namespace WebCore {
IntSize::IntSize(const WKCSize& size)
    : m_width(size.fWidth)
    , m_height(size.fHeight)
{
}

IntSize::operator WKCSize() const
{
    WKCSize s = { m_width, m_height };
    return s;
}
}

#include "IntRect.h"
namespace WebCore {
IntRect::IntRect(const WKCRect& rect)
    : m_location(IntPoint(rect.fX, rect.fY))
    , m_size(IntSize(rect.fWidth, rect.fHeight))
{
}

IntRect::operator WKCRect() const
{
    WKCRect r = { x(), y(), width(), height() };
    return r;
}
}

#include "FloatPoint.h"

namespace WebCore {

FloatPoint::FloatPoint(const WKCFloatPoint& pos)
    : m_x(pos.fX)
    , m_y(pos.fY)
{
}

FloatPoint::operator WKCFloatPoint() const
{
    WKCFloatPoint p = { m_x, m_y };
    return p;
}

FloatPoint::operator WKCPoint() const
{
    WKCPoint p = { (int)m_x, (int)m_y };
    return p;
}

}


#include "FloatSize.h"

namespace WebCore {
FloatSize::FloatSize(const WKCFloatSize& size)
    : m_width(size.fWidth)
    , m_height(size.fHeight)
{
}

FloatSize::operator WKCFloatSize() const
{
    WKCFloatSize s = { m_width, m_height };
    return s;
}
}

#include "FloatRect.h"
namespace WebCore {
FloatRect::FloatRect(const WKCFloatRect& rect)
    : m_location(FloatPoint(rect.fX, rect.fY))
    , m_size(FloatSize(rect.fWidth, rect.fHeight))
{
}

FloatRect::operator WKCFloatRect() const
{
    WKCFloatRect r = { x(), y(), width(), height() };
    return r;
}
}

#include "LayoutPoint.h"
namespace WebCore {
LayoutPoint::LayoutPoint(const WKCPoint& pos)
    : m_x(pos.fX)
    , m_y(pos.fY)
{
}

LayoutPoint::operator WKCPoint() const
{
    WKCPoint p = { m_x, m_y };
    return p;
}
}

#include "LayoutRect.h"
namespace WebCore {
LayoutRect::LayoutRect(const WKCRect& rect)
    : m_location(LayoutPoint(rect.fX, rect.fY))
    , m_size(LayoutSize(rect.fWidth, rect.fHeight))
{
}

LayoutRect::operator WKCRect() const
{
    WKCRect r = { x(), y(), width(), height() };
    return r;
}
}
