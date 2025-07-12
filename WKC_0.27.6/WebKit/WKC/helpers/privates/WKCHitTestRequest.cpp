/*
 * Copyright (c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "helpers/WKCHitTestRequest.h"
#include "helpers/privates/WKCHitTestRequestPrivate.h"

#include "HitTestRequest.h"

#include <string.h>

namespace WKC {

HitTestRequestPrivate::HitTestRequestPrivate(int type)
    : m_webcore(new WebCore::HitTestRequest(type))
{
}

HitTestRequestPrivate::~HitTestRequestPrivate()
{
    delete m_webcore;
}

HitTestRequestPrivate::HitTestRequestPrivate(const HitTestRequestPrivate& other)
{
    if (this!=&other)
        ::memcpy(this, &other, sizeof(HitTestRequestPrivate));
}

HitTestRequestPrivate&
HitTestRequestPrivate::operator =(const HitTestRequestPrivate& other)
{
    if (this!=&other)
        ::memcpy(this, &other, sizeof(HitTestRequestPrivate));
    return *this;
}

HitTestRequest::HitTestRequest(int type)
    : m_private(new HitTestRequestPrivate(type))
{
}

HitTestRequest::~HitTestRequest()
{
    delete m_private;
}

HitTestRequest::HitTestRequest(const HitTestRequest& other)
    : m_private(new HitTestRequestPrivate(other.m_private->webcore()->type()))
{
}

HitTestRequest&
HitTestRequest::operator=(const HitTestRequest& other)
{
    if (this!=&other) {
        delete m_private;
        m_private = new HitTestRequestPrivate(other.m_private->webcore()->type());
    }
    return *this;
}

} // WKC
