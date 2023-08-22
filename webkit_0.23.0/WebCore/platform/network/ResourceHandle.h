/*
 * Copyright (C) 2004, 2006, 2011, 2013 Apple Inc. All rights reserved.
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ResourceHandle_h
#define ResourceHandle_h

#include "AuthenticationClient.h"
#include "HTTPHeaderMap.h"
#include "ResourceHandleTypes.h"
#include "ResourceLoadPriority.h"
#include <wtf/RefCounted.h>

#if PLATFORM(COCOA) || USE(CFNETWORK)
#include <wtf/RetainPtr.h>
#endif

#if USE(QUICK_LOOK)
#include "QuickLook.h"
#endif

#if USE(SOUP)
typedef struct _GTlsCertificate GTlsCertificate;
typedef struct _SoupSession SoupSession;
typedef struct _SoupRequest SoupRequest;
#endif

#if USE(CF)
typedef const struct __CFData * CFDataRef;
#endif

#if PLATFORM(COCOA)
OBJC_CLASS NSCachedURLResponse;
OBJC_CLASS NSData;
OBJC_CLASS NSDictionary;
OBJC_CLASS NSError;
OBJC_CLASS NSURLConnection;
#ifndef __OBJC__
typedef struct objc_object *id;
#endif
#endif

#if USE(CFNETWORK)
typedef const struct _CFCachedURLResponse* CFCachedURLResponseRef;
typedef struct _CFURLConnection* CFURLConnectionRef;
typedef int CFHTTPCookieStorageAcceptPolicy;
typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
#endif

#if PLATFORM(COCOA) || USE(CFNETWORK)
typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
#endif

namespace WTF {
class SchedulePair;
}

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class Frame;
#if PLATFORM(WKC)
class FrameLoaderClient;
#endif
class URL;
class NetworkingContext;
class ProtectionSpace;
class ResourceError;
class ResourceHandleClient;
class ResourceHandleInternal;
class ResourceLoadTiming;
class ResourceRequest;
class ResourceResponse;
class SharedBuffer;
class Timer;

class ResourceHandle : public RefCounted<ResourceHandle>
#if PLATFORM(COCOA) || USE(CFNETWORK) || USE(CURL) || USE(SOUP)
    , public AuthenticationClient
#endif
    {
public:
    WEBCORE_EXPORT static PassRefPtr<ResourceHandle> create(NetworkingContext*, const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff);
    WEBCORE_EXPORT static void loadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentials, ResourceError&, ResourceResponse&, Vector<char>& data);

    WEBCORE_EXPORT virtual ~ResourceHandle();

#if PLATFORM(COCOA) || USE(CFNETWORK)
    void willSendRequest(ResourceRequest&, const ResourceResponse& redirectResponse);
#endif

#if PLATFORM(COCOA) || USE(CFNETWORK) || USE(CURL) || USE(SOUP)
    bool shouldUseCredentialStorage();
    void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
    virtual void receivedCredential(const AuthenticationChallenge&, const Credential&) override;
    virtual void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&) override;
    virtual void receivedCancellation(const AuthenticationChallenge&) override;
    virtual void receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&) override;
    virtual void receivedChallengeRejection(const AuthenticationChallenge&) override;
#endif

#if PLATFORM(COCOA) || USE(CFNETWORK)
    bool tryHandlePasswordBasedAuthentication(const AuthenticationChallenge&);
#endif

#if PLATFORM(COCOA) && USE(PROTECTION_SPACE_AUTH_CALLBACK)
    bool canAuthenticateAgainstProtectionSpace(const ProtectionSpace&);
#endif

#if PLATFORM(COCOA) && !USE(CFNETWORK)
    void didCancelAuthenticationChallenge(const AuthenticationChallenge&);
    WEBCORE_EXPORT NSURLConnection *connection() const;
    id makeDelegate(bool);
    id delegate();
    void releaseDelegate();
#endif
        
#if PLATFORM(COCOA) && ENABLE(WEB_TIMING)
    void setCollectsTimingData();
#if USE(CFNETWORK)
    static void getConnectionTimingData(CFURLConnectionRef, ResourceLoadTiming&);
#else
    static void getConnectionTimingData(NSURLConnection *, ResourceLoadTiming&);
#endif
#endif
        
#if PLATFORM(COCOA)
    void schedule(WTF::SchedulePair&);
    void unschedule(WTF::SchedulePair&);
#endif

#if USE(CFNETWORK)
    CFURLStorageSessionRef storageSession() const;
    CFURLConnectionRef connection() const;
    WEBCORE_EXPORT RetainPtr<CFURLConnectionRef> releaseConnectionForDownload();
    const ResourceRequest& currentRequest() const;
    static void setHostAllowsAnyHTTPSCertificate(const String&);
    static void setClientCertificate(const String& host, CFDataRef);
#endif

#if USE(QUICK_LOOK)
    QuickLookHandle* quickLookHandle() { return m_quickLook.get(); }
    void setQuickLookHandle(std::unique_ptr<QuickLookHandle> handle) { m_quickLook = WTF::move(handle); }
#endif

#if PLATFORM(WIN) && USE(CURL)
    static void setHostAllowsAnyHTTPSCertificate(const String&);
    static void setClientCertificateInfo(const String&, const String&, const String&);
#endif

#if PLATFORM(WIN) && USE(CURL) && USE(CF)
    static void setClientCertificate(const String& host, CFDataRef);
#endif

    bool shouldContentSniff() const;
    static bool shouldContentSniffURL(const URL&);

    WEBCORE_EXPORT static void forceContentSniffing();

#if USE(CURL) || USE(SOUP)
    ResourceHandleInternal* getInternal() { return d.get(); }
#endif

#if USE(SOUP)
    RefPtr<ResourceHandle> releaseForDownload(ResourceHandleClient*);
    void continueDidReceiveAuthenticationChallenge(const Credential& credentialFromPersistentStorage);
    void sendPendingRequest();
    bool cancelledOrClientless();
    void ensureReadBuffer();
    size_t currentStreamPosition() const;
    void didStartRequest();
    static void setHostAllowsAnyHTTPSCertificate(const String&);
    static void setClientCertificate(const String& host, GTlsCertificate*);
    static void setIgnoreSSLErrors(bool);
    double m_requestTime;
#endif

    bool hasAuthenticationChallenge() const;
    void clearAuthentication();
    WEBCORE_EXPORT virtual void cancel();

    // The client may be 0, in which case no callbacks will be made.
    WEBCORE_EXPORT ResourceHandleClient* client() const;
    WEBCORE_EXPORT void setClient(ResourceHandleClient*);

    // Called in response to ResourceHandleClient::willSendRequestAsync().
    WEBCORE_EXPORT void continueWillSendRequest(const ResourceRequest&);

    // Called in response to ResourceHandleClient::didReceiveResponseAsync().
    WEBCORE_EXPORT virtual void continueDidReceiveResponse();

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    // Called in response to ResourceHandleClient::canAuthenticateAgainstProtectionSpaceAsync().
    WEBCORE_EXPORT void continueCanAuthenticateAgainstProtectionSpace(bool);
#endif

    // Called in response to ResourceHandleClient::willCacheResponseAsync().
#if USE(CFNETWORK)
    WEBCORE_EXPORT void continueWillCacheResponse(CFCachedURLResponseRef);
#endif
#if PLATFORM(COCOA) && !USE(CFNETWORK)
    WEBCORE_EXPORT void continueWillCacheResponse(NSCachedURLResponse *);
#endif

    WEBCORE_EXPORT void setDefersLoading(bool);

    WEBCORE_EXPORT ResourceRequest& firstRequest();
    const String& lastHTTPMethod() const;

    void failureTimerFired();

    NetworkingContext* context() const;

    using RefCounted<ResourceHandle>::ref;
    using RefCounted<ResourceHandle>::deref;

#if PLATFORM(COCOA) || USE(CFNETWORK)
    WEBCORE_EXPORT static CFStringRef synchronousLoadRunLoopMode();
#endif

#if PLATFORM(IOS) && USE(CFNETWORK)
    static CFMutableDictionaryRef createSSLPropertiesFromNSURLRequest(const ResourceRequest&);
#endif

#if PLATFORM(WKC)
    static void resetVariables();
    static void cancelByFrame(Frame* frame);
    void setDataSchemeDownloading(bool downloading);
    bool dataSchemeDownloading();
    void setFrame(Frame* frame) { m_frame = frame; }
    void setMainFrame(Frame* mainFrame, FrameLoaderClient* client) { m_mainFrame = mainFrame; m_frameloaderclinet = client; }
    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an onUnload handler, so let's just block it.
    const Frame* frame(void) { return (const Frame*)m_frame; }
    const Frame* mainFrame(void) { return (const Frame*)m_mainFrame; }
    const FrameLoaderClient* frameloaderclient(void) { return (const FrameLoaderClient*)m_frameloaderclinet; }
#endif

    typedef PassRefPtr<ResourceHandle> (*BuiltinConstructor)(const ResourceRequest& request, ResourceHandleClient* client);
    static void registerBuiltinConstructor(const AtomicString& protocol, BuiltinConstructor);

    typedef void (*BuiltinSynchronousLoader)(NetworkingContext*, const ResourceRequest&, StoredCredentials, ResourceError&, ResourceResponse&, Vector<char>& data);
    static void registerBuiltinSynchronousLoader(const AtomicString& protocol, BuiltinSynchronousLoader);

protected:
    ResourceHandle(NetworkingContext*, const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff);

private:
    enum FailureType {
        NoFailure,
        BlockedFailure,
        InvalidURLFailure
    };

    void platformSetDefersLoading(bool);

    void scheduleFailure(FailureType);

    bool start();
    static void platformLoadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentials, ResourceError&, ResourceResponse&, Vector<char>& data);

    virtual void refAuthenticationClient() override { ref(); }
    virtual void derefAuthenticationClient() override { deref(); }

#if PLATFORM(COCOA) || USE(CFNETWORK)
    enum class SchedulingBehavior { Asynchronous, Synchronous };
#endif

#if USE(CFNETWORK)
    void createCFURLConnection(bool shouldUseCredentialStorage, bool shouldContentSniff, SchedulingBehavior, CFDictionaryRef clientProperties);
#endif

#if PLATFORM(MAC) && !USE(CFNETWORK)
    void createNSURLConnection(id delegate, bool shouldUseCredentialStorage, bool shouldContentSniff, SchedulingBehavior);
#endif

#if PLATFORM(IOS) && !USE(CFNETWORK)
    void createNSURLConnection(id delegate, bool shouldUseCredentialStorage, bool shouldContentSniff, SchedulingBehavior, NSDictionary *connectionProperties);
#endif

#if PLATFORM(COCOA) && ENABLE(WEB_TIMING)
static void getConnectionTimingData(NSDictionary *timingData, ResourceLoadTiming&);
#endif

    friend class ResourceHandleInternal;
    std::unique_ptr<ResourceHandleInternal> d;

#if USE(QUICK_LOOK)
    std::unique_ptr<QuickLookHandle> m_quickLook;
#endif

#if PLATFORM(WKC)
    Frame *m_frame;  // Pages current Frame
    Frame *m_mainFrame;  // Pages mainFrame
    FrameLoaderClient *m_frameloaderclinet; // FrameLoaderClientWKC of mainFrame
#endif
};

}

#endif // ResourceHandle_h
