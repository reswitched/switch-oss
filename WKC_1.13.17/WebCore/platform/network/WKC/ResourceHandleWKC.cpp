/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005, 2006 Michael Emmel mike.emmel@gmail.com
 * All rights reserved.
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ResourceHandle.h"
#include "ResourceLoader.h"

#include "Page.h"
#include "ProgressTracker.h"
#include "FrameLoaderClient.h"

#include "NotImplemented.h"
#include "ResourceHandleInternalWKC.h"
#include "ResourceHandleManagerWKC.h"

#include "DocumentLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderClientWKC.h"
#include "FrameLoader.h"

#include "HTTPParsers.h"
#include <wtf/text/Base64.h>

#if PLATFORM(WIN) && PLATFORM(CF)
#include <wtf/RetainPtr.h>
#endif

#include <openssl/ssl.h>
#include <openssl/x509v3.h>

namespace WebCore {

class WebCoreSynchronousLoader : public ResourceLoader {
public:
    WebCoreSynchronousLoader(Frame&);

    virtual void didReceiveResponse(const ResourceResponse&, CompletionHandler<void()>&& policyCompletionHandler) override;
    virtual void didReceiveData(const char*, unsigned, long long lengthReceived, DataPayloadType) override;
    virtual void didFinishLoading(const NetworkLoadMetrics&) override;
    virtual void didFail(const ResourceError&) override;
    virtual void didReceiveAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge&) override;

    ResourceResponse resourceResponse() const { return m_response; }
    ResourceError resourceError() const { return m_error; }
    Vector<char> data() const { return m_data; }

private:
    virtual void willCancel(const ResourceError&);
    virtual void didCancel(const ResourceError&);

private:
    ResourceResponse m_response;
    ResourceError m_error;
    Vector<char> m_data;
    Frame& m_frame;
    unsigned long m_identifier;
};

WebCoreSynchronousLoader::WebCoreSynchronousLoader(Frame& frame)
    : ResourceLoader(frame, ResourceLoaderOptions(SendCallbackPolicy::SendCallbacks, ContentSniffingPolicy::SniffContent, DataBufferingPolicy::BufferData, StoredCredentialsPolicy::DoNotUse, ClientCredentialPolicy::CannotAskClientForCredentials, FetchOptions::Credentials::Omit, SecurityCheckPolicy::DoSecurityCheck, FetchOptions::Mode::NoCors, CertificateInfoPolicy::IncludeCertificateInfo, ContentSecurityPolicyImposition::DoPolicyCheck, DefersLoadingPolicy::AllowDefersLoading, CachingPolicy::AllowCaching))
    , m_frame(frame)
    , m_identifier(0)
{
}

void WebCoreSynchronousLoader::didReceiveResponse(const ResourceResponse& response, CompletionHandler<void()>&& policyCompletionHandler)
{
    m_response = response;
    policyCompletionHandler();
}

void WebCoreSynchronousLoader::didReceiveData(const char* data, unsigned length, long long encodedLength, DataPayloadType)
{
    m_data.append(data, length);
}

void WebCoreSynchronousLoader::didFinishLoading(const NetworkLoadMetrics&)
{
}

void WebCoreSynchronousLoader::didFail(const ResourceError& error)
{
    m_error = error;
}

void WebCoreSynchronousLoader::didReceiveAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge& challenge)
{
    FrameLoader& frameLoader = m_frame.loader();
    if (!m_identifier) {
        m_identifier = m_frame.page()->progress().createUniqueIdentifier();
    }
    frameLoader.client().dispatchDidReceiveAuthenticationChallenge(frameLoader.documentLoader(), m_identifier, challenge);
}

void WebCoreSynchronousLoader::willCancel(const ResourceError&)
{
    notImplemented();
}

void WebCoreSynchronousLoader::didCancel(const ResourceError&)
{
    notImplemented();
}


/*
static HashSet<String>& allowsAnyHTTPSCertificateHosts()
{
    static HashSet<String> hosts;

    return hosts;
}
*/

ResourceHandleInternal::~ResourceHandleInternal()
{
    if (m_url) {
        fastFree(m_url);
        m_url = 0;
    }
    if (m_urlhost) {
        fastFree(m_urlhost);
        m_urlhost = 0;
    }

    if (m_customHeaders) {
        curl_slist_free_all(m_customHeaders);
        m_customHeaders = 0;
    }

    if (m_curlDataMutex) {
        wkcMutexDeletePeer(m_curlDataMutex);
    }

    if (m_sslCipherVersion) {
        fastFree(m_sslCipherVersion);
        m_sslCipherVersion = 0;
    }

    if (m_sslCipherName) {
        fastFree(m_sslCipherName);
        m_sslCipherName = 0;
    }

    m_client = 0;
}

ResourceHandle::~ResourceHandle()
{
    setFrame(0);
    setMainFrame(0, 0);
    cancel();
}

bool ResourceHandle::start()
{
    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an onUnload handler, so let's just block it.
    NetworkingContext* ctx = context();
    if (!ctx)
        return false;

    setFrame(0);
    setMainFrame(0,0);

    FrameLoaderClient *flc = ctx->frameLoaderClient();
    if (flc && flc->byWKC()) {
        Frame* frame = reinterpret_cast<WKC::FrameNetworkingContextWKC *>(ctx)->coreFrame();
        if (!frame || !frame->page())
            return false;
        setFrame(frame);

        frame = &(frame->page()->mainFrame());
        if (frame && frame->loader().client().byWKC()) {
            setMainFrame(frame, &frame->loader().client());
        }
    }
    ResourceHandleManager::sharedInstance()->add(this);
    return true;
}

void ResourceHandle::cancel()
{
    if (ResourceHandleManager::sharedInstance()) {
        ResourceHandleManager::sharedInstance()->cancel(this);
    }
}

void ResourceHandle::cancelByFrame(Frame* frame)
{
    if (ResourceHandleManager::sharedInstance()) {
        ResourceHandleManager::sharedInstance()->cancelByFrame(frame);
    }
}

#if PLATFORM(WIN) && PLATFORM(CF)
void ResourceHandle::setHostAllowsAnyHTTPSCertificate(const String& host)
{
    allowsAnyHTTPSCertificateHosts().add(host.lower());
}
#endif

#if PLATFORM(WIN) && PLATFORM(CF)
// FIXME:  The CFDataRef will need to be something else when
// building without 
static HashMap<String, RetainPtr<CFDataRef> >& clientCerts()
{
    static HashMap<String, RetainPtr<CFDataRef> > certs;
    return certs;
}

void ResourceHandle::setClientCertificate(const String& host, CFDataRef cert)
{
    clientCerts().set(host.lower(), cert);
}
#endif

void ResourceHandle::setDefersLoading(bool defers)
{
    if (d->m_defersLoading == defers)
        return;

#if LIBCURL_VERSION_NUM > 0x071200
    if (!d->m_handle)
        d->m_defersLoading = defers;
    else if (defers) {
        ResourceHandleManager* manager = ResourceHandleManager::sharedInstance();
        manager->lockThreadMutex();
        CURLcode error = curl_easy_pause(d->m_handle, CURLPAUSE_ALL);
        manager->unlockThreadMutex();
        // If we could not defer the handle, so don't do it.
        if (error != CURLE_OK)
            return;

        d->m_defersLoading = defers;
    } else {
        // We need to set defersLoading before restarting a connection
        // or libcURL will call the callbacks in curl_easy_pause and
        // we would ASSERT.
        d->m_defersLoading = defers;

        ResourceHandleManager* manager = ResourceHandleManager::sharedInstance();
        manager->lockThreadMutex();
        CURLcode error = curl_easy_pause(d->m_handle, CURLPAUSE_CONT);
        manager->unlockThreadMutex();
        if (error != CURLE_OK)
            // Restarting the handle has failed so just cancel it.
            cancel();
    }
#else
    d->m_defersLoading = defers;
    LOG_ERROR("Deferred loading is implemented if libcURL version is above 7.18.0");
#endif
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* ctx, const ResourceRequest& request, StoredCredentialsPolicy storedCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    Frame* frame = reinterpret_cast<WKC::FrameNetworkingContextWKC *>(ctx)->coreFrame();
    if (!frame)
        return;
    RefPtr<WebCoreSynchronousLoader> syncLoader = adoptRef(new WebCoreSynchronousLoader(*frame));
    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(ctx, request, (ResourceHandleClient *)syncLoader.get(), true, false, false));

    handle->setFrame(0);
    handle->setMainFrame(0,0);

    FrameLoaderClient *flc = ctx->frameLoaderClient();
    if (flc && flc->byWKC()) {
        if (frame && frame->page()) {
            handle->setFrame(frame);

            frame = &(frame->page()->mainFrame());
            if (frame && frame->loader().client().byWKC()) {
                handle->setMainFrame(frame, &frame->loader().client());
            }
        }
    }

    ResourceHandleManager* manager = ResourceHandleManager::sharedInstance();
    manager->dispatchSynchronousJob(handle.get());

    error = syncLoader->resourceError();
    data = syncLoader->data();
    response = syncLoader->resourceResponse();
    syncLoader->releaseResources();
}

//
// Authenticate server
//
void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge) 
{
    ResourceHandleInternal* d = getInternal();
    if (!d) return;

    if (d->client())
        d->client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential) 
{
    ASSERT(!challenge.isNull());
    ResourceHandleInternal* d = getInternal();
    if (challenge != d->m_currentWebChallenge)
        return;

    d->m_currentWebChallenge = AuthenticationChallenge(challenge.protectionSpace(),
        credential,
        challenge.previousFailureCount(),
        challenge.failureResponse(),
        challenge.error(),
        this);

    ResourceHandleManager::sharedInstance()->didAuthChallenge(this, challenge.protectionSpace().isProxy());
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    ASSERT(!challenge.isNull());
    ResourceHandleInternal* d = getInternal();

    d->m_currentWebChallenge.nullify();
    if (challenge != d->m_currentWebChallenge)
        return;

    if (challenge.protectionSpace().isProxy())
        d->m_proxyAuthScheme = ProtectionSpaceAuthenticationSchemeUnknown;
    else
        d->m_webAuthScheme = ProtectionSpaceAuthenticationSchemeUnknown;

    ResourceHandleManager::sharedInstance()->cancelAuthChallenge(this, challenge.protectionSpace().isProxy());
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    notImplemented();
}


void ResourceHandle::setDataSchemeDownloading(bool downloading)
{
    d->m_dataSchemeDownloading = downloading;
}

bool ResourceHandle::dataSchemeDownloading()
{
    return d->m_dataSchemeDownloading;
}

void continueWillSendRequest(const ResourceRequest&)
{
    notImplemented();
}

void ResourceHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
}

void ResourceHandle::receivedChallengeRejection(const AuthenticationChallenge&)
{
}

void ResourceHandle::handleDataURL()
{
    if (client())
        return;
    if (getInternal() && getInternal()->m_cancelled) {
        client()->cannotShowURL(this);
        return;
    }

    ASSERT(firstRequest().url().protocolIsData());
    String url = firstRequest().url().string();

    int index = url.find(',');
    if (index == -1) {
        client()->cannotShowURL(this);
        return;
    }

    String mediaType = url.substring(5, index - 5);
    String data = url.substring(index + 1);

    bool base64 = mediaType.endsWithIgnoringASCIICase(";base64");
    if (base64)
        mediaType = mediaType.left(mediaType.length() - 7);

    if (mediaType.isEmpty())
        mediaType = "text/plain";

    String mimeType = extractMIMETypeFromMediaType(mediaType);
    String charset = extractCharsetFromMediaType(mediaType);

    if (charset.isEmpty())
        charset = "US-ASCII";

    ResourceResponse response;
    response.setMimeType(mimeType);
    response.setTextEncodingName(charset);
    response.setURL(firstRequest().url());

    if (client())
        client()->wkcRef();

    if (base64) {
        data = decodeURLEscapeSequences(data);
        if (client())
            client()->didReceiveResponseAsync(this, WTFMove(response), [](){});

        // didReceiveResponse might cause the client to be deleted.
        if (client()) {
            Vector<char> out;
            if (base64Decode(data, out, Base64IgnoreSpacesAndNewLines) && out.size() > 0) {
                response.setExpectedContentLength(out.size());
                client()->didReceiveData(this, out.data(), out.size(), 0);
            }
        }
    }
    else {
        TextEncoding encoding(charset);
        data = decodeURLEscapeSequences(data, encoding);
        if (client())
            client()->didReceiveResponseAsync(this, WTFMove(response), [](){});

        // didReceiveResponse might cause the client to be deleted.
        if (client()) {
            Vector<uint8_t> encodedData = encoding.encode(data, UnencodableHandling::URLEncodedEntities);
            response.setExpectedContentLength(encodedData.size());
            if (encodedData.size()) {
                if (getInternal() && getInternal()->m_cancelled) {
                    client()->wkcDeref();
                    return;
                }
                client()->didReceiveData(this, reinterpret_cast<const char*>(encodedData.data()), encodedData.size(), 0);
            }
        }
    }

    if (client())
        client()->didFinishLoading(this);

    if (client())
        client()->wkcDeref();
}

} // namespace WebCore
