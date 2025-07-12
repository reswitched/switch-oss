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

#include "helpers/WKCLengthSize.h"
#include "helpers/privates/WKCLengthPrivate.h"

namespace WebCore {
class LengthSize;
} // namespace

namespace WKC {

struct LengthSizeWrap : public LengthSize {
    WTF_MAKE_FAST_ALLOCATED;
public:
    LengthSizeWrap(LengthSizePrivate& parent) : LengthSize(parent) {}
    ~LengthSizeWrap() {}
};

struct LengthSizePrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    LengthSizePrivate(const WebCore::LengthSize*);
    ~LengthSizePrivate();

    const WebCore::LengthSize* webcore() const { return m_webcore; }
    const LengthSize& wkc() const { return m_wkc; }

    const Length& width() const;
    const Length& height() const;

private:
    const WebCore::LengthSize* m_webcore;
    LengthSizeWrap m_wkc;

    LengthPrivate m_width;
    LengthPrivate m_height;
};

} // namespace
