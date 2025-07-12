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

#include "helpers/WKCResourceRequest.h"
#include "helpers/privates/WKCResourceRequestPrivate.h"

#include "ResourceRequest.h"

#include "WTFString.h"
#include "URL.h"

#include "helpers/WKCString.h"
#include "helpers/WKCKURL.h"
#include "helpers/WKCFormData.h"
#include "helpers/privates/WKCFormDataPrivate.h"

#include "NotImplemented.h"

namespace WKC {

ResourceRequestPrivate::ResourceRequestPrivate(const WebCore::ResourceRequest& parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_url()
    , m_httpMethod()
{
}


ResourceRequestPrivate::~ResourceRequestPrivate()
{
}


const KURL&
ResourceRequestPrivate::url()
{
    m_url = m_webcore.url();
    return m_url;
}

void
ResourceRequestPrivate::setURL(const KURL& url)
{
    const_cast<WebCore::ResourceRequest&>(m_webcore).setURL(url);
}

void
ResourceRequestPrivate::clearHTTPReferrer()
{
    const_cast<WebCore::ResourceRequest&>(m_webcore).clearHTTPReferrer();
}

void
ResourceRequestPrivate::setHTTPHeaderField(const char* hdr, const char* str)
{
    const_cast<WebCore::ResourceRequest&>(m_webcore).setHTTPHeaderField(hdr, str);
}

void
ResourceRequestPrivate::setHTTPBody(FormData* httpBody)
{
    WTF::RefPtr<WebCore::FormData> rp = 0;
    if (httpBody)
        rp = httpBody->priv().webcore();
    ((WebCore::ResourceRequest&)m_webcore).setHTTPBody(rp);
}

void
ResourceRequestPrivate::setHTTPMethod(const String& httpMethod)
{
    const_cast<WebCore::ResourceRequest&>(m_webcore).setHTTPMethod(httpMethod);
}

String
ResourceRequestPrivate::httpHeaderField(const char* name) const
{
    return const_cast<WebCore::ResourceRequest&>(m_webcore).httpHeaderField(name);
}
 
const String&
ResourceRequestPrivate::httpMethod()
{
    m_httpMethod = const_cast<WebCore::ResourceRequest&>(m_webcore).httpMethod();
    return m_httpMethod;
}

bool
ResourceRequestPrivate::isNull() const
{
    return m_webcore.isNull();
}


ResourceRequest::TargetType
ResourceRequestPrivate::targetType() const
{
    return (ResourceRequest::TargetType)m_webcore.targetType();
}

bool
ResourceRequestPrivate::isMainResource() const
{
    return m_webcore.isMainResource();
}

////////////////////////////////////////////////////////////////////////////////

ResourceRequest::ResourceRequest(ResourceRequestPrivate& parent)
    : m_private(parent)
{
}

ResourceRequest::~ResourceRequest()
{
}

const KURL&
ResourceRequest::url() const
{
    return m_private.url();
}

void
ResourceRequest::setURL(const KURL& url)
{
    m_private.setURL(url);
}

void
ResourceRequest::clearHTTPReferrer()
{
    m_private.clearHTTPReferrer();
}

void
ResourceRequest::setHTTPHeaderField(const char* name, const char* value)
{
    m_private.setHTTPHeaderField(name, value);
}

void
ResourceRequest::setHTTPBody(FormData* httpBody)
{
    m_private.setHTTPBody(httpBody);
}

void
ResourceRequest::setHTTPMethod(const String& httpMethod)
{
    m_private.setHTTPMethod(httpMethod);
}

String
ResourceRequest::httpHeaderField(const char* name) const
{
    return m_private.httpHeaderField(name);
}

const String&
ResourceRequest::httpMethod() const
{
    return m_private.httpMethod();
}

bool 
ResourceRequest::isNull() const
{
    return m_private.isNull();
}

ResourceRequest::TargetType
ResourceRequest::targetType() const
{
    return m_private.targetType();
}

bool
ResourceRequest::compare(const ResourceRequest& other) const
{
    const WebCore::ResourceRequest& a_parent = this->priv().webcore();
    const WebCore::ResourceRequest& b_parent = other.priv().webcore();

    return a_parent == b_parent;// WebCore::ResourceRequestBase::operator==
}

bool 
ResourceRequest::isMainResource() const
{
    return m_private.isMainResource();
}

} // namespace
