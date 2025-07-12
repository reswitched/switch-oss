/*
 *  Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCDocLoader.h"
#include "helpers/privates/WKCDocLoaderPrivate.h"

#include "DocLoader.h"
#include "helpers/privates/WKCCachedResourcePrivate.h"


namespace WKC {

DocLoaderPrivate::DocLoaderPrivate(WebCore::DocLoader* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_cachedResource(0)
{
}

DocLoaderPrivate::~DocLoaderPrivate()
{
    delete m_cachedResource;
}

CachedResource*
DocLoaderPrivate::cachedResource(const String& str)
{
    WebCore::CachedResource* res = m_webcore->cachedResource(str);
    if (!res)
        return 0;
    if (!m_cachedResource || m_cachedResource->webcore()!=res) {
        delete m_cachedResource;
        m_cachedResource = new CachedResourcePrivate(res);
    }
    return &m_cachedResource->wkc();
}

////////////////////////////////////////////////////////////////////////////////

DocLoader::DocLoader(DocLoaderPrivate& parent)
    : m_private(parent)
{
}

DocLoader::~DocLoader()
{
}

CachedResource*
DocLoader::cachedResource(const String& str) const
{
    return m_private.cachedResource(str);
}
    
} // namespace

