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

#include "helpers/WKCLength.h"

namespace WebCore {
class Length;
} // namespace

namespace WKC {

struct LengthWrap : public Length {
    WTF_MAKE_FAST_ALLOCATED;
public:
    LengthWrap(LengthPrivate& parent) : Length(parent) {}
    ~LengthWrap() {}
};

struct LengthPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    LengthPrivate(const WebCore::Length*);
    ~LengthPrivate();

    const WebCore::Length* webcore() const { return m_webcore; }
    const Length& wkc() const { return m_wkc; }

    LengthType type() const;
    bool isZero() const;
    bool isFloat() const;

    float value() const;
    int intValue() const;
    float percent() const;

private:
    const WebCore::Length* m_webcore;
    LengthWrap m_wkc;
};

} // namespace
