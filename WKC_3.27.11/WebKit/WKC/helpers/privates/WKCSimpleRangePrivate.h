/*
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

#pragma once

#include "helpers/WKCSimpleRange.h"

namespace WebCore {
struct SimpleRange;
} // namespace

namespace WKC {

class SimpleRangeWrap : public SimpleRange {
    WTF_MAKE_FAST_ALLOCATED;
public:
    SimpleRangeWrap(SimpleRangePrivate& priv) : SimpleRange(priv) {}
    ~SimpleRangeWrap() {}
};

class SimpleRangePrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    SimpleRangePrivate(WebCore::SimpleRange* value);
    ~SimpleRangePrivate();

    SimpleRangePrivate(const SimpleRangePrivate&) = delete;

    WebCore::SimpleRange* webcore() { return m_webcore; }
    SimpleRange& wkc() { return m_wkc; }

private:
    SimpleRangePrivate& operator =(const SimpleRangePrivate&) = delete; // not needed

    WebCore::SimpleRange* m_webcore;
    SimpleRangeWrap m_wkc;
};

} // namespace
