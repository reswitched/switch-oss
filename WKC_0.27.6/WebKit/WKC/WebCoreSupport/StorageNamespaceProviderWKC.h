/*
 * Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef StorageNamespaceProviderWKC_h
#define StorageNamespaceProviderWKC_h

#include "StorageNamespaceProvider.h"

namespace WKC {

class WKCWebViewPrivate;

class StoragenamespaceProviderWKC : public WebCore::StorageNamespaceProvider
{
public:
    static WTF::PassRefPtr<StoragenamespaceProviderWKC> create(WKCWebViewPrivate*);
    virtual ~StoragenamespaceProviderWKC() override;

    virtual WTF::RefPtr<WebCore::StorageNamespace> createSessionStorageNamespace(WebCore::Page&, unsigned quota) override;

private:
    StoragenamespaceProviderWKC(WKCWebViewPrivate*);

    virtual WTF::RefPtr<WebCore::StorageNamespace> createLocalStorageNamespace(unsigned quota) override;
    virtual WTF::RefPtr<WebCore::StorageNamespace> createTransientLocalStorageNamespace(WebCore::SecurityOrigin&, unsigned quota) override;

private:
    WKCWebViewPrivate* m_view;
};

} // namespace

#endif // StorageNamespaceProviderWKC_h
