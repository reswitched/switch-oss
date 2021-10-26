/*
 * Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCCachedFrame.h"
#include "helpers/privates/WKCCachedFramePrivate.h"

#include "CachedFrame.h"

namespace WKC {

CachedFramePrivate::CachedFramePrivate(WebCore::CachedFrame* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

CachedFramePrivate::~CachedFramePrivate()
{
}

////////////////////////////////////////////////////////////////////////////////

CachedFrame::CachedFrame(CachedFramePrivate& parent)
    : m_private(parent)
{
}

CachedFrame::~CachedFrame()
{
}

} // namespace
