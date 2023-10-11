/*
 * WKCDownload.cpp
 *
 * Copyright (c) 2010-2023 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include "config.h"

#include "WKCDownload.h"
#include "WKCWebView.h"
#include "WKCWebFrame.h"

#include "CString.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "Noncopyable.h"
#include "NotImplemented.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternalWKC.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"

#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCFrameLoaderPrivate.h"
#include "helpers/privates/WKCResourceHandlePrivate.h"
#include "helpers/privates/WKCResourceRequestPrivate.h"
#include "helpers/privates/WKCResourceResponsePrivate.h"

#include "wkc/wkcpeer.h"

namespace WKC {

// private class implementations

class WKCDownloadClientPrivate;

class WKCDownloadPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static WKCDownloadPrivate* create(WKCDownload*, WKCWebView*, WKCDownloadClient&, const ResourceRequest&);
    ~WKCDownloadPrivate();
    void notifyForceTerminate();

    bool setResponse(WebCore::ResourceHandle*, const WebCore::ResourceResponse&);
    bool start();
    void cancel();
    const char* getUri() const;
    const char* getSuggestedFilename() const;

    int getProgressInPercent() const;
    unsigned int getElapsedTimeInMilliSeconds() const;
    long long getTotalSize() const;
    long long getCurrentSize() const;
    inline int getStatus() const { return m_status; };
    int getError() const;

    inline const ResourceRequest& request() { return m_request; };

private:
    WKCDownloadPrivate(WKCDownload*, WKCWebView*, WKCDownloadClient&, const ResourceRequest&);
    bool construct();
    bool construct(WebCore::ResourceHandle*, const WebCore::ResourceResponse&);

    inline WKCDownload* parent() { return m_parent; };
    inline WKCWebView* view() { return m_view; };

    void setStatus(int);

private:
    friend class WKCDownloadClientPrivate;
    void setResponseInfo(const WebCore::ResourceResponse&);
    void notifyReceivedData(const char* data, size_t length);
    void notifyFinishedLoading();
    void notifyErrorInLoading(const WebCore::ResourceError& error);
    bool notifyWillReceiveData(int length);

private:
    WKCDownload* m_parent;
    WKCDownloadClient& m_appclient;
    WKCDownloadClientPrivate* m_client;

    WKCWebView* m_view;
    RefPtr<WebCore::ResourceHandle> m_resourceHandle;
    const ResourceRequest& m_request;

    char* m_uri;
    char* m_suggestedFilename;
    int m_status;
    unsigned int m_startTime;
    long long m_currentLength;
    long long m_contentLength;
    int m_error;

    bool m_createdResourceHandle;
};

class WKCDownloadClientPrivate : public WebCore::ResourceHandleClient {
    WTF_MAKE_NONCOPYABLE(WKCDownloadClientPrivate);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static WKCDownloadClientPrivate* create(WKCDownloadPrivate*);
    ~WKCDownloadClientPrivate();

    // ResourceHandleClient
    virtual void didReceiveData(WebCore::ResourceHandle*, const char*, unsigned, int) override final;
    virtual void didFinishLoading(WebCore::ResourceHandle*) override final;
    virtual void didFail(WebCore::ResourceHandle*, const WebCore::ResourceError&) override final;
    virtual void wasBlocked(WebCore::ResourceHandle*) override final;
    virtual void cannotShowURL(WebCore::ResourceHandle*) override final;

    virtual void willSendRequestAsync(WebCore::ResourceHandle*, WebCore::ResourceRequest&&, WebCore::ResourceResponse&&, WTF::CompletionHandler<void(WebCore::ResourceRequest&&)>&&) override final;
    virtual void didReceiveResponseAsync(WebCore::ResourceHandle*, WebCore::ResourceResponse&&, WTF::CompletionHandler<void()>&&) override final;

private:
    WKCDownloadClientPrivate(WKCDownloadPrivate*);

private:
    WKCDownloadPrivate* m_download;
};


WKCDownloadPrivate::WKCDownloadPrivate(WKCDownload* in_parent, WKCWebView* in_view, WKCDownloadClient& in_client, const ResourceRequest& in_request)
     : m_parent(in_parent)
     , m_appclient(in_client)
     , m_client(0)
     , m_view(in_view)
     , m_resourceHandle(0)
     , m_request(in_request)
     , m_uri(0)
     , m_suggestedFilename(0)
     , m_status(WKCDownload::ECreated)
     , m_startTime(0)
     , m_currentLength(0)
     , m_contentLength(0)
     , m_error(WKCDownload::EErrorNone)
     , m_createdResourceHandle(false)
{
}

WKCDownloadPrivate::~WKCDownloadPrivate()
{
    if (m_resourceHandle) {
        if (m_status==WKCDownload::EStarted) {
            m_resourceHandle->clearClient();
            m_resourceHandle->cancel();
        }
        if (!m_createdResourceHandle || m_resourceHandle->dataSchemeDownloading()) {
            m_resourceHandle->deref();
        }
    }
    delete m_client;

    if (m_uri) {
        wkc_free(m_uri);
    }
    if (m_suggestedFilename) {
        wkc_free(m_suggestedFilename);
    }
}

WKCDownloadPrivate*
WKCDownloadPrivate::create(WKCDownload* in_parent, WKCWebView* in_view, WKCDownloadClient& in_client, const ResourceRequest& in_request)
{
    WKCDownloadPrivate* self = 0;
    self = new WKCDownloadPrivate(in_parent, in_view, in_client, in_request);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
WKCDownloadPrivate::construct()
{
    m_client = WKCDownloadClientPrivate::create(this);
    if (!m_client) return false;

    if (m_request.isNull()) return false;

    URL url = m_request.priv().webcore().url();
    if (url.isEmpty()) return false;

    if (url.lastPathComponent().isEmpty() || url.path().endsWith("/")) {
        m_suggestedFilename = WTF::fastStrDup("download.dat");
    } else {
        m_suggestedFilename = WTF::fastStrDup(url.lastPathComponent().utf8().data());
    }
    return true;
}

void
WKCDownloadPrivate::notifyForceTerminate()
{
    if (m_resourceHandle) {
        if (m_status==WKCDownload::EStarted) {
            m_resourceHandle->clearClient();
            m_resourceHandle->cancel();
        }
    }
}

bool
WKCDownloadPrivate::setResponse(WebCore::ResourceHandle* in_handle, const WebCore::ResourceResponse& in_response)
{
    if (m_resourceHandle) return false;

    m_resourceHandle = in_handle;
    m_resourceHandle->ref();

    // Separate from a frame to avoid the download being cancelled when the frame is deleted.
    // Note that the callbacks of FrameLoaderClient for the download will never be called if you set the m_frameloaderclinet to 0 by setMainFrame(0,0) or the main frame is deleted.
    // You may need to take care to implement ResourceHandleManagerWKC.cpp to call the callbacks for WKCDownloadClientPrivate and other clients based on ResourceHandleClient.
    m_resourceHandle->setFrame(0);
    m_resourceHandle->setMainFrame(0,0);

    setResponseInfo(in_response);
    return true;
}

void
WKCDownloadPrivate::setResponseInfo(const WebCore::ResourceResponse& response)
{
    if (response.isNull()) return;

    if (!response.suggestedFilename().isEmpty()) {
        if (m_suggestedFilename) {
            wkc_free(m_suggestedFilename);
        }
        m_suggestedFilename = WTF::fastStrDup(response.suggestedFilename().utf8().data());
    }
    m_contentLength = response.expectedContentLength();

    if (m_uri) {
        wkc_free(m_uri);
        m_uri = 0;
    }
    if (!response.url().isEmpty()) {
        m_uri = WTF::fastStrDup(response.url().string().utf8().data());
    }
}

void
WKCDownloadPrivate::setStatus(int in_status)
{
    m_status = in_status;
}


bool
WKCDownloadPrivate::start()
{
    if (!m_resourceHandle) {
        WebCore::NetworkingContext* nctx = 0;
        if (m_view && m_view->mainFrame() && m_view->mainFrame()->core()->loader())
            nctx = m_view->mainFrame()->core()->loader()->priv().webcore()->networkingContext();
        if (!nctx)
            return false;
        m_resourceHandle = WebCore::ResourceHandle::create(nctx, m_request.priv().webcore(), m_client, false, false, false);
        if (!m_resourceHandle) return false;
        m_createdResourceHandle = true;
    } else {
        m_resourceHandle->setClient(m_client);
    }

    m_startTime = wkcGetTickCountPeer();
    setStatus(WKCDownload::EStarted);

    return true;
}

void
WKCDownloadPrivate::cancel()
{
    if (m_resourceHandle) {
        m_resourceHandle->cancel();
    }
    setStatus(WKCDownload::ECancelled);
    m_error = WKCDownload::EErrorCancelled;

    m_appclient.didFail(parent(), WKCDownload::EErrorCancelled);
}

const char*
WKCDownloadPrivate::getUri() const
{
    return m_uri;
}

const char*
WKCDownloadPrivate::getSuggestedFilename() const
{
    return m_suggestedFilename;
}

int
WKCDownloadPrivate::getProgressInPercent() const
{
    long long cur = getCurrentSize();
    long long total = getTotalSize();
    return 100 * cur / total;
}

unsigned int
WKCDownloadPrivate::getElapsedTimeInMilliSeconds() const
{
    unsigned int cur = wkcGetTickCountPeer();
    return cur - m_startTime;
}

long long
WKCDownloadPrivate::getTotalSize() const
{
    return std::max(m_currentLength, m_contentLength);
}

long long
WKCDownloadPrivate::getCurrentSize() const
{
    return m_currentLength;
}

int
WKCDownloadPrivate::getError() const
{
    return m_error;
}

void
WKCDownloadPrivate::notifyReceivedData(const char* data, size_t length)
{
    m_currentLength += length;
    m_appclient.didReceiveData(parent(), data, length, m_currentLength);
}

void
WKCDownloadPrivate::notifyFinishedLoading()
{
    m_status = WKCDownload::EFinished;
    m_appclient.didFinishLoading(parent());
}

void
WKCDownloadPrivate::notifyErrorInLoading(const WebCore::ResourceError& error)
{
    m_status = WKCDownload::EError;
    m_error = WKCDownload::EErrorNetwork;
    m_appclient.didFail(parent(), WKCDownload::EErrorNetwork);
}

bool
WKCDownloadPrivate::notifyWillReceiveData(int length)
{
    return m_appclient.willReceiveData(parent(), length);
}

WKCDownloadClientPrivate::WKCDownloadClientPrivate(WKCDownloadPrivate* parent)
    : m_download(parent)
{
}

WKCDownloadClientPrivate::~WKCDownloadClientPrivate()
{
}

WKCDownloadClientPrivate*
WKCDownloadClientPrivate::create(WKCDownloadPrivate* parent)
{
    WKCDownloadClientPrivate* self = 0;
    self = new WKCDownloadClientPrivate(parent);
    if (!self) return 0;
    return self;
}

void
WKCDownloadClientPrivate::didReceiveData(WebCore::ResourceHandle*, const char* data, unsigned length, int /*lengthReceived*/)
{
    m_download->notifyReceivedData(data, length);
}

void
WKCDownloadClientPrivate::didFinishLoading(WebCore::ResourceHandle*)
{
    m_download->notifyFinishedLoading();
}

void
WKCDownloadClientPrivate::didFail(WebCore::ResourceHandle*, const WebCore::ResourceError& error)
{
    m_download->notifyErrorInLoading(error);
}

void
WKCDownloadClientPrivate::wasBlocked(WebCore::ResourceHandle*)
{
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    notImplemented();
}

void
WKCDownloadClientPrivate::cannotShowURL(WebCore::ResourceHandle*)
{
    // FIXME: Implement this when we have the new frame loader signals
    // and error handling.
    notImplemented();
}

void
WKCDownloadClientPrivate::willSendRequestAsync(WebCore::ResourceHandle*, WebCore::ResourceRequest&&, WebCore::ResourceResponse&&, WTF::CompletionHandler<void(WebCore::ResourceRequest&&)>&&)
{
    notImplemented();
}

void
WKCDownloadClientPrivate::didReceiveResponseAsync(WebCore::ResourceHandle*, WebCore::ResourceResponse&& response, WTF::CompletionHandler<void()>&& completion)
{
    m_download->setResponseInfo(response);
    completion();
}

// API class implementations

WKCDownload::WKCDownload()
     : m_private(0)
{
}

WKCDownload::~WKCDownload()
{
    delete m_private;
}

WKCDownload*
WKCDownload::create(WKCWebView* view, WKCDownloadClient& client, const WKC::ResourceRequest& request)
{
    WKCDownload* self = 0;
    self = (WKCDownload *)WTF::fastMalloc(sizeof(WKCDownload));
    self = new (self) WKCDownload();
    if (!self) return 0;
    if (!self->construct(view, client, request)) {
        deleteWKCDownload(self);
        return 0;
    }
    return self;
}

void
WKCDownload::deleteWKCDownload(WKCDownload* self)
{
    if (!self)
        return;
    self->~WKCDownload();
    WTF::fastFree(self);
}

bool
WKCDownload::construct(WKCWebView* view, WKCDownloadClient& client, const ResourceRequest& request)
{
    m_private = WKCDownloadPrivate::create(this, view, client, request);
    if (!m_private) return false;
    return true;
}

void
WKCDownload::notifyForceTerminate()
{
    m_private->notifyForceTerminate();
}

bool
WKCDownload::setResponse(WKC::ResourceHandle* resourceHandle, const WKC::ResourceResponse& response)
{
    WebCore::ResourceHandle* h = resourceHandle ? resourceHandle->priv().webcore() : 0;
    return m_private->setResponse(h, response.priv()->webcore());
}

bool
WKCDownload::start()
{
    return m_private->start();
}

void
WKCDownload::cancel()
{
    m_private->cancel();
}

const char*
WKCDownload::getUri() const
{
    return m_private->getUri();
}

const char*
WKCDownload::getSuggestedFilename() const
{
    return m_private->getSuggestedFilename();
}

int
WKCDownload::getProgressInPercent() const
{
    return m_private->getProgressInPercent();
}

unsigned int
WKCDownload::getElapsedTimeInMilliSeconds() const
{
    return m_private->getElapsedTimeInMilliSeconds();
}

long long
WKCDownload::getTotalSize() const
{
    return m_private->getTotalSize();
}

long long
WKCDownload::getCurrentSize() const
{
    return m_private->getCurrentSize();
}

int
WKCDownload::getStatus() const
{
    return m_private->getStatus();
}

int
WKCDownload::getError() const
{
    return m_private->getError();
}

const WKC::ResourceRequest&
WKCDownload::request() const
{
    return m_private->request();
}

} // namespace
