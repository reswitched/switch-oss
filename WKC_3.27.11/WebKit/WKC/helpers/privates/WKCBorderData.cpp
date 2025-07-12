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

#include "helpers/privates/WKCBorderDataPrivate.h"
#include "helpers/privates/WKCLengthSizePrivate.h"

#include "BorderData.h"

namespace WKC {

BorderDataPrivate::BorderDataPrivate(const WebCore::BorderData* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_topLeftRadius(nullptr)
    , m_topRightRadius(nullptr)
    , m_bottomLeftRadius(nullptr)
    , m_bottomRightRadius(nullptr)
{
}

BorderDataPrivate::~BorderDataPrivate()
{
    delete m_topLeftRadius;
    delete m_topRightRadius;
    delete m_bottomLeftRadius;
    delete m_bottomRightRadius;
}

bool
BorderDataPrivate::hasBorder() const
{
    return m_webcore->hasBorder();
}

float
BorderDataPrivate::borderLeftWidth() const
{
    return m_webcore->borderLeftWidth();
}

float
BorderDataPrivate::borderRightWidth() const
{
    return m_webcore->borderRightWidth();
}

float
BorderDataPrivate::borderTopWidth() const
{
    return m_webcore->borderTopWidth();
}

float
BorderDataPrivate::borderBottomWidth() const
{
    return m_webcore->borderBottomWidth();
}

bool
BorderDataPrivate::hasBorderRadius() const
{
    return m_webcore->hasBorderRadius();
}

const LengthSize&
BorderDataPrivate::topLeftRadius()
{
    const WebCore::LengthSize& lengthSize = webcore()->topLeftRadius();

    delete m_topLeftRadius;
    m_topLeftRadius = new LengthSizePrivate(&lengthSize);

    return m_topLeftRadius->wkc();
}

const LengthSize&
BorderDataPrivate::topRightRadius()
{
    const WebCore::LengthSize& lengthSize = webcore()->topRightRadius();

    delete m_topRightRadius;
    m_topRightRadius = new LengthSizePrivate(&lengthSize);

    return m_topRightRadius->wkc();
}

const LengthSize&
BorderDataPrivate::bottomLeftRadius()
{
    const WebCore::LengthSize& lengthSize = webcore()->bottomLeftRadius();

    delete m_bottomLeftRadius;
    m_bottomLeftRadius = new LengthSizePrivate(&lengthSize);

    return m_bottomLeftRadius->wkc();
}

const LengthSize&
BorderDataPrivate::bottomRightRadius()
{
    const WebCore::LengthSize& lengthSize = webcore()->bottomRightRadius();

    delete m_bottomRightRadius;
    m_bottomRightRadius = new LengthSizePrivate(&lengthSize);

    return m_bottomRightRadius->wkc();
}

////////////////////////////////////////////////////////////////////////////////

BorderData::BorderData(BorderDataPrivate& parent)
    : m_private(parent)
{
}

BorderData::~BorderData()
{
}

bool
BorderData::hasBorder() const
{
    return m_private.hasBorder();
}

float
BorderData::borderLeftWidth() const
{
    return m_private.borderLeftWidth();
}

float
BorderData::borderRightWidth() const
{
    return m_private.borderRightWidth();
}

float
BorderData::borderTopWidth() const
{
    return m_private.borderTopWidth();
}

float
BorderData::borderBottomWidth() const
{
    return m_private.borderBottomWidth();
}

bool
BorderData::hasBorderRadius() const
{
    return m_private.hasBorderRadius();
}

const LengthSize&
BorderData::topLeftRadius() const
{
    return m_private.topLeftRadius();
}

const LengthSize&
BorderData::topRightRadius() const
{
    return m_private.topRightRadius();
}

const LengthSize&
BorderData::bottomLeftRadius() const
{
    return m_private.bottomLeftRadius();
}

const LengthSize&
BorderData::bottomRightRadius() const
{
    return m_private.bottomRightRadius();
}

} // namespace
