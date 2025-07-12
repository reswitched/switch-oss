/*
 * Copyright (c) 2015-2016 ACCESS CO., LTD. All rights reserved.
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

#include "StorageNamespaceProviderWKC.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "Page.h"
#include "StorageNamespace.h"
#include "StorageArea.h"
#include "StorageEventDispatcher.h"
#include "Widget.h"

#include "WKCWebViewPrivate.h"
#include "helpers/StorageAreaIf.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

namespace WKC {

class StorageAreaWKC : public WebCore::StorageArea {
public:
    StorageAreaWKC(WTF::PassRefPtr<WebCore::SecurityOrigin> origin, WebCore::StorageType type, unsigned quota, WKC::WKCWebViewPrivate* view)
        : m_origin(origin)
        , m_type(type)
        , m_quota(quota)
        , m_view(view)
    {
        ASSERT(m_view);
        WKC::String originString = m_origin.get()->databaseIdentifier();
        WKC::StorageType wkc_type = toWKCStorageType(type);
        m_client = m_view->clientBuilders().createStorageAreaClient(m_view->parent(), originString, wkc_type, quota);
        if (!m_client)
            CRASH();
    }
    virtual ~StorageAreaWKC()
    {
        if (m_client && WKC::WKCWebViewPrivate::isValidView(m_view))
            m_view->clientBuilders().deleteStorageAreaClient(m_client);
    }

    virtual unsigned length() override
    {
        return m_client->length();
    }
    virtual WTF::String key(unsigned index) override
    {
        return m_client->key(index);
    }
    virtual WTF::String item(const WTF::String& key) override
    {
        return m_client->item(key);
    }
    virtual void setItem(WebCore::Frame* sourceFrame, const WTF::String& key, const WTF::String& value, bool& quotaException) override
    {
        WKC::FramePrivate f(sourceFrame);
        WTF::String oldValue = m_client->item(key);

        m_client->setItem(&f.wkc(), key, value, quotaException);
        if (quotaException)
            return;
        if (value == oldValue)
            return;

        if (m_type == LocalStorage)
            WebCore::StorageEventDispatcher::dispatchLocalStorageEvents(key, oldValue, value, m_origin.get(), sourceFrame);
        else
            WebCore::StorageEventDispatcher::dispatchSessionStorageEvents(key, oldValue, value, m_origin.get(), sourceFrame);
    }
    virtual void removeItem(WebCore::Frame* sourceFrame, const WTF::String& key) override
    {
        WKC::FramePrivate f(sourceFrame);
        WTF::String oldValue = m_client->item(key);
        m_client->removeItem(&f.wkc(), key);

        if (oldValue.isNull())
            return;

        if (m_type == LocalStorage)
            WebCore::StorageEventDispatcher::dispatchLocalStorageEvents(key, oldValue, WTF::String(), m_origin.get(), sourceFrame);
        else
            WebCore::StorageEventDispatcher::dispatchSessionStorageEvents(key, oldValue, WTF::String(), m_origin.get(), sourceFrame);
    }
    virtual void clear(WebCore::Frame* sourceFrame) override
    {
        if (!m_client->length())
            return;

        WKC::FramePrivate f(sourceFrame);
        m_client->clear(&f.wkc());

        if (m_type == LocalStorage)
            WebCore::StorageEventDispatcher::dispatchLocalStorageEvents(WTF::String(), WTF::String(), WTF::String(), m_origin.get(), sourceFrame);
        else
            WebCore::StorageEventDispatcher::dispatchSessionStorageEvents(WTF::String(), WTF::String(), WTF::String(), m_origin.get(), sourceFrame);
    }
    virtual bool contains(const WTF::String& key) override
    {
        return m_client->contains(key);
    }

    virtual bool canAccessStorage(WebCore::Frame* frame) override
    {
        if (!frame) {
            return false;
        }
        WKC::FramePrivate f(frame);
        return m_client->canAccessStorage(&f.wkc());
    }

    virtual WebCore::StorageType storageType() const
    { 
        return m_type;
    }

    virtual size_t memoryBytesUsedByCache() override
    {
        return m_client->memoryBytesUsedByCache();
    }

    virtual void incrementAccessCount() override
    {
        m_client->incrementAccessCount();
    }
    virtual void decrementAccessCount() override
    {
        m_client->decrementAccessCount();
    }
    virtual void closeDatabaseIfIdle() override
    {
        m_client->closeDatabaseIfIdle();
    }

    virtual WebCore::SecurityOrigin& securityOrigin() override { return *m_origin.get(); }

private:
    WTF::RefPtr<WebCore::SecurityOrigin> m_origin;
    WebCore::StorageType m_type;
    unsigned m_quota;

    WKC::WKCWebViewPrivate* m_view;
    WKC::StorageAreaIf* m_client;
};

class StorageNamespaceWKC : public WebCore::StorageNamespace {
public:
    StorageNamespaceWKC(WebCore::Page* page, unsigned quota, WebCore::SecurityOrigin* origin, WebCore::StorageType type, WKC::WKCWebViewPrivate* view)
        : m_page(page)
        , m_quota(quota)
        , m_origin(origin)
        , m_type(type)
        , m_view(view)
    {}
    virtual ~StorageNamespaceWKC()
    {
    }

    virtual WTF::PassRefPtr<WebCore::StorageArea> storageArea(WTF::PassRefPtr<WebCore::SecurityOrigin> origin) override
    {
        return adoptRef(new StorageAreaWKC(origin, m_type, m_quota, m_view));
    }

    virtual WTF::PassRefPtr<WebCore::StorageNamespace> copy(WebCore::Page* newPage)
    {
        PlatformPageClient client = nullptr;
        if (newPage) {
            client = newPage->chrome().client().platformPageClient();
        }
        return adoptRef(new StorageNamespaceWKC(newPage, m_quota, m_origin, m_type, (WKC::WKCWebViewPrivate *)client));
    }

private:
    WebCore::Page* m_page;
    unsigned m_quota;
    WebCore::SecurityOrigin* m_origin;
    WebCore::StorageType m_type;
    WKC::WKCWebViewPrivate* m_view;
};

StoragenamespaceProviderWKC::StoragenamespaceProviderWKC(WKCWebViewPrivate* view)
    : StorageNamespaceProvider()
    , m_view(view)
{
}

StoragenamespaceProviderWKC::~StoragenamespaceProviderWKC()
{
}

WTF::PassRefPtr<StoragenamespaceProviderWKC>
StoragenamespaceProviderWKC::create(WKCWebViewPrivate* view)
{
    return adoptRef(new StoragenamespaceProviderWKC(view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createSessionStorageNamespace(WebCore::Page& page, unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(&page, quota, nullptr, WebCore::SessionStorage, m_view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createLocalStorageNamespace(unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(nullptr, quota, nullptr, WebCore::LocalStorage, m_view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createTransientLocalStorageNamespace(WebCore::SecurityOrigin& origin, unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(nullptr, quota, &origin, WebCore::LocalStorage, m_view));
}

} // namespace WKC
