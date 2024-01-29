/*
 *  WKCWebFrame.cpp
 *
 *  Copyright (c) 2010-2022 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "WKCWebFrame.h"
#include "WKCWebFramePrivate.h"
#include "FrameLoaderClientWKC.h"
#include "WKCWebView.h"
#include "WKCWebViewPrivate.h"

#include "ArchiveFactory.h"
#include "BitmapImage.h"
#include "CSSAnimationController.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "Image.h"
#include "Page.h"
#include "URL.h"
#include "SharedBuffer.h"
#include "WTFString.h"
#include "JavaScriptCore/APICast.h"
#include "JSDOMBinding.h"
#include "Settings.h"
#include "ScriptController.h"
#include "SubstituteData.h"
#include "SubresourceLoader.h"
#include "IconLoader.h"
#include "ImageWKC.h"
#include "bindings/ScriptValue.h"
#if ENABLE(MHTML)
#include "MHTMLArchive.h"
#endif

#include "helpers/privates/WKCDOMWrapperWorldPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHTMLFrameOwnerElementPrivate.h"
#include "helpers/privates/WKCResourceRequestPrivate.h"

#include "NotImplemented.h"

static const unsigned short cNullWStr[] = {0};

namespace WKC {

// private container
WKCWebFramePrivate::WKCWebFramePrivate(WKCWebFrame* parent, WKCWebViewPrivate* view, WKCClientBuilders& builders, WebCore::HTMLFrameOwnerElement* ownerelement, bool ismainframe)
     : m_parent(parent),
       m_view(view),
       m_builders(builders),
       m_ownerElement(ownerelement),
       m_isMainFrame(ismainframe),
       m_coreFrame(0),
       m_wkcCoreFrame(0),
       m_loadStatus(ELoadStatusNone),
       m_uri(0),
       m_title(0),
       m_name(0)
#ifdef WKC_ENABLE_CUSTOMJS
     , m_customJSList(0)
     , m_customJSListInternal(0)
     , m_customJSStringList(0)
     , m_customJSStringListInternal(0)
#endif // WKC_ENABLE_CUSTOMJS
     , m_mhtmlBuffer(0)
     , m_mhtmlProgressPos(0)
{
    m_forceTerminated = false;
}

WKCWebFramePrivate::~WKCWebFramePrivate()
{
    if (m_forceTerminated) {
        return;

    }

    if (m_wkcCoreFrame) {
        delete m_wkcCoreFrame;
        m_wkcCoreFrame = 0;
    }

    if (m_coreFrame) {
        m_coreFrame->loader().cancelAndClear();
        m_coreFrame = 0;
        // m_coreFrame would be deleted automatically
    }

    if (m_uri) {
        fastFree(m_uri);
        m_uri = 0;
    }
    if (m_title) {
        fastFree(m_title);
        m_title = 0;
    }
    if (m_name) {
        fastFree(m_name);
        m_name = 0;
    }
#ifdef WKC_ENABLE_CUSTOMJS
    if (m_customJSList) {
        delete m_customJSList;
    }
    if (m_customJSListInternal) {
        delete m_customJSListInternal;
    }
    if (m_customJSStringList) {
        delete m_customJSStringList;
    }
    if (m_customJSStringListInternal) {
        delete m_customJSStringListInternal;
    }
#endif // WKC_ENABLE_CUSTOMJS

    m_mhtmlBuffer = nullptr;
}

WKCWebFramePrivate*
WKCWebFramePrivate::create(WKCWebFrame* parent, WKCWebViewPrivate* view, WKCClientBuilders& builders, WebCore::HTMLFrameOwnerElement* ownerelement, bool ismainframe)
{
    WKCWebFramePrivate* self = 0;
    self = new WKCWebFramePrivate(parent, view, builders, ownerelement, ismainframe);
    if (!self) return self;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
WKCWebFramePrivate::construct()
{
    WebCore::HTMLFrameOwnerElement* parent = 0;

    ASSERT(m_view);

    if (m_ownerElement) {
        parent = m_ownerElement;
    }
    if (m_isMainFrame) {
        m_coreFrame = (WebCore::Frame *)&m_view->core()->mainFrame();

        if (m_ownerElement)
            m_coreFrame->ref();
        m_wkcCoreFrame = new FramePrivate(m_coreFrame);
        if (!m_wkcCoreFrame)
            wkcMemoryNotifyNoMemoryPeer(sizeof(FramePrivate));
    } else {
        WKC::FrameLoaderClientWKC* client = FrameLoaderClientWKC::create(this);
        if (!client) return false;
        RefPtr<WebCore::Frame> newframe = WebCore::Frame::create(m_view->core(), parent, client);
        if (!newframe) {
            delete client;
            return false;
        }
        m_coreFrame = newframe.get();

        if (m_ownerElement)
            m_coreFrame->ref();
        m_wkcCoreFrame = new FramePrivate(m_coreFrame);
        if (!m_wkcCoreFrame)
            wkcMemoryNotifyNoMemoryPeer(sizeof(FramePrivate));
    }

#ifdef WKC_ENABLE_CUSTOMJS
    m_customJSList = new CustomJSAPIListHashMap;
    m_customJSListInternal = new CustomJSAPIListHashMap;
    m_customJSStringList = new CustomJSAPIListHashMap;
    m_customJSStringListInternal = new CustomJSAPIListHashMap;
#endif // WKC_ENABLE_CUSTOMJS

    return true;
}

void WKCWebFrame::deleteWKCWebFrame(WKCWebFrame *self)
{
    if (!self)
        return;
    self->~WKCWebFrame();
    WTF::fastFree(self);
}

void
WKCWebFramePrivate::notifyForceTerminate()
{
    m_forceTerminated = true;
}

void
WKCWebFramePrivate::coreFrameDestroyed()
{
    m_coreFrame = 0;
}

#ifdef WKC_ENABLE_CUSTOMJS
void
WKCWebFramePrivate::initCustomJSAPIList()
{
    m_customJSList->clear();
    m_customJSListInternal->clear();
    m_customJSStringList->clear();
    m_customJSStringListInternal->clear();
}

bool
WKCWebFramePrivate::setCustomJSAPIList(const int listnum, const WKCCustomJSAPIList *list)
{
    if (listnum > 0){
        for (int i = 0; i < listnum; i++) {
            (m_customJSList)->set(list[i].CustomJSName, list[i]);
        }
    } else {
        return false;
    }

    return true;
}

bool
WKCWebFramePrivate::setCustomJSAPIListInternal(const int listnum, const WKCCustomJSAPIList *list)
{
    if (listnum > 0){
        for (int i = 0; i < listnum; i++) {
            (m_customJSListInternal)->set(list[i].CustomJSName, list[i]);
        }
    } else {
        return false;
    }

    return true;
}

bool
WKCWebFramePrivate::setCustomJSStringAPIList(const int listnum, const WKCCustomJSAPIList *list)
{
    if (listnum > 0){
        for (int i = 0; i < listnum; i++) {
            (m_customJSStringList)->set(list[i].CustomJSName, list[i]);
        }
    } else {
        return false;
    }

    return true;
}

bool
WKCWebFramePrivate::setCustomJSStringAPIListInternal(const int listnum, const WKCCustomJSAPIList *list)
{
    if (listnum > 0){
        for (int i = 0; i < listnum; i++) {
            (m_customJSStringListInternal)->set(list[i].CustomJSName, list[i]);
        }
    } else {
        return false;
    }

    return true;
}

WKCCustomJSAPIList*
WKCWebFramePrivate::getCustomJSAPI(const char* api_name)
{
    static WKCCustomJSAPIList apis;
    apis = (m_customJSList)->get(api_name);
    return &apis;
}
 
WKCCustomJSAPIList*
WKCWebFramePrivate::getCustomJSAPIInternal(const char* api_name)
{
    static WKCCustomJSAPIList apis;
    apis = (m_customJSListInternal)->get(api_name);
    return &apis;
}

WKCCustomJSAPIList*
WKCWebFramePrivate::getCustomJSStringAPI(const char* api_name)
{
    static WKCCustomJSAPIList apis;
    apis = (m_customJSStringList)->get(api_name);
    return &apis;
}
 
WKCCustomJSAPIList*
WKCWebFramePrivate::getCustomJSStringAPIInternal(const char* api_name)
{
    static WKCCustomJSAPIList apis;
    apis = (m_customJSStringListInternal)->get(api_name);
    return &apis;
}

#endif // WKC_ENABLE_CUSTOMJS

#if ENABLE(MHTML)
bool
WKCWebFramePrivate::contentSerializeStart()
{
    m_mhtmlBuffer = nullptr;
    WebCore::Page* page = core()->page();
    if (!page)
        return false;
    m_mhtmlBuffer = WebCore::MHTMLArchive::generateMHTMLData(page);
    if (!m_mhtmlBuffer || m_mhtmlBuffer->size()==0)
        return false;

    m_mhtmlProgressPos = 0;
    return true;
}

int
WKCWebFramePrivate::contentSerializeProgress(void* buffer, unsigned int length)
{
    unsigned int remains = m_mhtmlBuffer->size() - m_mhtmlProgressPos;
    int len = WKC_MIN(length, remains);
    const char* p = m_mhtmlBuffer->data() + m_mhtmlProgressPos;
    ::memcpy(buffer, p, len);
    m_mhtmlProgressPos += len;
    return len;
}

void
WKCWebFramePrivate::contentSerializeEnd()
{
    m_mhtmlBuffer = nullptr;
    m_mhtmlProgressPos = 0;
}
#else
bool
WKCWebFramePrivate::contentSerializeStart()
{
    notImplemented();
    return false;
}

int
WKCWebFramePrivate::contentSerializeProgress(void* buffer, unsigned int length)
{
    notImplemented();
    return -1;
}

void
WKCWebFramePrivate::contentSerializeEnd()
{
    notImplemented();
    return;
}
#endif

bool
WKCWebFramePrivate::isPageArchiveLoadFailed()
{
#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
    if (!core() || !core()->loader().activeDocumentLoader())
        return false;

    RefPtr<WebCore::DocumentLoader> dl = core()->loader().activeDocumentLoader();
    if (WebCore::ArchiveFactory::isArchiveMIMEType(dl->responseMIMEType())) {        
        if (!dl->parsedArchiveData())
            return true;
    }
#endif
    return false;
}

// implementations

WKCWebFrame::WKCWebFrame()
     : m_private(0)
{
}

WKCWebFrame::~WKCWebFrame()
{
    if (m_private) {
        delete m_private;
    }
}

WKCWebFrame*
WKCWebFrame::create(WKCWebView* view, WKCClientBuilders& builders)
{
    return WKCWebFrame::create(view->m_private, builders);
}


WKCWebFrame*
WKCWebFrame::create(WKCWebViewPrivate* view, WKCClientBuilders& builders, WKC::HTMLFrameOwnerElement* ownerelement, bool ismainframe)
{
    WKCWebFrame* self = 0;

    self = (WKCWebFrame *)WTF::fastMalloc(sizeof(WKCWebFrame));
    self = new (self) WKCWebFrame();
    if (!self) return 0;
    if (!self->construct(view, builders, ownerelement, ismainframe)) {
        self->~WKCWebFrame();
        WTF::fastFree(self);
        return 0;
    }
    return self;
}

bool
WKCWebFrame::construct(WKCWebViewPrivate* view, WKCClientBuilders& builders, WKC::HTMLFrameOwnerElement* ownerelement, bool ismainframe)
{
    WebCore::HTMLFrameOwnerElement* owner = 0;
    if (ownerelement) {
        owner = static_cast<WebCore::HTMLFrameOwnerElement*>(ownerelement->priv().webcore());
    }
    m_private = WKCWebFramePrivate::create(this, view, builders, owner, ismainframe);
    if (!m_private) return false;
    return true;
}

void
WKCWebFrame::notifyForceTerminate()
{
    if (m_private) {
        m_private->notifyForceTerminate();
    }
}

static WKCWebFrame*
kit(WebCore::Frame* coreFrame)
{
    if (!coreFrame)
      return 0;

    FrameLoaderClientWKC* client = static_cast<FrameLoaderClientWKC*>(&coreFrame->loader().client());
    return client ? client->webFrame() : 0;
}

// APIs
WKC::Frame*
WKCWebFrame::core() const
{
    return &m_private->wkcCore()->wkc();
}

bool
WKCWebFrame::compare(const WKC::Frame* frame) const
{
    if (!frame)
        return false;
    return (m_private->wkcCore()->webcore() == const_cast<WKC::Frame *>(frame)->priv().webcore());
}

WKCWebView*
WKCWebFrame::webView()
{
    return m_private->m_view->parent();
}

const unsigned short*
WKCWebFrame::name()
{
    if (m_private->m_name) {
        return m_private->m_name;
    }

    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return cNullWStr;
    }

    WTF::String string = coreFrame->tree().name();
    m_private->m_name = wkc_wstrdup(string.characters16());
    return m_private->m_name;
}
const unsigned short*
WKCWebFrame::title()
{
    return m_private->m_title;
}
const char*
WKCWebFrame::uri()
{
    return m_private->m_uri;
}
WKCWebFrame*
WKCWebFrame::parent()
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return 0;
    }

    return kit(coreFrame->tree().parent());
}

bool
WKCWebFrame::loadURI(const char* uri, const char* referrer)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return false;
    }

    auto utf8Uri(WTF::String::fromUTF8(uri));
    if (!utf8Uri) {
        return false;
    }

    if (referrer) {
        WTF::String refStr = WTF::String::fromUTF8(referrer);
        if (!refStr) {
            return false;
        }
        URL refUrl = URL(URL(), refStr);
        if (refUrl.isValid()) {
            // Use normalized referrer URL if it is valid
            refStr = refUrl.string();
        }
        WebCore::FrameLoadRequest req(*coreFrame, WebCore::ResourceRequest(URL(URL(), utf8Uri), refStr), WebCore::ShouldOpenExternalURLsPolicy::ShouldNotAllow);
        coreFrame->loader().load(WTFMove(req));
    } else {
        WebCore::ResourceRequest rr(URL(URL(), utf8Uri));
        rr.setTargetType(WebCore::ResourceRequest::TargetIsMainFrame);
        WebCore::FrameLoadRequest req(*coreFrame, rr, WebCore::ShouldOpenExternalURLsPolicy::ShouldNotAllow);
        coreFrame->loader().load(WTFMove(req));
    }
    return true;
}

#ifdef __MINGW32__
# ifdef LoadString
#  undef LoadString
# endif
#endif
void
WKCWebFrame::loadString(const char* content, const unsigned short* mime_type, const unsigned short* encoding, const char *base_uri, const char *unreachable_uri, bool replace)
{
    WebCore::Frame* coreFrame = m_private->core();
    WebCore::FrameLoader* loader = &coreFrame->loader();
    WebCore::DocumentLoader* provisionalDocumentLoader = loader->provisionalDocumentLoader();
    WebCore::ResourceRequest lastRequest;
    if (provisionalDocumentLoader){
        // Get the last request (may be a redirected request)
        lastRequest = provisionalDocumentLoader->request();
    }

    URL baseKURL = (base_uri && base_uri[0]) ? URL(URL(), WTF::String::fromUTF8(base_uri)) : WTF::blankURL();

    WebCore::ResourceRequest request(baseKURL);

    WTF::RefPtr<WebCore::SharedBuffer> sharedBuffer = WebCore::SharedBuffer::create(content, strlen(content));
    WebCore::ResourceResponse rsp(URL(URL(), WTF::String::fromUTF8(unreachable_uri)),
        mime_type ? WTF::String(mime_type) : WTF::String::fromUTF8("text/html"),
        0,
        encoding ? WTF::String(encoding) : WTF::String::fromUTF8("UTF-8")
        );
    WebCore::SubstituteData substituteData(WTFMove(sharedBuffer),
                                           URL(URL(), WTF::String::fromUTF8(unreachable_uri)),
                                           rsp,
                                           WebCore::SubstituteData::SessionHistoryVisibility::Visible,
                                           lastRequest);
    
    WebCore::FrameLoadRequest req(*coreFrame, request, WebCore::ShouldOpenExternalURLsPolicy::ShouldNotAllow, substituteData);
    loader->load(WTFMove(req));
    if (replace) {
        loader->setReplacing();
    }
}

void
WKCWebFrame::loadRequest(const WKC::ResourceRequest& request)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return;
    }

    WebCore::FrameLoadRequest req(*coreFrame, request.priv().webcore(), WebCore::ShouldOpenExternalURLsPolicy::ShouldNotAllow);
    coreFrame->loader().load(WTFMove(req));
}

void
WKCWebFrame::stopLoading()
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return;
    }

    coreFrame->loader().stopAllLoaders();
}
void
WKCWebFrame::reload()
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return;
    }

    coreFrame->loader().reload();
}

WKCWebFrame*
WKCWebFrame::findFrame(const unsigned short* name)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return 0;
    }

    WTF::String nameString = WTF::String(name);
    return kit(coreFrame->tree().find(WTF::AtomString(nameString), *coreFrame));
}

JSGlobalContextRef
WKCWebFrame::globalContext(WKC::DOMWrapperWorld* world)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return 0;
    }

    WebCore::DOMWrapperWorld* w = &WebCore::mainThreadNormalWorld();
    if (world) {
        w = world->priv().webcore();
    }

    return toGlobalRef(coreFrame->script().globalObject(*w));
}

WKC::LoadStatus
WKCWebFrame::loadStatus()
{
    return m_private->m_loadStatus;
}
WKC::ScrollbarMode
WKCWebFrame::horizontalScrollbarMode()
{
    WebCore::Frame* coreFrame = m_private->core();
    WebCore::FrameView* view = coreFrame->view();
    if (!view) {
        return WKC::EScrollbarAuto;
    }

    WebCore::ScrollbarMode hMode = view->horizontalScrollbarMode();

    if (hMode == WebCore::ScrollbarAlwaysOn) {
        return WKC::EScrollbarAlwaysOn;
    }

    if (hMode == WebCore::ScrollbarAlwaysOff) {
        return WKC::EScrollbarAlwaysOff;
    }

    return WKC::EScrollbarAuto;
}
WKC::ScrollbarMode
WKCWebFrame::verticalScrollbarMode()
{
    WebCore::Frame* coreFrame = m_private->core();
    WebCore::FrameView* view = coreFrame->view();
    if (!view) {
        return WKC::EScrollbarAuto;
    }

    WebCore::ScrollbarMode hMode = view->verticalScrollbarMode();

    if (hMode == WebCore::ScrollbarAlwaysOn) {
        return WKC::EScrollbarAlwaysOn;
    }

    if (hMode == WebCore::ScrollbarAlwaysOff) {
        return WKC::EScrollbarAlwaysOff;
    }

    return WKC::EScrollbarAuto;
}

WKCSecurityOrigin*
WKCWebFrame::securityOrigin()
{
    // Ugh!: implement it!
    // 100106 ACCESS Co.,Ltd.
    return 0;
}

bool
WKCWebFrame::contentSerializeStart()
{
    return m_private->contentSerializeStart();
}

int
WKCWebFrame::contentSerializeProgress(void* buffer, unsigned int length)
{
    return m_private->contentSerializeProgress(buffer, length);
}

void
WKCWebFrame::contentSerializeEnd()
{
    m_private->contentSerializeEnd();
}

bool
WKCWebFrame::isPageArchiveLoadFailed()
{
    return m_private->isPageArchiveLoadFailed();
}

#ifdef WKC_ENABLE_CUSTOMJS
bool
WKCWebFrame::setCustomJSAPIList(const int listnum, const WKCCustomJSAPIList *list)
{
    if (!m_private->m_customJSList)
        return false;
    return m_private->setCustomJSAPIList(listnum, list);
}

bool
WKCWebFrame::setCustomJSAPIListInternal(const int listnum, const WKCCustomJSAPIList *list)
{
    if (!m_private->m_customJSListInternal)
        return false;
    return m_private->setCustomJSAPIListInternal(listnum, list);
}

bool
WKCWebFrame::setCustomJSStringAPIList(const int listnum, const WKCCustomJSAPIList *list)
{
    if (!m_private->m_customJSStringList)
        return false;
    return m_private->setCustomJSStringAPIList(listnum, list);
}

bool
WKCWebFrame::setCustomJSStringAPIListInternal(const int listnum, const WKCCustomJSAPIList *list)
{
    if (!m_private->m_customJSStringListInternal)
        return false;
    return m_private->setCustomJSStringAPIListInternal(listnum, list);
}

WKCCustomJSAPIList*
WKCWebFrame::getCustomJSAPI(const char* api_name)
{
    if (!m_private->m_customJSList)
        return 0;
    return m_private->getCustomJSAPI(api_name);
}

WKCCustomJSAPIList*
WKCWebFrame::getCustomJSAPIInternal(const char* api_name)
{
    if (!m_private->m_customJSListInternal)
        return 0;
    return m_private->getCustomJSAPIInternal(api_name);
}

WKCCustomJSAPIList*
WKCWebFrame::getCustomJSStringAPI(const char* api_name)
{
    if (!m_private->m_customJSStringList)
        return 0;
    return m_private->getCustomJSStringAPI(api_name);
}

WKCCustomJSAPIList*
WKCWebFrame::getCustomJSStringAPIInternal(const char* api_name)
{
    if (!m_private->m_customJSStringListInternal)
        return 0;
    return m_private->getCustomJSStringAPIInternal(api_name);
}

void
WKCWebFrame::setForcedSandboxNavigation()
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return;
    }

    coreFrame->loader().forceSandboxFlags(WebCore::SandboxNavigation);
}

#endif // WKC_ENABLE_CUSTOMJS

void
WKCWebFrame::executeScriptIgnoringException(const char* script, bool forceUserGesture)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return;
    }

    coreFrame->script().executeScriptIgnoringException(WTF::String::fromUTF8(script), forceUserGesture);
}

void
WKCWebFrame::executeUserAgentScriptInWorldIgnoringException(DOMWrapperWorld& world, const char* script, bool forceUserGesture)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame) {
        return;
    }

    if (world.priv().webcore()) {
        return;
    }

    coreFrame->script().executeUserAgentScriptInWorldIgnoringException(*world.priv().webcore(), WTF::String::fromUTF8(script), forceUserGesture);
}

void
WKCWebFrame::setJavaScriptPaused(bool pause)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame)
        return;

    coreFrame->script().setPaused(pause);
}

bool
WKCWebFrame::isJavaScriptPaused() const
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame)
        return false;

    return coreFrame->script().isPaused();
}

void
WKCWebFrame::suspendAnimations()
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame)
        return;

    coreFrame->animation().suspendAnimations();
}

void
WKCWebFrame::resumeAnimations()
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame)
        return;

    coreFrame->animation().resumeAnimations();
}

void
WKCWebFrame::setSpatialNavigationEnabled(bool enable)
{
    WebCore::Frame* coreFrame = m_private->core();
    if (!coreFrame)
        return;
    coreFrame->settings().setSpatialNavigationEnabled(enable);
}

} // namespace
