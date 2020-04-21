/*
 *  WebKitWebFramePrivate.h
 *
 *  Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCWebFramePrivate_h
#define WKCWebFramePrivate_h

// class definition

#include "WKCClientBuilders.h"

#ifdef WKC_ENABLE_CUSTOMJS
#include <wkc/wkccustomjs.h>

#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>
#include <wtf/text/StringHash.h>
#endif // WKC_ENABLE_CUSTOMJS

#include <wtf/RefPtr.h>

namespace WebCore {
    class Frame;
    class HTMLFrameOwnerElement;
    class SharedBuffer;
}

namespace WKC {

#ifdef WKC_ENABLE_CUSTOMJS
typedef HashMap<WTF::String, WKCCustomJSAPIList> CustomJSAPIListHashMap;
#endif // WKC_ENABLE_CUSTOMJS

class WKCWebViewPrivate;
class WKCWebFrame;
class FrameLoaderClientWKC;
class FramePrivate;

class WKCWebFramePrivate
{
WTF_MAKE_FAST_ALLOCATED;
    friend class WKCWebFrame;
    friend class FrameLoaderClientWKC;

public:
    static WKCWebFramePrivate* create(WKCWebFrame* parent, WKCWebViewPrivate* view, WKCClientBuilders& buliders, WebCore::HTMLFrameOwnerElement* ownerElement = 0, bool ismainframe=false);
    ~WKCWebFramePrivate();

    void notifyForceTerminate();

    inline WKCWebFrame* parent() const { return m_parent; };
    WebCore::Frame* core() const { return m_coreFrame; };
    FramePrivate* wkcCore() const { return m_wkcCoreFrame; }
    inline WKCClientBuilders& clientBuilders() const { return m_builders; };

    // pagesave
    bool contentSerializeStart();
    int contentSerializeProgress(void* buffer, unsigned int length);
    void contentSerializeEnd();
    bool isPageArchiveLoadFailed();

private:
    WKCWebFramePrivate(WKCWebFrame* parent, WKCWebViewPrivate* view, WKCClientBuilders& builders, WebCore::HTMLFrameOwnerElement* ownerElement, bool ismainframe);
    bool construct();

    void coreFrameDestroyed();

#ifdef WKC_ENABLE_CUSTOMJS
    void initCustomJSAPIList();

    bool setCustomJSAPIList(const int listnum, const WKCCustomJSAPIList *list);
    bool setCustomJSAPIListInternal(const int listnum, const WKCCustomJSAPIList *list);
    bool setCustomJSStringAPIList(const int listnum, const WKCCustomJSAPIList *list);
    bool setCustomJSStringAPIListInternal(const int listnum, const WKCCustomJSAPIList *list);

    CustomJSAPIListHashMap* getCustomJSAPIList() { return m_customJSList; }
    CustomJSAPIListHashMap* getCustomJSAPIListInternal() { return m_customJSListInternal; }
    CustomJSAPIListHashMap* getCustomJSStringAPIList() { return m_customJSStringList; }
    CustomJSAPIListHashMap* getCustomJSStringAPIListInternal() { return m_customJSStringListInternal; }

    WKCCustomJSAPIList* getCustomJSAPI(const char* api_name);
    WKCCustomJSAPIList* getCustomJSAPIInternal(const char* api_name);
    WKCCustomJSAPIList* getCustomJSStringAPI(const char* api_name);
    WKCCustomJSAPIList* getCustomJSStringAPIInternal(const char* api_name);

#endif // WKC_ENABLE_CUSTOMJS

private:
    WKCWebFrame* m_parent;
    WKCWebViewPrivate* m_view;
    WKCClientBuilders& m_builders;
    WebCore::HTMLFrameOwnerElement* m_ownerElement;
    bool m_isMainFrame;

    WebCore::Frame* m_coreFrame;
    FramePrivate* m_wkcCoreFrame;

    // status
    WKC::LoadStatus m_loadStatus;

    // strings
    char* m_uri;
    unsigned short* m_title;
    unsigned short* m_name;

    bool m_forceTerminated;

    char* m_faviconURL;

#ifdef WKC_ENABLE_CUSTOMJS
    CustomJSAPIListHashMap* m_customJSList;
    CustomJSAPIListHashMap* m_customJSListInternal;
    CustomJSAPIListHashMap* m_customJSStringList;
    CustomJSAPIListHashMap* m_customJSStringListInternal;
#endif // WKC_ENABLE_CUSTOMJS

    WTF::RefPtr<WebCore::SharedBuffer> m_mhtmlBuffer;
    unsigned int m_mhtmlProgressPos;
};

} // namespace

#endif // WKCWebFramePrivate_h
