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

#ifndef _WKC_HELPERS_PRIVATE_RESOURCERESPONSE_H_
#define _WKC_HELPERS_PRIVATE_RESOURCERESPONSE_H_

#include "helpers/WKCResourceResponse.h"
#include "helpers/WKCString.h"

namespace WebCore {
class ResourceResponse;
} // namespace

namespace WKC {
class ResourceHandlePrivate;
class KURL;
class String;

class ResourceResponseWrap : public ResourceResponse {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceResponseWrap(ResourceResponsePrivate& parent) : ResourceResponse(parent) {}
    ~ResourceResponseWrap() {}
};

class ResourceResponsePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceResponsePrivate(const WebCore::ResourceResponse&);
    ~ResourceResponsePrivate();

    const WebCore::ResourceResponse& webcore() const { return m_webcore; }
    const ResourceResponse& wkc() const { return m_wkc; }

    const KURL url() const;
    const String mimeType() const;

    bool isAttachment() const;
    bool isNull() const;
    int httpStatusCode() const;
    long long expectedContentLength() const;
    const String& httpStatusText();
    const String httpHeaderField(const char* name) const;
    bool wasCached() const;

    ResourceHandle* resourceHandle();

private:
    ResourceResponsePrivate(const ResourceResponsePrivate&);
    ResourceResponsePrivate& operator=(const ResourceResponsePrivate&);

    const WebCore::ResourceResponse& m_webcore;
    const ResourceResponseWrap m_wkc;

    ResourceHandlePrivate* m_resourceHandle;
    String m_httpStatusText;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_RESOURCERESPONSE_H_
