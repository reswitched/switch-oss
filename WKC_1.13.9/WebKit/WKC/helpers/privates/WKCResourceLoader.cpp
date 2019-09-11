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

#include "helpers/WKCResourceLoader.h"
#include "helpers/privates/WKCResourceLoaderPrivate.h"

#include "ResourceHandle.h"
#include "ResourceLoader.h"
#include "helpers/privates/WKCResourceHandlePrivate.h"
#include "helpers/privates/WKCDocumentLoaderPrivate.h"

namespace WKC {
ResourceLoaderPrivate::ResourceLoaderPrivate(WebCore::ResourceLoader* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_response(parent->response())
    , m_documentLoader(0)
    , m_handle(0)
{
}

ResourceLoaderPrivate::~ResourceLoaderPrivate()
{
    delete m_documentLoader;
}


void
ResourceLoaderPrivate::cancel()
{
    m_webcore->cancel();
}


DocumentLoader*
ResourceLoaderPrivate::documentLoader()
{
    if (m_documentLoader)
        delete m_documentLoader;
    m_documentLoader = new DocumentLoaderPrivate(m_webcore->documentLoader());
    return &m_documentLoader->wkc();
}

ResourceHandle* 
ResourceLoaderPrivate::handle()
{
    WebCore::ResourceHandle *handle = m_webcore->handle();
    if (!m_handle || handle != m_handle->webcore())  {
        delete m_handle;
        m_handle = new ResourceHandlePrivate(handle);
    }

    return &m_handle->wkc();
}

////////////////////////////////////////////////////////////////////////////////

ResourceLoader::ResourceLoader(ResourceLoaderPrivate& parent)
    : m_private(parent)
{
}

ResourceLoader::~ResourceLoader()
{
}

DocumentLoader*
ResourceLoader::documentLoader() const
{
    return m_private.documentLoader();
}

void
ResourceLoader::cancel()
{
    m_private.cancel();
}


const ResourceResponse&
ResourceLoader::response() const
{
    return m_private.response().wkc();
}

ResourceHandle* 
ResourceLoader::handle() const 
{
    return m_private.handle(); 
}

} // namespace

