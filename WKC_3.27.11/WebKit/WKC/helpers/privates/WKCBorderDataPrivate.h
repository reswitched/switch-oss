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

#pragma once

#include "helpers/WKCBorderData.h"

namespace WebCore {
class BorderData;
} // namespace

namespace WKC {

class BorderDataWrap : public BorderData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    BorderDataWrap(BorderDataPrivate& parent) : BorderData(parent) {}
    ~BorderDataWrap() {}
};

class BorderDataPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    BorderDataPrivate(const WebCore::BorderData*);
    ~BorderDataPrivate();

    const WebCore::BorderData* webcore() const { return m_webcore; }
    const BorderData& wkc() const { return m_wkc; }

    bool hasBorder() const;
    float borderLeftWidth() const;
    float borderRightWidth() const;
    float borderTopWidth() const;
    float borderBottomWidth() const;

    bool hasBorderRadius() const;
    const LengthSize& topLeftRadius();
    const LengthSize& topRightRadius();
    const LengthSize& bottomLeftRadius();
    const LengthSize& bottomRightRadius();

private:
    const WebCore::BorderData* m_webcore;
    BorderDataWrap m_wkc;

    LengthSizePrivate* m_topLeftRadius;
    LengthSizePrivate* m_topRightRadius;
    LengthSizePrivate* m_bottomLeftRadius;
    LengthSizePrivate* m_bottomRightRadius;
};

} // namespace
