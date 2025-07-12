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

#ifndef _WKC_HELPERS_PRIVATE_RESOURCEREQUEST_H_
#define _WKC_HELPERS_PRIVATE_RESOURCEREQUEST_H_

#include "helpers/WKCResourceRequest.h"
#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"

namespace WebCore {
class ResourceRequest;
class FormData;
} // namespace

namespace WKC {

class ResourceRequestWrap : public ResourceRequest {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceRequestWrap(ResourceRequestPrivate& parent) : ResourceRequest(parent) {}
    ~ResourceRequestWrap() {}
};

class ResourceRequestPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceRequestPrivate(const WebCore::ResourceRequest& parent);
    ~ResourceRequestPrivate();

    const WebCore::ResourceRequest& webcore() const { return m_webcore; }
    ResourceRequest& wkc() { return m_wkc; }

    const KURL& url();
    void setURL(const KURL&);

    void clearHTTPReferrer();
    void setHTTPHeaderField(const char*, const char*);
    void setHTTPBody(FormData*);
    void setHTTPMethod(const String&);
    String httpHeaderField(const char*) const;
    const String& httpMethod();

    bool isNull() const;

    ResourceRequest::TargetType targetType() const;

    bool isMainResource() const;

private:
    const WebCore::ResourceRequest& m_webcore;
    ResourceRequestWrap m_wkc;

    KURL m_url;
    String m_httpMethod;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_RESOURCEREQUEST_H_
