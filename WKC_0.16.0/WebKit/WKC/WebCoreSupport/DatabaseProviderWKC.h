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

#ifndef Databasepprovider_h
#define Databasepprovider_h

#include "DatabaseProvider.h"

#include "IDBFactoryBackendInterface.h"

namespace WKC {

class WKCWebViewPrivate;

class DatabaseProviderWKC : public WebCore::DatabaseProvider {
public:
    static PassRefPtr<DatabaseProviderWKC> create(WKCWebViewPrivate*);

    virtual ~DatabaseProviderWKC();

#if ENABLE(INDEXED_DATABASE)
    virtual WTF::RefPtr<WebCore::IDBFactoryBackendInterface> createIDBFactoryBackend();
#endif

    WKCWebViewPrivate* view() { return m_view; }

private:
    DatabaseProviderWKC(WKCWebViewPrivate*);

private:
    WKCWebViewPrivate* m_view;
};

#if ENABLE(INDEXED_DATABASE)

class IDBFactoryBackendInterfaceWKC : public WebCore::IDBFactoryBackendInterface
{
public:
    static WTF::PassRefPtr<IDBFactoryBackendInterfaceWKC> create(DatabaseProviderWKC*);

    virtual ~IDBFactoryBackendInterfaceWKC();

    virtual void getDatabaseNames(WTF::PassRefPtr<WebCore::IDBCallbacks>, const WebCore::SecurityOrigin& openingOrigin, const WebCore::SecurityOrigin& mainFrameOrigin, WebCore::ScriptExecutionContext*);
    virtual void open(const WTF::String& name, uint64_t version, int64_t transactionId, WTF::PassRefPtr<WebCore::IDBCallbacks>, WTF::PassRefPtr<WebCore::IDBDatabaseCallbacks>, const WebCore::SecurityOrigin& openingOrigin, const WebCore::SecurityOrigin& mainFrameOrigin);
    virtual void deleteDatabase(const WTF::String& name, const WebCore::SecurityOrigin& openingOrigin, const WebCore::SecurityOrigin& mainFrameOrigin, WTF::PassRefPtr<WebCore::IDBCallbacks>, WebCore::ScriptExecutionContext*);

    virtual void removeIDBDatabaseBackend(const WTF::String& uniqueIdentifier);

private:
    IDBFactoryBackendInterfaceWKC(DatabaseProviderWKC*);
    DatabaseProviderWKC* m_db;
};
#endif

} // namespace

#endif // Databasepprovider_h
