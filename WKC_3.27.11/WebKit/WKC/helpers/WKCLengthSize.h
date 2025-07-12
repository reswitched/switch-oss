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

namespace WKC {

class LengthSizePrivate;

struct WKC_API LengthSize {
public:
    const Length& width() const;
    const Length& height() const;

    bool isEmpty() const { return width().isZero() || height().isZero(); }
    bool isZero() const { return width().isZero() && height().isZero(); }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    LengthSize(LengthSizePrivate&);
    ~LengthSize();

private:
    LengthSize(const LengthSize&) = delete;
    LengthSize& operator=(const LengthSize&) = delete;
    LengthSize(LengthSize&&) = delete;
    LengthSize& operator=(LengthSize&&) = delete;

    LengthSizePrivate& m_private;
};

} // namespace
