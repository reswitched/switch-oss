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

#include "helpers/WKCAttribute.h"
#include "helpers/privates/WKCAttributePrivate.h"

#include "helpers/privates/WKCAtomStringPrivate.h"

#include "Attribute.h"
#include "AtomString.h"

namespace WKC {

AttributePrivate::AttributePrivate(const WebCore::Attribute *webcore)
    : m_webcore(webcore)
    , m_value()
    , m_wkc(*this)
    , m_atomicstring_priv(0)
{
}

AttributePrivate::~AttributePrivate()
{
    if (m_atomicstring_priv)
        delete m_atomicstring_priv;
}

const AtomString&
AttributePrivate::value()
{
    m_value = m_webcore->value();

    if (m_atomicstring_priv)
        delete m_atomicstring_priv;

    m_atomicstring_priv = new AtomStringPrivate(&m_value);
    return m_atomicstring_priv->wkc();
}

Attribute::Attribute(AttributePrivate& parent)
    : m_private(parent)
{
}

Attribute::~Attribute()
{
}

const AtomString&
Attribute::value() const
{
    return m_private.value();
}

} // namespace
