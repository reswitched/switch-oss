/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBCursor_h
#define IDBCursor_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBKey.h"
#include "IDBTransaction.h"
#include "IndexedDB.h"
#include "ScriptWrappable.h"
#include <bindings/ScriptValue.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class DOMRequestState;
class IDBAny;
class IDBCallbacks;
class IDBCursorBackend;
class IDBRequest;
class ScriptExecutionContext;

typedef int ExceptionCode;

class IDBCursor : public ScriptWrappable, public RefCounted<IDBCursor> {
public:
    static const AtomicString& directionNext();
    static const AtomicString& directionNextUnique();
    static const AtomicString& directionPrev();
    static const AtomicString& directionPrevUnique();

    static IndexedDB::CursorDirection stringToDirection(const String& modeString, ExceptionCode&);
    static const AtomicString& directionToString(IndexedDB::CursorDirection mode);

    static Ref<IDBCursor> create(PassRefPtr<IDBCursorBackend>, IndexedDB::CursorDirection, IDBRequest*, IDBAny* source, IDBTransaction*);
    virtual ~IDBCursor();

    // Implement the IDL
    const String& direction() const;
    const Deprecated::ScriptValue& key() const;
    const Deprecated::ScriptValue& primaryKey() const;
    const Deprecated::ScriptValue& value() const;
    IDBAny* source() const;

    PassRefPtr<IDBRequest> update(JSC::ExecState*, Deprecated::ScriptValue&, ExceptionCode&);
    void advance(unsigned long, ExceptionCode&);
    // FIXME: Try to modify the code generator so this overload is unneeded.
    void continueFunction(ScriptExecutionContext*, ExceptionCode& ec) { continueFunction(static_cast<IDBKey*>(nullptr), ec); }
    void continueFunction(ScriptExecutionContext*, const Deprecated::ScriptValue& key, ExceptionCode&);
    PassRefPtr<IDBRequest> deleteFunction(ScriptExecutionContext*, ExceptionCode&);

    void continueFunction(PassRefPtr<IDBKey>, ExceptionCode&);
    void postSuccessHandlerCallback();
    void close();
    void setValueReady(DOMRequestState*, PassRefPtr<IDBKey>, PassRefPtr<IDBKey> primaryKey, Deprecated::ScriptValue&);
    PassRefPtr<IDBKey> idbPrimaryKey() { return m_currentPrimaryKey; }

protected:
    IDBCursor(PassRefPtr<IDBCursorBackend>, IndexedDB::CursorDirection, IDBRequest*, IDBAny* source, IDBTransaction*);
    virtual bool isKeyCursor() const { return true; }

private:
    PassRefPtr<IDBObjectStore> effectiveObjectStore();

    RefPtr<IDBCursorBackend> m_backend;
    RefPtr<IDBRequest> m_request;
    const IndexedDB::CursorDirection m_direction;
    RefPtr<IDBAny> m_source;
    RefPtr<IDBTransaction> m_transaction;
    IDBTransaction::OpenCursorNotifier m_transactionNotifier;
    bool m_gotValue;
    // These values are held because m_backend may advance while they
    // are still valid for the current success handlers.
    Deprecated::ScriptValue m_currentKeyValue;
    Deprecated::ScriptValue m_currentPrimaryKeyValue;
    RefPtr<IDBKey> m_currentKey;
    RefPtr<IDBKey> m_currentPrimaryKey;
    Deprecated::ScriptValue m_currentValue;
};

} // namespace WebCore

#endif

#endif // IDBCursor_h
