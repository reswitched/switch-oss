/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006, 2010, 2011 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "FontCascade.h"

#include "CharacterProperties.h"
#include "FloatRect.h"
#include "FontCache.h"
#include "GlyphBuffer.h"
#include "LayoutRect.h"
#include "TextRun.h"
#include "WidthIterator.h"
#include <wtf/MainThread.h>
#include <wtf/MathExtras.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(WIN)
#include "UniscribeController.h"
#endif

using namespace WTF;
using namespace Unicode;

namespace WebCore {

static Ref<FontCascadeFonts> retrieveOrAddCachedFonts(const FontDescription&, PassRefPtr<FontSelector>);

const uint8_t FontCascade::s_roundingHackCharacterTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /*\t*/, 1 /*\n*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1 /*space*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /*-*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 /*?*/,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1 /*no-break space*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static bool isDrawnWithSVGFont(const TextRun& run)
{
    return run.renderingContext();
}

static bool useBackslashAsYenSignForFamily(const AtomicString& family)
{
    if (family.isEmpty())
        return false;
#if !PLATFORM(WKC)
    static HashSet<AtomicString>* set;
#else
    WKC_DEFINE_STATIC_PTR(HashSet<AtomicString>*, set, 0);
#endif
    if (!set) {
        set = new HashSet<AtomicString>;
        set->add("MS PGothic");
        UChar unicodeNameMSPGothic[] = {0xFF2D, 0xFF33, 0x0020, 0xFF30, 0x30B4, 0x30B7, 0x30C3, 0x30AF};
        set->add(AtomicString(unicodeNameMSPGothic, WTF_ARRAY_LENGTH(unicodeNameMSPGothic)));

        set->add("MS PMincho");
        UChar unicodeNameMSPMincho[] = {0xFF2D, 0xFF33, 0x0020, 0xFF30, 0x660E, 0x671D};
        set->add(AtomicString(unicodeNameMSPMincho, WTF_ARRAY_LENGTH(unicodeNameMSPMincho)));

        set->add("MS Gothic");
        UChar unicodeNameMSGothic[] = {0xFF2D, 0xFF33, 0x0020, 0x30B4, 0x30B7, 0x30C3, 0x30AF};
        set->add(AtomicString(unicodeNameMSGothic, WTF_ARRAY_LENGTH(unicodeNameMSGothic)));

        set->add("MS Mincho");
        UChar unicodeNameMSMincho[] = {0xFF2D, 0xFF33, 0x0020, 0x660E, 0x671D};
        set->add(AtomicString(unicodeNameMSMincho, WTF_ARRAY_LENGTH(unicodeNameMSMincho)));

        set->add("Meiryo");
        UChar unicodeNameMeiryo[] = {0x30E1, 0x30A4, 0x30EA, 0x30AA};
        set->add(AtomicString(unicodeNameMeiryo, WTF_ARRAY_LENGTH(unicodeNameMeiryo)));
    }
    return set->contains(family);
}

#if !PLATFORM(WKC)
FontCascade::CodePath FontCascade::s_codePath = Auto;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(FontCascade::CodePath, FontCascade, s_codePath, Auto)
#endif


// ============================================================================================
// FontCascade Implementation (Cross-Platform Portion)
// ============================================================================================

FontCascade::FontCascade()
    : m_weakPtrFactory(this)
    , m_letterSpacing(0)
    , m_wordSpacing(0)
    , m_useBackslashAsYenSymbol(false)
    , m_enableKerning(computeEnableKerning())
    , m_requiresShaping(computeRequiresShaping())
{
}

FontCascade::FontCascade(const FontDescription& fd, float letterSpacing, float wordSpacing)
    : m_fontDescription(fd)
    , m_weakPtrFactory(this)
    , m_letterSpacing(letterSpacing)
    , m_wordSpacing(wordSpacing)
    , m_useBackslashAsYenSymbol(useBackslashAsYenSignForFamily(fd.firstFamily()))
    , m_enableKerning(computeEnableKerning())
    , m_requiresShaping(computeRequiresShaping())
{
}

// FIXME: We should make this constructor platform-independent.
FontCascade::FontCascade(const FontPlatformData& fontData, FontSmoothingMode fontSmoothingMode)
    : m_fonts(FontCascadeFonts::createForPlatformFont(fontData))
    , m_weakPtrFactory(this)
    , m_letterSpacing(0)
    , m_wordSpacing(0)
    , m_useBackslashAsYenSymbol(false)
    , m_enableKerning(computeEnableKerning())
    , m_requiresShaping(computeRequiresShaping())
{
    m_fontDescription.setFontSmoothing(fontSmoothingMode);
#if PLATFORM(IOS)
    m_fontDescription.setSpecifiedSize(CTFontGetSize(fontData.font()));
    m_fontDescription.setComputedSize(CTFontGetSize(fontData.font()));
    m_fontDescription.setIsItalic(CTFontGetSymbolicTraits(fontData.font()) & kCTFontTraitItalic);
    m_fontDescription.setWeight((CTFontGetSymbolicTraits(fontData.font()) & kCTFontTraitBold) ? FontWeightBold : FontWeightNormal);
#endif
}

// FIXME: We should make this constructor platform-independent.
#if PLATFORM(IOS)
FontCascade::FontCascade(const FontPlatformData& fontData, PassRefPtr<FontSelector> fontSelector)
    : m_weakPtrFactory(this)
    , m_letterSpacing(0)
    , m_wordSpacing(0)
    , m_enableKerning(computeEnableKerning())
    , m_requiresShaping(computeRequiresShaping())
{
    CTFontRef primaryFont = fontData.font();
    m_fontDescription.setSpecifiedSize(CTFontGetSize(primaryFont));
    m_fontDescription.setComputedSize(CTFontGetSize(primaryFont));
    m_fontDescription.setIsItalic(CTFontGetSymbolicTraits(primaryFont) & kCTFontTraitItalic);
    m_fontDescription.setWeight((CTFontGetSymbolicTraits(primaryFont) & kCTFontTraitBold) ? FontWeightBold : FontWeightNormal);
    m_fonts = retrieveOrAddCachedFonts(m_fontDescription, fontSelector.get());
}
#endif

FontCascade::FontCascade(const FontCascade& other)
    : m_fontDescription(other.m_fontDescription)
    , m_fonts(other.m_fonts)
    , m_weakPtrFactory(this)
    , m_letterSpacing(other.m_letterSpacing)
    , m_wordSpacing(other.m_wordSpacing)
    , m_useBackslashAsYenSymbol(other.m_useBackslashAsYenSymbol)
    , m_enableKerning(computeEnableKerning())
    , m_requiresShaping(computeRequiresShaping())
{
}

FontCascade& FontCascade::operator=(const FontCascade& other)
{
    m_fontDescription = other.m_fontDescription;
    m_fonts = other.m_fonts;
    m_letterSpacing = other.m_letterSpacing;
    m_wordSpacing = other.m_wordSpacing;
    m_useBackslashAsYenSymbol = other.m_useBackslashAsYenSymbol;
    m_enableKerning = other.m_enableKerning;
    m_requiresShaping = other.m_requiresShaping;
    return *this;
}

bool FontCascade::operator==(const FontCascade& other) const
{
    if (isLoadingCustomFonts() || other.isLoadingCustomFonts())
        return false;

    if (m_fontDescription != other.m_fontDescription || m_letterSpacing != other.m_letterSpacing || m_wordSpacing != other.m_wordSpacing)
        return false;
    if (m_fonts == other.m_fonts)
        return true;
    if (!m_fonts || !other.m_fonts)
        return false;
    if (m_fonts->fontSelector() != other.m_fonts->fontSelector())
        return false;
    // Can these cases actually somehow occur? All fonts should get wiped out by full style recalc.
    if (m_fonts->fontSelectorVersion() != other.m_fonts->fontSelectorVersion())
        return false;
    if (m_fonts->generation() != other.m_fonts->generation())
        return false;
    return true;
}

struct FontCascadeCacheKey {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    // This part of the key is shared with the lower level FontCache (caching FontData objects).
    FontDescriptionFontDataCacheKey fontDescriptionCacheKey;
    Vector<AtomicString, 3> families;
    unsigned fontSelectorId;
    unsigned fontSelectorVersion;
};

struct FontCascadeCacheEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FontCascadeCacheEntry(FontCascadeCacheKey&& key, Ref<FontCascadeFonts>&& fonts)
        : key(WTF::move(key))
        , fonts(WTF::move(fonts))
    { }
    FontCascadeCacheKey key;
    Ref<FontCascadeFonts> fonts;
};

typedef HashMap<unsigned, std::unique_ptr<FontCascadeCacheEntry>, AlreadyHashed> FontCascadeCache;

static bool operator==(const FontCascadeCacheKey& a, const FontCascadeCacheKey& b)
{
    if (a.fontDescriptionCacheKey != b.fontDescriptionCacheKey)
        return false;
    if (a.fontSelectorId != b.fontSelectorId || a.fontSelectorVersion != b.fontSelectorVersion)
        return false;
    if (a.families.size() != b.families.size())
        return false;
    for (unsigned i = 0; i < a.families.size(); ++i) {
        if (!equalIgnoringCase(a.families[i].impl(), b.families[i].impl()))
            return false;
    }
    return true;
}

static FontCascadeCache& fontCascadeCache()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(FontCascadeCache, cache, ());
    return cache;
}

void invalidateFontCascadeCache()
{
    fontCascadeCache().clear();
}

void clearWidthCaches()
{
    for (auto& value : fontCascadeCache().values())
        value->fonts.get().widthCache().clear();
}

static FontCascadeCacheKey makeFontCascadeCacheKey(const FontDescription& description, FontSelector* fontSelector)
{
    FontCascadeCacheKey key;
    key.fontDescriptionCacheKey = FontDescriptionFontDataCacheKey(description);
    for (unsigned i = 0; i < description.familyCount(); ++i)
        key.families.append(description.familyAt(i));
    key.fontSelectorId = fontSelector ? fontSelector->uniqueId() : 0;
    key.fontSelectorVersion = fontSelector ? fontSelector->version() : 0;
    return key;
}

static unsigned computeFontCascadeCacheHash(const FontCascadeCacheKey& key)
{
    Vector<unsigned, 7> hashCodes;
    hashCodes.reserveInitialCapacity(4 + key.families.size());

    hashCodes.uncheckedAppend(key.fontDescriptionCacheKey.computeHash());
    hashCodes.uncheckedAppend(key.fontSelectorId);
    hashCodes.uncheckedAppend(key.fontSelectorVersion);
    for (unsigned i = 0; i < key.families.size(); ++i)
        hashCodes.uncheckedAppend(key.families[i].impl() ? CaseFoldingHash::hash(key.families[i]) : 0);

    return StringHasher::hashMemory(hashCodes.data(), hashCodes.size() * sizeof(unsigned));
}

void pruneUnreferencedEntriesFromFontCascadeCache()
{
    fontCascadeCache().removeIf([](FontCascadeCache::KeyValuePairType& entry) {
        return entry.value->fonts.get().hasOneRef();
    });
}

void pruneSystemFallbackFonts()
{
    for (auto& entry : fontCascadeCache().values())
        entry->fonts->pruneSystemFallbacks();
}

static Ref<FontCascadeFonts> retrieveOrAddCachedFonts(const FontDescription& fontDescription, PassRefPtr<FontSelector> fontSelector)
{
    auto key = makeFontCascadeCacheKey(fontDescription, fontSelector.get());

    unsigned hash = computeFontCascadeCacheHash(key);
    auto addResult = fontCascadeCache().add(hash, std::unique_ptr<FontCascadeCacheEntry>());
    if (!addResult.isNewEntry && addResult.iterator->value->key == key)
        return addResult.iterator->value->fonts.get();

    auto& newEntry = addResult.iterator->value;
    newEntry = std::make_unique<FontCascadeCacheEntry>(WTF::move(key), FontCascadeFonts::create(fontSelector));
    Ref<FontCascadeFonts> glyphs = newEntry->fonts.get();

    static const unsigned unreferencedPruneInterval = 50;
    static const int maximumEntries = 400;
#if !PLATFORM(WKC)
    static unsigned pruneCounter;
#else
    WKC_DEFINE_STATIC_UINT(pruneCounter, 0);
#endif
    // Referenced FontCascadeFonts would exist anyway so pruning them saves little memory.
    if (!(++pruneCounter % unreferencedPruneInterval))
        pruneUnreferencedEntriesFromFontCascadeCache();
    // Prevent pathological growth.
    if (fontCascadeCache().size() > maximumEntries)
        fontCascadeCache().remove(fontCascadeCache().begin());
    return glyphs;
}

void FontCascade::update(PassRefPtr<FontSelector> fontSelector) const
{
    m_fonts = retrieveOrAddCachedFonts(m_fontDescription, fontSelector.get());
    m_useBackslashAsYenSymbol = useBackslashAsYenSignForFamily(firstFamily());
    m_enableKerning = computeEnableKerning();
    m_requiresShaping = computeRequiresShaping();
}

float FontCascade::drawText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to, CustomFontNotReadyAction customFontNotReadyAction) const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    // Don't draw anything while we are using custom fonts that are in the process of loading,
    // except if the 'force' argument is set to true (in which case it will use a fallback
    // font).
    if (isLoadingCustomFonts() && customFontNotReadyAction == DoNotPaintIfFontNotReady)
        return 0;

    to = (to == -1 ? run.length() : to);

    CodePath codePathToUse = codePath(run);
    // FIXME: Use the fast code path once it handles partial runs with kerning and ligatures. See http://webkit.org/b/100050
    if (codePathToUse != Complex && (enableKerning() || requiresShaping()) && (from || static_cast<unsigned>(to) != run.length()) && !isDrawnWithSVGFont(run))
        codePathToUse = Complex;

    if (codePathToUse != Complex)
        return drawSimpleText(context, run, point, from, to);

    return drawComplexText(context, run, point, from, to);
}

void FontCascade::drawEmphasisMarks(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    if (isLoadingCustomFonts())
        return;

    if (to < 0)
        to = run.length();

    CodePath codePathToUse = codePath(run);
    // FIXME: Use the fast code path once it handles partial runs with kerning and ligatures. See http://webkit.org/b/100050
    if (codePathToUse != Complex && (enableKerning() || requiresShaping()) && (from || static_cast<unsigned>(to) != run.length()) && !isDrawnWithSVGFont(run))
        codePathToUse = Complex;

    if (codePathToUse != Complex)
        drawEmphasisMarksForSimpleText(context, run, mark, point, from, to);
    else
        drawEmphasisMarksForComplexText(context, run, mark, point, from, to);
}

float FontCascade::width(const TextRun& run, HashSet<const Font*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    CodePath codePathToUse = codePath(run);
    if (codePathToUse != Complex) {
        // The complex path is more restrictive about returning fallback fonts than the simple path, so we need an explicit test to make their behaviors match.
        if (!canReturnFallbackFontsForComplexText())
            fallbackFonts = 0;
        // The simple path can optimize the case where glyph overflow is not observable.
        if (codePathToUse != SimpleWithGlyphOverflow && (glyphOverflow && !glyphOverflow->computeBounds))
            glyphOverflow = 0;
    }

    bool hasWordSpacingOrLetterSpacing = wordSpacing() || letterSpacing();
    float* cacheEntry = m_fonts->widthCache().add(run, std::numeric_limits<float>::quiet_NaN(), enableKerning() || requiresShaping(), hasWordSpacingOrLetterSpacing, glyphOverflow);
    if (cacheEntry && !std::isnan(*cacheEntry))
        return *cacheEntry;

    HashSet<const Font*> localFallbackFonts;
    if (!fallbackFonts)
        fallbackFonts = &localFallbackFonts;

    float result;
    if (codePathToUse == Complex)
        result = floatWidthForComplexText(run, fallbackFonts, glyphOverflow);
    else
        result = floatWidthForSimpleText(run, fallbackFonts, glyphOverflow);

    if (cacheEntry && fallbackFonts->isEmpty())
        *cacheEntry = result;
    return result;
}

float FontCascade::width(const TextRun& run, int& charsConsumed, String& glyphName) const
{
#if ENABLE(SVG_FONTS)
    if (isDrawnWithSVGFont(run))
        return run.renderingContext()->floatWidthUsingSVGFont(*this, run, charsConsumed, glyphName);
#endif

    charsConsumed = run.length();
    glyphName = "";
    return width(run);
}

GlyphData FontCascade::glyphDataForCharacter(UChar32 c, bool mirror, FontVariant variant) const
{
    if (variant == AutoVariant) {
        if (m_fontDescription.variantCaps() == FontVariantCaps::Small && !primaryFont().isSVGFont()) {
            UChar32 upperC = u_toupper(c);
            if (upperC != c) {
                c = upperC;
                variant = SmallCapsVariant;
            } else
                variant = NormalVariant;
        } else
            variant = NormalVariant;
    }

    if (mirror)
        c = u_charMirror(c);

    return m_fonts->glyphDataForCharacter(c, m_fontDescription, variant);
}

#if !PLATFORM(COCOA)

std::unique_ptr<TextLayout, TextLayoutDeleter> FontCascade::createLayout(RenderText&, float, bool) const
{
    return nullptr;
}

void TextLayoutDeleter::operator()(TextLayout*) const
{
}

float FontCascade::width(TextLayout&, unsigned, unsigned, HashSet<const Font*>*)
{
    ASSERT_NOT_REACHED();
    return 0;
}

#endif

static const char* fontFamiliesWithInvalidCharWidth[] = {
    "American Typewriter",
    "Arial Hebrew",
    "Chalkboard",
    "Cochin",
    "Corsiva Hebrew",
    "Courier",
    "Euphemia UCAS",
    "Geneva",
    "Gill Sans",
    "Hei",
    "Helvetica",
    "Hoefler Text",
    "InaiMathi",
    "Kai",
    "Lucida Grande",
    "Marker Felt",
    "Monaco",
    "Mshtakan",
    "New Peninim MT",
    "Osaka",
    "Raanana",
    "STHeiti",
    "Symbol",
    "Times",
    "Apple Braille",
    "Apple LiGothic",
    "Apple LiSung",
    "Apple Symbols",
    "AppleGothic",
    "AppleMyungjo",
    "#GungSeo",
    "#HeadLineA",
    "#PCMyungjo",
    "#PilGi",
};

// For font families where any of the fonts don't have a valid entry in the OS/2 table
// for avgCharWidth, fallback to the legacy webkit behavior of getting the avgCharWidth
// from the width of a '0'. This only seems to apply to a fixed number of Mac fonts,
// but, in order to get similar rendering across platforms, we do this check for
// all platforms.
bool FontCascade::hasValidAverageCharWidth() const
{
    AtomicString family = firstFamily();
    if (family.isEmpty())
        return false;

#if PLATFORM(MAC) || PLATFORM(IOS)
    // Internal fonts on OS X and iOS also have an invalid entry in the table for avgCharWidth.
    if (primaryFontIsSystemFont())
        return false;
#endif

#if !PLATFORM(WKC)
    static HashSet<AtomicString>* fontFamiliesWithInvalidCharWidthMap = 0;
#else
    WKC_DEFINE_STATIC_PTR(HashSet<AtomicString>*, fontFamiliesWithInvalidCharWidthMap, 0);
#endif

    if (!fontFamiliesWithInvalidCharWidthMap) {
        fontFamiliesWithInvalidCharWidthMap = new HashSet<AtomicString>;

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(fontFamiliesWithInvalidCharWidth); ++i)
            fontFamiliesWithInvalidCharWidthMap->add(AtomicString(fontFamiliesWithInvalidCharWidth[i]));
    }

    return !fontFamiliesWithInvalidCharWidthMap->contains(family);
}

bool FontCascade::fastAverageCharWidthIfAvailable(float& width) const
{
    bool success = hasValidAverageCharWidth();
    if (success)
        width = roundf(primaryFont().avgCharWidth()); // FIXME: primaryFont() might not correspond to firstFamily().
    return success;
}

void FontCascade::adjustSelectionRectForText(const TextRun& run, LayoutRect& selectionRect, int from, int to) const
{
    to = (to == -1 ? run.length() : to);

    CodePath codePathToUse = codePath(run);
    // FIXME: Use the fast code path once it handles partial runs with kerning and ligatures. See http://webkit.org/b/100050
    if (codePathToUse != Complex && (enableKerning() || requiresShaping()) && (from || static_cast<unsigned>(to) != run.length()) && !isDrawnWithSVGFont(run))
        codePathToUse = Complex;

    if (codePathToUse != Complex)
        return adjustSelectionRectForSimpleText(run, selectionRect, from, to);

    return adjustSelectionRectForComplexText(run, selectionRect, from, to);
}

int FontCascade::offsetForPosition(const TextRun& run, float x, bool includePartialGlyphs) const
{
    // FIXME: Use the fast code path once it handles partial runs with kerning and ligatures. See http://webkit.org/b/100050
    if (codePath(run) != Complex && (!(enableKerning() || requiresShaping()) || isDrawnWithSVGFont(run)))
        return offsetForPositionForSimpleText(run, x, includePartialGlyphs);

    return offsetForPositionForComplexText(run, x, includePartialGlyphs);
}

template <typename CharacterType>
static inline String normalizeSpacesInternal(const CharacterType* characters, unsigned length)
{
    StringBuilder normalized;
    normalized.reserveCapacity(length);

    for (unsigned i = 0; i < length; ++i)
        normalized.append(FontCascade::normalizeSpaces(characters[i]));

    return normalized.toString();
}

String FontCascade::normalizeSpaces(const LChar* characters, unsigned length)
{
    return normalizeSpacesInternal(characters, length);
}

String FontCascade::normalizeSpaces(const UChar* characters, unsigned length)
{
    return normalizeSpacesInternal(characters, length);
}

static bool shouldUseFontSmoothing = true;

void FontCascade::setShouldUseSmoothing(bool shouldUseSmoothing)
{
    ASSERT(isMainThread());
    shouldUseFontSmoothing = shouldUseSmoothing;
}

bool FontCascade::shouldUseSmoothing()
{
    return shouldUseFontSmoothing;
}

#if !PLATFORM(WKC)
static bool antialiasedFontDilationIsEnabled = false;
#else
WKC_DEFINE_GLOBAL_BOOL(antialiasedFontDilationIsEnabled, false);
#endif

void FontCascade::setAntialiasedFontDilationEnabled(bool enabled)
{
    antialiasedFontDilationIsEnabled = enabled;
}

bool FontCascade::antialiasedFontDilationEnabled()
{
    return antialiasedFontDilationIsEnabled;
}

void FontCascade::setCodePath(CodePath p)
{
    s_codePath = p;
}

FontCascade::CodePath FontCascade::codePath()
{
    return s_codePath;
}

FontCascade::CodePath FontCascade::codePath(const TextRun& run) const
{
    if (s_codePath != Auto)
        return s_codePath;

#if ENABLE(SVG_FONTS)
    if (isDrawnWithSVGFont(run))
        return Simple;
#endif

#if PLATFORM(COCOA)
    // Because Font::applyTransforms() doesn't know which features to enable/disable in the simple code path, it can't properly handle feature or variant settings.
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=150791: @font-face features should also cause this to be complex.
    if (m_fontDescription.featureSettings().size() > 0 || !m_fontDescription.variantSettings().isAllNormal())
        return Complex;

#else

    if (run.length() > 1 && (enableKerning() || requiresShaping()))
        return Complex;
#endif

    if (!run.characterScanForCodePath())
        return Simple;

    if (run.is8Bit())
        return Simple;

    // Start from 0 since drawing and highlighting also measure the characters before run->from.
    return characterRangeCodePath(run.characters16(), run.length());
}

FontCascade::CodePath FontCascade::characterRangeCodePath(const UChar* characters, unsigned len)
{
    // FIXME: Should use a UnicodeSet in ports where ICU is used. Note that we 
    // can't simply use UnicodeCharacter Property/class because some characters
    // are not 'combining', but still need to go to the complex path.
    // Alternatively, we may as well consider binary search over a sorted
    // list of ranges.
    CodePath result = Simple;
    bool previousCharacterIsEmojiGroupCandidate = false;
    for (unsigned i = 0; i < len; i++) {
        const UChar c = characters[i];
        if (c == zeroWidthJoiner && previousCharacterIsEmojiGroupCandidate)
            return Complex;
        
        previousCharacterIsEmojiGroupCandidate = false;
        if (c < 0x2E5) // U+02E5 through U+02E9 (Modifier Letters : Tone letters)  
            continue;
        if (c <= 0x2E9) 
            return Complex;

        if (c < 0x300) // U+0300 through U+036F Combining diacritical marks
            continue;
        if (c <= 0x36F)
            return Complex;

        if (c < 0x0591 || c == 0x05BE) // U+0591 through U+05CF excluding U+05BE Hebrew combining marks, Hebrew punctuation Paseq, Sof Pasuq and Nun Hafukha
            continue;
        if (c <= 0x05CF)
            return Complex;

        // U+0600 through U+109F Arabic, Syriac, Thaana, NKo, Samaritan, Mandaic,
        // Devanagari, Bengali, Gurmukhi, Gujarati, Oriya, Tamil, Telugu, Kannada, 
        // Malayalam, Sinhala, Thai, Lao, Tibetan, Myanmar
        if (c < 0x0600) 
            continue;
        if (c <= 0x109F)
            return Complex;

        // U+1100 through U+11FF Hangul Jamo (only Ancient Korean should be left here if you precompose;
        // Modern Korean will be precomposed as a result of step A)
        if (c < 0x1100)
            continue;
        if (c <= 0x11FF)
            return Complex;

        if (c < 0x135D) // U+135D through U+135F Ethiopic combining marks
            continue;
        if (c <= 0x135F)
            return Complex;

        if (c < 0x1700) // U+1780 through U+18AF Tagalog, Hanunoo, Buhid, Taghanwa,Khmer, Mongolian
            continue;
        if (c <= 0x18AF)
            return Complex;

        if (c < 0x1900) // U+1900 through U+194F Limbu (Unicode 4.0)
            continue;
        if (c <= 0x194F)
            return Complex;

        if (c < 0x1980) // U+1980 through U+19DF New Tai Lue
            continue;
        if (c <= 0x19DF)
            return Complex;

        if (c < 0x1A00) // U+1A00 through U+1CFF Buginese, Tai Tham, Balinese, Batak, Lepcha, Vedic
            continue;
        if (c <= 0x1CFF)
            return Complex;

        if (c < 0x1DC0) // U+1DC0 through U+1DFF Comining diacritical mark supplement
            continue;
        if (c <= 0x1DFF)
            return Complex;

        // U+1E00 through U+2000 characters with diacritics and stacked diacritics
        if (c <= 0x2000) {
            result = SimpleWithGlyphOverflow;
            continue;
        }

        if (c < 0x20D0) // U+20D0 through U+20FF Combining marks for symbols
            continue;
        if (c <= 0x20FF)
            return Complex;

        if (c < 0x2CEF) // U+2CEF through U+2CF1 Combining marks for Coptic
            continue;
        if (c <= 0x2CF1)
            return Complex;

        if (c < 0x302A) // U+302A through U+302F Ideographic and Hangul Tone marks
            continue;
        if (c <= 0x302F)
            return Complex;

        if (c < 0xA67C) // U+A67C through U+A67D Combining marks for old Cyrillic
            continue;
        if (c <= 0xA67D)
            return Complex;

        if (c < 0xA6F0) // U+A6F0 through U+A6F1 Combining mark for Bamum
            continue;
        if (c <= 0xA6F1)
            return Complex;

        // U+A800 through U+ABFF Nagri, Phags-pa, Saurashtra, Devanagari Extended,
        // Hangul Jamo Ext. A, Javanese, Myanmar Extended A, Tai Viet, Meetei Mayek,
        if (c < 0xA800) 
            continue;
        if (c <= 0xABFF)
            return Complex;

        if (c < 0xD7B0) // U+D7B0 through U+D7FF Hangul Jamo Ext. B
            continue;
        if (c <= 0xD7FF)
            return Complex;

        if (c <= 0xDBFF) {
            // High surrogate

            if (i == len - 1)
                continue;

            UChar next = characters[++i];
            if (!U16_IS_TRAIL(next))
                continue;

            UChar32 supplementaryCharacter = U16_GET_SUPPLEMENTARY(c, next);

            if (supplementaryCharacter < 0x1F1E6) // U+1F1E6 through U+1F1FF Regional Indicator Symbols
                continue;
            if (supplementaryCharacter <= 0x1F1FF)
                return Complex;

            if (supplementaryCharacter == 0x1F441 || supplementaryCharacter == 0x1F5E8 || (supplementaryCharacter >= 0x1F466 && supplementaryCharacter <= 0x1F469)) {
                previousCharacterIsEmojiGroupCandidate = true;
                continue;
            }
            if (isEmojiModifier(supplementaryCharacter))
                return Complex;
            if (supplementaryCharacter < 0xE0100) // U+E0100 through U+E01EF Unicode variation selectors.
                continue;
            if (supplementaryCharacter <= 0xE01EF)
                return Complex;

            // FIXME: Check for Brahmi (U+11000 block), Kaithi (U+11080 block) and other complex scripts
            // in plane 1 or higher.

            continue;
        }

        if (c < 0xFE00) // U+FE00 through U+FE0F Unicode variation selectors
            continue;
        if (c <= 0xFE0F)
            return Complex;

        if (c < 0xFE20) // U+FE20 through U+FE2F Combining half marks
            continue;
        if (c <= 0xFE2F)
            return Complex;
    }
    return result;
}

bool FontCascade::isCJKIdeograph(UChar32 c)
{
    // The basic CJK Unified Ideographs block.
    if (c >= 0x4E00 && c <= 0x9FFF)
        return true;
    
    // CJK Unified Ideographs Extension A.
    if (c >= 0x3400 && c <= 0x4DBF)
        return true;
    
    // CJK Radicals Supplement.
    if (c >= 0x2E80 && c <= 0x2EFF)
        return true;
    
    // Kangxi Radicals.
    if (c >= 0x2F00 && c <= 0x2FDF)
        return true;
    
    // CJK Strokes.
    if (c >= 0x31C0 && c <= 0x31EF)
        return true;
    
    // CJK Compatibility Ideographs.
    if (c >= 0xF900 && c <= 0xFAFF)
        return true;

    // CJK Unified Ideographs Extension B.
    if (c >= 0x20000 && c <= 0x2A6DF)
        return true;

    // CJK Unified Ideographs Extension C.
    if (c >= 0x2A700 && c <= 0x2B73F)
        return true;
    
    // CJK Unified Ideographs Extension D.
    if (c >= 0x2B740 && c <= 0x2B81F)
        return true;
    
    // CJK Compatibility Ideographs Supplement.
    if (c >= 0x2F800 && c <= 0x2FA1F)
        return true;

    return false;
}

bool FontCascade::isCJKIdeographOrSymbol(UChar32 c)
{
    // 0x2C7 Caron, Mandarin Chinese 3rd Tone
    // 0x2CA Modifier Letter Acute Accent, Mandarin Chinese 2nd Tone
    // 0x2CB Modifier Letter Grave Access, Mandarin Chinese 4th Tone 
    // 0x2D9 Dot Above, Mandarin Chinese 5th Tone 
    if ((c == 0x2C7) || (c == 0x2CA) || (c == 0x2CB) || (c == 0x2D9))
        return true;

    if ((c == 0x2020) || (c == 0x2021) || (c == 0x2030) || (c == 0x203B) || (c == 0x203C)
        || (c == 0x2042) || (c == 0x2047) || (c == 0x2048) || (c == 0x2049) || (c == 0x2051)
        || (c == 0x20DD) || (c == 0x20DE) || (c == 0x2100) || (c == 0x2103) || (c == 0x2105)
        || (c == 0x2109) || (c == 0x210A) || (c == 0x2113) || (c == 0x2116) || (c == 0x2121)
        || (c == 0x212B) || (c == 0x213B) || (c == 0x2150) || (c == 0x2151) || (c == 0x2152))
        return true;

    if (c >= 0x2156 && c <= 0x215A)
        return true;

    if (c >= 0x2160 && c <= 0x216B)
        return true;

    if (c >= 0x2170 && c <= 0x217B)
        return true;

    if ((c == 0x217F) || (c == 0x2189) || (c == 0x2307) || (c == 0x2312) || (c == 0x23BE) || (c == 0x23BF))
        return true;

    if (c >= 0x23C0 && c <= 0x23CC)
        return true;

    if ((c == 0x23CE) || (c == 0x2423))
        return true;

    if (c >= 0x2460 && c <= 0x2492)
        return true;

    if (c >= 0x249C && c <= 0x24FF)
        return true;

    if ((c == 0x25A0) || (c == 0x25A1) || (c == 0x25A2) || (c == 0x25AA) || (c == 0x25AB))
        return true;

    if ((c == 0x25B1) || (c == 0x25B2) || (c == 0x25B3) || (c == 0x25B6) || (c == 0x25B7) || (c == 0x25BC) || (c == 0x25BD))
        return true;
    
    if ((c == 0x25C0) || (c == 0x25C1) || (c == 0x25C6) || (c == 0x25C7) || (c == 0x25C9) || (c == 0x25CB) || (c == 0x25CC))
        return true;

    if (c >= 0x25CE && c <= 0x25D3)
        return true;

    if (c >= 0x25E2 && c <= 0x25E6)
        return true;

    if (c == 0x25EF)
        return true;

    if (c >= 0x2600 && c <= 0x2603)
        return true;

    if ((c == 0x2605) || (c == 0x2606) || (c == 0x260E) || (c == 0x2616) || (c == 0x2617) || (c == 0x2640) || (c == 0x2642))
        return true;

    if (c >= 0x2660 && c <= 0x266F)
        return true;

    if (c >= 0x2672 && c <= 0x267D)
        return true;

    if ((c == 0x26A0) || (c == 0x26BD) || (c == 0x26BE) || (c == 0x2713) || (c == 0x271A) || (c == 0x273F) || (c == 0x2740) || (c == 0x2756))
        return true;

    if (c >= 0x2776 && c <= 0x277F)
        return true;

    if (c == 0x2B1A)
        return true;

    // Ideographic Description Characters.
    if (c >= 0x2FF0 && c <= 0x2FFF)
        return true;

    // CJK Symbols and Punctuation, excluding 0x3030.
    if (c >= 0x3000 && c < 0x3030)
        return true;

    if (c > 0x3030 && c <= 0x303F)
        return true;

    // Hiragana
    if (c >= 0x3040 && c <= 0x309F)
        return true;

    // Katakana 
    if (c >= 0x30A0 && c <= 0x30FF)
        return true;

    // Bopomofo
    if (c >= 0x3100 && c <= 0x312F)
        return true;

    if (c >= 0x3190 && c <= 0x319F)
        return true;

    // Bopomofo Extended
    if (c >= 0x31A0 && c <= 0x31BF)
        return true;

    // Enclosed CJK Letters and Months.
    if (c >= 0x3200 && c <= 0x32FF)
        return true;
    
    // CJK Compatibility.
    if (c >= 0x3300 && c <= 0x33FF)
        return true;

    if (c >= 0xF860 && c <= 0xF862)
        return true;

    // CJK Compatibility Forms.
    if (c >= 0xFE30 && c <= 0xFE4F)
        return true;

    if ((c == 0xFE10) || (c == 0xFE11) || (c == 0xFE12) || (c == 0xFE19))
        return true;

    if ((c == 0xFF0D) || (c == 0xFF1B) || (c == 0xFF1C) || (c == 0xFF1E))
        return false;

    // Halfwidth and Fullwidth Forms
    // Usually only used in CJK
    if (c >= 0xFF00 && c <= 0xFFEF)
        return true;

    // Emoji.
    if (c == 0x1F100)
        return true;

    if (c >= 0x1F110 && c <= 0x1F129)
        return true;

    if (c >= 0x1F130 && c <= 0x1F149)
        return true;

    if (c >= 0x1F150 && c <= 0x1F169)
        return true;

    if (c >= 0x1F170 && c <= 0x1F189)
        return true;

    if (c >= 0x1F200 && c <= 0x1F6C5)
        return true;

    return isCJKIdeograph(c);
}

std::pair<unsigned, bool> FontCascade::expansionOpportunityCountInternal(const LChar* characters, size_t length, TextDirection direction, ExpansionBehavior expansionBehavior)
{
    unsigned count = 0;
    bool isAfterExpansion = (expansionBehavior & LeadingExpansionMask) == ForbidLeadingExpansion;
    if ((expansionBehavior & LeadingExpansionMask) == ForceLeadingExpansion) {
        ++count;
        isAfterExpansion = true;
    }
    if (direction == LTR) {
        for (size_t i = 0; i < length; ++i) {
            if (treatAsSpace(characters[i])) {
                count++;
                isAfterExpansion = true;
            } else
                isAfterExpansion = false;
        }
    } else {
        for (size_t i = length; i > 0; --i) {
            if (treatAsSpace(characters[i - 1])) {
                count++;
                isAfterExpansion = true;
            } else
                isAfterExpansion = false;
        }
    }
    if (!isAfterExpansion && (expansionBehavior & TrailingExpansionMask) == ForceTrailingExpansion) {
        ++count;
        isAfterExpansion = true;
    } else if (isAfterExpansion && (expansionBehavior & TrailingExpansionMask) == ForbidTrailingExpansion) {
        --count;
        isAfterExpansion = false;
    }
    return std::make_pair(count, isAfterExpansion);
}

std::pair<unsigned, bool> FontCascade::expansionOpportunityCountInternal(const UChar* characters, size_t length, TextDirection direction, ExpansionBehavior expansionBehavior)
{
#if !PLATFORM(WKC)
    static bool expandAroundIdeographs = canExpandAroundIdeographsInComplexText();
#else
    WKC_DEFINE_STATIC_BOOL(expandAroundIdeographs, canExpandAroundIdeographsInComplexText());
#endif
    unsigned count = 0;
    bool isAfterExpansion = (expansionBehavior & LeadingExpansionMask) == ForbidLeadingExpansion;
    if ((expansionBehavior & LeadingExpansionMask) == ForceLeadingExpansion) {
        ++count;
        isAfterExpansion = true;
    }
    if (direction == LTR) {
        for (size_t i = 0; i < length; ++i) {
            UChar32 character = characters[i];
            if (treatAsSpace(character)) {
                count++;
                isAfterExpansion = true;
                continue;
            }
            if (U16_IS_LEAD(character) && i + 1 < length && U16_IS_TRAIL(characters[i + 1])) {
                character = U16_GET_SUPPLEMENTARY(character, characters[i + 1]);
                i++;
            }
            if (expandAroundIdeographs && isCJKIdeographOrSymbol(character)) {
                if (!isAfterExpansion)
                    count++;
                count++;
                isAfterExpansion = true;
                continue;
            }
            isAfterExpansion = false;
        }
    } else {
        for (size_t i = length; i > 0; --i) {
            UChar32 character = characters[i - 1];
            if (treatAsSpace(character)) {
                count++;
                isAfterExpansion = true;
                continue;
            }
            if (U16_IS_TRAIL(character) && i > 1 && U16_IS_LEAD(characters[i - 2])) {
                character = U16_GET_SUPPLEMENTARY(characters[i - 2], character);
                i--;
            }
            if (expandAroundIdeographs && isCJKIdeographOrSymbol(character)) {
                if (!isAfterExpansion)
                    count++;
                count++;
                isAfterExpansion = true;
                continue;
            }
            isAfterExpansion = false;
        }
    }
    if (!isAfterExpansion && (expansionBehavior & TrailingExpansionMask) == ForceTrailingExpansion) {
        ++count;
        isAfterExpansion = true;
    } else if (isAfterExpansion && (expansionBehavior & TrailingExpansionMask) == ForbidTrailingExpansion) {
        --count;
        isAfterExpansion = false;
    }
    return std::make_pair(count, isAfterExpansion);
}

std::pair<unsigned, bool> FontCascade::expansionOpportunityCount(const StringView& stringView, TextDirection direction, ExpansionBehavior expansionBehavior)
{
    // For each character, iterating from left to right:
    //   If it is recognized as a space, insert an opportunity after it
    //   If it is an ideograph, insert one opportunity before it and one opportunity after it
    // Do this such a way so that there are not two opportunities next to each other.
    if (stringView.is8Bit())
        return expansionOpportunityCountInternal(stringView.characters8(), stringView.length(), direction, expansionBehavior);
    return expansionOpportunityCountInternal(stringView.characters16(), stringView.length(), direction, expansionBehavior);
}

bool FontCascade::leadingExpansionOpportunity(const StringView& stringView, TextDirection direction)
{
    if (!stringView.length())
        return false;

    UChar32 initialCharacter;
    if (direction == LTR) {
        initialCharacter = stringView[0];
        if (U16_IS_LEAD(initialCharacter) && stringView.length() > 1 && U16_IS_TRAIL(stringView[1]))
            initialCharacter = U16_GET_SUPPLEMENTARY(initialCharacter, stringView[1]);
    } else {
        initialCharacter = stringView[stringView.length() - 1];
        if (U16_IS_TRAIL(initialCharacter) && stringView.length() > 1 && U16_IS_LEAD(stringView[stringView.length() - 2]))
            initialCharacter = U16_GET_SUPPLEMENTARY(stringView[stringView.length() - 2], initialCharacter);
    }

    return canExpandAroundIdeographsInComplexText() && isCJKIdeographOrSymbol(initialCharacter);
}

bool FontCascade::trailingExpansionOpportunity(const StringView& stringView, TextDirection direction)
{
    if (!stringView.length())
        return false;

    UChar32 finalCharacter;
    if (direction == LTR) {
        finalCharacter = stringView[stringView.length() - 1];
        if (U16_IS_TRAIL(finalCharacter) && stringView.length() > 1 && U16_IS_LEAD(stringView[stringView.length() - 2]))
            finalCharacter = U16_GET_SUPPLEMENTARY(stringView[stringView.length() - 2], finalCharacter);
    } else {
        finalCharacter = stringView[0];
        if (U16_IS_LEAD(finalCharacter) && stringView.length() > 1 && U16_IS_TRAIL(stringView[1]))
            finalCharacter = U16_GET_SUPPLEMENTARY(finalCharacter, stringView[1]);
    }

    return treatAsSpace(finalCharacter) || (canExpandAroundIdeographsInComplexText() && isCJKIdeographOrSymbol(finalCharacter));
}

bool FontCascade::canReceiveTextEmphasis(UChar32 c)
{
    if (U_GET_GC_MASK(c) & (U_GC_Z_MASK | U_GC_CN_MASK | U_GC_CC_MASK | U_GC_CF_MASK))
        return false;

    // Additional word-separator characters listed in CSS Text Level 3 Editor's Draft 3 November 2010.
    if (c == ethiopicWordspace || c == aegeanWordSeparatorLine || c == aegeanWordSeparatorDot
        || c == ugariticWordDivider || c == tibetanMarkIntersyllabicTsheg || c == tibetanMarkDelimiterTshegBstar)
        return false;

    return true;
}

bool FontCascade::isLoadingCustomFonts() const
{
    return m_fonts && m_fonts->isLoadingCustomFonts();
}
    
GlyphToPathTranslator::GlyphUnderlineType computeUnderlineType(const TextRun& textRun, const GlyphBuffer& glyphBuffer, int index)
{
    // In general, we want to skip descenders. However, skipping descenders on CJK characters leads to undesirable renderings,
    // so we want to draw through CJK characters (on a character-by-character basis).
    UChar32 baseCharacter;
    unsigned offsetInString = glyphBuffer.offsetInString(index);

    if (offsetInString == GlyphBuffer::noOffset || offsetInString >= textRun.length()) {
        // We have no idea which character spawned this glyph. Bail.
        ASSERT_WITH_SECURITY_IMPLICATION(offsetInString < textRun.length());
        return GlyphToPathTranslator::GlyphUnderlineType::DrawOverGlyph;
    }
    
    if (textRun.is8Bit())
        baseCharacter = textRun.characters8()[offsetInString];
    else
        U16_NEXT(textRun.characters16(), offsetInString, textRun.length(), baseCharacter);
    
    // u_getIntPropertyValue with UCHAR_IDEOGRAPHIC doesn't return true for Japanese or Korean codepoints.
    // Instead, we can use the "Unicode allocation block" for the character.
    UBlockCode blockCode = ublock_getCode(baseCharacter);
    switch (blockCode) {
    case UBLOCK_CJK_RADICALS_SUPPLEMENT:
    case UBLOCK_CJK_SYMBOLS_AND_PUNCTUATION:
    case UBLOCK_ENCLOSED_CJK_LETTERS_AND_MONTHS:
    case UBLOCK_CJK_COMPATIBILITY:
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A:
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS:
    case UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS:
    case UBLOCK_CJK_COMPATIBILITY_FORMS:
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B:
    case UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT:
    case UBLOCK_CJK_STROKES:
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C:
    case UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D:
    case UBLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS:
    case UBLOCK_LINEAR_B_IDEOGRAMS:
    case UBLOCK_ENCLOSED_IDEOGRAPHIC_SUPPLEMENT:
    case UBLOCK_HIRAGANA:
    case UBLOCK_KATAKANA:
    case UBLOCK_BOPOMOFO:
    case UBLOCK_BOPOMOFO_EXTENDED:
    case UBLOCK_HANGUL_JAMO:
    case UBLOCK_HANGUL_COMPATIBILITY_JAMO:
    case UBLOCK_HANGUL_SYLLABLES:
    case UBLOCK_HANGUL_JAMO_EXTENDED_A:
    case UBLOCK_HANGUL_JAMO_EXTENDED_B:
        return GlyphToPathTranslator::GlyphUnderlineType::DrawOverGlyph;
    default:
        return GlyphToPathTranslator::GlyphUnderlineType::SkipDescenders;
    }
}

// FIXME: This function may not work if the emphasis mark uses a complex script, but none of the
// standard emphasis marks do so.
bool FontCascade::getEmphasisMarkGlyphData(const AtomicString& mark, GlyphData& glyphData) const
{
    if (mark.isEmpty())
        return false;

    UChar32 character = mark[0];

    if (U16_IS_SURROGATE(character)) {
        if (!U16_IS_SURROGATE_LEAD(character))
            return false;

        if (mark.length() < 2)
            return false;

        UChar low = mark[1];
        if (!U16_IS_TRAIL(low))
            return false;

        character = U16_GET_SUPPLEMENTARY(character, low);
    }

    glyphData = glyphDataForCharacter(character, false, EmphasisMarkVariant);
    return true;
}

int FontCascade::emphasisMarkAscent(const AtomicString& mark) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return 0;

    const Font* markFontData = markGlyphData.font;
    ASSERT(markFontData);
    if (!markFontData)
        return 0;

    return markFontData->fontMetrics().ascent();
}

int FontCascade::emphasisMarkDescent(const AtomicString& mark) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return 0;

    const Font* markFontData = markGlyphData.font;
    ASSERT(markFontData);
    if (!markFontData)
        return 0;

    return markFontData->fontMetrics().descent();
}

int FontCascade::emphasisMarkHeight(const AtomicString& mark) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return 0;

    const Font* markFontData = markGlyphData.font;
    ASSERT(markFontData);
    if (!markFontData)
        return 0;

    return markFontData->fontMetrics().height();
}

float FontCascade::getGlyphsAndAdvancesForSimpleText(const TextRun& run, int from, int to, GlyphBuffer& glyphBuffer, ForTextEmphasisOrNot forTextEmphasis) const
{
    float initialAdvance;

    WidthIterator it(this, run, 0, false, forTextEmphasis);
    // FIXME: Using separate glyph buffers for the prefix and the suffix is incorrect when kerning or
    // ligatures are enabled.
    GlyphBuffer localGlyphBuffer;
    it.advance(from, &localGlyphBuffer);
    float beforeWidth = it.m_runWidthSoFar;
    it.advance(to, &glyphBuffer);

    if (glyphBuffer.isEmpty())
        return 0;

    float afterWidth = it.m_runWidthSoFar;

    if (run.rtl()) {
        float finalRoundingWidth = it.m_finalRoundingWidth;
        it.advance(run.length(), &localGlyphBuffer);
        initialAdvance = finalRoundingWidth + it.m_runWidthSoFar - afterWidth;
    } else
        initialAdvance = beforeWidth;

    if (run.rtl())
        glyphBuffer.reverse(0, glyphBuffer.size());

    return initialAdvance;
}

float FontCascade::drawSimpleText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    // This glyph buffer holds our glyphs+advances+font data for each glyph.
    GlyphBuffer glyphBuffer;

    float startX = point.x() + getGlyphsAndAdvancesForSimpleText(run, from, to, glyphBuffer);

    if (glyphBuffer.isEmpty())
        return 0;

    FloatPoint startPoint(startX, point.y());
    drawGlyphBuffer(context, run, glyphBuffer, startPoint);

    return startPoint.x() - startX;
}

void FontCascade::drawEmphasisMarksForSimpleText(GraphicsContext* context, const TextRun& run, const AtomicString& mark, const FloatPoint& point, int from, int to) const
{
    GlyphBuffer glyphBuffer;
    float initialAdvance = getGlyphsAndAdvancesForSimpleText(run, from, to, glyphBuffer, ForTextEmphasis);

    if (glyphBuffer.isEmpty())
        return;

    drawEmphasisMarks(context, run, glyphBuffer, mark, FloatPoint(point.x() + initialAdvance, point.y()));
}

void FontCascade::drawGlyphBuffer(GraphicsContext* context, const TextRun& run, const GlyphBuffer& glyphBuffer, FloatPoint& point) const
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
#if !ENABLE(SVG_FONTS)
    UNUSED_PARAM(run);
#endif

    // Draw each contiguous run of glyphs that use the same font data.
    const Font* fontData = glyphBuffer.fontAt(0);
    // FIXME: Why do we subtract the initial advance's height but not its width???
    FloatPoint startPoint(point.x(), point.y() - glyphBuffer.initialAdvance().height());
    float nextX = startPoint.x() + glyphBuffer.advanceAt(0).width();
    float nextY = startPoint.y() + glyphBuffer.advanceAt(0).height();
    int lastFrom = 0;
    int nextGlyph = 1;
#if ENABLE(SVG_FONTS)
    TextRun::RenderingContext* renderingContext = run.renderingContext();
#endif
    while (nextGlyph < glyphBuffer.size()) {
        const Font* nextFontData = glyphBuffer.fontAt(nextGlyph);

#if !PLATFORM(WKC)
        if (nextFontData != fontData) {
#else
        if (nextFontData != fontData &&
            wkcUnicodeCategoryPeer(glyphBuffer.glyphAt(nextGlyph)) != WKC_UNICODE_CATEGORY_MARKNONSPACING) {
#endif
#if ENABLE(SVG_FONTS)
            if (renderingContext && fontData->isSVGFont())
                renderingContext->drawSVGGlyphs(context, fontData, glyphBuffer, lastFrom, nextGlyph - lastFrom, startPoint);
            else
#endif
                drawGlyphs(context, fontData, glyphBuffer, lastFrom, nextGlyph - lastFrom, startPoint);

            lastFrom = nextGlyph;
            fontData = nextFontData;
            startPoint.setX(nextX);
            startPoint.setY(nextY);
        }
        nextX += glyphBuffer.advanceAt(nextGlyph).width();
        nextY += glyphBuffer.advanceAt(nextGlyph).height();
        nextGlyph++;
    }

#if ENABLE(SVG_FONTS)
    if (renderingContext && fontData->isSVGFont())
        renderingContext->drawSVGGlyphs(context, fontData, glyphBuffer, lastFrom, nextGlyph - lastFrom, startPoint);
    else
#endif
    {
        drawGlyphs(context, fontData, glyphBuffer, lastFrom, nextGlyph - lastFrom, startPoint);
        point.setX(nextX);
    }
}

inline static float offsetToMiddleOfGlyph(const Font* fontData, Glyph glyph)
{
    if (fontData->platformData().orientation() == Horizontal) {
        FloatRect bounds = fontData->boundsForGlyph(glyph);
        return bounds.x() + bounds.width() / 2;
    }
    // FIXME: Use glyph bounds once they make sense for vertical fonts.
    return fontData->widthForGlyph(glyph) / 2;
}

inline static float offsetToMiddleOfGlyphAtIndex(const GlyphBuffer& glyphBuffer, size_t i)
{
    return offsetToMiddleOfGlyph(glyphBuffer.fontAt(i), glyphBuffer.glyphAt(i));
}

void FontCascade::drawEmphasisMarks(GraphicsContext* context, const TextRun& run, const GlyphBuffer& glyphBuffer, const AtomicString& mark, const FloatPoint& point) const
{
    GlyphData markGlyphData;
    if (!getEmphasisMarkGlyphData(mark, markGlyphData))
        return;

    const Font* markFontData = markGlyphData.font;
    ASSERT(markFontData);
    if (!markFontData)
        return;

    Glyph markGlyph = markGlyphData.glyph;
    Glyph spaceGlyph = markFontData->spaceGlyph();

    float middleOfLastGlyph = offsetToMiddleOfGlyphAtIndex(glyphBuffer, 0);
    FloatPoint startPoint(point.x() + middleOfLastGlyph - offsetToMiddleOfGlyph(markFontData, markGlyph), point.y());

    GlyphBuffer markBuffer;
    for (int i = 0; i + 1 < glyphBuffer.size(); ++i) {
        float middleOfNextGlyph = offsetToMiddleOfGlyphAtIndex(glyphBuffer, i + 1);
        float advance = glyphBuffer.advanceAt(i).width() - middleOfLastGlyph + middleOfNextGlyph;
        markBuffer.add(glyphBuffer.glyphAt(i) ? markGlyph : spaceGlyph, markFontData, advance);
        middleOfLastGlyph = middleOfNextGlyph;
    }
    markBuffer.add(glyphBuffer.glyphAt(glyphBuffer.size() - 1) ? markGlyph : spaceGlyph, markFontData, 0);

    drawGlyphBuffer(context, run, markBuffer, startPoint);
}

float FontCascade::floatWidthForSimpleText(const TextRun& run, HashSet<const Font*>* fallbackFonts, GlyphOverflow* glyphOverflow) const
{
    WidthIterator it(this, run, fallbackFonts, glyphOverflow);
    GlyphBuffer glyphBuffer;
    it.advance(run.length(), (enableKerning() || requiresShaping()) ? &glyphBuffer : nullptr);

    if (glyphOverflow) {
        glyphOverflow->top = std::max<int>(glyphOverflow->top, ceilf(-it.minGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().ascent()));
        glyphOverflow->bottom = std::max<int>(glyphOverflow->bottom, ceilf(it.maxGlyphBoundingBoxY()) - (glyphOverflow->computeBounds ? 0 : fontMetrics().descent()));
        glyphOverflow->left = ceilf(it.firstGlyphOverflow());
        glyphOverflow->right = ceilf(it.lastGlyphOverflow());
    }

    return it.m_runWidthSoFar;
}

void FontCascade::adjustSelectionRectForSimpleText(const TextRun& run, LayoutRect& selectionRect, int from, int to) const
{
    GlyphBuffer glyphBuffer;
    WidthIterator it(this, run);
    it.advance(from, &glyphBuffer);
    float beforeWidth = it.m_runWidthSoFar;
    it.advance(to, &glyphBuffer);
    float afterWidth = it.m_runWidthSoFar;
    float totalWidth = -1;

    if (run.rtl()) {
        it.advance(run.length(), &glyphBuffer);
        totalWidth = it.m_runWidthSoFar;
        selectionRect.move(totalWidth - afterWidth, 0);
    } else
        selectionRect.move(beforeWidth, 0);
    selectionRect.setWidth(LayoutUnit::fromFloatCeil(afterWidth - beforeWidth));
}

int FontCascade::offsetForPositionForSimpleText(const TextRun& run, float x, bool includePartialGlyphs) const
{
    float delta = x;

    WidthIterator it(this, run);
    GlyphBuffer localGlyphBuffer;
    unsigned offset;
    if (run.rtl()) {
        delta -= floatWidthForSimpleText(run);
        while (1) {
            offset = it.m_currentCharacter;
            float w;
            if (!it.advanceOneCharacter(w, localGlyphBuffer))
                break;
            delta += w;
            if (includePartialGlyphs) {
                if (delta - w / 2 >= 0)
                    break;
            } else {
                if (delta >= 0)
                    break;
            }
        }
    } else {
        while (1) {
            offset = it.m_currentCharacter;
            float w;
            if (!it.advanceOneCharacter(w, localGlyphBuffer))
                break;
            delta -= w;
            if (includePartialGlyphs) {
                if (delta + w / 2 <= 0)
                    break;
            } else {
                if (delta <= 0)
                    break;
            }
        }
    }

    return offset;
}


}
