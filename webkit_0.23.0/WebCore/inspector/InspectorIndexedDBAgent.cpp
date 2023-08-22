/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INDEXED_DATABASE)

#include "InspectorIndexedDBAgent.h"

#include "DOMStringList.h"
#include "DOMWindow.h"
#include "DOMWindowIndexedDatabase.h"
#include "Document.h"
#include "Event.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "IDBCursor.h"
#include "IDBCursorWithValue.h"
#include "IDBDatabase.h"
#include "IDBDatabaseCallbacks.h"
#include "IDBDatabaseMetadata.h"
#include "IDBFactory.h"
#include "IDBIndex.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBObjectStore.h"
#include "IDBOpenDBRequest.h"
#include "IDBPendingTransactionMonitor.h"
#include "IDBRequest.h"
#include "IDBTransaction.h"
#include "InspectorPageAgent.h"
#include "InstrumentingAgents.h"
#include "SecurityOrigin.h"
#include <inspector/InjectedScript.h>
#include <inspector/InjectedScriptManager.h>
#if !PLATFORM(WKC)
#include <inspector/InspectorFrontendDispatchers.h>
#include <inspector/InspectorFrontendRouter.h>
#include <inspector/InspectorValues.h>
#else
#include <InspectorFrontendDispatchers.h>
#include <InspectorValues.h>
#endif
#include <wtf/NeverDestroyed.h>
#include <wtf/Vector.h>

using Inspector::Protocol::Array;
using Inspector::Protocol::IndexedDB::DatabaseWithObjectStores;
using Inspector::Protocol::IndexedDB::DataEntry;
using Inspector::Protocol::IndexedDB::Key;
using Inspector::Protocol::IndexedDB::KeyPath;
using Inspector::Protocol::IndexedDB::KeyRange;
using Inspector::Protocol::IndexedDB::ObjectStore;
using Inspector::Protocol::IndexedDB::ObjectStoreIndex;

typedef Inspector::BackendDispatcher::CallbackBase RequestCallback;
typedef Inspector::IndexedDBBackendDispatcherHandler::RequestDatabaseNamesCallback RequestDatabaseNamesCallback;
typedef Inspector::IndexedDBBackendDispatcherHandler::RequestDatabaseCallback RequestDatabaseCallback;
typedef Inspector::IndexedDBBackendDispatcherHandler::RequestDataCallback RequestDataCallback;
typedef Inspector::IndexedDBBackendDispatcherHandler::ClearObjectStoreCallback ClearObjectStoreCallback;

using namespace Inspector;

namespace WebCore {

namespace {

class GetDatabaseNamesCallback : public EventListener {
    WTF_MAKE_NONCOPYABLE(GetDatabaseNamesCallback);
public:
    static Ref<GetDatabaseNamesCallback> create(Ref<RequestDatabaseNamesCallback>&& requestCallback, const String& securityOrigin)
    {
        return adoptRef(*new GetDatabaseNamesCallback(WTF::move(requestCallback), securityOrigin));
    }

    virtual ~GetDatabaseNamesCallback() { }

    virtual bool operator==(const EventListener& other) override
    {
        return this == &other;
    }

    virtual void handleEvent(ScriptExecutionContext*, Event* event) override
    {
        if (!m_requestCallback->isActive())
            return;
        if (event->type() != eventNames().successEvent) {
            m_requestCallback->sendFailure("Unexpected event type.");
            return;
        }

        IDBRequest* idbRequest = static_cast<IDBRequest*>(event->target());
        ExceptionCode ec = 0;
        RefPtr<IDBAny> requestResult = idbRequest->result(ec);
        if (ec) {
            m_requestCallback->sendFailure("Could not get result in callback.");
            return;
        }
        if (requestResult->type() != IDBAny::DOMStringListType) {
            m_requestCallback->sendFailure("Unexpected result type.");
            return;
        }

        RefPtr<DOMStringList> databaseNamesList = requestResult->domStringList();
        Ref<Inspector::Protocol::Array<String>> databaseNames = Inspector::Protocol::Array<String>::create();
        for (size_t i = 0; i < databaseNamesList->length(); ++i)
            databaseNames->addItem(databaseNamesList->item(i));
        m_requestCallback->sendSuccess(WTF::move(databaseNames));
    }

private:
    GetDatabaseNamesCallback(Ref<RequestDatabaseNamesCallback>&& requestCallback, const String& securityOrigin)
        : EventListener(EventListener::CPPEventListenerType)
        , m_requestCallback(WTF::move(requestCallback))
        , m_securityOrigin(securityOrigin) { }
    Ref<RequestDatabaseNamesCallback> m_requestCallback;
    String m_securityOrigin;
};

class ExecutableWithDatabase : public RefCounted<ExecutableWithDatabase> {
public:
    ExecutableWithDatabase(ScriptExecutionContext* context)
        : m_context(context) { }
    virtual ~ExecutableWithDatabase() { };
    void start(IDBFactory*, SecurityOrigin*, const String& databaseName);
    virtual void execute(RefPtr<IDBDatabase>&&) = 0;
    virtual RequestCallback& requestCallback() = 0;
    ScriptExecutionContext* context() { return m_context; };
private:
    ScriptExecutionContext* m_context;
};

class OpenDatabaseCallback : public EventListener {
public:
    static Ref<OpenDatabaseCallback> create(ExecutableWithDatabase* executableWithDatabase)
    {
        return adoptRef(*new OpenDatabaseCallback(executableWithDatabase));
    }

    virtual ~OpenDatabaseCallback() { }

    virtual bool operator==(const EventListener& other) override
    {
        return this == &other;
    }

    virtual void handleEvent(ScriptExecutionContext*, Event* event) override
    {
        if (event->type() != eventNames().successEvent) {
            m_executableWithDatabase->requestCallback().sendFailure("Unexpected event type.");
            return;
        }

        IDBOpenDBRequest* idbOpenDBRequest = static_cast<IDBOpenDBRequest*>(event->target());
        ExceptionCode ec = 0;
        RefPtr<IDBAny> requestResult = idbOpenDBRequest->result(ec);
        if (ec) {
            m_executableWithDatabase->requestCallback().sendFailure("Could not get result in callback.");
            return;
        }
        if (requestResult->type() != IDBAny::IDBDatabaseType) {
            m_executableWithDatabase->requestCallback().sendFailure("Unexpected result type.");
            return;
        }

        RefPtr<IDBDatabase> idbDatabase = requestResult->idbDatabase();
        m_executableWithDatabase->execute(WTF::move(idbDatabase));
        IDBPendingTransactionMonitor::deactivateNewTransactions();
        idbDatabase->close();
    }

private:
    OpenDatabaseCallback(ExecutableWithDatabase* executableWithDatabase)
        : EventListener(EventListener::CPPEventListenerType)
        , m_executableWithDatabase(executableWithDatabase) { }
    RefPtr<ExecutableWithDatabase> m_executableWithDatabase;
};

void ExecutableWithDatabase::start(IDBFactory* idbFactory, SecurityOrigin*, const String& databaseName)
{
    Ref<OpenDatabaseCallback> callback = OpenDatabaseCallback::create(this);
    ExceptionCode ec = 0;
    RefPtr<IDBOpenDBRequest> idbOpenDBRequest = idbFactory->open(context(), databaseName, ec);
    if (ec) {
        requestCallback().sendFailure("Could not open database.");
        return;
    }
    idbOpenDBRequest->addEventListener(eventNames().successEvent, WTF::move(callback), false);
}

static RefPtr<IDBTransaction> transactionForDatabase(ScriptExecutionContext* scriptExecutionContext, IDBDatabase* idbDatabase, const String& objectStoreName, const String& mode = IDBTransaction::modeReadOnly())
{
    ExceptionCode ec = 0;
    RefPtr<IDBTransaction> idbTransaction = idbDatabase->transaction(scriptExecutionContext, objectStoreName, mode, ec);
    if (ec)
        return nullptr;
    return WTF::move(idbTransaction);
}

static RefPtr<IDBObjectStore> objectStoreForTransaction(IDBTransaction* idbTransaction, const String& objectStoreName)
{
    ExceptionCode ec = 0;
    RefPtr<IDBObjectStore> idbObjectStore = idbTransaction->objectStore(objectStoreName, ec);
    if (ec)
        return nullptr;
    return WTF::move(idbObjectStore);
}

static RefPtr<IDBIndex> indexForObjectStore(IDBObjectStore* idbObjectStore, const String& indexName)
{
    ExceptionCode ec = 0;
    RefPtr<IDBIndex> idbIndex = idbObjectStore->index(indexName, ec);
    if (ec)
        return nullptr;
    return WTF::move(idbIndex);
}

static RefPtr<KeyPath> keyPathFromIDBKeyPath(const IDBKeyPath& idbKeyPath)
{
    RefPtr<KeyPath> keyPath;
    switch (idbKeyPath.type()) {
    case IDBKeyPath::NullType:
        keyPath = KeyPath::create()
            .setType(KeyPath::Type::Null)
            .release();
        break;
    case IDBKeyPath::StringType:
        keyPath = KeyPath::create()
            .setType(KeyPath::Type::String)
            .release();
        keyPath->setString(idbKeyPath.string());

        break;
    case IDBKeyPath::ArrayType: {
        auto array = Inspector::Protocol::Array<String>::create();
        const Vector<String>& stringArray = idbKeyPath.array();
        for (size_t i = 0; i < stringArray.size(); ++i)
            array->addItem(stringArray[i]);
        keyPath = KeyPath::create()
            .setType(KeyPath::Type::Array)
            .release();
        keyPath->setArray(WTF::move(array));
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }

    return WTF::move(keyPath);
}

class DatabaseLoader : public ExecutableWithDatabase {
public:
    static Ref<DatabaseLoader> create(ScriptExecutionContext* context, Ref<RequestDatabaseCallback>&& requestCallback)
    {
        return adoptRef(*new DatabaseLoader(context, WTF::move(requestCallback)));
    }

    virtual ~DatabaseLoader() { }

    virtual void execute(RefPtr<IDBDatabase>&& database) override
    {
        if (!requestCallback().isActive())
            return;

        const IDBDatabaseMetadata databaseMetadata = database->metadata();

        auto objectStores = Inspector::Protocol::Array<Inspector::Protocol::IndexedDB::ObjectStore>::create();

        for (const IDBObjectStoreMetadata& objectStoreMetadata : databaseMetadata.objectStores.values()) {
            auto indexes = Inspector::Protocol::Array<Inspector::Protocol::IndexedDB::ObjectStoreIndex>::create();

            for (const IDBIndexMetadata& indexMetadata : objectStoreMetadata.indexes.values()) {
                Ref<ObjectStoreIndex> objectStoreIndex = ObjectStoreIndex::create()
                    .setName(indexMetadata.name)
                    .setKeyPath(keyPathFromIDBKeyPath(indexMetadata.keyPath))
                    .setUnique(indexMetadata.unique)
                    .setMultiEntry(indexMetadata.multiEntry)
                    .release();
                indexes->addItem(WTF::move(objectStoreIndex));
            }

            Ref<ObjectStore> objectStore = ObjectStore::create()
                .setName(objectStoreMetadata.name)
                .setKeyPath(keyPathFromIDBKeyPath(objectStoreMetadata.keyPath))
                .setAutoIncrement(objectStoreMetadata.autoIncrement)
                .setIndexes(WTF::move(indexes))
                .release();

            objectStores->addItem(WTF::move(objectStore));
        }

        Ref<DatabaseWithObjectStores> result = DatabaseWithObjectStores::create()
            .setName(databaseMetadata.name)
            .setVersion(databaseMetadata.version)
            .setObjectStores(WTF::move(objectStores))
            .release();

        m_requestCallback->sendSuccess(WTF::move(result));
    }

    virtual RequestCallback& requestCallback() override { return m_requestCallback.get(); }
private:
    DatabaseLoader(ScriptExecutionContext* context, Ref<RequestDatabaseCallback>&& requestCallback)
        : ExecutableWithDatabase(context)
        , m_requestCallback(WTF::move(requestCallback)) { }
    Ref<RequestDatabaseCallback> m_requestCallback;
};

static RefPtr<IDBKey> idbKeyFromInspectorObject(InspectorObject* key)
{
    RefPtr<IDBKey> idbKey;

    String type;
    if (!key->getString("type", type))
        return nullptr;

#if !PLATFORM(WKC)
    NeverDestroyed<const String> numberType(ASCIILiteral("number"));
    NeverDestroyed<const String> stringType(ASCIILiteral("string"));
    NeverDestroyed<const String> dateType(ASCIILiteral("date"));
    NeverDestroyed<const String> arrayType(ASCIILiteral("array"));
#else
    WKC_DEFINE_STATIC_PTR(const String*, numberType_, new String(ASCIILiteral("number")));
    WKC_DEFINE_STATIC_PTR(const String*, stringType_, new String(ASCIILiteral("string")));
    WKC_DEFINE_STATIC_PTR(const String*, dateType_, new String(ASCIILiteral("date")));
    WKC_DEFINE_STATIC_PTR(const String*, arrayType_, new String(ASCIILiteral("array")));
    const String& numberType = *numberType_;
    const String& stringType = *stringType_;
    const String& dateType = *dateType_;
    const String& arrayType = *arrayType_;
#endif

    if (type == numberType) {
        double number;
        if (!key->getDouble("number", number))
            return nullptr;
        idbKey = IDBKey::createNumber(number);
    } else if (type == stringType) {
        String string;
        if (!key->getString("string", string))
            return nullptr;
        idbKey = IDBKey::createString(string);
    } else if (type == dateType) {
        double date;
        if (!key->getDouble("date", date))
            return nullptr;
        idbKey = IDBKey::createDate(date);
    } else if (type == arrayType) {
        IDBKey::KeyArray keyArray;
        RefPtr<InspectorArray> array;
        if (!key->getArray("array", array))
            return nullptr;
        for (size_t i = 0; i < array->length(); ++i) {
            RefPtr<InspectorValue> value = array->get(i);
            RefPtr<InspectorObject> object;
            if (!value->asObject(object))
                return nullptr;
            keyArray.append(idbKeyFromInspectorObject(object.get()));
        }
        idbKey = IDBKey::createArray(keyArray);
    } else
        return nullptr;

    return idbKey.release();
}

static RefPtr<IDBKeyRange> idbKeyRangeFromKeyRange(const InspectorObject* keyRange)
{
    RefPtr<InspectorObject> lower;
    if (!keyRange->getObject("lower", lower))
        return nullptr;
    RefPtr<IDBKey> idbLower = idbKeyFromInspectorObject(lower.get());
    if (!idbLower)
        return nullptr;

    RefPtr<InspectorObject> upper;
    if (!keyRange->getObject("upper", upper))
        return nullptr;
    RefPtr<IDBKey> idbUpper = idbKeyFromInspectorObject(upper.get());
    if (!idbUpper)
        return nullptr;

    bool lowerOpen;
    if (!keyRange->getBoolean("lowerOpen", lowerOpen))
        return nullptr;
    IDBKeyRange::LowerBoundType lowerBoundType = lowerOpen ? IDBKeyRange::LowerBoundOpen : IDBKeyRange::LowerBoundClosed;

    bool upperOpen;
    if (!keyRange->getBoolean("upperOpen", upperOpen))
        return nullptr;
    IDBKeyRange::UpperBoundType upperBoundType = upperOpen ? IDBKeyRange::UpperBoundOpen : IDBKeyRange::UpperBoundClosed;

    return IDBKeyRange::create(idbLower, idbUpper, lowerBoundType, upperBoundType);
}

class DataLoader;

class OpenCursorCallback : public EventListener {
public:
    static Ref<OpenCursorCallback> create(InjectedScript injectedScript, Ref<RequestDataCallback>&& requestCallback, int skipCount, unsigned pageSize)
    {
        return adoptRef(*new OpenCursorCallback(injectedScript, WTF::move(requestCallback), skipCount, pageSize));
    }

    virtual ~OpenCursorCallback() { }

    virtual bool operator==(const EventListener& other) override
    {
        return this == &other;
    }

    virtual void handleEvent(ScriptExecutionContext*, Event* event) override
    {
        if (event->type() != eventNames().successEvent) {
            m_requestCallback->sendFailure("Unexpected event type.");
            return;
        }

        IDBRequest* idbRequest = static_cast<IDBRequest*>(event->target());
        ExceptionCode ec = 0;
        RefPtr<IDBAny> requestResult = idbRequest->result(ec);
        if (ec) {
            m_requestCallback->sendFailure("Could not get result in callback.");
            return;
        }
        if (requestResult->type() == IDBAny::ScriptValueType) {
            end(false);
            return;
        }
        if (requestResult->type() != IDBAny::IDBCursorWithValueType) {
            m_requestCallback->sendFailure("Unexpected result type.");
            return;
        }

        RefPtr<IDBCursorWithValue> idbCursor = requestResult->idbCursorWithValue();

        if (m_skipCount) {
            ExceptionCode ec = 0;
            idbCursor->advance(m_skipCount, ec);
            if (ec)
                m_requestCallback->sendFailure("Could not advance cursor.");
            m_skipCount = 0;
            return;
        }

        if (m_result->length() == m_pageSize) {
            end(true);
            return;
        }

        // Continue cursor before making injected script calls, otherwise transaction might be finished.
        idbCursor->continueFunction(nullptr, ec);
        if (ec) {
            m_requestCallback->sendFailure("Could not continue cursor.");
            return;
        }

        RefPtr<DataEntry> dataEntry = DataEntry::create()
            .setKey(m_injectedScript.wrapObject(idbCursor->key(), String(), true))
            .setPrimaryKey(m_injectedScript.wrapObject(idbCursor->primaryKey(), String(), true))
            .setValue(m_injectedScript.wrapObject(idbCursor->value(), String(), true))
            .release();
        m_result->addItem(WTF::move(dataEntry));

    }

    void end(bool hasMore)
    {
        if (!m_requestCallback->isActive())
            return;
        m_requestCallback->sendSuccess(WTF::move(m_result), hasMore);
    }

private:
    OpenCursorCallback(InjectedScript injectedScript, Ref<RequestDataCallback>&& requestCallback, int skipCount, unsigned pageSize)
        : EventListener(EventListener::CPPEventListenerType)
        , m_injectedScript(injectedScript)
        , m_requestCallback(WTF::move(requestCallback))
        , m_skipCount(skipCount)
        , m_pageSize(pageSize)
        , m_result(Array<DataEntry>::create())
    {
    }
    InjectedScript m_injectedScript;
    Ref<RequestDataCallback> m_requestCallback;
    int m_skipCount;
    unsigned m_pageSize;
    Ref<Array<DataEntry>> m_result;
};

class DataLoader : public ExecutableWithDatabase {
public:
    static Ref<DataLoader> create(ScriptExecutionContext* context, Ref<RequestDataCallback>&& requestCallback, const InjectedScript& injectedScript, const String& objectStoreName, const String& indexName, RefPtr<IDBKeyRange>&& idbKeyRange, int skipCount, unsigned pageSize)
    {
        return adoptRef(*new DataLoader(context, WTF::move(requestCallback), injectedScript, objectStoreName, indexName, WTF::move(idbKeyRange), skipCount, pageSize));
    }

    virtual ~DataLoader() { }

    virtual void execute(RefPtr<IDBDatabase>&& database) override
    {
        if (!requestCallback().isActive())
            return;
        RefPtr<IDBTransaction> idbTransaction = transactionForDatabase(context(), database.get(), m_objectStoreName);
        if (!idbTransaction) {
            m_requestCallback->sendFailure("Could not get transaction");
            return;
        }
        RefPtr<IDBObjectStore> idbObjectStore = objectStoreForTransaction(idbTransaction.get(), m_objectStoreName);
        if (!idbObjectStore) {
            m_requestCallback->sendFailure("Could not get object store");
            return;
        }

        Ref<OpenCursorCallback> openCursorCallback = OpenCursorCallback::create(m_injectedScript, m_requestCallback.copyRef(), m_skipCount, m_pageSize);

        ExceptionCode ec = 0;
        RefPtr<IDBRequest> idbRequest;
        if (!m_indexName.isEmpty()) {
            RefPtr<IDBIndex> idbIndex = indexForObjectStore(idbObjectStore.get(), m_indexName);
            if (!idbIndex) {
                m_requestCallback->sendFailure("Could not get index");
                return;
            }

            idbRequest = idbIndex->openCursor(context(), m_idbKeyRange.copyRef(), ec);
        } else
            idbRequest = idbObjectStore->openCursor(context(), m_idbKeyRange.copyRef(), ec);
        idbRequest->addEventListener(eventNames().successEvent, WTF::move(openCursorCallback), false);
    }

    virtual RequestCallback& requestCallback() override { return m_requestCallback.get(); }
    DataLoader(ScriptExecutionContext* scriptExecutionContext, Ref<RequestDataCallback>&& requestCallback, const InjectedScript& injectedScript, const String& objectStoreName, const String& indexName, RefPtr<IDBKeyRange> idbKeyRange, int skipCount, unsigned pageSize)
        : ExecutableWithDatabase(scriptExecutionContext)
        , m_requestCallback(WTF::move(requestCallback))
        , m_injectedScript(injectedScript)
        , m_objectStoreName(objectStoreName)
        , m_indexName(indexName)
        , m_idbKeyRange(WTF::move(idbKeyRange))
        , m_skipCount(skipCount)
        , m_pageSize(pageSize) { }
    Ref<RequestDataCallback> m_requestCallback;
    InjectedScript m_injectedScript;
    String m_objectStoreName;
    String m_indexName;
    RefPtr<IDBKeyRange> m_idbKeyRange;
    int m_skipCount;
    unsigned m_pageSize;
};

} // namespace

InspectorIndexedDBAgent::InspectorIndexedDBAgent(InstrumentingAgents* instrumentingAgents, InjectedScriptManager* injectedScriptManager, InspectorPageAgent* pageAgent)
    : InspectorAgentBase(ASCIILiteral("IndexedDB"), instrumentingAgents)
    , m_injectedScriptManager(injectedScriptManager)
    , m_pageAgent(pageAgent)
{
}

InspectorIndexedDBAgent::~InspectorIndexedDBAgent()
{
}

void InspectorIndexedDBAgent::didCreateFrontendAndBackend(Inspector::FrontendChannel*, Inspector::BackendDispatcher* backendDispatcher)
{
    m_backendDispatcher = Inspector::IndexedDBBackendDispatcher::create(backendDispatcher, this);
}

void InspectorIndexedDBAgent::willDestroyFrontendAndBackend(Inspector::DisconnectReason)
{
    m_backendDispatcher = nullptr;

    ErrorString unused;
    disable(unused);
}

void InspectorIndexedDBAgent::enable(ErrorString&)
{
}

void InspectorIndexedDBAgent::disable(ErrorString&)
{
}

static Document* assertDocument(ErrorString& errorString, Frame* frame)
{
    Document* document = frame ? frame->document() : nullptr;
    if (!document)
        errorString = ASCIILiteral("No document for given frame found");
    return document;
}

static IDBFactory* assertIDBFactory(ErrorString& errorString, Document* document)
{
    DOMWindow* domWindow = document->domWindow();
    if (!domWindow) {
        errorString = ASCIILiteral("No IndexedDB factory for given frame found");
        return nullptr;
    }

    IDBFactory* idbFactory = DOMWindowIndexedDatabase::indexedDB(domWindow);
    if (!idbFactory)
        errorString = ASCIILiteral("No IndexedDB factory for given frame found");

    return idbFactory;
}

void InspectorIndexedDBAgent::requestDatabaseNames(ErrorString& errorString, const String& securityOrigin, Ref<RequestDatabaseNamesCallback>&& requestCallback)
{
    Frame* frame = m_pageAgent->findFrameWithSecurityOrigin(securityOrigin);
    Document* document = assertDocument(errorString, frame);
    if (!document)
        return;

    IDBFactory* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    ExceptionCode ec = 0;
    RefPtr<IDBRequest> idbRequest = idbFactory->getDatabaseNames(document, ec);
    if (!idbRequest || ec) {
        requestCallback->sendFailure("Could not obtain database names.");
        return;
    }

    idbRequest->addEventListener(eventNames().successEvent, GetDatabaseNamesCallback::create(WTF::move(requestCallback), document->securityOrigin()->toRawString()), false);
}

void InspectorIndexedDBAgent::requestDatabase(ErrorString& errorString, const String& securityOrigin, const String& databaseName, Ref<RequestDatabaseCallback>&& requestCallback)
{
    Frame* frame = m_pageAgent->findFrameWithSecurityOrigin(securityOrigin);
    Document* document = assertDocument(errorString, frame);
    if (!document)
        return;

    IDBFactory* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    Ref<DatabaseLoader> databaseLoader = DatabaseLoader::create(document, WTF::move(requestCallback));
    databaseLoader->start(idbFactory, &document->securityOrigin(), databaseName);
}

void InspectorIndexedDBAgent::requestData(ErrorString& errorString, const String& securityOrigin, const String& databaseName, const String& objectStoreName, const String& indexName, int skipCount, int pageSize, const InspectorObject* keyRange, Ref<RequestDataCallback>&& requestCallback)
{
    Frame* frame = m_pageAgent->findFrameWithSecurityOrigin(securityOrigin);
    Document* document = assertDocument(errorString, frame);
    if (!document)
        return;

    IDBFactory* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldExecState(frame));

    RefPtr<IDBKeyRange> idbKeyRange = keyRange ? idbKeyRangeFromKeyRange(keyRange) : nullptr;
    if (keyRange && !idbKeyRange) {
        errorString = ASCIILiteral("Can not parse key range.");
        return;
    }

    Ref<DataLoader> dataLoader = DataLoader::create(document, WTF::move(requestCallback), injectedScript, objectStoreName, indexName, WTF::move(idbKeyRange), skipCount, pageSize);
    dataLoader->start(idbFactory, &document->securityOrigin(), databaseName);
}

class ClearObjectStoreListener : public EventListener {
    WTF_MAKE_NONCOPYABLE(ClearObjectStoreListener);
public:
    static Ref<ClearObjectStoreListener> create(Ref<ClearObjectStoreCallback> requestCallback)
    {
        return adoptRef(*new ClearObjectStoreListener(WTF::move(requestCallback)));
    }

    virtual ~ClearObjectStoreListener() { }

    virtual bool operator==(const EventListener& other) override
    {
        return this == &other;
    }

    virtual void handleEvent(ScriptExecutionContext*, Event* event) override
    {
        if (!m_requestCallback->isActive())
            return;
        if (event->type() != eventNames().completeEvent) {
            m_requestCallback->sendFailure("Unexpected event type.");
            return;
        }

        m_requestCallback->sendSuccess();
    }
private:
    ClearObjectStoreListener(Ref<ClearObjectStoreCallback>&& requestCallback)
        : EventListener(EventListener::CPPEventListenerType)
        , m_requestCallback(WTF::move(requestCallback))
    {
    }

    Ref<ClearObjectStoreCallback> m_requestCallback;
};


class ClearObjectStore : public ExecutableWithDatabase {
public:
    static Ref<ClearObjectStore> create(ScriptExecutionContext* context, const String& objectStoreName, Ref<ClearObjectStoreCallback>&& requestCallback)
    {
        return adoptRef(*new ClearObjectStore(context, objectStoreName, WTF::move(requestCallback)));
    }

    ClearObjectStore(ScriptExecutionContext* context, const String& objectStoreName, Ref<ClearObjectStoreCallback>&& requestCallback)
        : ExecutableWithDatabase(context)
        , m_objectStoreName(objectStoreName)
        , m_requestCallback(WTF::move(requestCallback))
    {
    }

    virtual void execute(RefPtr<IDBDatabase>&& database) override
    {
        if (!requestCallback().isActive())
            return;
        RefPtr<IDBTransaction> idbTransaction = transactionForDatabase(context(), database.get(), m_objectStoreName, IDBTransaction::modeReadWrite());
        if (!idbTransaction) {
            m_requestCallback->sendFailure("Could not get transaction");
            return;
        }
        RefPtr<IDBObjectStore> idbObjectStore = objectStoreForTransaction(idbTransaction.get(), m_objectStoreName);
        if (!idbObjectStore) {
            m_requestCallback->sendFailure("Could not get object store");
            return;
        }

        ExceptionCode ec = 0;
        RefPtr<IDBRequest> idbRequest = idbObjectStore->clear(context(), ec);
        ASSERT(!ec);
        if (ec) {
            m_requestCallback->sendFailure(String::format("Could not clear object store '%s': %d", m_objectStoreName.utf8().data(), ec));
            return;
        }
        idbTransaction->addEventListener(eventNames().completeEvent, ClearObjectStoreListener::create(m_requestCallback.copyRef()), false);
    }

    virtual RequestCallback& requestCallback() override { return m_requestCallback.get(); }
private:
    const String m_objectStoreName;
    Ref<ClearObjectStoreCallback> m_requestCallback;
};

void InspectorIndexedDBAgent::clearObjectStore(ErrorString& errorString, const String& securityOrigin, const String& databaseName, const String& objectStoreName, Ref<ClearObjectStoreCallback>&& requestCallback)
{
    Frame* frame = m_pageAgent->findFrameWithSecurityOrigin(securityOrigin);
    Document* document = assertDocument(errorString, frame);
    if (!document)
        return;
    IDBFactory* idbFactory = assertIDBFactory(errorString, document);
    if (!idbFactory)
        return;

    Ref<ClearObjectStore> clearObjectStore = ClearObjectStore::create(document, objectStoreName, WTF::move(requestCallback));
    clearObjectStore->start(idbFactory, &document->securityOrigin(), databaseName);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
