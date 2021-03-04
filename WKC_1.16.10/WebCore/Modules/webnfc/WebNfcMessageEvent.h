/*
 *  WebNfcMessageEvent.h
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

#ifndef WebNfcMessageEvent_h
#define WebNfcMessageEvent_h

#if ENABLE(WKC_WEB_NFC)

#include "Event.h"

namespace WebCore {

class WebNfcMessage;

class WebNfcMessageEvent : public Event {
public:
    ~WebNfcMessageEvent();
    static Ref<WebNfcMessageEvent> create();
    static Ref<WebNfcMessageEvent> create(const AtomicString&, RefPtr<WebNfcMessage>);

    virtual EventInterface eventInterface() const override { return WebNfcMessageEventInterfaceType; };

    WebNfcMessage* message() const { return m_message.get(); }

private:
    WebNfcMessageEvent();
    WebNfcMessageEvent(const AtomicString&, RefPtr<WebNfcMessage>);

    RefPtr<WebNfcMessage> m_message;
};

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfcMessageEvent_h
