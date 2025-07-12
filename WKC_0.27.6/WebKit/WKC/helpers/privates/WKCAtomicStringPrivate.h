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

#ifndef _WKC_HELPERS_PRIVATE_ATOMICSTRING_H_
#define _WKC_HELPERS_PRIVATE_ATOMICSTRING_H_

#include "helpers/WKCAtomicString.h"
#include "helpers/WKCString.h"

namespace WTF {
class String;
class AtomicString;
} // namespace

namespace WKC {

class AtomicStringWrap : public AtomicString {
WTF_MAKE_FAST_ALLOCATED;
public:
    AtomicStringWrap(AtomicStringPrivate* priv) : AtomicString(priv) {}
    ~AtomicStringWrap() {}
};

class AtomicStringPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    AtomicStringPrivate(WTF::AtomicString* value);
    ~AtomicStringPrivate();

    AtomicStringPrivate(const AtomicStringPrivate&);

    WTF::AtomicString* webcore(){ return m_webcore; }
    AtomicString& wkc() const { return *m_wkc; }

    String& string();

private:
    AtomicStringPrivate& operator =(const AtomicStringPrivate&); // not needed

    WTF::AtomicString* m_webcore;
    AtomicStringWrap* m_wkc;


    String m_wkc_string;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_ATOMICSTRING_H_