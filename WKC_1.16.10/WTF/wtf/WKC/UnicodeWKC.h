/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2007 Apple Computer, Inc. All rights reserved.
 *  Copyright (C) 2008 Jurg Billeter <j@bitron.ch>
 *  Copyright (C) 2008 Dominik Rottsches <dominik.roettsches@access-company.com>
 *  Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef UnicodeWKC_h
#define UnicodeWKC_h

//#include "UnicodeMacrosFromICU.h"
#include <stdint.h>
#include <wkc/wkcpeer.h>

typedef uint16_t UChar;
typedef int32_t UChar32;

// from uscript.h
typedef enum UScriptCode {
      USCRIPT_INVALID_CODE = -1,
      USCRIPT_COMMON       =  0 , /* Zyyy */
      USCRIPT_INHERITED    =  1,  /* Qaai */
      USCRIPT_ARABIC       =  2,  /* Arab */
      USCRIPT_ARMENIAN     =  3,  /* Armn */
      USCRIPT_BENGALI      =  4,  /* Beng */
      USCRIPT_BOPOMOFO     =  5,  /* Bopo */
      USCRIPT_CHEROKEE     =  6,  /* Cher */
      USCRIPT_COPTIC       =  7,  /* Copt */
      USCRIPT_CYRILLIC     =  8,  /* Cyrl */
      USCRIPT_DESERET      =  9,  /* Dsrt */
      USCRIPT_DEVANAGARI   = 10,  /* Deva */
      USCRIPT_ETHIOPIC     = 11,  /* Ethi */
      USCRIPT_GEORGIAN     = 12,  /* Geor */
      USCRIPT_GOTHIC       = 13,  /* Goth */
      USCRIPT_GREEK        = 14,  /* Grek */
      USCRIPT_GUJARATI     = 15,  /* Gujr */
      USCRIPT_GURMUKHI     = 16,  /* Guru */
      USCRIPT_HAN          = 17,  /* Hani */
      USCRIPT_HANGUL       = 18,  /* Hang */
      USCRIPT_HEBREW       = 19,  /* Hebr */
      USCRIPT_HIRAGANA     = 20,  /* Hira */
      USCRIPT_KANNADA      = 21,  /* Knda */
      USCRIPT_KATAKANA     = 22,  /* Kana */
      USCRIPT_KHMER        = 23,  /* Khmr */
      USCRIPT_LAO          = 24,  /* Laoo */
      USCRIPT_LATIN        = 25,  /* Latn */
      USCRIPT_MALAYALAM    = 26,  /* Mlym */
      USCRIPT_MONGOLIAN    = 27,  /* Mong */
      USCRIPT_MYANMAR      = 28,  /* Mymr */
      USCRIPT_OGHAM        = 29,  /* Ogam */
      USCRIPT_OLD_ITALIC   = 30,  /* Ital */
      USCRIPT_ORIYA        = 31,  /* Orya */
      USCRIPT_RUNIC        = 32,  /* Runr */
      USCRIPT_SINHALA      = 33,  /* Sinh */
      USCRIPT_SYRIAC       = 34,  /* Syrc */
      USCRIPT_TAMIL        = 35,  /* Taml */
      USCRIPT_TELUGU       = 36,  /* Telu */
      USCRIPT_THAANA       = 37,  /* Thaa */
      USCRIPT_THAI         = 38,  /* Thai */
      USCRIPT_TIBETAN      = 39,  /* Tibt */
      /** Canadian_Aboriginal script. @stable ICU 2.6 */
      USCRIPT_CANADIAN_ABORIGINAL = 40,  /* Cans */
      /** Canadian_Aboriginal script (alias). @stable ICU 2.2 */
      USCRIPT_UCAS         = USCRIPT_CANADIAN_ABORIGINAL,
      USCRIPT_YI           = 41,  /* Yiii */
      USCRIPT_TAGALOG      = 42,  /* Tglg */
      USCRIPT_HANUNOO      = 43,  /* Hano */
      USCRIPT_BUHID        = 44,  /* Buhd */
      USCRIPT_TAGBANWA     = 45,  /* Tagb */

      /* New scripts in Unicode 4 @stable ICU 2.6 */
      USCRIPT_BRAILLE      = 46,  /* Brai */
      USCRIPT_CYPRIOT      = 47,  /* Cprt */
      USCRIPT_LIMBU        = 48,  /* Limb */
      USCRIPT_LINEAR_B     = 49,  /* Linb */
      USCRIPT_OSMANYA      = 50,  /* Osma */
      USCRIPT_SHAVIAN      = 51,  /* Shaw */
      USCRIPT_TAI_LE       = 52,  /* Tale */
      USCRIPT_UGARITIC     = 53,  /* Ugar */

      /** New script code in Unicode 4.0.1 @stable ICU 3.0 */
      USCRIPT_KATAKANA_OR_HIRAGANA = 54,/*Hrkt */
      
      /* New scripts in Unicode 4.1 @draft ICU 3.4 */
      USCRIPT_BUGINESE      = 55, /* Bugi */
      USCRIPT_GLAGOLITIC    = 56, /* Glag */
      USCRIPT_KHAROSHTHI    = 57, /* Khar */
      USCRIPT_SYLOTI_NAGRI  = 58, /* Sylo */
      USCRIPT_NEW_TAI_LUE   = 59, /* Talu */
      USCRIPT_TIFINAGH      = 60, /* Tfng */
      USCRIPT_OLD_PERSIAN   = 61, /* Xpeo */

      /* New script codes from ISO 15924 @draft ICU 3.6 */
      USCRIPT_BALINESE                      = 62, /* Bali */
      USCRIPT_BATAK                         = 63, /* Batk */
      USCRIPT_BLISSYMBOLS                   = 64, /* Blis */
      USCRIPT_BRAHMI                        = 65, /* Brah */
      USCRIPT_CHAM                          = 66, /* Cham */
      USCRIPT_CIRTH                         = 67, /* Cirt */
      USCRIPT_OLD_CHURCH_SLAVONIC_CYRILLIC  = 68, /* Cyrs */
      USCRIPT_DEMOTIC_EGYPTIAN              = 69, /* Egyd */
      USCRIPT_HIERATIC_EGYPTIAN             = 70, /* Egyh */
      USCRIPT_EGYPTIAN_HIEROGLYPHS          = 71, /* Egyp */
      USCRIPT_KHUTSURI                      = 72, /* Geok */
      USCRIPT_SIMPLIFIED_HAN                = 73, /* Hans */
      USCRIPT_TRADITIONAL_HAN               = 74, /* Hant */
      USCRIPT_PAHAWH_HMONG                  = 75, /* Hmng */
      USCRIPT_OLD_HUNGARIAN                 = 76, /* Hung */
      USCRIPT_HARAPPAN_INDUS                = 77, /* Inds */
      USCRIPT_JAVANESE                      = 78, /* Java */
      USCRIPT_KAYAH_LI                      = 79, /* Kali */
      USCRIPT_LATIN_FRAKTUR                 = 80, /* Latf */
      USCRIPT_LATIN_GAELIC                  = 81, /* Latg */
      USCRIPT_LEPCHA                        = 82, /* Lepc */
      USCRIPT_LINEAR_A                      = 83, /* Lina */
      USCRIPT_MANDAEAN                      = 84, /* Mand */
      USCRIPT_MAYAN_HIEROGLYPHS             = 85, /* Maya */
      USCRIPT_MEROITIC                      = 86, /* Mero */
      USCRIPT_NKO                           = 87, /* Nkoo */
      USCRIPT_ORKHON                        = 88, /* Orkh */
      USCRIPT_OLD_PERMIC                    = 89, /* Perm */
      USCRIPT_PHAGS_PA                      = 90, /* Phag */
      USCRIPT_PHOENICIAN                    = 91, /* Phnx */
      USCRIPT_PHONETIC_POLLARD              = 92, /* Plrd */
      USCRIPT_RONGORONGO                    = 93, /* Roro */
      USCRIPT_SARATI                        = 94, /* Sara */
      USCRIPT_ESTRANGELO_SYRIAC             = 95, /* Syre */
      USCRIPT_WESTERN_SYRIAC                = 96, /* Syrj */
      USCRIPT_EASTERN_SYRIAC                = 97, /* Syrn */
      USCRIPT_TENGWAR                       = 98, /* Teng */
      USCRIPT_VAI                           = 99, /* Vaii */
      USCRIPT_VISIBLE_SPEECH                = 100, /* Visp */
      USCRIPT_CUNEIFORM                     = 101,/* Xsux */
      USCRIPT_UNWRITTEN_LANGUAGES           = 102,/* Zxxx */
      USCRIPT_UNKNOWN                       = 103,/* Zzzz */ /* Unknown="Code for uncoded script", for unassigned code points */
      /* Private use codes from Qaaa - Qabx are not supported*/

      USCRIPT_CODE_LIMIT    = 104
} UScriptCode;

namespace WTF {
namespace Unicode {

enum Direction {
    LeftToRight                = WKC_UNICODE_DIRECTION_LEFTTORIGHT,
    RightToLeft                = WKC_UNICODE_DIRECTION_RIGHTTOLEFT,
    EuropeanNumber             = WKC_UNICODE_DIRECTION_EUROPEANNUMBER,
    EuropeanNumberSeparator    = WKC_UNICODE_DIRECTION_EUROPEANNUMBERSEPARATOR,
    EuropeanNumberTerminator   = WKC_UNICODE_DIRECTION_EUROPEANNUMBERTERMINATOR,
    ArabicNumber               = WKC_UNICODE_DIRECTION_ARABICNUMBER,
    CommonNumberSeparator      = WKC_UNICODE_DIRECTION_COMMONNUMBERSEPARATOR,
    BlockSeparator             = WKC_UNICODE_DIRECTION_BLOCKSEPARATOR,
    SegmentSeparator           = WKC_UNICODE_DIRECTION_SEGMENTSEPARATOR,
    WhiteSpaceNeutral          = WKC_UNICODE_DIRECTION_WHITESPACENEUTRAL,
    OtherNeutral               = WKC_UNICODE_DIRECTION_OTHERNEUTRAL,
    LeftToRightEmbedding       = WKC_UNICODE_DIRECTION_LEFTTORIGHTEMBEDDING,
    LeftToRightOverride        = WKC_UNICODE_DIRECTION_LEFTTORIGHTOVERRIDE,
    RightToLeftArabic          = WKC_UNICODE_DIRECTION_RIGHTTOLEFTARABIC,
    RightToLeftEmbedding       = WKC_UNICODE_DIRECTION_RIGHTTOLEFTEMBEDDING,
    RightToLeftOverride        = WKC_UNICODE_DIRECTION_RIGHTTOLEFTOVERRIDE,
    PopDirectionalFormat       = WKC_UNICODE_DIRECTION_POPDIRECTIONALFORMAT,
    NonSpacingMark             = WKC_UNICODE_DIRECTION_NONSPACINGMARK,
    BoundaryNeutral            = WKC_UNICODE_DIRECTION_BOUNDARYNEUTRAL,
    LeftToRightIsolate         = WKC_UNICODE_DIRECTION_LEFTTORIGHTISOLATE,
    RightToLeftIsolate         = WKC_UNICODE_DIRECTION_RIGHTTOLEFTISOLATE,
    FirstStrongIsolate         = WKC_UNICODE_DIRECTION_FIRSTSTRONGISOLATE,
    PopDirectionalIsolate      = WKC_UNICODE_DIRECTION_POPDIRECTIONALISOLATE,
};

enum DecompositionType {
    DecompositionNone,
    DecompositionCanonical,
    DecompositionCompat,
    DecompositionCircle,
    DecompositionFinal,
    DecompositionFont,
    DecompositionFraction,
    DecompositionInitial,
    DecompositionIsolated,
    DecompositionMedial,
    DecompositionNarrow,
    DecompositionNoBreak,
    DecompositionSmall,
    DecompositionSquare,
    DecompositionSub,
    DecompositionSuper,
    DecompositionVertical,
    DecompositionWide,
};

enum CharCategory {
    NoCategory               = WKC_UNICODE_CATEGORY_NOCATEGORY,
    Other_NotAssigned        = WKC_UNICODE_CATEGORY_OTHERNOTASSIGNED,
    Letter_Uppercase         = WKC_UNICODE_CATEGORY_LETTERUPPERCASE,
    Letter_Lowercase         = WKC_UNICODE_CATEGORY_LETTERLOWERCASE,
    Letter_Titlecase         = WKC_UNICODE_CATEGORY_LETTERTITLECASE,
    Letter_Modifier          = WKC_UNICODE_CATEGORY_LETTERMODIFIER,
    Letter_Other             = WKC_UNICODE_CATEGORY_LETTEROTHER,

    Mark_NonSpacing          = WKC_UNICODE_CATEGORY_MARKNONSPACING,
    Mark_Enclosing           = WKC_UNICODE_CATEGORY_MARKENCLOSING,
    Mark_SpacingCombining    = WKC_UNICODE_CATEGORY_MARKSPACINGCOMBINING,

    Number_DecimalDigit      = WKC_UNICODE_CATEGORY_NUMBERDECIMALDIGIT,
    Number_Letter            = WKC_UNICODE_CATEGORY_NUMBERLETTER,
    Number_Other             = WKC_UNICODE_CATEGORY_NUMBEROTHER,

    Separator_Space          = WKC_UNICODE_CATEGORY_SEPARATORSPACE,
    Separator_Line           = WKC_UNICODE_CATEGORY_SEPARATORLINE,
    Separator_Paragraph      = WKC_UNICODE_CATEGORY_SEPARATORPARAGRAPH,

    Other_Control            = WKC_UNICODE_CATEGORY_OTHERCONTROL,
    Other_Format             = WKC_UNICODE_CATEGORY_OTHERFORMAT,
    Other_PrivateUse         = WKC_UNICODE_CATEGORY_OTHERPRIVATEUSE,
    Other_Surrogate          = WKC_UNICODE_CATEGORY_OTHERSURROGATE,

    Punctuation_Dash         = WKC_UNICODE_CATEGORY_PUNCTUATIONDASH,
    Punctuation_Open         = WKC_UNICODE_CATEGORY_PUNCTUATIONOPEN,
    Punctuation_Close        = WKC_UNICODE_CATEGORY_PUNCTUATIONCLOSE,
    Punctuation_Connector    = WKC_UNICODE_CATEGORY_PUNCTUATIONCONNECTOR,
    Punctuation_Other        = WKC_UNICODE_CATEGORY_PUNCTUATIONOTHER,

    Symbol_Math              = WKC_UNICODE_CATEGORY_SYMBOLMATH,
    Symbol_Currency          = WKC_UNICODE_CATEGORY_SYMBOLCURRENCY,
    Symbol_Modifier          = WKC_UNICODE_CATEGORY_SYMBOLMODIFIER,
    Symbol_Other             = WKC_UNICODE_CATEGORY_SYMBOLOTHER,

    Punctuation_InitialQuote = WKC_UNICODE_CATEGORY_PUNCTUATIONINITIALQUOTE,
    Punctuation_FinalQuote   = WKC_UNICODE_CATEGORY_PUNCTUATIONFINALQUOTE,
};

int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);
int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);

inline UChar32 foldCase(UChar32 ch)
{
    return wkcUnicodeFoldCasePeer(ch);
}

inline UChar32 toLower(UChar32 c)
{
    return wkcUnicodeToLowerPeer(c);
}

inline UChar32 toUpper(UChar32 c)
{
    return wkcUnicodeToUpperPeer(c);
}

inline UChar32 toTitleCase(UChar32 c)
{
    return wkcUnicodeToTitlePeer(c);
}

inline bool isArabicChar(UChar32 c)
{
    return c >= 0x0600 && c <= 0x06FF;
}

inline bool isAlphanumeric(UChar32 c)
{
    return (wkcUnicodeIsAlnumPeer(c)!=0);
}

inline bool isFormatChar(UChar32 c)
{
    return ((wkcUnicodeCategoryPeer(c) & WKC_UNICODE_CATEGORY_OTHERFORMAT) ? true : false);
}

inline bool isSeparatorSpace(UChar32 c)
{
    return ((wkcUnicodeCategoryPeer(c) & WKC_UNICODE_CATEGORY_SEPARATORSPACE) ? true : false);
}

inline bool isPrintableChar(UChar32 c)
{
    return (wkcUnicodeIsPrintPeer(c)!=0);
}

inline bool isDigit(UChar32 c)
{
    return (wkcUnicodeIsDigitPeer(c)!=0);
}

inline bool isPunct(UChar32 c)
{
    return (wkcUnicodeIsPunctPeer(c)!=0);
}

inline bool hasLineBreakingPropertyComplexContext(UChar32 c)
{
    // FIXME
    return false;
}

inline bool hasLineBreakingPropertyComplexContextOrIdeographic(UChar32 c)
{
    // FIXME
    return false;
}

inline UChar32 mirroredChar(UChar32 c)
{
    UChar32 mirror = 0;
    if (wkcUnicodeGetMirrorCharPeer(c, (int *)&mirror))
        return mirror;
    return c;
}

inline CharCategory category(UChar32 c)
{
    return (CharCategory)wkcUnicodeCategoryPeer(c);
}


inline Direction direction(UChar32 c)
{
    return (Direction)wkcUnicodeDirectionTypePeer(c);
}

inline bool isLower(UChar32 c)
{
    return (wkcUnicodeIsLowerPeer(c)!=0);
}

inline int digitValue(UChar32 c)
{
    return wkcUnicodeDigitValuePeer(c);
}

inline uint8_t combiningClass(UChar32 c)
{
    // FIXME
    // return g_unichar_combining_class(c);
    return 0;
}

inline DecompositionType decompositionType(UChar32 c)
{
    // FIXME
    return DecompositionNone;
}

inline bool requiresComplexContextForWordBreaking(UChar32 c)
{
    int cat = wkcUnicodeCategoryPeer(c);
    return cat==WKC_UNICODE_LINEBREAKCATEGORY_SA || cat==WKC_UNICODE_LINEBREAKCATEGORY_CJ || cat==WKC_UNICODE_LINEBREAKCATEGORY_ID;
}

inline int umemcasecmp(const UChar* a, const UChar* b, int len)
{
    return wkcUnicodeUCharMemCaseCmpPeer(a, b, len);
}

}
}

#endif

