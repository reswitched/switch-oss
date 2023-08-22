/*
 * Copyright (c) 2011-2017 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPER_WKC_STRING_H_
#define _WKC_HELPER_WKC_STRING_H_

#include <wkc/wkcbase.h>

#include <stddef.h>

#include "helpers/WKCHelpersEnums.h"

namespace WKC {

const size_t notFound = static_cast<size_t>(-1);

class WKC_API CString {
public:
    CString(const char*, int);
    ~CString();

    CString& operator=(const CString&);

    const char* data() const;
    int length() const;

protected:
    // Heap allocation by operator new in applications is disallowed (allowed only in WKC).
    // This restriction is to avoid memory leaks or crashes.
    void* operator new(size_t);
    void* operator new[](size_t);

private:
    CString(const CString&); // not needed for now

    void* m_parent;
};

class StringPrivate;

class WKC_API String {
public:
    String();
    String(const String&);
    String(const char*);
    String(const char*, unsigned int);
    String(const unsigned short*);
    String(const unsigned short*, unsigned int);
    String(StringPrivate*);
    ~String();

    TextDirection direction() const;
    void setDirection(TextDirection);

    String& operator=(const String&);

    bool operator==(const char* b) const;
    bool operator!=(const char* b) const;

    void append(const String&);
    void append(const char*);
    void append(const unsigned short*);
    void append(const unsigned short*, unsigned int);

    size_t find(const String&);
    size_t find(unsigned short ch, int);
    size_t reverseFind(unsigned short ch) const;

    String& replace(const unsigned short* a, const unsigned short* b);
    String& replace(const unsigned short* a, const String& b);
    String& replace(const String& a, const String& b);
    String& replace(unsigned index, unsigned len, const String& b);

    void truncate(unsigned int);

    void insert(const unsigned short*, unsigned int, unsigned int);
    void remove(unsigned int, unsigned int);

    String substring(unsigned int, unsigned int) const;

    String lower() const;
    String upper() const;

    static String fromUTF8(const char*);
    static String fromUTF8(const char*, size_t);

    static String format(const char *, ...);

    const unsigned short* characters() const;
    const unsigned char* characters8() const;
    unsigned int length() const;

    CString& utf8() const;
    CString& latin1() const;

    const unsigned short* charactersWithNullTermination() const;

    bool isNull() const;
    bool isEmpty() const;

    bool endsWith(const String&) const;

    StringPrivate* impl() const { return m_private; } 

private:
    StringPrivate* m_private;
};
} // namespace
#endif // _WKC_HELPER_WKC_STRING_H_
