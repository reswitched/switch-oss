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

#include "helpers/WKCQualifiedName.h"
#include "helpers/privates/WKCQualifiedNamePrivate.h"

#include "helpers/WKCString.h"
#include "helpers/WKCAtomString.h"
#include "helpers/privates/WKCAtomStringPrivate.h"

#include "QualifiedName.h"

namespace WKC {

QualifiedNamePrivate::QualifiedNamePrivate(const AtomString& p, const String& l, const AtomString& n)
    : m_webcore(new WebCore::QualifiedName(*(p.priv()->webcore()), WTF::String(l), *(n.priv()->webcore())))
    , m_webcore_local_name(l)
    , m_webcore_prefix(p.priv()->webcore())
    , m_webcore_namespace(n.priv()->webcore())
{
}

QualifiedNamePrivate::~QualifiedNamePrivate()
{
    delete m_webcore;
}

QualifiedNamePrivate::QualifiedNamePrivate(const QualifiedNamePrivate& other)
#if 1 // keep the former behavior
    : m_webcore(new WebCore::QualifiedName(*other.m_webcore_prefix, other.m_webcore_local_name, *other.m_webcore_namespace))
    , m_webcore_local_name(other.m_webcore_local_name)
    , m_webcore_prefix(0)
    , m_webcore_namespace(0)
#else
    : m_webcore(new WebCore::QualifiedName(*(other.m_webcore)))
    , m_webcore_local_name(other.m_webcore_local_name)
    , m_webcore_prefix(other.m_webcore_prefix)
    , m_webcore_namespace(other.m_webcore_namespace)
#endif
{
}

////////////////////////////////////////////////////////////////////////////////

QualifiedName::QualifiedName(const AtomString& p, const String& l, const AtomString& n)
{
    m_private = new QualifiedNamePrivate(p, l, n);
}

QualifiedName::~QualifiedName()
{
    delete m_private;
}

QualifiedName::QualifiedName(const QualifiedName& other)
{
    m_private = new QualifiedNamePrivate(*(other.m_private));
}

QualifiedName&
QualifiedName::operator=(const QualifiedName& other)
{
    if (this!=&other) {
        delete m_private;
        m_private = new QualifiedNamePrivate(*(other.m_private));
    }
    return *this;
}

QualifiedNamePrivate*
QualifiedName::priv() const
{
    return m_private;
}

} // namespece
