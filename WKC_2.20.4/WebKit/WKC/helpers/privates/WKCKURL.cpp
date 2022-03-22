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
#include "URL.h"
#include "URLParser.h"
#include "TextEncoding.h"
#include "helpers/WKCKURL.h"
#include "helpers/WKCString.h"

#define PARENT() ((WTF::URL *)m_parent)

namespace WKC {

KURL::KURL(KURLPrivate* parent)
    : m_parent((KURLPrivate *)new WTF::URL({}, ((WTF::URL *)parent)->string()))
{
}

KURL::KURL()
    : m_parent((KURLPrivate *)new WTF::URL())
{
}

KURL::KURL(const KURL& base, const char* str)
    : m_parent((KURLPrivate *)new WTF::URL(*(WTF::URL*)(&base), str))
{
}

KURL::KURL(const KURL& url)
    : m_parent((KURLPrivate *)new WTF::URL({}, ((WTF::URL *)url.parent())->string()))
{
}

KURL::KURL(WKCURLParsedEnum, const char* url)
    : m_parent((KURLPrivate *)new WTF::URL({}, url))
{
}

KURL::KURL(const KURL& base, const String& relative)
    : m_parent((KURLPrivate *)new WTF::URL(base, relative))
{
}

KURL::~KURL()
{
    delete (WTF::URL *)m_parent;
}

KURL&
KURL::operator=(const KURL& orig)
{
    if (this!=&orig) {
        delete (WTF::URL *)m_parent;
        m_parent = (KURLPrivate *)new WTF::URL({} , ((WTF::URL *)orig.parent())->string());
    }
    return *this;
}


KURL::operator String() const
{
    return string();
}

const String
KURL::string() const
{
    return PARENT()->string();
}

const String
WKC::KURL::protocol() const
{
    return PARENT()->protocol().toString();
}

const String
WKC::KURL::host() const
{
    return PARENT()->host().toString();
}

unsigned short
WKC::KURL::port() const
{
    return PARENT()->port().valueOr(0);
}

const String
WKC::KURL::path() const
{
    return PARENT()->path();
}

const String
WKC::KURL::lastPathComponent() const
{
    return PARENT()->lastPathComponent();
}

bool
WKC::KURL::setProtocol(const String& str)
{
    return PARENT()->setProtocol(str);
}

String
decodeURLEscapeSequences(const String& str)
{
    return WebCore::decodeURLEscapeSequences(str);
}

String
encodeWithURLEscapeSequences(const String& str)
{
    return WTF::encodeWithURLEscapeSequences(str);
}

bool
protocolIs(const String& url, const char* protocol)
{
    return WTF::URL({}, url).protocolIs(protocol);
}

} // namespace

namespace WTF {
URL::URL(const WKC::KURL& url)
{
    WTF::URL* parent = (WTF::URL *)url.parent();
    if (parent) {
        URLParser parser(parent->string());
        *this = parser.result();
    } else {
        invalidate();
    }
}

URL::operator ::WKC::KURL() const
{
    return ::WKC::KURL((::WKC::KURLPrivate *)this);
}
} // namespace
