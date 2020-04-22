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

#ifndef _WKC_HELPERS_PRIVATE_RESOURCEERROR_H_
#define _WKC_HELPERS_PRIVATE_RESOURCEERROR_H_

#include "helpers/WKCResourceError.h"
#include "helpers/WKCString.h"

namespace WebCore {
class ResourceError;
} // namespace

namespace WKC {
class ResourceHandle;

class ResourceErrorWrap : public ResourceError {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceErrorWrap(ResourceErrorPrivate* parent) : ResourceError(parent) {}
    ~ResourceErrorWrap() {}

    using ResourceError::operator new;
    using ResourceError::operator new[];
};

class ResourceErrorPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceErrorPrivate(const WebCore::ResourceError&); // instance from core to app
    ResourceErrorPrivate(const ResourceError&, const String& domain, int errorCode,
        const String& failingURL, const String& localizedDescription,
        ResourceHandle* resourceHandle); // instance from app to core
    ~ResourceErrorPrivate();

    ResourceErrorPrivate(const ResourceErrorPrivate&);

    const WebCore::ResourceError& webcore() const { return *m_webcore; }
    const ResourceError& wkc() const { return *m_wkc; }

    bool isNull() const;
    int errorCode() const;
    bool isCancellation() const;

    const String& failingURL();
    const String& domain();
    const String& localizedDescription();

    int   contentComposition() const;

private:
    ResourceErrorPrivate& operator=(const ResourceErrorPrivate&); // not needed

    const WebCore::ResourceError* m_webcore;
    const ResourceErrorWrap* m_wkc;
    const bool m_isWebcoreOwned;
    const bool m_isWkcOwned;

    String m_failingURL;
    String m_domain;
    String m_localizedDescription;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_RESOURCEERROR_H_
