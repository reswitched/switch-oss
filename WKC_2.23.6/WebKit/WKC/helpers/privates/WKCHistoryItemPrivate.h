/*
 * Copyright (c) 2011-2022 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_HISTORYITEM_H_
#define _WKC_HELPERS_PRIVATE_HISTORYITEM_H_

#include "helpers/WKCHistoryItem.h"
#include "helpers/WKCString.h"

namespace WebCore {
class HistoryItem;
}

namespace WTF {
namespace Persistence {
class Encoder;
}
}

namespace WKC {
class ImagePrivate;

class HistoryItemWrap : public HistoryItem {
WTF_MAKE_FAST_ALLOCATED;
public:
    HistoryItemWrap(HistoryItemPrivate& parent) : HistoryItem(parent) {}
    ~HistoryItemWrap() {}
};

class HistoryItemPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HistoryItemPrivate(WebCore::HistoryItem*);
    ~HistoryItemPrivate();

    WebCore::HistoryItem* webcore() const { return m_webcore; }
    HistoryItem& wkc() { return m_wkc; }

    const String& title();
    const String& urlString();
    const String& originalURLString();
    bool isInBackForwardCache() const;
    WKCPoint scrollPoint() const;
    void setScrollPoint(const WKCPoint& point);

    void setURL(const KURL&);
    void setURLString(const String&);

    void setItemSequenceNumber(long long number);
    long long itemSequenceNumber() const;

    void setDocumentSequenceNumber(long long number);
    long long documentSequenceNumber() const;

    void setStateObject(const uint8_t* data, size_t size);
    void stateObject(const uint8_t** data, size_t* size);

    int refCount() const;

private:
    WebCore::HistoryItem* m_webcore;
    HistoryItemWrap m_wkc;

    String m_title;
    String m_urlString;
    String m_originalURLString;
    WTF::Persistence::Encoder* m_encoder;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_HISTORYITEM_H_

