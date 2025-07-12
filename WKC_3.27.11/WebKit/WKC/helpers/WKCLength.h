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

#include "helpers/WKCHelpersEnums.h"

namespace WKC {

class LengthPrivate;

struct WKC_API Length {
public:
    LengthType type() const;
    bool isZero() const;
    bool isFloat() const;

    float value() const;
    int intValue() const;
    float percent() const;

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    Length(LengthPrivate&);
    ~Length();

private:
    Length(const Length&) = delete;
    Length& operator=(const Length&) = delete;
    Length(Length&&) = delete;
    Length& operator=(Length&&) = delete;

    LengthPrivate& m_private;
};

} // namespace
