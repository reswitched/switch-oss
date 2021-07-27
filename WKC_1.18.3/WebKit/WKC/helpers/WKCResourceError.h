/*
 * Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_RESOURCEERROR_H_
#define _WKC_HELPERS_WKC_RESOURCEERROR_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHelpersEnums.h"

namespace WKC {
class String;
class ResourceHandle;
class ResourceErrorPrivate;

class WKC_API ResourceError {
public:
    ResourceError(const String& domain, int errorCode, const String& failingURL, const String& localizedDescription, ResourceErrorType type, ResourceHandle* resourceHandle);
    ~ResourceError();

    ResourceError(const ResourceError&);
    ResourceError& operator=(const ResourceError&);

    bool isNull() const;
    int errorCode() const;
    bool isCancellation() const;

    const String& failingURL() const;
    const String& domain() const;
    const String& localizedDescription() const;
    ResourceErrorType type() const;

    int contentComposition() const;

    ResourceErrorPrivate& priv() const { return *m_private; }

protected:
    ResourceError(ResourceErrorPrivate*); // Not for applications, only for WKC.

    // Heap allocation by operator new in applications is disallowed (allowed only in WKC).
    // This restriction is to avoid memory leaks or crashes.
    // Some functions such as WKC::FrameLoaderClientIf::cancelledError() return
    // instances of this class as values, so constructor/destructor must be public.
    void* operator new(size_t);
    void* operator new[](size_t);

private:
    ResourceErrorPrivate* m_private;
    bool m_owned;
};
}

#endif // _WKC_HELPERS_WKC_RESOURCEERROR_H_
