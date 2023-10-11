/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2013 Apple Inc. All rights reserved.
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
#include "CSSImageValue.h"

#include "CSSCursorImageValue.h"
#include "CSSParser.h"
#include "CSSValueKeywords.h"
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CachedResourceRequestInitiators.h"
#include "CrossOriginAccessControl.h"
#include "Document.h"
#include "Element.h"
#include "MemoryCache.h"
#include "StyleCachedImage.h"
#include "StylePendingImage.h"

namespace WebCore {

CSSImageValue::CSSImageValue(const String& url)
    : CSSValue(ImageClass)
    , m_url(url)
    , m_accessedImage(false)
{
}

CSSImageValue::CSSImageValue(const String& url, StyleImage* image)
    : CSSValue(ImageClass)
    , m_url(url)
    , m_image(image)
    , m_accessedImage(true)
{
}

inline void CSSImageValue::detachPendingImage()
{
    if (is<StylePendingImage>(m_image.get()))
        downcast<StylePendingImage>(*m_image).detachFromCSSValue();
}

CSSImageValue::~CSSImageValue()
{
    detachPendingImage();
}

StyleImage* CSSImageValue::cachedOrPendingImage()
{
    if (!m_image)
        m_image = StylePendingImage::create(this);

    return m_image.get();
}

StyleCachedImage* CSSImageValue::cachedImage(CachedResourceLoader& loader, const ResourceLoaderOptions& options)
{
    if (!m_accessedImage) {
        m_accessedImage = true;

        CachedResourceRequest request(ResourceRequest(loader.document()->completeURL(m_url)), options);
        if (m_initiatorName.isEmpty())
            request.setInitiator(cachedResourceRequestInitiators().css);
        else
            request.setInitiator(m_initiatorName);

        if (options.requestOriginPolicy() == PotentiallyCrossOriginEnabled)
            updateRequestForAccessControl(request.mutableResourceRequest(), loader.document()->securityOrigin(), options.allowCredentials());

        if (CachedResourceHandle<CachedImage> cachedImage = loader.requestImage(request)) {
            detachPendingImage();
            m_image = StyleCachedImage::create(cachedImage.get());
        }
    }

    return is<StyleCachedImage>(m_image.get()) ? downcast<StyleCachedImage>(m_image.get()) : nullptr;
}

bool CSSImageValue::traverseSubresources(const std::function<bool (const CachedResource&)>& handler) const
{
    if (!is<StyleCachedImage>(m_image.get()))
        return false;
    CachedResource* cachedResource = downcast<StyleCachedImage>(*m_image).cachedImage();
    ASSERT(cachedResource);
    return handler(*cachedResource);
}

bool CSSImageValue::equals(const CSSImageValue& other) const
{
    return m_url == other.m_url;
}

String CSSImageValue::customCSSText() const
{
    return "url(" + quoteCSSURLIfNeeded(m_url) + ')';
}

PassRefPtr<CSSValue> CSSImageValue::cloneForCSSOM() const
{
    // NOTE: We expose CSSImageValues as URI primitive values in CSSOM to maintain old behavior.
    RefPtr<CSSPrimitiveValue> uriValue = CSSPrimitiveValue::create(m_url, CSSPrimitiveValue::CSS_URI);
    uriValue->setCSSOMSafe();
    return uriValue.release();
}

bool CSSImageValue::knownToBeOpaque(const RenderElement* renderer) const
{
    return m_image ? m_image->knownToBeOpaque(renderer) : false;
}

} // namespace WebCore
