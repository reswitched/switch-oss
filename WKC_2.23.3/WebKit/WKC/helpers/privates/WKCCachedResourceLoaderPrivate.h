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

#ifndef _WKC_HELPERS_PRIVATE_CACHEDRESOURCELOADER_H_
#define _WKC_HELPERS_PRIVATE_CACHEDRESOURCELOADER_H_

#include "helpers/WKCCachedResourceLoader.h"

namespace WebCore {
class CachedResourceLoader;
} // namespace

namespace WKC {

class CachedResource;
class CachedResourcePrivate;

class CachedResourceLoaderWrap : public CachedResourceLoader {
WTF_MAKE_FAST_ALLOCATED;
public:
    CachedResourceLoaderWrap(CachedResourceLoaderPrivate& parent) : CachedResourceLoader(parent) {}
    ~CachedResourceLoaderWrap() {}
};

class CachedResourceLoaderPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    CachedResourceLoaderPrivate(WebCore::CachedResourceLoader*);
    ~CachedResourceLoaderPrivate();

    WebCore::CachedResourceLoader* webcore() const { return m_webcore; }
    CachedResourceLoader& wkc() { return m_wkc; }

    CachedResource* cachedResource(const String&);

private:
    WebCore::CachedResourceLoader* m_webcore;
    CachedResourceLoaderWrap m_wkc;

    CachedResourcePrivate* m_cachedResource;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_CACHEDRESOURCELOADER_H_
