/*
 *  WebNfcAdapter.h
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

#ifndef WebNfcAdapter_h
#define WebNfcAdapter_h

#if ENABLE(WKC_WEB_NFC)

#include "ActiveDOMObject.h"
#include "EventTarget.h"

#include "WebNfcController.h"

namespace WebCore {

class ScriptExecutionContext;
class Document;
class DeferredWrapper;
class WebNfcMessage;

class WebNfcAdapter : public ActiveDOMObject, public RefCounted<WebNfcAdapter>, public EventTarget {
public:
    virtual ~WebNfcAdapter();
    static Ref<WebNfcAdapter> create(Document*, WebNfcController*);

    // EventTarget implementation
    virtual EventTargetInterface eventTargetInterface() const override { return WebNfcAdapterEventTargetInterfaceType; }
    virtual ScriptExecutionContext* scriptExecutionContext() const override { return ActiveDOMObject::scriptExecutionContext(); }

    EventListener* onmessage() { return getAttributeEventListener(eventNames().messageEvent); }
    void setOnmessage(RefPtr<EventListener> listener) {
        if (setAttributeEventListener(eventNames().messageEvent, listener)) {
            m_controller->startRequestMessage();
        } else {
            m_controller->stopRequestMessage();
        }
    }
    void send(DeferredWrapper*, RefPtr<WebNfcMessage>, const String&);

    Document* document();

    void didChangeMessage(RefPtr<Event>);
    void webNfcControllerDestroyed() { m_controller = 0; }

    using RefCounted<WebNfcAdapter>::ref;
    using RefCounted<WebNfcAdapter>::deref;

    // ActiveDOMObject implementation.
    virtual bool canSuspendForPageCache() const override { return true; }
    virtual void suspend(ReasonForSuspension) override { };
    virtual void resume() override { };
    virtual void stop() override { };
    virtual const char* activeDOMObjectName() const override { return "WebNfc"; }

protected:
    virtual EventTargetData* eventTargetData() override { return &m_eventTargetData; }
    virtual EventTargetData& ensureEventTargetData() override { return m_eventTargetData; }

private:
    explicit WebNfcAdapter(Document*, WebNfcController*);

    // EventTarget implementation.
    virtual void refEventTarget() override { ref(); }
    virtual void derefEventTarget() override { deref(); }

    WebNfcController* m_controller;
    EventTargetData m_eventTargetData;
};

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfcAdapter_h
