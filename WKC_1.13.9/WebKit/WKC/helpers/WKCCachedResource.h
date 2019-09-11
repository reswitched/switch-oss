/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
    Copyright (c) 2010-2014 ACCESS CO., LTD. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _WKC_HELPERS_WKC_CACHEDRESOURCE_H_
#define _WKC_HELPERS_WKC_CACHEDRESOURCE_H_

#include <wkc/wkcbase.h>

namespace WKC {
class CachedResourcePrivate;

class WKC_API CachedResource {
public:
    enum Status {
        NotCached,    // this URL is not cached
        Unknown,      // let cache decide what to do with it
        New,          // inserting new item
        Pending,      // only partially loaded
        Cached        // regular case
    };

public:
    Status status() const;

    CachedResourcePrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    CachedResource(CachedResourcePrivate&);
    ~CachedResource();

private:
    CachedResource(const CachedResource&);
    CachedResource& operator=(const CachedResource&);

    CachedResourcePrivate& m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_CACHEDRESOURCE_H_
