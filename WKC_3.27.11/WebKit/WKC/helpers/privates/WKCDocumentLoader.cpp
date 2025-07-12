/*
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

#include "helpers/WKCDocumentLoader.h"
#include "helpers/privates/WKCDocumentLoaderPrivate.h"

#include "DocumentLoader.h"
#include "helpers/WKCDocumentLoader.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCResourceLoaderPrivate.h"
#include "helpers/privates/WKCResourceRequestPrivate.h"
#include "helpers/privates/WKCResourceResponsePrivate.h"

namespace WKC {
DocumentLoaderPrivate::DocumentLoaderPrivate(WebCore::DocumentLoader* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_request(nullptr)
    , m_response(nullptr)
    , m_originalRequest(nullptr)
    , m_url()
    , m_urlForHistory()
    , m_responseMIMEType()
    , m_mainResourceLoader(nullptr)
{
}

DocumentLoaderPrivate::~DocumentLoaderPrivate()
{
    delete m_request;
    delete m_response;
    delete m_originalRequest;
    delete m_mainResourceLoader;
}

bool
DocumentLoaderPrivate::isLoadingMainResource() const
{
    return m_webcore->isLoadingMainResource();
}

void
DocumentLoaderPrivate::clearMainResourceData()
{
    m_webcore->clearMainResourceData();
}

bool
DocumentLoaderPrivate::isLoadingSubresources() const
{
    return !m_webcore->isLoadingMainResource();
}

const String&
DocumentLoaderPrivate::responseMIMEType()
{
    m_responseMIMEType = m_webcore->responseMIMEType();
    return m_responseMIMEType;
}

const KURL&
DocumentLoaderPrivate::url()
{
    m_url = m_webcore->url();
    return m_url;
}

const ResourceResponse&
DocumentLoaderPrivate::response()
{
    const WebCore::ResourceResponse& rsp = m_webcore->response();
    if (!m_response || &m_response->webcore() != &rsp) {
        delete m_response;
        m_response = new ResourceResponsePrivate(rsp);
    }
    return m_response->wkc();
}

ResourceRequest&
DocumentLoaderPrivate::request()
{
    WebCore::ResourceRequest& req = m_webcore->request();
    if (!m_request || req != m_request->webcore()) {
        delete m_request;
        m_request = new ResourceRequestPrivate(req);
    }
    return m_request->wkc();
}

const ResourceRequest&
DocumentLoaderPrivate::originalRequest()
{
    const WebCore::ResourceRequest& req = m_webcore->originalRequest();
    if (!m_originalRequest || !(req == m_originalRequest->webcore())) {
        delete m_originalRequest;
        m_originalRequest = new ResourceRequestPrivate(req);
    }
    return m_originalRequest->wkc();
}

KURL
DocumentLoaderPrivate::urlForHistory()
{
	m_urlForHistory = m_webcore->urlForHistory();
	return m_urlForHistory;
}

void
DocumentLoaderPrivate::replaceRequestURLForSameDocumentNavigation(const KURL& url)
{
    m_webcore->replaceRequestURLForSameDocumentNavigation(url);
}

ResourceLoader*
DocumentLoaderPrivate::mainResourceLoader()
{
    WebCore::ResourceLoader *loader = (WebCore::ResourceLoader *)webcore()->mainResourceLoader();
    if (!m_mainResourceLoader || loader != m_mainResourceLoader->webcore()) {
        delete m_mainResourceLoader;
        m_mainResourceLoader = new ResourceLoaderPrivate(loader);
    }

    return &m_mainResourceLoader->wkc();
}

WKC::MediaSourcePolicy
DocumentLoaderPrivate::mediaSourcePolicy() const
{
    return toWKCMediaSourcePolicy(m_webcore->mediaSourcePolicy());
}

void
DocumentLoaderPrivate::setMediaSourcePolicy(WKC::MediaSourcePolicy policy)
{
    m_webcore->setMediaSourcePolicy(toWebCoreMediaSourcePolicy(policy));
}

void
DocumentLoaderPrivate::applyPoliciesToSettings()
{
    m_webcore->applyPoliciesToSettings();
}

////////////////////////////////////////////////////////////////////////////////

DocumentLoader::DocumentLoader(DocumentLoaderPrivate& parent)
    : m_private(parent)
{
}

DocumentLoader::~DocumentLoader()
{
}

bool
DocumentLoader::isLoadingMainResource() const
{
    return m_private.isLoadingMainResource();
}

void
DocumentLoader::clearMainResourceData()
{
    m_private.clearMainResourceData();
}

bool
DocumentLoader::isLoadingSubresources() const
{
    return m_private.isLoadingSubresources();
}

const String&
DocumentLoader::responseMIMEType() const
{
    return m_private.responseMIMEType();
}

const KURL&
DocumentLoader::url() const
{
    return m_private.url();
}


const ResourceResponse&
DocumentLoader::response() const
{
    return m_private.response();
}

const ResourceRequest&
DocumentLoader::request() const
{
    return m_private.request();
}

ResourceRequest&
DocumentLoader::request()
{
    return m_private.request();
}

const ResourceRequest&
DocumentLoader::originalRequest() const
{
    return m_private.originalRequest();
}

KURL
DocumentLoader::urlForHistory() const
{
	return m_private.urlForHistory();
}

void
DocumentLoader::replaceRequestURLForSameDocumentNavigation(const KURL& url)
{
    m_private.replaceRequestURLForSameDocumentNavigation(url);
}

ResourceLoader*
DocumentLoader::mainResourceLoader() const
{
    return m_private.mainResourceLoader();
}

WKC::MediaSourcePolicy
DocumentLoader::mediaSourcePolicy() const
{
    return m_private.mediaSourcePolicy();
}

void
DocumentLoader::setMediaSourcePolicy(WKC::MediaSourcePolicy policy)
{
    m_private.setMediaSourcePolicy(policy);
}

void
DocumentLoader::applyPoliciesToSettings()
{
    m_private.applyPoliciesToSettings();
}

} // namespace
