/*
 *  wkc_ConstWTFStringAccessor.h
 *
 *  Copyright (c) 2012,2013 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef WKC_CUSTOMER_PATCH_0304674

#pragma once

namespace WTF {

class String;

} // namespace WTF


// TODO 可能であれば、外部から取ってくる
typedef unsigned short uint16_t;
typedef uint16_t UChar;


namespace WKC {

class ConstWTFStringAccessor
{
public:

    ConstWTFStringAccessor( const WTF::String* obj )
     : m_string( obj )
    {}

    const WTF::String* ptr() const { return static_cast<const WTF::String*>( m_string ); }

    unsigned length() const;
    const UChar* characters() const;

private:
    const void* m_string;
};



} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
