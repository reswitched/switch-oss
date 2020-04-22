/*
 *  wkc_AttributeAccessor.cpp
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

#include <wkc_AttributeAccessor.h>

#undef ASSERT
#include <wkcplatform.h>
#include <wkcglobalwrapper.h>
#include <WebCore/config.h>
#include <WebCore/dom/Attribute.h>
#include <WebCore/dom/QualifiedName.h>

namespace WKC {

bool
AttributeAccessor::CheckValue( const char* value )
{
    WTF::AtomicString value_str( value );

    return value_str == ptr()->value();
}

bool
AttributeAccessor::CheckValue( const WTF::AtomicString* value )
{
    return *value == ptr()->value();
}

const WTF::AtomicString*
AttributeAccessor::GetValue()
{
    return &ptr()->value();
}

} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
