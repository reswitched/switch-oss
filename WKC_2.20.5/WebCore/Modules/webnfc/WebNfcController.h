/*
 *  WebNfcController.h
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef WebNfcController_h
#define WebNfcController_h

#if ENABLE(WKC_WEB_NFC)

#include "Page.h"
#include "Document.h"
#include "WebNfcMessage.h"

#include "JSDOMPromise.h"

namespace WebCore {

class WebNfcClient;
class WebNfcAdapter;
class DeferredWrapper;
class Dictionary;
class Timer;

enum WebNfcRequestType {
    WebNfcRequestTypeRequestAdapter,
    WebNfcRequestTypeSend,
};
enum WebNfcPromiseResult {
    WebNfcPromiseResultOK,
    WebNfcPromiseResultSecurityError,
    WebNfcPromiseResultNotSupportedError,
    WebNfcPromiseResultTimeoutError,
    WebNfcPromiseResultInvalidAccessError,
};

class PromiseHolder {
public:
    PromiseHolder(int id, DeferredWrapper* deferredWrapper)
        : m_type(WebNfcRequestTypeRequestAdapter)
        , m_id(id)
        , m_deferredWrapper(deferredWrapper)
        , m_message(0)
        , m_target(String())
        , m_requested(false)
        , m_finished(false) { }
    PromiseHolder(int id, DeferredWrapper* deferredWrapper, RefPtr<WebNfcMessage> message, const String& target)
        : m_type(WebNfcRequestTypeSend)
        , m_id(id)
        , m_deferredWrapper(deferredWrapper)
        , m_message(message)
        , m_target(target)
        , m_requested(false)
        , m_finished(false) { }
    ~PromiseHolder() { delete m_deferredWrapper; }

    WebNfcRequestType type() { return m_type; }
    int id() { return m_id; }
    DeferredWrapper* deferredWrapper() { return m_deferredWrapper; }
    RefPtr<WebNfcMessage> message() { return m_message; }
    const String& target() { return m_target; }
    bool wasRequested() { return m_requested; }
    bool wasFinished() { return m_finished; }

    void setRequested() { m_requested = true; }
    void setFinished() { m_finished = true; }

private:
    WebNfcRequestType m_type;
    int m_id;
    DeferredWrapper* m_deferredWrapper;
    RefPtr<WebNfcMessage> m_message; // only use send() method
    const String m_target; // only use send() method
    bool m_requested;
    bool m_finished;
};

class WebNfcController : public Supplement<Page> {
public:
    explicit WebNfcController(WebNfcClient*);
    ~WebNfcController();

    static const char* supplementName();
    static WebNfcController* from(Page*);

    void setDocument(Document* document) { m_document = document; }
    Document* document() { return m_document; }

    void requestAdapter(DeferredWrapper*);
    void requestAdapterProc();
    void startRequestMessage();
    void stopRequestMessage();
    void send(DeferredWrapper*, RefPtr<WebNfcMessage>, const String&);
    void sendProc();

    void updateAdapter(int, int);
    void updateMessage(RefPtr<WebNfcMessage>);
    void notifySendResult(int, int);

private:
    typedef Vector<PromiseHolder*> PromiseHolderVector;

    WebNfcClient* client() { return m_client; }
    PromiseHolderVector& promiseHolderVector() { return m_promiseHolderVector; }
    void resetPromiseHolder();
    void rejectPromise(PromiseHolder*, const String&);

    WebNfcClient* m_client;
    Document* m_document;
    PromiseHolderVector m_promiseHolderVector;
    Timer m_requestAdapterTimer;
    Timer m_sendTimer;
    RefPtr<WebNfcAdapter> m_adapter;
};

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfcController_h
