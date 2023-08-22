/*
 * Copyright (C) 2005, 2008, 2010, 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Font.h"

#if PLATFORM(COCOA)
#include "CoreTextSPI.h"
#endif
#include "FontCache.h"
#include "FontCascade.h"
#include "OpenTypeMathData.h"
#include <wtf/MathExtras.h>
#include <wtf/NeverDestroyed.h>

#if ENABLE(OPENTYPE_VERTICAL)
#include "OpenTypeVerticalData.h"
#endif

namespace WebCore {

#if !PLATFORM(WKC)
unsigned GlyphPage::s_count = 0;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(unsigned, GlyphPage, s_count, 0);
#endif

const float smallCapsFontSizeMultiplier = 0.7f;
const float emphasisMarkFontSizeMultiplier = 0.5f;

Font::Font(const FontPlatformData& platformData, std::unique_ptr<SVGData>&& svgData, bool isCustomFont, bool isLoading, bool isTextOrientationFallback)
    : m_maxCharWidth(-1)
    , m_avgCharWidth(-1)
    , m_platformData(platformData)
    , m_svgData(WTF::move(svgData))
    , m_mathData(nullptr)
    , m_treatAsFixedPitch(false)
    , m_isCustomFont(isCustomFont)
    , m_isLoading(isLoading)
    , m_isTextOrientationFallback(isTextOrientationFallback)
    , m_isBrokenIdeographFallback(false)
    , m_hasVerticalGlyphs(false)
    , m_isUsedInSystemFallbackCache(false)
#if PLATFORM(COCOA) || PLATFORM(WIN)
    , m_isSystemFont(false)
#endif
#if PLATFORM(IOS)
    , m_shouldNotBeUsedForArabic(false)
#endif
{
}

Font::Font(const FontPlatformData& platformData, bool isCustomFont, bool isLoading, bool isTextOrientationFallback)
    : Font(platformData, std::unique_ptr<SVGData>(), isCustomFont, isLoading, isTextOrientationFallback)
{
    platformInit();
    platformGlyphInit();
    platformCharWidthInit();
#if ENABLE(OPENTYPE_VERTICAL)
    if (platformData.orientation() == Vertical && !isTextOrientationFallback) {
        m_verticalData = platformData.verticalData();
        m_hasVerticalGlyphs = m_verticalData.get() && m_verticalData->hasVerticalMetrics();
    }
#endif
}

Font::Font(std::unique_ptr<SVGData> svgData, float fontSize, bool syntheticBold, bool syntheticItalic)
    : Font(FontPlatformData(fontSize, syntheticBold, syntheticItalic), WTF::move(svgData), true, false, false)
{
    m_svgData->initializeFont(this, fontSize);
}

// Estimates of avgCharWidth and maxCharWidth for platforms that don't support accessing these values from the font.
void Font::initCharWidths()
{
    auto* glyphPageZero = glyphPage(0);

    // Treat the width of a '0' as the avgCharWidth.
    if (m_avgCharWidth <= 0.f && glyphPageZero) {
        static const UChar32 digitZeroChar = '0';
        Glyph digitZeroGlyph = glyphPageZero->glyphDataForCharacter(digitZeroChar).glyph;
        if (digitZeroGlyph)
            m_avgCharWidth = widthForGlyph(digitZeroGlyph);
    }

    // If we can't retrieve the width of a '0', fall back to the x height.
    if (m_avgCharWidth <= 0.f)
        m_avgCharWidth = m_fontMetrics.xHeight();

    if (m_maxCharWidth <= 0.f)
        m_maxCharWidth = std::max(m_avgCharWidth, m_fontMetrics.floatAscent());
}

void Font::platformGlyphInit()
{
    auto* glyphPageZero = glyphPage(0);
    if (!glyphPageZero) {
        m_spaceGlyph = 0;
        m_spaceWidth = 0;
        m_zeroGlyph = 0;
        m_adjustedSpaceWidth = 0;
        determinePitch();
        m_zeroWidthSpaceGlyph = 0;
        return;
    }

    // Ask for the glyph for 0 to avoid paging in ZERO WIDTH SPACE. Control characters, including 0,
    // are mapped to the ZERO WIDTH SPACE glyph.
    m_zeroWidthSpaceGlyph = glyphPageZero->glyphDataForCharacter(0).glyph;

    // Nasty hack to determine if we should round or ceil space widths.
    // If the font is monospace or fake monospace we ceil to ensure that 
    // every character and the space are the same width. Otherwise we round.
    m_spaceGlyph = glyphPageZero->glyphDataForCharacter(' ').glyph;
    float width = widthForGlyph(m_spaceGlyph);
    m_spaceWidth = width;
    m_zeroGlyph = glyphPageZero->glyphDataForCharacter('0').glyph;
    m_fontMetrics.setZeroWidth(widthForGlyph(m_zeroGlyph));
    determinePitch();
    m_adjustedSpaceWidth = m_treatAsFixedPitch ? ceilf(width) : roundf(width);

    // Force the glyph for ZERO WIDTH SPACE to have zero width, unless it is shared with SPACE.
    // Helvetica is an example of a non-zero width ZERO WIDTH SPACE glyph.
    // See <http://bugs.webkit.org/show_bug.cgi?id=13178> and Font::isZeroWidthSpaceGlyph()
    if (m_zeroWidthSpaceGlyph == m_spaceGlyph)
        m_zeroWidthSpaceGlyph = 0;
}

Font::~Font()
{
    removeFromSystemFallbackCache();
}

static bool fillGlyphPage(GlyphPage& pageToFill, unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const Font* font)
{
#if ENABLE(SVG_FONTS)
    if (auto* svgData = font->svgData())
        return svgData->fillSVGGlyphPage(&pageToFill, offset, length, buffer, bufferLength, font);
#endif
    bool hasGlyphs = pageToFill.fill(offset, length, buffer, bufferLength, font);
#if ENABLE(OPENTYPE_VERTICAL)
    if (hasGlyphs && font->verticalData())
        font->verticalData()->substituteWithVerticalGlyphs(font, &pageToFill, offset, length);
#endif
    return hasGlyphs;
}

static RefPtr<GlyphPage> createAndFillGlyphPage(unsigned pageNumber, const Font* font)
{
#if PLATFORM(IOS)
    // FIXME: Times New Roman contains Arabic glyphs, but Core Text doesn't know how to shape them. See <rdar://problem/9823975>.
    // Once we have the fix for <rdar://problem/9823975> then remove this code together with Font::shouldNotBeUsedForArabic()
    // in <rdar://problem/12096835>.
    if (pageNumber == 6 && font->shouldNotBeUsedForArabic())
        return nullptr;
#endif

    unsigned start = pageNumber * GlyphPage::size;
    UChar buffer[GlyphPage::size * 2 + 2];
    unsigned bufferLength;
    // Fill in a buffer with the entire "page" of characters that we want to look up glyphs for.
    if (U_IS_BMP(start)) {
        bufferLength = GlyphPage::size;
        for (unsigned i = 0; i < GlyphPage::size; i++)
            buffer[i] = start + i;

        if (!start) {
            // Control characters must not render at all.
            for (unsigned i = 0; i < 0x20; ++i)
                buffer[i] = zeroWidthSpace;
            for (unsigned i = 0x7F; i < 0xA0; i++)
                buffer[i] = zeroWidthSpace;
            buffer[softHyphen] = zeroWidthSpace;

            // \n, \t, and nonbreaking space must render as a space.
            buffer[(int)'\n'] = ' ';
            buffer[(int)'\t'] = ' ';
            buffer[noBreakSpace] = ' ';
        } else if (start == (leftToRightMark & ~(GlyphPage::size - 1))) {
            // LRM, RLM, LRE, RLE, ZWNJ, ZWJ, and PDF must not render at all.
            buffer[leftToRightMark - start] = zeroWidthSpace;
            buffer[rightToLeftMark - start] = zeroWidthSpace;
            buffer[leftToRightEmbed - start] = zeroWidthSpace;
            buffer[rightToLeftEmbed - start] = zeroWidthSpace;
            buffer[leftToRightOverride - start] = zeroWidthSpace;
            buffer[rightToLeftOverride - start] = zeroWidthSpace;
            buffer[zeroWidthNonJoiner - start] = zeroWidthSpace;
            buffer[zeroWidthJoiner - start] = zeroWidthSpace;
            buffer[popDirectionalFormatting - start] = zeroWidthSpace;
        } else if (start == (objectReplacementCharacter & ~(GlyphPage::size - 1))) {
            // Object replacement character must not render at all.
            buffer[objectReplacementCharacter - start] = zeroWidthSpace;
        } else if (start == (zeroWidthNoBreakSpace & ~(GlyphPage::size - 1))) {
            // ZWNBS/BOM must not render at all.
            buffer[zeroWidthNoBreakSpace - start] = zeroWidthSpace;
        }
    } else {
        bufferLength = GlyphPage::size * 2;
        for (unsigned i = 0; i < GlyphPage::size; i++) {
            int c = i + start;
            buffer[i * 2] = U16_LEAD(c);
            buffer[i * 2 + 1] = U16_TRAIL(c);
        }
    }

    // Now that we have a buffer full of characters, we want to get back an array
    // of glyph indices. This part involves calling into the platform-specific
    // routine of our glyph map for actually filling in the page with the glyphs.
    // Success is not guaranteed. For example, Times fails to fill page 260, giving glyph data
    // for only 128 out of 256 characters.
    RefPtr<GlyphPage> glyphPage;
    if (GlyphPage::mayUseMixedFontsWhenFilling(buffer, bufferLength, font))
        glyphPage = GlyphPage::createForMixedFonts();
    else
        glyphPage = GlyphPage::createForSingleFont(font);

    bool haveGlyphs = fillGlyphPage(*glyphPage, 0, GlyphPage::size, buffer, bufferLength, font);
    if (!haveGlyphs)
        return nullptr;

    glyphPage->setImmutable();
    return glyphPage;
}

const GlyphPage* Font::glyphPage(unsigned pageNumber) const
{
    if (!pageNumber) {
        if (!m_glyphPageZero)
            m_glyphPageZero = createAndFillGlyphPage(0, this);
        return m_glyphPageZero.get();
    }
    auto addResult = m_glyphPages.add(pageNumber, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = createAndFillGlyphPage(pageNumber, this);

    return addResult.iterator->value.get();
}

Glyph Font::glyphForCharacter(UChar32 character) const
{
    auto* page = glyphPage(character / GlyphPage::size);
    if (!page)
        return 0;
    return page->glyphAt(character % GlyphPage::size);
}

GlyphData Font::glyphDataForCharacter(UChar32 character) const
{
    auto* page = glyphPage(character / GlyphPage::size);
    if (!page)
        return GlyphData();
    return page->glyphDataForCharacter(character);
}

PassRefPtr<Font> Font::verticalRightOrientationFont() const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->verticalRightOrientation) {
        FontPlatformData verticalRightPlatformData(m_platformData);
        verticalRightPlatformData.setOrientation(Horizontal);
        m_derivedFontData->verticalRightOrientation = create(verticalRightPlatformData, isCustomFont(), false, true);
    }
    ASSERT(m_derivedFontData->verticalRightOrientation != this);
    return m_derivedFontData->verticalRightOrientation;
}

PassRefPtr<Font> Font::uprightOrientationFont() const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->uprightOrientation)
        m_derivedFontData->uprightOrientation = create(m_platformData, isCustomFont(), false, true);
    ASSERT(m_derivedFontData->uprightOrientation != this);
    return m_derivedFontData->uprightOrientation;
}

PassRefPtr<Font> Font::smallCapsFont(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = createScaledFont(fontDescription, smallCapsFontSizeMultiplier);
    ASSERT(m_derivedFontData->smallCaps != this);
    return m_derivedFontData->smallCaps;
}

#if (PLATFORM(COCOA) && PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
const Font& Font::noSynthesizableFeaturesFont() const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->noSynthesizableFeatures)
        m_derivedFontData->noSynthesizableFeatures = createFontWithoutSynthesizableFeatures();
    ASSERT(m_derivedFontData->noSynthesizableFeatures != this);
    return *m_derivedFontData->noSynthesizableFeatures;
}
#endif

PassRefPtr<Font> Font::emphasisMarkFont(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = createScaledFont(fontDescription, emphasisMarkFontSizeMultiplier);
    ASSERT(m_derivedFontData->emphasisMark != this);
    return m_derivedFontData->emphasisMark;
}

PassRefPtr<Font> Font::brokenIdeographFont() const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->brokenIdeograph) {
        m_derivedFontData->brokenIdeograph = create(m_platformData, isCustomFont(), false);
        m_derivedFontData->brokenIdeograph->m_isBrokenIdeographFallback = true;
    }
    ASSERT(m_derivedFontData->brokenIdeograph != this);
    return m_derivedFontData->brokenIdeograph;
}

PassRefPtr<Font> Font::nonSyntheticItalicFont() const
{
    if (!m_derivedFontData)
        m_derivedFontData = std::make_unique<DerivedFontData>(isCustomFont());
    if (!m_derivedFontData->nonSyntheticItalic) {
        FontPlatformData nonSyntheticItalicFontPlatformData(m_platformData);
#if PLATFORM(COCOA) || USE(CAIRO)
        nonSyntheticItalicFontPlatformData.setSyntheticOblique(false);
#endif
        m_derivedFontData->nonSyntheticItalic = create(nonSyntheticItalicFontPlatformData, isCustomFont());
    }
    ASSERT(m_derivedFontData->nonSyntheticItalic != this);
    return m_derivedFontData->nonSyntheticItalic;
}

#if !LOG_DISABLED
String Font::description() const
{
    if (isSVGFont())
        return "[SVG font]";
    if (isCustomFont())
        return "[custom font]";

    return platformData().description();
}
#endif

const OpenTypeMathData* Font::mathData() const
{
    if (m_isLoading)
        return nullptr;
    if (!m_mathData) {
        m_mathData = OpenTypeMathData::create(m_platformData);
        if (!m_mathData->hasMathData())
            m_mathData = nullptr;
    }
    return m_mathData.get();
}

Font::DerivedFontData::~DerivedFontData()
{
}

PassRefPtr<Font> Font::createScaledFont(const FontDescription& fontDescription, float scaleFactor) const
{
    if (isSVGFont())
        return nullptr;

    return platformCreateScaledFont(fontDescription, scaleFactor);
}

bool Font::applyTransforms(GlyphBufferGlyph* glyphs, GlyphBufferAdvance* advances, size_t glyphCount, bool enableKerning, bool requiresShaping) const
{
    // We need to handle transforms on SVG fonts internally, since they are rendered internally.
    ASSERT(!isSVGFont());
#if PLATFORM(COCOA)
    CTFontTransformOptions options = (enableKerning ? kCTFontTransformApplyPositioning : 0) | (requiresShaping ? kCTFontTransformApplyShaping : 0);
    return CTFontTransformGlyphs(m_platformData.ctFont(), glyphs, reinterpret_cast<CGSize*>(advances), glyphCount, options);
#else
    UNUSED_PARAM(glyphs);
    UNUSED_PARAM(advances);
    UNUSED_PARAM(glyphCount);
    UNUSED_PARAM(enableKerning);
    UNUSED_PARAM(requiresShaping);
    return false;
#endif
}

// Fonts are not ref'd to avoid cycles.
typedef std::pair<UChar32, bool /* isForPlatformFont */> CharacterFallbackMapKey;
typedef HashMap<CharacterFallbackMapKey, Font*> CharacterFallbackMap;
typedef HashMap<const Font*, CharacterFallbackMap> SystemFallbackCache;

static SystemFallbackCache& systemFallbackCache()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<SystemFallbackCache> map;
    return map.get();
#else
    WKC_DEFINE_STATIC_PTR(SystemFallbackCache*, map, 0);
    if (!map)
        map = new SystemFallbackCache();
    return *map;
#endif
}

RefPtr<Font> Font::systemFallbackFontForCharacter(UChar32 character, const FontDescription& description, bool isForPlatformFont) const
{
    auto fontAddResult = systemFallbackCache().add(this, CharacterFallbackMap());

    if (!character) {
        UChar codeUnit = 0;
        return FontCache::singleton().systemFallbackForCharacters(description, this, isForPlatformFont, &codeUnit, 1);
    }

    auto key = std::make_pair(character, isForPlatformFont);
    auto characterAddResult = fontAddResult.iterator->value.add(key, nullptr);

    Font*& fallbackFont = characterAddResult.iterator->value;

    if (!fallbackFont) {
        UChar codeUnits[2];
        unsigned codeUnitsLength;
        if (U_IS_BMP(character)) {
            codeUnits[0] = FontCascade::normalizeSpaces(character);
            codeUnitsLength = 1;
        } else {
            codeUnits[0] = U16_LEAD(character);
            codeUnits[1] = U16_TRAIL(character);
            codeUnitsLength = 2;
        }

        fallbackFont = FontCache::singleton().systemFallbackForCharacters(description, this, isForPlatformFont, codeUnits, codeUnitsLength).get();
        if (fallbackFont)
            fallbackFont->m_isUsedInSystemFallbackCache = true;
    }

    return fallbackFont;
}

void Font::removeFromSystemFallbackCache()
{
    systemFallbackCache().remove(this);

    if (!m_isUsedInSystemFallbackCache)
        return;

    for (auto& characterMap : systemFallbackCache().values()) {
        Vector<CharacterFallbackMapKey, 512> toRemove;
        for (auto& entry : characterMap) {
            if (entry.value == this)
                toRemove.append(entry.key);
        }
        for (auto& key : toRemove)
            characterMap.remove(key);
    }
}

} // namespace WebCore
