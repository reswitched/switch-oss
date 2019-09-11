/*
 * Copyright (c) 2015-2019 ACCESS CO., LTD. All rights reserved.
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
#include "SecurityOriginData.h"
#include "StorageNamespace.h"
#include "StorageArea.h"
#include "StorageEventDispatcher.h"
#include "StorageType.h"
#include "Widget.h"

#include "WKCWebViewPrivate.h"
#include "helpers/StorageAreaIf.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

namespace WKC {

class StorageAreaWKC : public WebCore::StorageArea {
public:
    StorageAreaWKC(WTF::RefPtr<WebCore::SecurityOrigin> origin, WebCore::StorageType type, unsigned quota, WKC::WKCWebViewPrivate* view)
        : m_origin(origin)
        , m_type(type)
        , m_quota(quota)
        , m_view(view)
    {
        ASSERT(m_view);
        WKC::String originString = securityOrigin().databaseIdentifier();
        WKC::StorageType wkc_type = toWKCStorageType(type);
        m_client = m_view->clientBuilders().createStorageAreaClient(m_view->parent(), originString, wkc_type, quota);
        if (!m_client)
            CRASH();
    }

    StorageAreaWKC(const StorageAreaWKC&) = delete;
    StorageAreaWKC& operator=(const StorageAreaWKC&) = delete;

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

        if (WebCore::isLocalStorage(m_type))
            WebCore::StorageEventDispatcher::dispatchLocalStorageEvents(key, oldValue, value, securityOrigin(), sourceFrame);
        else
            WebCore::StorageEventDispatcher::dispatchSessionStorageEvents(key, oldValue, value, securityOrigin(), sourceFrame);
    }
    virtual void removeItem(WebCore::Frame* sourceFrame, const WTF::String& key) override
    {
        WKC::FramePrivate f(sourceFrame);
        WTF::String oldValue = m_client->item(key);
        m_client->removeItem(&f.wkc(), key);

        if (oldValue.isNull())
            return;

        if (WebCore::isLocalStorage(m_type))
            WebCore::StorageEventDispatcher::dispatchLocalStorageEvents(key, oldValue, WTF::String(), securityOrigin(), sourceFrame);
        else
            WebCore::StorageEventDispatcher::dispatchSessionStorageEvents(key, oldValue, WTF::String(), securityOrigin(), sourceFrame);
    }
    virtual void clear(WebCore::Frame* sourceFrame) override
    {
        if (!m_client->length())
            return;

        WKC::FramePrivate f(sourceFrame);
        m_client->clear(&f.wkc());

        if (WebCore::isLocalStorage(m_type))
            WebCore::StorageEventDispatcher::dispatchLocalStorageEvents(WTF::String(), WTF::String(), WTF::String(), securityOrigin(), sourceFrame);
        else
            WebCore::StorageEventDispatcher::dispatchSessionStorageEvents(WTF::String(), WTF::String(), WTF::String(), securityOrigin(), sourceFrame);
    }
    virtual bool contains(const WTF::String& key) override
    {
        return m_client->contains(key);
    }

    virtual WebCore::StorageType storageType() const override
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

    virtual const WebCore::SecurityOriginData& securityOrigin() const override {
        return m_origin->data();
    }

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

    virtual WTF::RefPtr<WebCore::StorageArea> storageArea(const WebCore::SecurityOriginData& origin) override
    {
        return adoptRef(*new StorageAreaWKC(origin.securityOrigin(), m_type, m_quota, m_view));
    }

    virtual WTF::RefPtr<WebCore::StorageNamespace> copy(WebCore::Page* newPage) override
    {
        PlatformPageClient client = nullptr;
        if (newPage) {
            client = newPage->chrome().client().platformPageClient();
        }
        return adoptRef(*new StorageNamespaceWKC(newPage, m_quota, m_origin, m_type, (WKC::WKCWebViewPrivate *)client));
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

WTF::Ref<StoragenamespaceProviderWKC>
StoragenamespaceProviderWKC::create(WKCWebViewPrivate* view)
{
    return adoptRef(*new StoragenamespaceProviderWKC(view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createSessionStorageNamespace(WebCore::Page& page, unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(&page, quota, nullptr, WebCore::StorageType::Session, m_view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createEphemeralLocalStorageNamespace(WebCore::Page &, unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(nullptr, quota, nullptr, WebCore::StorageType::EphemeralLocal, m_view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createLocalStorageNamespace(unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(nullptr, quota, nullptr, WebCore::StorageType::Local, m_view));
}

WTF::RefPtr<WebCore::StorageNamespace>
StoragenamespaceProviderWKC::createTransientLocalStorageNamespace(WebCore::SecurityOrigin& origin, unsigned quota)
{
    return WTF::adoptRef(new StorageNamespaceWKC(nullptr, quota, &origin, WebCore::StorageType::TransientLocal, m_view));
}

} // namespace WKC
