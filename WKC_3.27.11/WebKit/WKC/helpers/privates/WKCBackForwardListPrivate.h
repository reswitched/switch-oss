/*
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

#ifndef _WKC_PRIVATE_BACKFORWARDLIST_H_
#define _WKC_PRIVATE_BACKFORWARDLIST_H_

#include "helpers/WKCBackForwardList.h"

namespace WebCore {
class BackForwardClient;
} // namespace

namespace WKC {

class HistoryItem;
class HistoryItemPrivate;

class BackForwardListWrap : public BackForwardList {
WTF_MAKE_FAST_ALLOCATED;
public:
    BackForwardListWrap(BackForwardListPrivate& parent) : BackForwardList(parent) {}
    ~BackForwardListWrap() {}
};

class BackForwardListPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    BackForwardListPrivate(WebCore::BackForwardClient*);
    ~BackForwardListPrivate();

    WebCore::BackForwardClient* webcore() const { return m_webcore; }
    BackForwardList& wkc() { return m_wkc; }

    HistoryItem* itemAtIndex(int);
    void addItem(HistoryItem*);

    int backListCount();
    int forwardListCount();

private:
    WebCore::BackForwardClient* m_webcore;
    BackForwardListWrap m_wkc;

    HistoryItemPrivate* m_itemAtIndex;
};

} // namespace

#endif // _WKC_PRIVATE_BACKFORWARDLIST_H_
