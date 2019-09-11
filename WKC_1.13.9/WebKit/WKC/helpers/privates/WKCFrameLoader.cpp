/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCFrameLoader.h"
#include "helpers/privates/WKCFrameLoaderPrivate.h"

#include "FrameLoader.h"
#include "DocumentLoader.h"
#include "MIMETypeRegistry.h"
#include "PolicyChecker.h"
#include "ResourceRequest.h"
#include "helpers/privates/WKCDocumentLoaderPrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCPolicyCheckerPrivate.h"
#include "helpers/privates/WKCResourceRequestPrivate.h"

namespace WKC {
FrameLoaderPrivate::FrameLoaderPrivate(WebCore::FrameLoader* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_documentLoader(0)
    , m_activeDocumentLoader(0)
    , m_provisionalDocumentLoader(0)
    , m_originalRequest(0)
    , m_policyChecker(0)
{
}

FrameLoaderPrivate::~FrameLoaderPrivate()
{
    delete m_documentLoader;
    delete m_activeDocumentLoader;
    delete m_provisionalDocumentLoader;
    delete m_originalRequest;
    delete m_policyChecker;
}

DocumentLoader*
FrameLoaderPrivate::documentLoader()
{
    WebCore::DocumentLoader* loader = m_webcore->documentLoader();
    if (!loader) return 0;

    if (!m_documentLoader || m_documentLoader->webcore() != loader) {
        delete m_documentLoader;
        m_documentLoader = new DocumentLoaderPrivate(loader);
    }

    return &m_documentLoader->wkc();
}

DocumentLoader*
FrameLoaderPrivate::activeDocumentLoader()
{
    WebCore::DocumentLoader* loader = m_webcore->activeDocumentLoader();
    if (!loader) return 0;

    if (!m_activeDocumentLoader || m_activeDocumentLoader->webcore() != loader) {
        delete m_activeDocumentLoader;
        m_activeDocumentLoader = new DocumentLoaderPrivate(loader);
    }

    return &m_activeDocumentLoader->wkc();
}

DocumentLoader*
FrameLoaderPrivate::provisionalDocumentLoader()
{
    WebCore::DocumentLoader* loader = m_webcore->provisionalDocumentLoader();
    if (!loader)
        return 0;

    if (!m_provisionalDocumentLoader || m_provisionalDocumentLoader->webcore() != loader) {
        delete m_provisionalDocumentLoader;
        m_provisionalDocumentLoader = new DocumentLoaderPrivate(loader);
    }

    return &m_provisionalDocumentLoader->wkc();
}

ObjectContentType
FrameLoaderPrivate::defaultObjectContentType(const KURL& url, const String& mimeTypeIn)
{
    WTF::String mimeType = mimeTypeIn;

    if (mimeType.isEmpty()) {
        WTF::String decodedPath = decodeURLEscapeSequences(url.path());
        WTF::String extension = decodedPath.substring(decodedPath.reverseFind('.') + 1);

        // We don't use MIMETypeRegistry::getMIMETypeForPath() because it returns "application/octet-stream" upon failure
        mimeType = WebCore::MIMETypeRegistry::getMIMETypeForExtension(extension);
    }

#if ENABLE(NETSCAPE_PLUGIN_API) && !PLATFORM(COCOA) && !PLATFORM(EFL) // Mac has no PluginDatabase, nor does EFL
    if (mimeType.isEmpty()) {
        WTF::String decodedPath = WebCore::decodeURLEscapeSequences(url.path());
        mimeType = PluginDatabase::installedPlugins()->MIMETypeForExtension(decodedPath.substring(decodedPath.reverseFind('.') + 1));
    }
#endif

    if (mimeType.isEmpty())
        return ObjectContentFrame; // Go ahead and hope that we can display the content.

#if ENABLE(NETSCAPE_PLUGIN_API) && !PLATFORM(COCOA) && !PLATFORM(EFL) // Mac has no PluginDatabase, nor does EFL
    bool plugInSupportsMIMEType = PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType);
#else
    bool plugInSupportsMIMEType = false;
#endif

    if (WebCore::MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return ObjectContentImage;

    if (plugInSupportsMIMEType)
        return ObjectContentPlugin;

    if (WebCore::MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentFrame;

    return ObjectContentNone;
}

const ResourceRequest&
FrameLoaderPrivate::originalRequest()
{
    const WebCore::ResourceRequest& req = m_webcore->originalRequest();

    delete m_originalRequest;
    m_originalRequest = new ResourceRequestPrivate(req);

    return m_originalRequest->wkc();
}

FrameLoadType
FrameLoaderPrivate::loadType() const
{
    return toWKCFrameLoadType(m_webcore->loadType());
}

bool
FrameLoaderPrivate::isLoading() const
{
    return m_webcore->isLoading();
}

PolicyChecker*
FrameLoaderPrivate::policyChecker()
{
    WebCore::PolicyChecker* policyChecker = &m_webcore->policyChecker();

    if (!m_policyChecker || m_policyChecker->webcore() != policyChecker) {
        delete m_policyChecker;
        m_policyChecker = new PolicyCheckerPrivate(policyChecker);
    }

    return &m_policyChecker->wkc();
}

////////////////////////////////////////////////////////////////////////////////

FrameLoader::FrameLoader(FrameLoaderPrivate& parent)
    : m_private(parent)
{
}

FrameLoader::~FrameLoader()
{
}

DocumentLoader*
FrameLoader::documentLoader()
{
    return m_private.documentLoader();
}

DocumentLoader*
FrameLoader::activeDocumentLoader()
{
    return m_private.activeDocumentLoader();
}

DocumentLoader*
FrameLoader::provisionalDocumentLoader()
{
    return m_private.provisionalDocumentLoader();
}

ObjectContentType
FrameLoader::defaultObjectContentType(const KURL& url, const String& mimeType)
{
    return FrameLoaderPrivate::defaultObjectContentType(url, mimeType);
}

const ResourceRequest&
FrameLoader::originalRequest() const
{
    return m_private.originalRequest();
}

FrameLoadType
FrameLoader::loadType() const
{
    return m_private.loadType();
}

bool
FrameLoader::isLoading() const
{
    return m_private.isLoading();
}

PolicyChecker*
FrameLoader::policyChecker() const
{
    return m_private.policyChecker();
}

} // namespace
