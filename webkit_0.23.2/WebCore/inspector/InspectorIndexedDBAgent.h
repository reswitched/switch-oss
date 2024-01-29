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

#ifndef InspectorIndexedDBAgent_h
#define InspectorIndexedDBAgent_h

#if ENABLE(INDEXED_DATABASE)

#include "InspectorWebAgentBase.h"
#if !PLATFORM(WKC)
#include <inspector/InspectorBackendDispatchers.h>
#else
#include <InspectorBackendDispatchers.h>
#endif
#include <wtf/text/WTFString.h>

namespace Inspector {
class InjectedScriptManager;
}

namespace WebCore {

class InspectorPageAgent;

typedef String ErrorString;

class InspectorIndexedDBAgent final : public InspectorAgentBase, public Inspector::IndexedDBBackendDispatcherHandler {
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorIndexedDBAgent(InstrumentingAgents*, Inspector::InjectedScriptManager*, InspectorPageAgent*);
    virtual ~InspectorIndexedDBAgent();

    virtual void didCreateFrontendAndBackend(Inspector::FrontendChannel*, Inspector::BackendDispatcher*) override;
    virtual void willDestroyFrontendAndBackend(Inspector::DisconnectReason) override;

    // Called from the front-end.
    virtual void enable(ErrorString&) override;
    virtual void disable(ErrorString&) override;
    virtual void requestDatabaseNames(ErrorString&, const String& securityOrigin, Ref<RequestDatabaseNamesCallback>&&) override;
    virtual void requestDatabase(ErrorString&, const String& securityOrigin, const String& databaseName, Ref<RequestDatabaseCallback>&&) override;
    virtual void requestData(ErrorString&, const String& securityOrigin, const String& databaseName, const String& objectStoreName, const String& indexName, int skipCount, int pageSize, const Inspector::InspectorObject* keyRange, Ref<RequestDataCallback>&&) override;
    virtual void clearObjectStore(ErrorString&, const String& in_securityOrigin, const String& in_databaseName, const String& in_objectStoreName, Ref<ClearObjectStoreCallback>&&) override;

private:
    Inspector::InjectedScriptManager* m_injectedScriptManager;
    InspectorPageAgent* m_pageAgent;
    RefPtr<Inspector::IndexedDBBackendDispatcher> m_backendDispatcher;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
#endif // !defined(InspectorIndexedDBAgent_h)
