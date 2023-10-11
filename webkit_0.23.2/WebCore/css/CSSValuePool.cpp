/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CSSValuePool.h"

#include "CSSParser.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"

namespace WebCore {

CSSValuePool& cssValuePool()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<CSSValuePool> pool;
    return pool;
#else
    WKC_DEFINE_STATIC_PTR(CSSValuePool*, pool, new CSSValuePool());
    return *pool;
#endif
}

CSSValuePool::CSSValuePool()
    : m_inheritedValue(CSSInheritedValue::create())
    , m_implicitInitialValue(CSSInitialValue::createImplicit())
    , m_explicitInitialValue(CSSInitialValue::createExplicit())
    , m_unsetValue(CSSUnsetValue::create())
    , m_revertValue(CSSRevertValue::create())
    , m_colorTransparent(CSSPrimitiveValue::createColor(Color::transparent))
    , m_colorWhite(CSSPrimitiveValue::createColor(Color::white))
    , m_colorBlack(CSSPrimitiveValue::createColor(Color::black))
{
}

Ref<CSSPrimitiveValue> CSSValuePool::createIdentifierValue(CSSValueID ident)
{
    if (!ident)
        return CSSPrimitiveValue::createIdentifier(ident);

    RELEASE_ASSERT(ident > 0 && ident < numCSSValueKeywords);
    if (!m_identifierValueCache[ident])
        m_identifierValueCache[ident] = CSSPrimitiveValue::createIdentifier(ident);
    return *m_identifierValueCache[ident];
}

Ref<CSSPrimitiveValue> CSSValuePool::createIdentifierValue(CSSPropertyID ident)
{
    return CSSPrimitiveValue::createIdentifier(ident);
}

Ref<CSSPrimitiveValue> CSSValuePool::createColorValue(unsigned rgbValue)
{
    // These are the empty and deleted values of the hash table.
    if (rgbValue == Color::transparent)
        return m_colorTransparent.copyRef();
    if (rgbValue == Color::white)
        return m_colorWhite.copyRef();
    // Just because it is common.
    if (rgbValue == Color::black)
        return m_colorBlack.copyRef();

    // Remove one entry at random if the cache grows too large.
    const int maximumColorCacheSize = 512;
    if (m_colorValueCache.size() >= maximumColorCacheSize)
        m_colorValueCache.remove(m_colorValueCache.begin());

    ColorValueCache::AddResult entry = m_colorValueCache.add(rgbValue, nullptr);
    if (entry.isNewEntry)
        entry.iterator->value = CSSPrimitiveValue::createColor(rgbValue);
    return *entry.iterator->value;
}

Ref<CSSPrimitiveValue> CSSValuePool::createValue(double value, CSSPrimitiveValue::UnitTypes type)
{
    ASSERT(std::isfinite(value));

    if (value < 0 || value > maximumCacheableIntegerValue)
        return CSSPrimitiveValue::create(value, type);

    int intValue = static_cast<int>(value);
    if (value != intValue)
        return CSSPrimitiveValue::create(value, type);

    RefPtr<CSSPrimitiveValue>* cache;
    switch (type) {
    case CSSPrimitiveValue::CSS_PX:
        cache = m_pixelValueCache;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        cache = m_percentValueCache;
        break;
    case CSSPrimitiveValue::CSS_NUMBER:
        cache = m_numberValueCache;
        break;
    default:
        return CSSPrimitiveValue::create(value, type);
    }

    if (!cache[intValue])
        cache[intValue] = CSSPrimitiveValue::create(value, type);
    return *cache[intValue];
}

Ref<CSSPrimitiveValue> CSSValuePool::createFontFamilyValue(const String& familyName, FromSystemFontID fromSystemFontID)
{
    // Remove one entry at random if the cache grows too large.
    const int maximumFontFamilyCacheSize = 128;
    if (m_fontFamilyValueCache.size() >= maximumFontFamilyCacheSize)
        m_fontFamilyValueCache.remove(m_fontFamilyValueCache.begin());

    bool isFromSystemID = fromSystemFontID == FromSystemFontID::Yes;
    RefPtr<CSSPrimitiveValue>& value = m_fontFamilyValueCache.add({familyName, isFromSystemID}, nullptr).iterator->value;
    if (!value)
        value = CSSPrimitiveValue::create(CSSFontFamily{familyName, isFromSystemID});
    return *value;
}

PassRefPtr<CSSValueList> CSSValuePool::createFontFaceValue(const AtomicString& string)
{
    // Remove one entry at random if the cache grows too large.
    const int maximumFontFaceCacheSize = 128;
    if (m_fontFaceValueCache.size() >= maximumFontFaceCacheSize)
        m_fontFaceValueCache.remove(m_fontFaceValueCache.begin());

    RefPtr<CSSValueList>& value = m_fontFaceValueCache.add(string, nullptr).iterator->value;
    if (!value)
        value = CSSParser::parseFontFaceValue(string);
    return value;
}

void CSSValuePool::drain()
{
    m_colorValueCache.clear();
    m_fontFaceValueCache.clear();
    m_fontFamilyValueCache.clear();

    for (int i = 0; i < numCSSValueKeywords; ++i)
        m_identifierValueCache[i] = nullptr;

    for (int i = 0; i < maximumCacheableIntegerValue; ++i) {
        m_pixelValueCache[i] = nullptr;
        m_percentValueCache[i] = nullptr;
        m_numberValueCache[i] = nullptr;
    }
}

}
