/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCEvent.h"
#include "helpers/privates/WKCEventPrivate.h"

#include "helpers/privates/WKCEventTargetPrivate.h"

#include "Event.h"

namespace WKC {

EventPrivate::EventPrivate(WebCore::Event* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_target(0)
{
}

EventPrivate::~EventPrivate()
{
    delete m_target;
}

void
EventPrivate::setDefaultHandled()
{
    m_webcore->setDefaultHandled();
}

EventTarget*
EventPrivate::target()
{
    WebCore::EventTarget* t = m_webcore->target();
    if (!t)
        return 0;
    if (!m_target || m_target->webcore()!=t) {
        delete m_target;
        m_target = new EventTargetPrivate(t);
    }
    return &m_target->wkc();
}

////////////////////////////////////////////////////////////////////////////////

Event::Event(EventPrivate& parent)
    : m_private(parent)
{
}

Event::~Event()
{
}

void
Event::setDefaultHandled()
{
    priv().setDefaultHandled();
}

EventTarget*
Event::target() const
{
    return priv().target();
}

} // namespace
