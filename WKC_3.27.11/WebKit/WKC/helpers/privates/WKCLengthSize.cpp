/*
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

#include "helpers/privates/WKCLengthSizePrivate.h"

#include "LengthSize.h"

namespace WKC {

LengthSizePrivate::LengthSizePrivate(const WebCore::LengthSize* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_width(&parent->width)
    , m_height(&parent->height)
{
}

LengthSizePrivate::~LengthSizePrivate()
{
}

const Length&
LengthSizePrivate::width() const
{
    return m_width.wkc();
}

const Length&
LengthSizePrivate::height() const
{
    return m_height.wkc();
}

////////////////////////////////////////////////////////////////////////////////

LengthSize::LengthSize(LengthSizePrivate& parent)
    : m_private(parent)
{
}

LengthSize::~LengthSize()
{
}

const Length&
LengthSize::width() const
{
    return m_private.width();
}

const Length&
LengthSize::height() const
{
    return m_private.height();
}

} // namespace
