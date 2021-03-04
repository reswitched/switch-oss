/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_HISTORYITEM_H_
#define _WKC_HELPERS_WKC_HISTORYITEM_H_

#include <wkc/wkcbase.h>

namespace WKC {
class HistoryItemPrivate;

class String;
class Image;
class KURL;

class WKC_API HistoryItem {
public:
    static HistoryItem* create(const String& urlString, const String& title);
    static HistoryItem* create(HistoryItem*, bool needsRef = false);
    static void destroy(HistoryItem* self);

    bool compare(const HistoryItem*) const;

    const String& title() const;
    const String& urlString() const;
    const String& originalURLString() const;
    bool isInPageCache() const;
    WKCPoint scrollPoint() const;
    void setScrollPoint(const WKCPoint& point);

    void setURL(const KURL&);
    void setURLString(const String&);

    void setViewState(void* state);
    void* viewState() const;

    int refCount() const;

    HistoryItemPrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    HistoryItem(HistoryItemPrivate&);
    HistoryItem(HistoryItem*, bool needsRef);
    ~HistoryItem();

private:
    HistoryItem(const HistoryItem&);
    HistoryItem& operator=(const HistoryItem&);

private:
    HistoryItemPrivate* m_ownedPrivate;
    HistoryItemPrivate& m_private;
    bool m_needsRef;
};
}

#endif // _WKC_HELPERS_WKC_HISTORYITEM_H_

