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

#ifndef _WKC_HELPERS_PRIVATE_WKCDOCUMENTLOADER_H_
#define _WKC_HELPERS_PRIVATE_WKCDOCUMENTLOADER_H_

#include "helpers/WKCDocumentLoader.h"
#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"

namespace WebCore {
class DocumentLoader;
}

namespace  WKC {

class DocumentLoader;
class ResourceLoaderPrivate;
class ResourceRequestPrivate;
class ResourceResponsePrivate;

class DocumentLoaderWrap : public DocumentLoader {
WTF_MAKE_FAST_ALLOCATED;
public:
    DocumentLoaderWrap(DocumentLoaderPrivate& parent) : DocumentLoader(parent) {}
    ~DocumentLoaderWrap() {}
};

class DocumentLoaderPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    DocumentLoaderPrivate(WebCore::DocumentLoader*);
    ~DocumentLoaderPrivate();

    const WebCore::DocumentLoader* webcore() const { return m_webcore; }
    DocumentLoader& wkc() { return m_wkc; }

    bool isLoadingMainResource() const;
    void clearMainResourceData();

    bool isLoadingSubresources() const;

    const String& responseMIMEType();

    ResourceRequest& request();
    const ResourceResponse& response();

    KURL urlForHistory();

    const KURL& url();
    void replaceRequestURLForSameDocumentNavigation(const KURL&);
    ResourceLoader* mainResourceLoader();

private:
    WebCore::DocumentLoader* m_webcore;
    DocumentLoaderWrap m_wkc;

    ResourceRequestPrivate* m_request;
    ResourceResponsePrivate* m_response;
    KURL m_url;
    KURL m_urlForHistory;
    String m_responseMIMEType;
    ResourceLoaderPrivate* m_mainResourceLoader;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_WKCDOCUMENTLOADER_H_
