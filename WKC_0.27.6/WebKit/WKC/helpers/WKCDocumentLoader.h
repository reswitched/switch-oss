/*
 * Copyright (c) 2011, 2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_DOCUMENTLOADER_H_
#define _WKC_HELPERS_WKC_DOCUMENTLOADER_H_

#include <wkc/wkcbase.h>

namespace WKC {
class DocumentLoaderPrivate;
class String;
class ResourceRequest;
class ResourceResponse;
class KURL;
class ResourceLoader;

class WKC_API DocumentLoader {
public:
    bool isLoadingMainResource() const;
    void clearMainResourceData();

    bool isLoadingSubresources() const;

    const String& responseMIMEType() const;

    const ResourceRequest& request() const;
    ResourceRequest& request();
    const ResourceResponse& response() const;

    KURL urlForHistory() const;

    const KURL& url() const;
    void replaceRequestURLForSameDocumentNavigation(const KURL&);
    ResourceLoader* mainResourceLoader() const;

    DocumentLoaderPrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    DocumentLoader(DocumentLoaderPrivate&);
    ~DocumentLoader();

private:
    DocumentLoader(const DocumentLoader&);
    DocumentLoader& operator=(const DocumentLoader&);

    DocumentLoaderPrivate& m_private;
};
}

#endif // _WKC_HELPERS_WKC_DOCUMENTLOADER_H_
