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

#include "config.h"

#include "helpers/WKCResourceError.h"
#include "helpers/privates/WKCResourceErrorPrivate.h"

#include "ResourceError.h"
#include "ResourceHandleInternalWKC.h"
#include "WTFString.h"
#include "helpers/WKCString.h"
#include "helpers/WKCKURL.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCResourceHandlePrivate.h"

namespace WKC {

ResourceErrorPrivate::ResourceErrorPrivate(const WebCore::ResourceError& parent)
    : m_webcore(&parent)
    , m_wkc(new ResourceErrorWrap(this))
    , m_isWebcoreOwned(false)
    , m_isWkcOwned(true)
    , m_failingURL()
    , m_domain()
    , m_localizedDescription()
{
}

ResourceErrorPrivate::ResourceErrorPrivate(const ResourceError& error,
        const String& domain, int errorCode, const String& failingURL,
        const String& localizedDescription, ResourceErrorType type,  ResourceHandle* resourceHandle)
    : m_wkc(static_cast<const ResourceErrorWrap*>(&error))
    , m_isWebcoreOwned(true)
    , m_isWkcOwned(false)
    , m_failingURL()
    , m_domain()
    , m_localizedDescription()
{
    WebCore::ResourceErrorBase::Type type_webcore = toWebCoreResourceErrorType(type);
    m_webcore = new WebCore::ResourceError(domain, errorCode, KURL(WKCURLParsed, failingURL.utf8().data()),
        localizedDescription, type_webcore, resourceHandle ? resourceHandle->priv().webcore() : 0);

}

ResourceErrorPrivate::~ResourceErrorPrivate()
{
    if (m_isWebcoreOwned)
        delete m_webcore;
    if (m_isWkcOwned)
        delete m_wkc;
}

ResourceErrorPrivate::ResourceErrorPrivate(const ResourceErrorPrivate& other)
    : m_webcore(other.m_isWebcoreOwned ? new WebCore::ResourceError(*(other.m_webcore)) : other.m_webcore)
    , m_wkc(other.m_isWkcOwned ? new ResourceErrorWrap(this) : other.m_wkc)
    , m_isWebcoreOwned(other.m_isWebcoreOwned)
    , m_isWkcOwned(other.m_isWkcOwned)
    , m_failingURL(other.m_failingURL)
    , m_domain(other.m_domain)
    , m_localizedDescription(other.m_localizedDescription)
{
}

ResourceErrorPrivate&
ResourceErrorPrivate::operator =(const ResourceErrorPrivate& other)
{
    ASSERT_NOT_REACHED(); // Implement correctly when this method is needed.
    return *this;
}

bool
ResourceErrorPrivate::isNull() const
{
    return webcore().isNull();
}

int
ResourceErrorPrivate::errorCode() const
{
    return webcore().errorCode();
}

bool
ResourceErrorPrivate::isCancellation() const
{
    return webcore().isCancellation();
}

ResourceErrorType
ResourceErrorPrivate::type() const
{
    return toWKCResourceErrorType(webcore().type());
}

const String&
ResourceErrorPrivate::failingURL()
{
    m_failingURL = webcore().failingURL().string();
    return m_failingURL;
}

const String&
ResourceErrorPrivate::domain()
{
    m_domain = webcore().domain();
    return m_domain;
}

const String&
ResourceErrorPrivate::localizedDescription()
{
    m_localizedDescription = webcore().localizedDescription();
    return m_localizedDescription;
}

int
ResourceErrorPrivate::contentComposition() const
{
    return webcore().m_composition;
}

////////////////////////////////////////////////////////////////////////////////

ResourceError::ResourceError(ResourceErrorPrivate* parent)
    : m_private(parent)
    , m_owned(false)
{
}

ResourceError::ResourceError(const String& domain, int errorCode, const String& failingURL, const String& localizedDescription, ResourceErrorType type, ResourceHandle* resourceHandle)
{
    m_private = new ResourceErrorPrivate(*this, domain, errorCode, failingURL, localizedDescription, type, resourceHandle);
    m_owned = true;
}

ResourceError::~ResourceError()
{
    if (m_owned)
        delete m_private;
}

ResourceError::ResourceError(const ResourceError& other)
    : m_private(other.m_private)
    , m_owned(other.m_owned)
{
    if (m_owned)
        m_private = new ResourceErrorPrivate(*(other.m_private)); // create my own copy
}

ResourceError&
ResourceError::operator=(const ResourceError& other)
{
    if (this != &other) {
        m_owned = other.m_owned;
        if (m_owned) {
            delete m_private;
            m_private = new ResourceErrorPrivate(*(other.m_private)); // create my own copy
        } else {
            m_private = other.m_private;
        }
    }
    return *this;
}

void*
ResourceError::operator new(size_t size)
{
    return WTF::fastMalloc(size);
}

void*
ResourceError::operator new[](size_t size)
{
    return WTF::fastMalloc(size);
}

bool
ResourceError::isNull() const
{
    return m_private->isNull();
}

int
ResourceError::errorCode() const
{
    return m_private->errorCode();
}

bool
ResourceError::isCancellation() const
{
    return m_private->isCancellation();
}

const String&
ResourceError::failingURL() const
{
    return m_private->failingURL();
}

const String&
ResourceError::domain() const
{
    return m_private->domain();
}

const String&
ResourceError::localizedDescription() const
{
    return m_private->localizedDescription();
}

ResourceErrorType
ResourceError::type() const
{
    return m_private->type();
}

int
ResourceError::contentComposition() const
{
    return m_private->contentComposition();
}

} // namespace
