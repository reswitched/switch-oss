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

#include "config.h"

#include "DatabaseProviderWKC.h"

#if ENABLE(INDEXED_DATABASE)
#include "IDBFactoryBackendInterface.h"

#include "IDBDatabase.h"
#include "IDBServerConnection.h"

#include "DOMStringList.h"
#include "Document.h"
#include "MainFrame.h"
#include "Page.h"
#endif

#include "WKCWebViewPrivate.h"

#include "NotImplemented.h"

namespace WKC {

DatabaseProviderWKC::DatabaseProviderWKC(WKCWebViewPrivate* view)
    : DatabaseProvider()
    , m_view(view)
{
}

DatabaseProviderWKC::~DatabaseProviderWKC()
{
}

WTF::PassRefPtr<DatabaseProviderWKC>
DatabaseProviderWKC::create(WKCWebViewPrivate* view)
{
    return adoptRef(new DatabaseProviderWKC(view));
}

#if ENABLE(INDEXED_DATABASE)
WTF::RefPtr<WebCore::IDBFactoryBackendInterface>
DatabaseProviderWKC::createIDBFactoryBackend()
{
    return IDBFactoryBackendInterfaceWKC::create(this);
}

IDBFactoryBackendInterfaceWKC::IDBFactoryBackendInterfaceWKC(DatabaseProviderWKC* db)
    : m_db(db)
{
}

IDBFactoryBackendInterfaceWKC::~IDBFactoryBackendInterfaceWKC()
{
}

WTF::PassRefPtr<IDBFactoryBackendInterfaceWKC>
IDBFactoryBackendInterfaceWKC::create(DatabaseProviderWKC* db)
{
    return adoptRef(new IDBFactoryBackendInterfaceWKC(db));
}

void
IDBFactoryBackendInterfaceWKC::getDatabaseNames(WTF::PassRefPtr<WebCore::IDBCallbacks> cb, const WebCore::SecurityOrigin& openingOrigin, const WebCore::SecurityOrigin& mainFrameOrigin, WebCore::ScriptExecutionContext*)
{
    notImplemented();
    WTF::RefPtr<WebCore::DOMStringList> ret = WebCore::DOMStringList::create();
    ret->append("WKC");
    cb->onSuccess(ret.release());
}

class IDBServerConnectionWKC : public WebCore::IDBServerConnection
{
public:
    IDBServerConnectionWKC(const WTF::String& name, uint64_t version, int64_t transactionId)
        : m_name(name)
        , m_version(version)
        , m_transactionId(transactionId)
    {
        relaxAdoptionRequirement();
    }
    virtual ~IDBServerConnectionWKC()
    {}

    virtual bool isClosed() override { return false; }

    virtual void getOrEstablishIDBDatabaseMetadata(GetIDBDatabaseMetadataFunction func) override
    {
        notImplemented();
        WebCore::IDBDatabaseMetadata data(m_name, m_transactionId, m_version, 65536);
        func(data, true);
    }
    virtual void close() override {}
    virtual void deleteDatabase(const WTF::String& name, BoolCallbackFunction successCallback) override
    {
        notImplemented();
        successCallback(true);
    }

    virtual void openTransaction(int64_t transactionID, const WTF::HashSet<int64_t>& objectStoreIds, WebCore::IndexedDB::TransactionMode, BoolCallbackFunction successCallback)
    {
        notImplemented();
        successCallback(true);
    }
    virtual void beginTransaction(int64_t transactionID, std::function<void()> completionCallback) override
    {
        notImplemented();
        completionCallback();
    }
    virtual void commitTransaction(int64_t transactionID, BoolCallbackFunction successCallback) override
    {
        notImplemented();
        successCallback(true);
    }
    virtual void resetTransaction(int64_t transactionID, std::function<void()> completionCallback) override
    {
        notImplemented();
        completionCallback();
    }
    virtual bool resetTransactionSync(int64_t transactionID) override { return true; }
    virtual void rollbackTransaction(int64_t transactionID, std::function<void()> completionCallback) override
    {
        notImplemented();
        completionCallback();
    }
    virtual bool rollbackTransactionSync(int64_t transactionID) override { return true; }

    virtual void setIndexKeys(int64_t transactionID, int64_t databaseID, int64_t objectStoreID, const WebCore::IDBObjectStoreMetadata&, WebCore::IDBKey& primaryKey, const WTF::Vector<int64_t, 1>& indexIDs, const WTF::Vector<WTF::Vector<WTF::RefPtr<WebCore::IDBKey>>, 1>& indexKeys, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }

    virtual void createObjectStore(WebCore::IDBTransactionBackend&, const WebCore::CreateObjectStoreOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }
    virtual void createIndex(WebCore::IDBTransactionBackend&, const WebCore::CreateIndexOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }
    virtual void deleteIndex(WebCore::IDBTransactionBackend&, const WebCore::DeleteIndexOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }
    virtual void get(WebCore::IDBTransactionBackend&, const WebCore::GetOperation&, std::function<void(const WebCore::IDBGetResult&, WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        const WebCore::IDBGetResult result;
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(result, err);
    }
    virtual void put(WebCore::IDBTransactionBackend&, const WebCore::PutOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::PassRefPtr<WebCore::IDBKey> key;
        WTF::PassRefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(key, err);
    }
    virtual void openCursor(WebCore::IDBTransactionBackend&, const WebCore::OpenCursorOperation&, std::function<void(int64_t, WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::SharedBuffer>, WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        int64_t v1 = 0;
        WTF::PassRefPtr<WebCore::IDBKey> v2;
        WTF::PassRefPtr<WebCore::IDBKey> v3;
        WTF::PassRefPtr<WebCore::SharedBuffer> v4;
        WTF::PassRefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(v1, v2, v3, v4, err);
    }
    virtual void count(WebCore::IDBTransactionBackend&, const WebCore::CountOperation&, std::function<void(int64_t, WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        int64_t v1 = 0;
        WTF::PassRefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(v1, err);
    }
    virtual void deleteRange(WebCore::IDBTransactionBackend&, const WebCore::DeleteRangeOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }
    virtual void clearObjectStore(WebCore::IDBTransactionBackend&, const WebCore::ClearObjectStoreOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }
    virtual void deleteObjectStore(WebCore::IDBTransactionBackend&, const WebCore::DeleteObjectStoreOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }
    virtual void changeDatabaseVersion(WebCore::IDBTransactionBackend&, const WebCore::IDBDatabaseBackend::VersionChangeOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::RefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(err);
    }

    virtual void cursorAdvance(WebCore::IDBCursorBackend&, const WebCore::CursorAdvanceOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::SharedBuffer>, WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::PassRefPtr<WebCore::IDBKey> v1;
        WTF::PassRefPtr<WebCore::IDBKey> v2;
        WTF::PassRefPtr<WebCore::SharedBuffer> v3;
        WTF::PassRefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(v1, v2, v3, err);
    }
    virtual void cursorIterate(WebCore::IDBCursorBackend&, const WebCore::CursorIterationOperation&, std::function<void(WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::IDBKey>, WTF::PassRefPtr<WebCore::SharedBuffer>, WTF::PassRefPtr<WebCore::IDBDatabaseError>)> completionCallback) override
    {
        notImplemented();
        WTF::PassRefPtr<WebCore::IDBKey> v1;
        WTF::PassRefPtr<WebCore::IDBKey> v2;
        WTF::PassRefPtr<WebCore::SharedBuffer> v3;
        WTF::PassRefPtr<WebCore::IDBDatabaseError> err;
        completionCallback(v1, v2, v3, err);
    }

private:
    const WTF::String m_name;
    uint64_t m_version;
    int64_t m_transactionId;
};

void
IDBFactoryBackendInterfaceWKC::open(const WTF::String& name, uint64_t version, int64_t transactionId, WTF::PassRefPtr<WebCore::IDBCallbacks> cb, WTF::PassRefPtr<WebCore::IDBDatabaseCallbacks> dcb, const WebCore::SecurityOrigin& openingOrigin, const WebCore::SecurityOrigin& mainFrameOrigin)
{
    notImplemented();
    IDBServerConnectionWKC* conn = new IDBServerConnectionWKC(name, version, transactionId);
    WTF::RefPtr<WebCore::IDBDatabaseBackend> backend = WebCore::IDBDatabaseBackend::create("WKC", "1024", this, *conn);
    WTF::RefPtr<WebCore::IDBDatabaseCallbacks> ddcb(dcb.get());
    WTF::RefPtr<WebCore::IDBDatabase> db = WebCore::IDBDatabase::create(m_db->view()->core()->mainFrame().document()->scriptExecutionContext(), backend, ddcb);

    dcb->connect(db.leakRef());

    cb->onSuccess();
}

void
IDBFactoryBackendInterfaceWKC::deleteDatabase(const WTF::String& name, const WebCore::SecurityOrigin& openingOrigin, const WebCore::SecurityOrigin& mainFrameOrigin, WTF::PassRefPtr<WebCore::IDBCallbacks> cb, WebCore::ScriptExecutionContext*)
{
    notImplemented();
    cb->onSuccess();
}

void
IDBFactoryBackendInterfaceWKC::removeIDBDatabaseBackend(const WTF::String& uniqueIdentifier)
{
    notImplemented();
}
#endif

} // namespace
