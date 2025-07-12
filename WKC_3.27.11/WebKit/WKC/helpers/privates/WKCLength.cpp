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

#include "helpers/privates/WKCLengthPrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

#include "Length.h"

namespace WKC {

LengthPrivate::LengthPrivate(const WebCore::Length* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

LengthPrivate::~LengthPrivate()
{
}

LengthType
LengthPrivate::type() const
{
    return toWKCLengthType(m_webcore->type());
}

bool
LengthPrivate::isZero() const
{
    return m_webcore->isZero();
}

bool
LengthPrivate::isFloat() const
{
    return m_webcore->isFloat();
}

float
LengthPrivate::value() const
{
    return m_webcore->value();
}

int
LengthPrivate::intValue() const
{
    return m_webcore->intValue();
}

float
LengthPrivate::percent() const
{
    return m_webcore->percent();
}

////////////////////////////////////////////////////////////////////////////////

Length::Length(LengthPrivate& parent)
    : m_private(parent)
{
}

Length::~Length()
{
}

LengthType
Length::type() const
{
    return m_private.type();
}

bool
Length::isZero() const
{
    return m_private.isZero();
}

bool
Length::isFloat() const
{
    return m_private.isFloat();
}

float
Length::value() const
{
    return m_private.value();
}

int
Length::intValue() const
{
    return m_private.intValue();
}

float
Length::percent() const
{
    return m_private.percent();
}

} // namespace
