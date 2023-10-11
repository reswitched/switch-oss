/*
 * Copyright (C) 2003, 2006 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef TextFlags_h
#define TextFlags_h

// <rdar://problem/16980736>: Web fonts crash on certain OSes when using CTFontManagerCreateFontDescriptorFromData()
// FIXME: When we have moved entirely to CORETEXT_WEB_FONTS, remove the isCustomFont member variable from Font, since it will no longer be used.
// See https://bug-145873-attachments.webkit.org/attachment.cgi?id=254710
#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED < 80000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101000)
#define CORETEXT_WEB_FONTS 0
#else
#define CORETEXT_WEB_FONTS 1
#endif

namespace WebCore {

enum TextRenderingMode { AutoTextRendering, OptimizeSpeed, OptimizeLegibility, GeometricPrecision };

enum FontSmoothingMode { AutoSmoothing, NoSmoothing, Antialiased, SubpixelAntialiased };

// This setting is used to provide ways of switching between multiple rendering modes that may have different
// metrics. It is used to switch between CG and GDI text on Windows.
enum FontRenderingMode { NormalRenderingMode, AlternateRenderingMode };

enum FontOrientation { Horizontal, Vertical };

enum NonCJKGlyphOrientation { NonCJKGlyphOrientationVerticalRight, NonCJKGlyphOrientationUpright };

// Here, "Leading" and "Trailing" are relevant after the line has been rearranged for bidi.
// ("Leading" means "left" and "Trailing" means "right.")
enum ExpansionBehaviorFlags {
    ForbidTrailingExpansion = 0 << 0,
    AllowTrailingExpansion = 1 << 0,
    ForceTrailingExpansion = 2 << 0,
    TrailingExpansionMask = 3 << 0,

    ForbidLeadingExpansion = 0 << 2,
    AllowLeadingExpansion = 1 << 2,
    ForceLeadingExpansion = 2 << 2,
    LeadingExpansionMask = 3 << 2,
};
typedef unsigned ExpansionBehavior;

enum FontSynthesisValues {
    FontSynthesisNone = 0x0,
    FontSynthesisWeight = 0x1,
    FontSynthesisStyle = 0x2
};
typedef unsigned FontSynthesis;
const unsigned FontSynthesisWidth = 2;

enum class FontVariantLigatures {
    Normal,
    Yes,
    No
};

enum class FontVariantPosition {
    Normal,
    Subscript,
    Superscript
};

enum class FontVariantCaps {
    Normal,
    Small,
    AllSmall,
    Petite,
    AllPetite,
    Unicase,
    Titling
};

enum class FontVariantNumericFigure {
    Normal,
    LiningNumbers,
    OldStyleNumbers
};

enum class FontVariantNumericSpacing {
    Normal,
    ProportionalNumbers,
    TabularNumbers
};

enum class FontVariantNumericFraction {
    Normal,
    DiagonalFractions,
    StackedFractions
};

enum class FontVariantNumericOrdinal {
    Normal,
    Yes
};

enum class FontVariantNumericSlashedZero {
    Normal,
    Yes
};

enum class FontVariantAlternates {
    Normal,
    HistoricalForms
};

enum class FontVariantEastAsianVariant {
    Normal,
    Jis78,
    Jis83,
    Jis90,
    Jis04,
    Simplified,
    Traditional
};

enum class FontVariantEastAsianWidth {
    Normal,
    Full,
    Proportional
};

enum class FontVariantEastAsianRuby {
    Normal,
    Yes
};

struct FontVariantSettings {
    FontVariantSettings()
        : commonLigatures(FontVariantLigatures::Normal)
        , discretionaryLigatures(FontVariantLigatures::Normal)
        , historicalLigatures(FontVariantLigatures::Normal)
        , contextualAlternates(FontVariantLigatures::Normal)
        , position(FontVariantPosition::Normal)
        , caps(FontVariantCaps::Normal)
        , numericFigure(FontVariantNumericFigure::Normal)
        , numericSpacing(FontVariantNumericSpacing::Normal)
        , numericFraction(FontVariantNumericFraction::Normal)
        , numericOrdinal(FontVariantNumericOrdinal::Normal)
        , numericSlashedZero(FontVariantNumericSlashedZero::Normal)
        , alternates(FontVariantAlternates::Normal)
        , eastAsianVariant(FontVariantEastAsianVariant::Normal)
        , eastAsianWidth(FontVariantEastAsianWidth::Normal)
        , eastAsianRuby(FontVariantEastAsianRuby::Normal)
    {
    }

    FontVariantSettings(
        FontVariantLigatures commonLigatures,
        FontVariantLigatures discretionaryLigatures,
        FontVariantLigatures historicalLigatures,
        FontVariantLigatures contextualAlternates,
        FontVariantPosition position,
        FontVariantCaps caps,
        FontVariantNumericFigure numericFigure,
        FontVariantNumericSpacing numericSpacing,
        FontVariantNumericFraction numericFraction,
        FontVariantNumericOrdinal numericOrdinal,
        FontVariantNumericSlashedZero numericSlashedZero,
        FontVariantAlternates alternates,
        FontVariantEastAsianVariant eastAsianVariant,
        FontVariantEastAsianWidth eastAsianWidth,
        FontVariantEastAsianRuby eastAsianRuby)
            : commonLigatures(commonLigatures)
            , discretionaryLigatures(discretionaryLigatures)
            , historicalLigatures(historicalLigatures)
            , contextualAlternates(contextualAlternates)
            , position(position)
            , caps(caps)
            , numericFigure(numericFigure)
            , numericSpacing(numericSpacing)
            , numericFraction(numericFraction)
            , numericOrdinal(numericOrdinal)
            , numericSlashedZero(numericSlashedZero)
            , alternates(alternates)
            , eastAsianVariant(eastAsianVariant)
            , eastAsianWidth(eastAsianWidth)
            , eastAsianRuby(eastAsianRuby)
    {
    }

    bool isAllNormal() const
    {
        return commonLigatures == FontVariantLigatures::Normal
            && discretionaryLigatures == FontVariantLigatures::Normal
            && historicalLigatures == FontVariantLigatures::Normal
            && contextualAlternates == FontVariantLigatures::Normal
            && position == FontVariantPosition::Normal
            && caps == FontVariantCaps::Normal
            && numericFigure == FontVariantNumericFigure::Normal
            && numericSpacing == FontVariantNumericSpacing::Normal
            && numericFraction == FontVariantNumericFraction::Normal
            && numericOrdinal == FontVariantNumericOrdinal::Normal
            && numericSlashedZero == FontVariantNumericSlashedZero::Normal
            && alternates == FontVariantAlternates::Normal
            && eastAsianVariant == FontVariantEastAsianVariant::Normal
            && eastAsianWidth == FontVariantEastAsianWidth::Normal
            && eastAsianRuby == FontVariantEastAsianRuby::Normal;
    }

    bool operator==(const FontVariantSettings& other)
    {
        return commonLigatures == other.commonLigatures
            && discretionaryLigatures == other.discretionaryLigatures
            && historicalLigatures == other.historicalLigatures
            && contextualAlternates == other.contextualAlternates
            && position == other.position
            && caps == other.caps
            && numericFigure == other.numericFigure
            && numericSpacing == other.numericSpacing
            && numericFraction == other.numericFraction
            && numericOrdinal == other.numericOrdinal
            && numericSlashedZero == other.numericSlashedZero
            && alternates == other.alternates
            && eastAsianVariant == other.eastAsianVariant
            && eastAsianWidth == other.eastAsianWidth
            && eastAsianRuby == other.eastAsianRuby;
    }

    FontVariantLigatures commonLigatures;
    FontVariantLigatures discretionaryLigatures;
    FontVariantLigatures historicalLigatures;
    FontVariantLigatures contextualAlternates;
    FontVariantPosition position;
    FontVariantCaps caps;
    FontVariantNumericFigure numericFigure;
    FontVariantNumericSpacing numericSpacing;
    FontVariantNumericFraction numericFraction;
    FontVariantNumericOrdinal numericOrdinal;
    FontVariantNumericSlashedZero numericSlashedZero;
    FontVariantAlternates alternates;
    FontVariantEastAsianVariant eastAsianVariant;
    FontVariantEastAsianWidth eastAsianWidth;
    FontVariantEastAsianRuby eastAsianRuby;
};

enum FontWidthVariant {
    RegularWidth,
    HalfWidth,
    ThirdWidth,
    QuarterWidth,
    LastFontWidthVariant = QuarterWidth
};

const unsigned FontWidthVariantWidth = 2;

COMPILE_ASSERT(!(LastFontWidthVariant >> FontWidthVariantWidth), FontWidthVariantWidth_is_correct);

enum FontWeight {
    FontWeight100,
    FontWeight200,
    FontWeight300,
    FontWeight400,
    FontWeight500,
    FontWeight600,
    FontWeight700,
    FontWeight800,
    FontWeight900,
    FontWeightNormal = FontWeight400,
    FontWeightBold = FontWeight700
};

enum FontItalic {
    FontItalicOff = 0,
    FontItalicOn = 1
};

enum FontSmallCaps {
    FontSmallCapsOff = 0,
    FontSmallCapsOn = 1
};

enum {
    FontStyleNormalBit = 0,
    FontStyleItalicBit,
    FontWeight100Bit,
    FontWeight200Bit,
    FontWeight300Bit,
    FontWeight400Bit,
    FontWeight500Bit,
    FontWeight600Bit,
    FontWeight700Bit,
    FontWeight800Bit,
    FontWeight900Bit,
    FontTraitsMaskWidth
};

enum FontTraitsMask {
    FontStyleNormalMask = 1 << FontStyleNormalBit,
    FontStyleItalicMask = 1 << FontStyleItalicBit,
    FontStyleMask = FontStyleNormalMask | FontStyleItalicMask,

    FontWeight100Mask = 1 << FontWeight100Bit,
    FontWeight200Mask = 1 << FontWeight200Bit,
    FontWeight300Mask = 1 << FontWeight300Bit,
    FontWeight400Mask = 1 << FontWeight400Bit,
    FontWeight500Mask = 1 << FontWeight500Bit,
    FontWeight600Mask = 1 << FontWeight600Bit,
    FontWeight700Mask = 1 << FontWeight700Bit,
    FontWeight800Mask = 1 << FontWeight800Bit,
    FontWeight900Mask = 1 << FontWeight900Bit,
    FontWeightMask = FontWeight100Mask | FontWeight200Mask | FontWeight300Mask | FontWeight400Mask | FontWeight500Mask | FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask
};

enum class Kerning {
    Auto,
    Normal,
    NoShift
};

}

#endif
