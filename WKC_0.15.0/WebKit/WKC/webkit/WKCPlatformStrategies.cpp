/*
    WKCPlatformStrategies.cpp
    Copyright (c) 2013, 2015 ACCESS CO., LTD. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"

#include "WKCPlatformStrategies.h"

#include "CookiesStrategy.h"
#include "LoaderStrategy.h"
#include "PasteboardStrategy.h"
#include "PluginStrategy.h"
#include "StorageNamespace.h"

#include "BlobRegistryImpl.h"
#include "DatabaseBackendBase.h"
#include "DatabaseServer.h"
#include "ResourceLoadScheduler.h"
#include "SecurityOrigin.h"
#include "StorageArea.h"

#include "NotImplemented.h"

namespace WKC {
PlatformStrategiesWKC::PlatformStrategiesWKC()
{
}

PlatformStrategiesWKC::~PlatformStrategiesWKC()
{
}

// callbacks

class CookiesStrategyWKC : public WebCore::CookiesStrategy
{
WTF_MAKE_FAST_ALLOCATED;
public:
    CookiesStrategyWKC() {}
    virtual ~CookiesStrategyWKC() {}

    virtual WTF::String cookiesForDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) { return WTF::String(); }
    virtual void setCookiesFromDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, const WTF::String& cookieString) {}
    virtual bool cookiesEnabled(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) { return true; }
    virtual WTF::String cookieRequestHeaderFieldValue(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) { return WTF::String(); }
    virtual bool getRawCookies(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, WTF::Vector<WebCore::Cookie>&) { return false; }
    virtual void deleteCookie(const WebCore::NetworkStorageSession&, const WebCore::URL&, const WTF::String& cookieName) {}
};

WebCore::CookiesStrategy*
PlatformStrategiesWKC::createCookiesStrategy()
{
    return new CookiesStrategyWKC();
}

class LoaderStrategyWKC : public WebCore::LoaderStrategy
{
WTF_MAKE_FAST_ALLOCATED;
public:
    LoaderStrategyWKC()
    {
    }
    virtual ~LoaderStrategyWKC()
    {
    }
};

WebCore::LoaderStrategy*
PlatformStrategiesWKC::createLoaderStrategy()
{
    return new LoaderStrategyWKC();
}

class PasteboardStrategyWKC : public WebCore::PasteboardStrategy
{
WTF_MAKE_FAST_ALLOCATED;
public:
    PasteboardStrategyWKC() {}
    virtual ~PasteboardStrategyWKC() {}
};

WebCore::PasteboardStrategy*
PlatformStrategiesWKC::createPasteboardStrategy()
{
    return new PasteboardStrategyWKC();
}

class PluginStrategyWKC : public WebCore::PluginStrategy
{
WTF_MAKE_FAST_ALLOCATED;
public:
    PluginStrategyWKC() {}
    virtual ~PluginStrategyWKC() {}

    virtual void refreshPlugins() {}
    virtual void getPluginInfo(const WebCore::Page*, WTF::Vector<WebCore::PluginInfo>&) {}
    virtual void getWebVisiblePluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>&) {}
};

WebCore::PluginStrategy*
PlatformStrategiesWKC::createPluginStrategy()
{
    return new PluginStrategyWKC();
}

} // namespace
