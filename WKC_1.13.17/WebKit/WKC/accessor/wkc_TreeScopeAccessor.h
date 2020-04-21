/*
 *  wkc_TreeScopeAccessor.h
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

#include <wkc_ContainerNodeAccessor.h>


namespace WKC {

class TreeScopeAccessor : public ContainerNodeAccessor
{
public:
    TreeScopeAccessor( WebCore::TreeScope* obj )
    : ContainerNodeAccessor( reinterpret_cast<WebCore::ContainerNode*>( obj ) )
    {}

    WebCore::TreeScope*       ptr()       { return static_cast<WebCore::TreeScope*>( m_webcore ); }
    const WebCore::TreeScope* ptr() const { return static_cast<const WebCore::TreeScope*>( m_webcore ); }


};

} // namespace WKC

#endif // WKC_CUSTOMER_PATCH_0304674
