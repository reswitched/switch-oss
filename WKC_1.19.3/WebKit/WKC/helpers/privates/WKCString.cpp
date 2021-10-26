/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2011 Torch Mobile, Inc.
 * Copyright (c) 2011-2021 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "CString.h"
#include "WTFString.h"
#include "StringImpl.h"

#include "helpers/WKCString.h"

#include <stdarg.h>

namespace WKC {

class CStringWrap : public CString {
WTF_MAKE_FAST_ALLOCATED;
public:
    CStringWrap(const char* data, int len) : CString(data, len) {}
    ~CStringWrap() {}

    using CString::operator new;
    using CString::operator new[];
};

CString::CString(const char* data, int len)
    : m_parent(new WTF::CString(data,len))
{
}

CString::~CString()
{
    delete static_cast<WTF::CString *>(m_parent);
}

CString&
CString::operator=(const CString& other)
{
    if (this != &other) {
        delete static_cast<WTF::CString *>(m_parent);
        m_parent = new WTF::CString(other.data(),other.length());
    }
    return *this;
}

void*
CString::operator new(size_t size)
{
    return WTF::fastMalloc(size);
}

void*
CString::operator new[](size_t size)
{
    return WTF::fastMalloc(size);
}

#define CPARENT() (static_cast<WTF::CString *>(m_parent))

const char*
CString::data() const
{
    return CPARENT()->data();
}


int 
CString::length() const
{
    return CPARENT()->length();
}

class StringPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    StringPrivate();
    ~StringPrivate();

    inline void setImpl(WTF::RefPtr<WTF::StringImpl> i) { if (i) m_impl = i; }
    inline WTF::StringImpl* impl() const { return m_impl.get(); }

    const unsigned short* charactersWithNullTermination();
    CString& latin1();
    CString& utf8();

    const unsigned short* characters16();

    inline void setDirection(TextDirection dir) { m_direction = dir; }
    inline TextDirection direction() const { return m_direction; }

private:
    WTF::RefPtr<WTF::StringImpl> m_impl;
    CStringWrap* m_latin1;
    CStringWrap* m_utf8;

    unsigned short* m_characters16;

    TextDirection m_direction;
};

StringPrivate::StringPrivate()
    : m_latin1(0)
    , m_utf8(0)
    , m_characters16(0)
    , m_direction(LTR)
{
}

StringPrivate::~StringPrivate()
{
    delete m_latin1;
    delete m_utf8;
    WTF::fastFree((void *)m_characters16);
}

const unsigned short*
StringPrivate::charactersWithNullTermination()
{
    return characters16();
}

CString&
StringPrivate::latin1()
{
    WTF::String a(m_impl.get());
    WTF::CString cs = a.latin1();

    if (m_latin1 && cs == m_latin1->data()) {
        return *m_latin1;
    }
    delete m_latin1;

    if (!cs.isNull()) {
        m_latin1 = new CStringWrap(cs.data(), cs.length());
    } else {
        m_latin1 = new CStringWrap(0,0);
    }
    return *m_latin1;
}

CString&
StringPrivate::utf8()
{
    WTF::String a(m_impl.get());
    WTF::CString cs = a.utf8();

    if (m_utf8 && cs == m_utf8->data()) {
       return *m_utf8;
    }
    delete m_utf8;

    if (!cs.isNull()) {
        m_utf8 = new CStringWrap(cs.data(), cs.length());
    } else {
        m_utf8 = new CStringWrap(0,0);
    }
    return *m_utf8;
}

const unsigned short*
StringPrivate::characters16()
{
    if (!m_impl)
        return 0;
    if (!m_impl->is8Bit())
        return m_impl->characters16();

    const LChar* src = m_impl->characters8();
    if (!src)
        return 0;
    size_t len = m_impl->length();
    if (!len)
        return 0;

    if (m_characters16)
        WTF::fastFree((void *)m_characters16);
    m_characters16 = (unsigned short *)WTF::fastMalloc((len+1)*sizeof(unsigned short));
    ::memset(m_characters16, 0, sizeof(unsigned short)*(len+1));

    for (size_t i=0; i<len; i++) {
        m_characters16[i] = (unsigned short)src[i];
    }
    return m_characters16;
}


#define IMPL() (m_private ? (m_private->impl()) : 0)

String::String()
    : m_private()
{
}

String::String(const char* str)
{
    m_private = new StringPrivate();
    WTF::RefPtr<WTF::StringImpl> s = WTF::StringImpl::create(str);
    m_private->setImpl(s);
}

String::String(const char* str, unsigned int len)
{
    m_private = new StringPrivate();
    WTF::RefPtr<WTF::StringImpl> s = WTF::StringImpl::create(str, len);
    m_private->setImpl(s);
}

String::String(const unsigned short* str)
{
    m_private = new StringPrivate();
    size_t len=0;
    const unsigned short* p = str;
    while (p && *p) {
        len++;
        p++;
    }

    WTF::RefPtr<WTF::StringImpl> s = WTF::StringImpl::create(str, len);
    m_private->setImpl(s);
}

String::String(const unsigned short* str, unsigned int len)
{
    m_private = new StringPrivate();
    WTF::RefPtr<WTF::StringImpl> s = WTF::StringImpl::create(str, len);
    m_private->setImpl(s);
}

String::String(const String& str)
{
    m_private = new StringPrivate();

    WTF::StringImpl* impl = str.impl()->impl();
    if (impl) {
        m_private->setImpl(impl);
    }
}

String::String(StringPrivate* parent)
{
    m_private = parent;
}

String::~String()
{
    delete m_private;
}

String&
String::operator=(const String& orig)
{
    if (this!=&orig) {
        if (!m_private)
            m_private = new StringPrivate();
        if (orig.m_private)
            m_private->setImpl(orig.m_private->impl());
    }
    return *this;
}

bool
String::operator==(const char* b) const
{
    const WTF::String a(IMPL());
    return (a==b);
}

bool
String::operator!=(const char* b) const
{
    const WTF::String a(IMPL());
    return (a!=b);
}


void
String::append(const String& str)
{
    WTF::String a(IMPL());
    a.append(str);
    m_private->setImpl(a.impl());
}


void
String::append(const char* str)
{
    WTF::String a(IMPL());
    a.append(str);
    m_private->setImpl(a.impl());
}

void
String::append(const unsigned short* str)
{
    WTF::String a(IMPL());
    a.append(str);
    m_private->setImpl(a.impl());
}

void 
String::append(const unsigned short* str, unsigned int len)
{
    WTF::String a(IMPL());
    a.append(str, len);
    m_private->setImpl(a.impl());
}

size_t
String::find(const String& str)
{
    const WTF::String a(IMPL());
    return a.find(str);
}

size_t
String::find(unsigned short ch, int len)
{
    const WTF::String a(IMPL());
    return a.find(ch, len);
}

size_t
String::reverseFind(unsigned short ch) const
{
    const WTF::String a(IMPL());
    size_t ret = a.reverseFind(ch);
    return ret == WTF::notFound ? WKC::notFound : ret;
}

String&
String::replace(const unsigned short* a, const unsigned short* b)
{
    WTF::String s(IMPL());
    s = s.replace(a, b);
    m_private->setImpl(s.impl());
    return *this;
}

String&
String::replace(const unsigned short* a, const String& b)
{
    WTF::String s(IMPL());
    WTF::String _b(b.impl());
    s = s.replace(a, _b);
    m_private->setImpl(s.impl());
    return *this;
}

String&
String::replace(const String& a, const String& b)
{
    WTF::String s(IMPL());
    WTF::String _a(a.impl());
    WTF::String _b(b.impl());
    s = s.replace(_a, _b);
    m_private->setImpl(s.impl());
    return *this;
}

String&
String::replace(unsigned index, unsigned len, const String& b)
{
    WTF::String s(IMPL());
    WTF::String _b(b.impl());
    s = s.replace(index, len, _b);
    m_private->setImpl(s.impl());
    return *this;
}

void
String::truncate(unsigned int len)
{
    WTF::String a(IMPL());
    a.truncate(len);
    m_private->setImpl(a.impl());
}

WKC::String
String::substring(unsigned int pos, unsigned int len) const
{
    WTF::String a(IMPL());
    WTF::String ret = a.substring(pos, len);
    if (ret.is8Bit())
        return String(reinterpret_cast<const char*>(ret.characters8()), ret.length());
    else
        return String(ret.characters16(), ret.length());
}

WKC::String
String::lower() const
{
    WTF::String a(IMPL());
    WTF::String ret = a.convertToASCIILowercase();
    if (ret.is8Bit())
        return String(reinterpret_cast<const char*>(ret.characters8()), ret.length());
    else
        return String(ret.characters16(), ret.length());
}

WKC::String
String::upper() const
{
    WTF::String a(IMPL());
    WTF::String ret = a.convertToASCIIUppercase();
    if (ret.is8Bit())
        return String(reinterpret_cast<const char*>(ret.characters8()), ret.length());
    else
        return String(ret.characters16(), ret.length());
}

WKC::String
String::fromUTF8(const char* utf8)
{
    return WTF::String::fromUTF8(utf8);
}

WKC::String
String::fromUTF8(const char* utf8, size_t len)
{
    return WTF::String::fromUTF8(utf8, len);
}

void
String::remove(unsigned int pos, unsigned int len)
{
    WTF::String a(IMPL());
    a.remove(pos, len);
    m_private->setImpl(a.impl());
}

void
String::insert(const unsigned short* str, unsigned int pos, unsigned int len)
{
    WTF::String a(IMPL());
    WTF::String s(str, len);
    a.insert(s, pos);
    m_private->setImpl(a.impl());
}

WKC::String
String::format(const char * format, ...)
{
    // copied from String.cpp

    va_list args;
    va_start(args, format);

    WTF::Vector<char, 256> buffer;

    // Do the format once to get the length.
#if COMPILER(MSVC)
    int result = _vscprintf(format, args);
#else
    char ch;
    int result = vsnprintf(&ch, 1, format, args);
    va_end(args);
    va_start(args, format);
#endif
    if (result==0) {
        return String("");
    } else if (result<0) {
        return String();
    }

    unsigned len = result;
    buffer.grow(len + 1);
    
    vsnprintf(buffer.data(), buffer.size(), format, args);

    va_end(args);

    return String(buffer.data(), len);
}



const unsigned short*
String::characters() const
{
    if (!m_private || !m_private->impl())
        return 0;
    if (IMPL()->is8Bit()) {
        return m_private->characters16();
    }
    return IMPL()->characters16();
}

const unsigned char*
String::characters8() const
{
    if (!m_private || !m_private->impl())
        return 0;
    if (!IMPL()->is8Bit())
        return 0;
    return IMPL()->characters8();
}

unsigned int
String::length() const
{
    if (!m_private || !m_private->impl())
        return 0;
    return IMPL()->length();
}

CString&
String::utf8() const
{
    return m_private->utf8();
}

CString&
String::latin1() const
{
    return m_private->latin1();
}

const unsigned short*
String::charactersWithNullTermination() const
{
    return m_private->charactersWithNullTermination();
}


bool 
String::isNull() const
{
    const WTF::String a(IMPL());
    return a.isNull();
}

bool 
String::isEmpty() const
{
    const WTF::String a(IMPL());
    return a.isEmpty();
}

bool
String::endsWith(const String& str) const
{
    const WTF::String a(IMPL());
    return a.endsWith(str);
}

TextDirection
String::direction() const
{
    return m_private->direction();
}

void
String::setDirection(TextDirection dir)
{
    m_private->setDirection(dir);
}


} // namespace

namespace WTF {
String::String(const WKC::String& str)
    : m_impl(0)
{
    if (str.impl()) {
        m_impl = str.impl()->impl();
    }
}

String::operator ::WKC::String() const
{
    ::WKC::StringPrivate* obj = new ::WKC::StringPrivate();

    obj->setImpl(impl());

    return ::WKC::String(obj);
}

} // namespace
