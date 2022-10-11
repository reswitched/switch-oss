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

#include "config.h"

#include "helpers/WKCHistoryItem.h"
#include "helpers/privates/WKCHistoryItemPrivate.h"

#include "HistoryItem.h"
#include "helpers/privates/WKCImagePrivate.h"

#include "SharedBuffer.h"
#include "URL.h"

namespace WKC {
HistoryItemPrivate::HistoryItemPrivate(WebCore::HistoryItem* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

HistoryItemPrivate::~HistoryItemPrivate()
{
}


const String&
HistoryItemPrivate::title()
{
    m_title = m_webcore->title();
    return m_title;
}

const String&
HistoryItemPrivate::urlString()
{
    m_urlString = m_webcore->urlString();
    return m_urlString;
}

const String&
HistoryItemPrivate::originalURLString()
{
    m_originalURLString = m_webcore->originalURLString();
    return m_originalURLString;
}

void
HistoryItemPrivate::setURL(const KURL& url)
{
    m_webcore->setURL(url);
}

void
HistoryItemPrivate::setURLString(const String& str)
{
    m_webcore->setURLString(str);
}

int
HistoryItemPrivate::refCount() const
{
    return m_webcore->refCount();
}

bool
HistoryItemPrivate::isInBackForwardCache() const
{
    return m_webcore->isInBackForwardCache();
}

WKCPoint
HistoryItemPrivate::scrollPoint() const
{
    return m_webcore->scrollPosition();
}

void
HistoryItemPrivate::setScrollPoint(const WKCPoint& point)
{
    m_webcore->setScrollPosition(point);
}

void
HistoryItemPrivate::setViewState(void* state)
{
    m_webcore->setViewState(state); 
}

void*
HistoryItemPrivate::viewState() const
{ 
    return m_webcore->viewState();
}

////////////////////////////////////////////////////////////////////////////////

HistoryItem*
HistoryItem::create(const String& urlString, const String& title)
{
    RefPtr<WebCore::HistoryItem> item = WebCore::HistoryItem::create(urlString, title);
    HistoryItemPrivate wobj(item.get());
    void* p = WTF::fastMalloc(sizeof(HistoryItem));
    return new (p) HistoryItem(&wobj.wkc(), true);
}

HistoryItem*
HistoryItem::create(HistoryItem* parent, bool needsRef)
{
    void* p = WTF::fastMalloc(sizeof(HistoryItem));
    return new (p) HistoryItem(parent, needsRef);
}

void
HistoryItem::destroy(HistoryItem* self)
{
    if (!self)
        return;
    self->~HistoryItem();
    WTF::fastFree(self);
}

HistoryItem::HistoryItem(HistoryItemPrivate& parent)
    : m_ownedPrivate(0)
    , m_private(parent)
    , m_needsRef(false)
{
}

HistoryItem::HistoryItem(HistoryItem* parent, bool needsRef)
    : m_ownedPrivate(new HistoryItemPrivate(parent->priv().webcore()))
    , m_private(*m_ownedPrivate)
    , m_needsRef(needsRef)
{
    if (needsRef)
        m_private.webcore()->ref();
}

HistoryItem::~HistoryItem()
{
    if (m_needsRef)
        m_private.webcore()->deref();
    if (m_ownedPrivate)
        delete m_ownedPrivate;
}

bool
HistoryItem::compare(const HistoryItem* other) const
{
    if (this==other)
        return true;
    if (!this || !other)
        return false;
    if (m_private.webcore() == other->m_private.webcore())
        return true;
    return false;
}

const String&
HistoryItem::urlString() const
{
    return m_private.urlString();
}

const String&
HistoryItem::originalURLString() const
{
    return m_private.originalURLString();
}

const String&
HistoryItem::title() const
{
    return m_private.title();
}

void
HistoryItem::setURL(const KURL& url)
{
    m_private.setURL(url);
}

void
HistoryItem::setURLString(const String& str)
{
    m_private.setURLString(str);
}

int
HistoryItem::refCount() const
{
    return m_private.refCount();
}

bool
HistoryItem::isInBackForwardCache() const
{
    return m_private.isInBackForwardCache();
}

WKCPoint
HistoryItem::scrollPoint() const
{
    return m_private.scrollPoint();
}

void
HistoryItem::setScrollPoint(const WKCPoint& point)
{
    m_private.setScrollPoint(point);
}

void
HistoryItem::setViewState(void* state)
{
    m_private.setViewState(state);
}

void*
HistoryItem::viewState() const
{ 
    return m_private.viewState();
}

} // namespace
