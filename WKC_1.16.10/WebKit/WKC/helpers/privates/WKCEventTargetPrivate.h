/*
 *  Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef _WKC_HELPERS_PRIVATE_EVENTTARGET_H_
#define _WKC_HELPERS_PRIVATE_EVENTTARGET_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCEventTarget.h"

namespace WebCore {
class EventTarget;
} // namespace

namespace WKC {

class EventTarget;
class NodePrivate;

class EventTargetWrap : public EventTarget {
WTF_MAKE_FAST_ALLOCATED;
public:
    EventTargetWrap(EventTargetPrivate& parent) : EventTarget(parent) {}
    ~EventTargetWrap() {}
};

class EventTargetPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    EventTargetPrivate(WebCore::EventTarget*);
    ~EventTargetPrivate();

    WebCore::EventTarget* webcore() const { return m_webcore; }
    EventTarget& wkc() { return m_wkc; }

    Node* toNode();

private:
    WebCore::EventTarget* m_webcore;
    EventTargetWrap m_wkc;

    NodePrivate* m_node;
};


} // namespace

#endif // _WKC_HELPERS_PRIVATE_EVENTTARGET_H_
