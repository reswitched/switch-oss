/*
 * Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCAtomicString.h"
#include "helpers/privates/WKCAtomicStringPrivate.h"

#include "helpers/WKCString.h"

#include "AtomicString.h"

namespace WKC {
    std::aligned_storage<sizeof(AtomicStringPrivate)>::type nullAtomStorage;
} // namespace

namespace WKC {

AtomicStringPrivate::AtomicStringPrivate(WTF::AtomicString* value)
    : m_webcore(value)
    , m_wkc(new AtomicStringWrap(this))
    , m_wkc_string()
{
}

AtomicStringPrivate::~AtomicStringPrivate()
{
    delete m_wkc;
}

AtomicStringPrivate::AtomicStringPrivate(const AtomicStringPrivate& other)
    : m_webcore(other.m_webcore)
    , m_wkc(new AtomicStringWrap(this))
    , m_wkc_string()
{
}

AtomicStringPrivate&
AtomicStringPrivate::operator =(const AtomicStringPrivate& other)
{
    ASSERT_NOT_REACHED(); // Implement correctly when this method is needed.
    return *this;
}

String&
AtomicStringPrivate::string()
{
    m_wkc_string = m_webcore->string();
    return m_wkc_string;
}

////////////////////////////////////////////////////////////////////////////////

AtomicString::AtomicString(AtomicStringPrivate* priv)
    : m_private(priv)
{
}

AtomicString::~AtomicString()
{
}

AtomicString::AtomicString(const AtomicString& other)
    : m_private(new AtomicStringPrivate(*(other.m_private)))
{
}

AtomicString&
AtomicString::operator =(const AtomicString& other)
{
    if (this != &other) {
        delete m_private;
        m_private = new AtomicStringPrivate(*(other.m_private));
    }
    return *this;
}

const String&
AtomicString::string() const
{
    return m_private->string();
}

const AtomicString&
AtomicString::nullAtom()
{
    WKC_DEFINE_STATIC_TYPE(AtomicStringPrivate*, nullAtom, new (&nullAtomStorage) AtomicStringPrivate(const_cast<WTF::AtomicString*>(&WTF::nullAtom())));

    return nullAtom->wkc();
}

} // namespace