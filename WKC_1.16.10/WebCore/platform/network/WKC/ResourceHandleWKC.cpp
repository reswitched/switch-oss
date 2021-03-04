/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005, 2006 Michael Emmel mike.emmel@gmail.com
 * All rights reserved.
 * Copyright (c) 2010-2020 ACCESS CO., LTD. All rights reserved.
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

#include "CredentialStorage.h"
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
#include "SynchronousLoaderClient.h"

#include "HTTPParsers.h"
#include <wtf/text/Base64.h>

#if PLATFORM(WIN) && PLATFORM(CF)
#include <wtf/RetainPtr.h>
#endif

#include <openssl/ssl.h>
#include <openssl/x509v3.h>

namespace WebCore {

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

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* ctx, const ResourceRequest& request, StoredCredentialsPolicy storedCredentialsPolicy, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    Frame* frame = reinterpret_cast<WKC::FrameNetworkingContextWKC *>(ctx)->coreFrame();
    if (!frame)
        return;

    SynchronousLoaderClient client;
    client.setAllowStoredCredentials(storedCredentialsPolicy == StoredCredentialsPolicy::Use);

    bool defersLoading = false;
    bool shouldContentSniff = true;
    bool shouldContentEncodingSniff = true;
    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(ctx, request, &client, defersLoading, shouldContentSniff, shouldContentEncodingSniff));

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

    error = client.error();
    data.swap(client.mutableData());
    response = client.response();
}

//
// Authenticate server
//
bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge) 
{
    if (!challenge.protectionSpace().isPasswordBased())
        return;

    d->m_currentWebChallenge = challenge;

    // Proxy authentication does not use the user:pass embeded in a URL.
    if (challenge.failureResponse().httpStatusCode() != 407) {
        if (!d->m_user.isEmpty() && !d->m_pass.isEmpty()) {
            Credential credential(d->m_user, d->m_pass, CredentialPersistenceNone);

            receivedCredential(challenge, credential);
            clearAuthentication();

            d->m_user = String();
            d->m_pass = String();
            // FIXME: Per the specification, the user shouldn't be asked for credentials if there were incorrect ones provided explicitly.
            return;
        }
    } else {
        ProtectionSpaceAuthenticationScheme authScheme = ResourceHandleManager::sharedInstance()->proxyAuthScheme();
        if (authScheme != challenge.protectionSpace().authenticationScheme()) {
            // Update proxy authentication scheme and retry.
            ResourceHandleManager::sharedInstance()->setProxyAuthScheme(challenge.protectionSpace().authenticationScheme());
            receivedCredential(challenge, ResourceHandleManager::sharedInstance()->proxyCredential());
            clearAuthentication();
            return;
        }
    }

    if (shouldUseCredentialStorage()) {
        String partition = firstRequest().cachePartition();
        Credential credential = CredentialStorage::defaultCredentialStorage().get(partition, challenge.protectionSpace());
        if (!credential.isEmpty()) {
            if (!d->m_initialCredential.isEmpty() && credential == d->m_initialCredential) {
                // The stored credential wasn't accepted, stop using it.
                // There is a race condition here, since a different credential might have already been stored by another ResourceHandle,
                // but the observable effect should be very minor, if any.
                CredentialStorage::defaultCredentialStorage().remove(partition, challenge.protectionSpace());
            } else {
                receivedCredential(challenge, credential);
                clearAuthentication();
                return;
            }
        }
    }

    if (d->client())
        d->client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);

    clearAuthentication();
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    ASSERT(!challenge.isNull());

    if (challenge != d->m_currentWebChallenge)
        return;

    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    String partition = firstRequest().cachePartition();

    if (shouldUseCredentialStorage()) {
        if (challenge.failureResponse().httpStatusCode() == 401) {
            URL urlToStore = challenge.failureResponse().url();
            CredentialStorage::defaultCredentialStorage().set(partition, credential, challenge.protectionSpace(), urlToStore);
        }
    }

    if (challenge.failureResponse().httpStatusCode() == 407)
        ResourceHandleManager::sharedInstance()->setProxyCredential(credential);
    else {
        d->m_initialCredential = credential;
        d->m_authScheme = challenge.protectionSpace().authenticationScheme();
    }

    d->m_doAuthChallenge = true;
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    // FIXME
    d->m_doAuthChallenge = false;

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    d->m_cancelled = true;
    d->m_doAuthChallenge = false;

    clearAuthentication();
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
