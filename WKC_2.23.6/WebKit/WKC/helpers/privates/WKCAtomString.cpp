/*
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCAtomString.h"
#include "helpers/privates/WKCAtomStringPrivate.h"

#include "helpers/WKCString.h"

#include "AtomString.h"

namespace WKC {
    std::aligned_storage<sizeof(AtomStringPrivate)>::type nullAtomStorage;
} // namespace

namespace WKC {

AtomStringPrivate::AtomStringPrivate(WTF::AtomString* value)
    : m_webcore(value)
    , m_wkc(new AtomStringWrap(this))
    , m_wkc_string()
{
}

AtomStringPrivate::~AtomStringPrivate()
{
    delete m_wkc;
}

AtomStringPrivate::AtomStringPrivate(const AtomStringPrivate& other)
    : m_webcore(other.m_webcore)
    , m_wkc(new AtomStringWrap(this))
    , m_wkc_string()
{
}

AtomStringPrivate&
AtomStringPrivate::operator =(const AtomStringPrivate& other)
{
    ASSERT_NOT_REACHED(); // Implement correctly when this method is needed.
    return *this;
}

String&
AtomStringPrivate::string()
{
    m_wkc_string = m_webcore->string();
    return m_wkc_string;
}

////////////////////////////////////////////////////////////////////////////////

AtomString::AtomString(AtomStringPrivate* priv)
    : m_private(priv)
{
}

AtomString::~AtomString()
{
}

AtomString::AtomString(const AtomString& other)
    : m_private(new AtomStringPrivate(*(other.m_private)))
{
}

AtomString&
AtomString::operator =(const AtomString& other)
{
    if (this != &other) {
        delete m_private;
        m_private = new AtomStringPrivate(*(other.m_private));
    }
    return *this;
}

const String&
AtomString::string() const
{
    return m_private->string();
}

const AtomString&
AtomString::nullAtom()
{
    WKC_DEFINE_STATIC_TYPE(AtomStringPrivate*, nullAtom, new (&nullAtomStorage) AtomStringPrivate(const_cast<WTF::AtomString*>(&WTF::nullAtom())));

    return nullAtom->wkc();
}

} // namespace