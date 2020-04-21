/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCResourceResponse.h"
#include "helpers/privates/WKCResourceResponsePrivate.h"

#include "ResourceResponse.h"

#include "URL.h"
#include "WTFString.h"
#include "ResourceRequest.h"

#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"
#include "helpers/privates/WKCResourceHandlePrivate.h"

namespace WKC {

ResourceResponsePrivate::ResourceResponsePrivate(const WebCore::ResourceResponse& parent)
	: m_webcore(parent)
	, m_wkc(*this)
	, m_resourceHandle(0)
	, m_httpStatusText()
{
}

ResourceResponsePrivate::~ResourceResponsePrivate()
{
    delete m_resourceHandle;
}

const KURL
ResourceResponsePrivate::url() const
{
    return webcore().url();
}

const String
ResourceResponsePrivate::mimeType() const
{
    return webcore().mimeType();
}

bool
ResourceResponsePrivate::isAttachment() const
{
    return webcore().isAttachment();
}

bool
ResourceResponsePrivate::isNull() const
{
    return webcore().isNull();
}

int
ResourceResponsePrivate::httpStatusCode() const
{
    return webcore().httpStatusCode();
}

long long
ResourceResponsePrivate::expectedContentLength() const
{
    return webcore().expectedContentLength();
}

const String&
ResourceResponsePrivate::httpStatusText()
{
    m_httpStatusText = webcore().httpStatusText();
    return m_httpStatusText;
}

const String
ResourceResponsePrivate::httpHeaderField(const char* name) const
{
    if (!name)
        return String();
    return webcore().httpHeaderField(name);
}

bool
ResourceResponsePrivate::wasCached() const
{
    return webcore().source() == WebCore::ResourceResponseBase::Source::DiskCache;
}

ResourceHandle*
ResourceResponsePrivate::resourceHandle()
{
    delete m_resourceHandle;
    m_resourceHandle = new ResourceHandlePrivate(webcore().resourceHandle());
    return &m_resourceHandle->wkc();

}

////////////////////////////////////////////////////////////////////////////////

ResourceResponse::ResourceResponse(ResourceResponsePrivate& parent)
    : m_private(parent)
{
}

ResourceResponse::~ResourceResponse()
{
}

const KURL
ResourceResponse::url() const
{
    return m_private.url();
}

const String
ResourceResponse::mimeType() const
{
    return m_private.mimeType();
}

bool
ResourceResponse::isAttachment() const
{
    return m_private.isAttachment();
}

bool
ResourceResponse::isNull() const
{
    return m_private.isNull();
}

int
ResourceResponse::httpStatusCode() const
{
    return m_private.httpStatusCode();
}

ResourceHandle*
ResourceResponse::resourceHandle() const
{
    return m_private.resourceHandle();
}

long long
ResourceResponse::expectedContentLength() const
{
    return m_private.expectedContentLength();
}

const String&
ResourceResponse::httpStatusText() const
{
    return m_private.httpStatusText();
}

const String
ResourceResponse::httpHeaderField(const char* name) const
{
    return m_private.httpHeaderField(name);
}

bool
ResourceResponse::wasCached() const
{
    return m_private.wasCached();
}

} // namespace
