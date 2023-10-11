/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2010, 2014 Apple Inc. All rights reserved.
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
#include "StyledElement.h"

#include "CSSImageValue.h"
#include "CSSParser.h"
#include "CSSStyleSheet.h"
#include "CSSValuePool.h"
#include "ContentSecurityPolicy.h"
#include "DOMTokenList.h"
#include "HTMLElement.h"
#include "HTMLParserIdioms.h"
#include "InspectorInstrumentation.h"
#include "PropertySetCSSStyleDeclaration.h"
#include "ScriptableDocumentParser.h"
#include "StyleProperties.h"
#include "StyleResolver.h"
#include <wtf/HashFunctions.h>

namespace WebCore {

COMPILE_ASSERT(sizeof(StyledElement) == sizeof(Element), styledelement_should_remain_same_size_as_element);

using namespace HTMLNames;

struct PresentationAttributeCacheKey {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    PresentationAttributeCacheKey() : tagName(0) { }
    AtomicStringImpl* tagName;
    // Only the values need refcounting.
    Vector<std::pair<AtomicStringImpl*, AtomicString>, 3> attributesAndValues;
};

struct PresentationAttributeCacheEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PresentationAttributeCacheKey key;
    RefPtr<StyleProperties> value;
};

typedef HashMap<unsigned, std::unique_ptr<PresentationAttributeCacheEntry>, AlreadyHashed> PresentationAttributeCache;
    
static bool operator!=(const PresentationAttributeCacheKey& a, const PresentationAttributeCacheKey& b)
{
    if (a.tagName != b.tagName)
        return true;
    return a.attributesAndValues != b.attributesAndValues;
}

static PresentationAttributeCache& presentationAttributeCache()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<PresentationAttributeCache> cache;
    return cache;
#else
    WKC_DEFINE_STATIC_PTR(PresentationAttributeCache*, cache, 0);
    if (!cache)
        cache = new PresentationAttributeCache();
    return *cache;
#endif
}

class PresentationAttributeCacheCleaner {
    WTF_MAKE_NONCOPYABLE(PresentationAttributeCacheCleaner); WTF_MAKE_FAST_ALLOCATED;
public:
    PresentationAttributeCacheCleaner()
        : m_hitCount(0)
        , m_cleanTimer(*this, &PresentationAttributeCacheCleaner::cleanCache)
    {
    }

    void didHitPresentationAttributeCache()
    {
        if (presentationAttributeCache().size() < minimumPresentationAttributeCacheSizeForCleaning)
            return;

        m_hitCount++;

        if (!m_cleanTimer.isActive())
            m_cleanTimer.startOneShot(presentationAttributeCacheCleanTimeInSeconds);
     }

private:
    static const unsigned presentationAttributeCacheCleanTimeInSeconds = 60;
    static const int minimumPresentationAttributeCacheSizeForCleaning = 100;
    static const unsigned minimumPresentationAttributeCacheHitCountPerMinute = (100 * presentationAttributeCacheCleanTimeInSeconds) / 60;

    void cleanCache()
    {
        unsigned hitCount = m_hitCount;
        m_hitCount = 0;
        if (hitCount > minimumPresentationAttributeCacheHitCountPerMinute)
            return;
        presentationAttributeCache().clear();
    }

    unsigned m_hitCount;
    Timer m_cleanTimer;
};

static PresentationAttributeCacheCleaner& presentationAttributeCacheCleaner()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<PresentationAttributeCacheCleaner> cleaner;
    return cleaner;
#else
    WKC_DEFINE_STATIC_PTR(PresentationAttributeCacheCleaner*, cleaner, 0);
    if (!cleaner)
        cleaner = new PresentationAttributeCacheCleaner();
    return *cleaner;
#endif
}

void StyledElement::clearPresentationAttributeCache()
{
    presentationAttributeCache().clear();
}

void StyledElement::synchronizeStyleAttributeInternal(StyledElement* styledElement)
{
    ASSERT(styledElement->elementData());
    ASSERT(styledElement->elementData()->styleAttributeIsDirty());
    styledElement->elementData()->setStyleAttributeIsDirty(false);
    if (const StyleProperties* inlineStyle = styledElement->inlineStyle())
        styledElement->setSynchronizedLazyAttribute(styleAttr, inlineStyle->asText());
}

StyledElement::~StyledElement()
{
    if (PropertySetCSSStyleDeclaration* cssomWrapper = inlineStyleCSSOMWrapper())
        cssomWrapper->clearParentElement();
}

CSSStyleDeclaration* StyledElement::style()
{
    return ensureMutableInlineStyle().ensureInlineCSSStyleDeclaration(this);
}

MutableStyleProperties& StyledElement::ensureMutableInlineStyle()
{
    RefPtr<StyleProperties>& inlineStyle = ensureUniqueElementData().m_inlineStyle;
    if (!inlineStyle)
        inlineStyle = MutableStyleProperties::create(strictToCSSParserMode(isHTMLElement() && !document().inQuirksMode()));
    else if (!is<MutableStyleProperties>(*inlineStyle))
        inlineStyle = inlineStyle->mutableCopy();
    return downcast<MutableStyleProperties>(*inlineStyle);
}

void StyledElement::attributeChanged(const QualifiedName& name, const AtomicString& oldValue, const AtomicString& newValue, AttributeModificationReason reason)
{
    if (name == styleAttr)
        styleAttributeChanged(newValue, reason);
    else if (isPresentationAttribute(name)) {
        elementData()->setPresentationAttributeStyleIsDirty(true);
        setNeedsStyleRecalc(InlineStyleChange);
    }

    Element::attributeChanged(name, oldValue, newValue, reason);
}

PropertySetCSSStyleDeclaration* StyledElement::inlineStyleCSSOMWrapper()
{
    if (!inlineStyle() || !inlineStyle()->hasCSSOMWrapper())
        return 0;
    PropertySetCSSStyleDeclaration* cssomWrapper = ensureMutableInlineStyle().cssStyleDeclaration();
    ASSERT(cssomWrapper && cssomWrapper->parentElement() == this);
    return cssomWrapper;
}

inline void StyledElement::setInlineStyleFromString(const AtomicString& newStyleString)
{
    RefPtr<StyleProperties>& inlineStyle = elementData()->m_inlineStyle;

    // Avoid redundant work if we're using shared attribute data with already parsed inline style.
    if (inlineStyle && !elementData()->isUnique())
        return;

    // We reconstruct the property set instead of mutating if there is no CSSOM wrapper.
    // This makes wrapperless property sets immutable and so cacheable.
    if (inlineStyle && !is<MutableStyleProperties>(*inlineStyle))
        inlineStyle = nullptr;

    if (!inlineStyle)
        inlineStyle = CSSParser::parseInlineStyleDeclaration(newStyleString, this);
    else
        downcast<MutableStyleProperties>(*inlineStyle).parseDeclaration(newStyleString, &document().elementSheet().contents());
}

void StyledElement::styleAttributeChanged(const AtomicString& newStyleString, AttributeModificationReason reason)
{
    WTF::OrdinalNumber startLineNumber = WTF::OrdinalNumber::beforeFirst();
    if (document().scriptableDocumentParser() && !document().isInDocumentWrite())
        startLineNumber = document().scriptableDocumentParser()->textPosition().m_line;

    if (newStyleString.isNull()) {
        if (PropertySetCSSStyleDeclaration* cssomWrapper = inlineStyleCSSOMWrapper())
            cssomWrapper->clearParentElement();
        ensureUniqueElementData().m_inlineStyle = nullptr;
    } else if (reason == ModifiedByCloning || document().contentSecurityPolicy()->allowInlineStyle(document().url(), startLineNumber, isInUserAgentShadowTree()))
        setInlineStyleFromString(newStyleString);

    elementData()->setStyleAttributeIsDirty(false);

    setNeedsStyleRecalc(InlineStyleChange);
    InspectorInstrumentation::didInvalidateStyleAttr(document(), *this);
}

void StyledElement::inlineStyleChanged()
{
    setNeedsStyleRecalc(InlineStyleChange);
    ASSERT(elementData());
    elementData()->setStyleAttributeIsDirty(true);
    InspectorInstrumentation::didInvalidateStyleAttr(document(), *this);
}
    
bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, CSSValueID identifier, bool important)
{
    ensureMutableInlineStyle().setProperty(propertyID, cssValuePool().createIdentifierValue(identifier), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, CSSPropertyID identifier, bool important)
{
    ensureMutableInlineStyle().setProperty(propertyID, cssValuePool().createIdentifierValue(identifier), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, double value, CSSPrimitiveValue::UnitTypes unit, bool important)
{
    ensureMutableInlineStyle().setProperty(propertyID, cssValuePool().createValue(value, unit), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(CSSPropertyID propertyID, const String& value, bool important)
{
    bool changes = ensureMutableInlineStyle().setProperty(propertyID, value, important, &document().elementSheet().contents());
    if (changes)
        inlineStyleChanged();
    return changes;
}

bool StyledElement::removeInlineStyleProperty(CSSPropertyID propertyID)
{
    if (!inlineStyle())
        return false;
    bool changes = ensureMutableInlineStyle().removeProperty(propertyID);
    if (changes)
        inlineStyleChanged();
    return changes;
}

void StyledElement::removeAllInlineStyleProperties()
{
    if (!inlineStyle() || inlineStyle()->isEmpty())
        return;
    ensureMutableInlineStyle().clear();
    inlineStyleChanged();
}

void StyledElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    if (const StyleProperties* inlineStyle = elementData() ? elementData()->inlineStyle() : 0)
        inlineStyle->addSubresourceStyleURLs(urls, &document().elementSheet().contents());
}

static inline bool attributeNameSort(const std::pair<AtomicStringImpl*, AtomicString>& p1, const std::pair<AtomicStringImpl*, AtomicString>& p2)
{
    // Sort based on the attribute name pointers. It doesn't matter what the order is as long as it is always the same. 
    return p1.first < p2.first;
}

void StyledElement::makePresentationAttributeCacheKey(PresentationAttributeCacheKey& result) const
{    
    // FIXME: Enable for SVG.
    if (namespaceURI() != xhtmlNamespaceURI)
        return;
    // Interpretation of the size attributes on <input> depends on the type attribute.
    if (hasTagName(inputTag))
        return;
    for (const Attribute& attribute : attributesIterator()) {
        if (!isPresentationAttribute(attribute.name()))
            continue;
        if (!attribute.namespaceURI().isNull())
            return;
        // FIXME: Background URL may depend on the base URL and can't be shared. Disallow caching.
        if (attribute.name() == backgroundAttr)
            return;
        result.attributesAndValues.append(std::make_pair(attribute.localName().impl(), attribute.value()));
    }
    if (result.attributesAndValues.isEmpty())
        return;
    // Attribute order doesn't matter. Sort for easy equality comparison.
    std::sort(result.attributesAndValues.begin(), result.attributesAndValues.end(), attributeNameSort);
    // The cache key is non-null when the tagName is set.
    result.tagName = localName().impl();
}

static unsigned computePresentationAttributeCacheHash(const PresentationAttributeCacheKey& key)
{
    if (!key.tagName)
        return 0;
    ASSERT(key.attributesAndValues.size());
    unsigned attributeHash = StringHasher::hashMemory(key.attributesAndValues.data(), key.attributesAndValues.size() * sizeof(key.attributesAndValues[0]));
    return WTF::pairIntHash(key.tagName->existingHash(), attributeHash);
}

void StyledElement::rebuildPresentationAttributeStyle()
{
    PresentationAttributeCacheKey cacheKey;
    makePresentationAttributeCacheKey(cacheKey);

    unsigned cacheHash = computePresentationAttributeCacheHash(cacheKey);

    PresentationAttributeCache::iterator cacheIterator;
    if (cacheHash) {
        cacheIterator = presentationAttributeCache().add(cacheHash, nullptr).iterator;
        if (cacheIterator->value && cacheIterator->value->key != cacheKey)
            cacheHash = 0;
    } else
        cacheIterator = presentationAttributeCache().end();

    RefPtr<StyleProperties> style;
    if (cacheHash && cacheIterator->value) {
        style = cacheIterator->value->value;
        presentationAttributeCacheCleaner().didHitPresentationAttributeCache();
    } else {
        style = MutableStyleProperties::create(isSVGElement() ? SVGAttributeMode : CSSQuirksMode);
        for (const Attribute& attribute : attributesIterator())
            collectStyleForPresentationAttribute(attribute.name(), attribute.value(), static_cast<MutableStyleProperties&>(*style));
    }

    // ShareableElementData doesn't store presentation attribute style, so make sure we have a UniqueElementData.
    UniqueElementData& elementData = ensureUniqueElementData();

    elementData.setPresentationAttributeStyleIsDirty(false);
    elementData.m_presentationAttributeStyle = style->isEmpty() ? 0 : style;

    if (!cacheHash || cacheIterator->value)
        return;

    std::unique_ptr<PresentationAttributeCacheEntry> newEntry = std::make_unique<PresentationAttributeCacheEntry>();
    newEntry->key = cacheKey;
    newEntry->value = style.release();

    static const int presentationAttributeCacheMaximumSize = 4096;
    if (presentationAttributeCache().size() > presentationAttributeCacheMaximumSize) {
        // Start building from scratch if the cache ever gets big.
        presentationAttributeCache().clear();
        presentationAttributeCache().set(cacheHash, WTF::move(newEntry));
    } else
        cacheIterator->value = WTF::move(newEntry);
}

void StyledElement::addPropertyToPresentationAttributeStyle(MutableStyleProperties& style, CSSPropertyID propertyID, CSSValueID identifier)
{
    style.setProperty(propertyID, cssValuePool().createIdentifierValue(identifier));
}

void StyledElement::addPropertyToPresentationAttributeStyle(MutableStyleProperties& style, CSSPropertyID propertyID, double value, CSSPrimitiveValue::UnitTypes unit)
{
    style.setProperty(propertyID, cssValuePool().createValue(value, unit));
}
    
void StyledElement::addPropertyToPresentationAttributeStyle(MutableStyleProperties& style, CSSPropertyID propertyID, const String& value)
{
    style.setProperty(propertyID, value, false, &document().elementSheet().contents());
}

}
