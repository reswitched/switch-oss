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

#ifndef _WKC_HELPERS_PRIVATE_RESOURCELOADER_H_
#define _WKC_HELPERS_PRIVATE_RESOURCELOADER_H_

#include "helpers/WKCResourceLoader.h"

#include "WKCResourceHandlePrivate.h"
#include "WKCResourceResponsePrivate.h"

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {
class ResourceLoader;
} // namespace

namespace WKC {
class DocumentLoaderPrivate;

class ResourceLoaderWrap : public ResourceLoader {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceLoaderWrap(ResourceLoaderPrivate& parent) : ResourceLoader(parent) {}
    ~ResourceLoaderWrap() {}
};

class ResourceLoaderPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceLoaderPrivate(WebCore::ResourceLoader*);
    ~ResourceLoaderPrivate();

    WebCore::ResourceLoader* webcore() const { return m_webcore; }
    ResourceLoader& wkc() { return m_wkc; }

    DocumentLoader* documentLoader();
    void cancel();

    const ResourceResponsePrivate& response() const { return m_response; }
    ResourceHandle* handle();

private:
    WebCore::ResourceLoader* m_webcore;
    ResourceLoaderWrap m_wkc;

    ResourceResponsePrivate m_response;
    DocumentLoaderPrivate* m_documentLoader;
    ResourceHandlePrivate* m_handle;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_RESOURCELOADER_H_
