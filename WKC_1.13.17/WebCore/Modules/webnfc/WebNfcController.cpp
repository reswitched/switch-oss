/*
 *  WebNfcController.cpp
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

#include "config.h"
#include "WebNfcController.h"

#if ENABLE(WKC_WEB_NFC)

#include "WebNfcClient.h"
#include "WebNfcAdapter.h"
#include "WebNfcMessage.h"
#include "WebNfcMessageEvent.h"
#include "Timer.h"

#include "helpers/WKCString.h"

#include "JSDOMPromise.h"
#include "JSDOMError.h"
#include "JSWebNfcAdapter.h"
#include "JSBlob.h"

namespace WebCore {

WebNfcController::WebNfcController(WebNfcClient* client)
    : m_client(client)
    , m_document(0)
    , m_promiseHolderVector(0)
    , m_requestAdapterTimer(*this, &WebNfcController::requestAdapterProc)
    , m_sendTimer(*this, &WebNfcController::sendProc)
    , m_adapter(0)
{
    ASSERT(m_client);
    m_client->setController(this);
}

WebNfcController::~WebNfcController()
{
    m_client->webNfcControllerDestroyed();
    if (m_adapter) {
        m_adapter->webNfcControllerDestroyed();
    }
    m_requestAdapterTimer.stop();
    m_sendTimer.stop();
}

const char* WebNfcController::supplementName()
{
    return "WebNfcController";
}

WebNfcController* WebNfcController::from(Page* page)
{
    return static_cast<WebNfcController*>(Supplement<Page>::from(page, supplementName()));
}

void provideWebNfcTo(Page* page, WebNfcClient* client)
{
    Supplement<Page>::provideTo(page, WebNfcController::supplementName(), std::make_unique<WebNfcController>(client));
}

void WebNfcController::requestAdapter(DeferredWrapper* deferredWrapper)
{
    int id = m_promiseHolderVector.size();
    m_promiseHolderVector.append(new PromiseHolder(id, deferredWrapper));
    m_requestAdapterTimer.startOneShot(0);
}

void WebNfcController::requestAdapterProc()
{
    for (int i = 0; i < m_promiseHolderVector.size(); ++i) {
        PromiseHolder*& holder = m_promiseHolderVector.at(i);
        if (!holder->wasRequested() && holder->type() == WebNfcRequestTypeRequestAdapter) {
            if (m_client->requestPermission()) {
                m_client->requestAdapter(holder->id());
            } else {
                rejectPromise(holder, "Security Error");
            }
            holder->setRequested();
        }
    }
}

void WebNfcController::send(DeferredWrapper* deferredWrapper, RefPtr<WebNfcMessage> message, const String& target)
{
    int id = m_promiseHolderVector.size();
    m_promiseHolderVector.append(new PromiseHolder(id, deferredWrapper, message, target));
    m_sendTimer.startOneShot(0);
}

void WebNfcController::sendProc()
{
    for (int i = 0; i < m_promiseHolderVector.size(); ++i) {
        PromiseHolder*& holder = m_promiseHolderVector.at(i);
        if (!holder->wasRequested() && holder->type() == WebNfcRequestTypeSend) {
            if (m_client->requestPermission()) {
                m_client->send(holder->id(), holder->message(), holder->target());
            } else {
                rejectPromise(holder, "Security Error");
            }
            holder->setRequested();
        }
    }
}

void WebNfcController::startRequestMessage()
{
    m_client->startRequestMessage();
}

void WebNfcController::stopRequestMessage()
{
    m_client->stopRequestMessage();
}

void WebNfcController::updateAdapter(int id, int result)
{
    PromiseHolder*& promiseHolder = m_promiseHolderVector.at(id);

    String errorString;
    switch (result) {
    case WebNfcPromiseResultOK:
        break;
    case WebNfcPromiseResultNotSupportedError:
        errorString = "NotSupportedError";
        break;
    default:
        errorString = "UndefinedError";
        break;
    }

    if (!errorString.isEmpty()) {
        rejectPromise(promiseHolder, errorString);
        return;
    }

    if (m_adapter) {
        m_adapter->webNfcControllerDestroyed();
        m_adapter.release();
    }
    m_adapter = WebNfcAdapter::create(m_document, this);

    promiseHolder->deferredWrapper()->resolve(m_adapter.get());
    promiseHolder->setFinished();
    resetPromiseHolder();
}

void WebNfcController::updateMessage(RefPtr<WebNfcMessage> message)
{
    RefPtr<Event> event = WebNfcMessageEvent::create(eventNames().messageEvent, message);
    m_adapter->didChangeMessage(event);
}

void WebNfcController::notifySendResult(int id, int result)
{
    PromiseHolder*& promiseHolder = m_promiseHolderVector.at(id);

    String errorString;
    switch (result) {
    case WebNfcPromiseResultOK:
        break;
    case WebNfcPromiseResultNotSupportedError:
        errorString = "NotSupportedError";
        break;
    case WebNfcPromiseResultTimeoutError:
        errorString = "TimeoutError";
        break;
    case WebNfcPromiseResultInvalidAccessError:
        errorString = "InvalidAccessError";
        break;
    default:
        errorString = "UndefinedError";
        break;
    }

    if (!errorString.isEmpty()) {
        rejectPromise(promiseHolder, errorString);
        return;
    }

    promiseHolder->deferredWrapper()->resolve(true);
    promiseHolder->setFinished();
    resetPromiseHolder();
}

void WebNfcController::resetPromiseHolder()
{
    for (PromiseHolderVector::iterator it = m_promiseHolderVector.begin(); it != m_promiseHolderVector.end(); ++it) {
        if (!(*it)->wasFinished()) {
            return;
        }
    }
    for (PromiseHolderVector::iterator it = m_promiseHolderVector.begin(); it != m_promiseHolderVector.end(); ++it) {
        delete (*it);
    }
    m_promiseHolderVector.clear();
}

void WebNfcController::rejectPromise(PromiseHolder* promiseHolder, const String& errorString)
{
    RefPtr<DOMError> error = DOMError::create(errorString);
    promiseHolder->deferredWrapper()->reject(error.get());
    promiseHolder->setFinished();
    resetPromiseHolder();
}

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
