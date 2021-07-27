/*
 * Copyright (c) 2015-2021 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include <uchar.h>
#include <ucol.h>
#include <unorm.h>
#include <ustring.h>
#include <uset.h>
#include <utypes.h>
#include <unicode/ubrk.h>
#include <unicode/uidna.h>
#include <unicode/usearch.h>
#include <unicode/utext.h>

#include <string.h>

#include <wkc/wkcpeer.h>
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

#include <algorithm>

#include "NotImplemented.h"

extern "C" {

UCharDirection
u_charDirection(UChar32 c)
{
    switch (wkcUnicodeDirectionTypePeer((int)c)) {
    case WKC_UNICODE_DIRECTION_LEFTTORIGHT:
        return U_LEFT_TO_RIGHT;
    case WKC_UNICODE_DIRECTION_RIGHTTOLEFT:
        return U_RIGHT_TO_LEFT;
    case WKC_UNICODE_DIRECTION_RIGHTTOLEFTARABIC:
        return U_RIGHT_TO_LEFT_ARABIC;
    case WKC_UNICODE_DIRECTION_LEFTTORIGHTEMBEDDING:
        return U_LEFT_TO_RIGHT_EMBEDDING;
    case WKC_UNICODE_DIRECTION_RIGHTTOLEFTEMBEDDING:
        return U_RIGHT_TO_LEFT_EMBEDDING;
    case WKC_UNICODE_DIRECTION_LEFTTORIGHTOVERRIDE:
        return U_LEFT_TO_RIGHT_OVERRIDE;
    case WKC_UNICODE_DIRECTION_RIGHTTOLEFTOVERRIDE:
        return U_RIGHT_TO_LEFT_OVERRIDE;
    case WKC_UNICODE_DIRECTION_POPDIRECTIONALFORMAT:
        return U_POP_DIRECTIONAL_FORMAT;
    case WKC_UNICODE_DIRECTION_EUROPEANNUMBER:
        return U_EUROPEAN_NUMBER;
    case WKC_UNICODE_DIRECTION_ARABICNUMBER:
        return U_ARABIC_NUMBER;
    case WKC_UNICODE_DIRECTION_EUROPEANNUMBERSEPARATOR:
        return U_EUROPEAN_NUMBER_SEPARATOR;
    case WKC_UNICODE_DIRECTION_EUROPEANNUMBERTERMINATOR:
        return U_EUROPEAN_NUMBER_TERMINATOR;
    case WKC_UNICODE_DIRECTION_COMMONNUMBERSEPARATOR:
        return U_COMMON_NUMBER_SEPARATOR;
    case WKC_UNICODE_DIRECTION_NONSPACINGMARK:
        return U_DIR_NON_SPACING_MARK;
    case WKC_UNICODE_DIRECTION_BOUNDARYNEUTRAL:
        return U_BOUNDARY_NEUTRAL;
    case WKC_UNICODE_DIRECTION_BLOCKSEPARATOR:
        return U_BLOCK_SEPARATOR;
    case WKC_UNICODE_DIRECTION_SEGMENTSEPARATOR:
        return U_SEGMENT_SEPARATOR;
    case WKC_UNICODE_DIRECTION_WHITESPACENEUTRAL:
        return U_WHITE_SPACE_NEUTRAL;
    case WKC_UNICODE_DIRECTION_OTHERNEUTRAL:
        return U_OTHER_NEUTRAL;
    case WKC_UNICODE_DIRECTION_LEFTTORIGHTISOLATE:
    case WKC_UNICODE_DIRECTION_RIGHTTOLEFTISOLATE:
    case WKC_UNICODE_DIRECTION_FIRSTSTRONGISOLATE:
    case WKC_UNICODE_DIRECTION_POPDIRECTIONALISOLATE:
    default:
        return U_LEFT_TO_RIGHT;
    }
}

UChar32
u_foldCase(UChar32 c, uint32_t options)
{
    return wkcUnicodeFoldCasePeer((int)c);
}

int8_t
u_charType(UChar32 c)
{
    int cat = wkcUnicodeCategoryPeer((int)c);
    int8_t ret = 0;

    if (cat&WKC_UNICODE_CATEGORY_LETTERUPPERCASE)
        return U_UPPERCASE_LETTER;
    if (cat&WKC_UNICODE_CATEGORY_LETTERLOWERCASE)
        return U_LOWERCASE_LETTER;
    if (cat&WKC_UNICODE_CATEGORY_LETTERTITLECASE)
        return U_TITLECASE_LETTER;
    if (cat&WKC_UNICODE_CATEGORY_LETTERMODIFIER)
        return U_MODIFIER_LETTER;
    if (cat&WKC_UNICODE_CATEGORY_LETTEROTHER)
        return U_OTHER_LETTER;
    if (cat&WKC_UNICODE_CATEGORY_MARKNONSPACING)
        return U_NON_SPACING_MARK;
    if (cat&WKC_UNICODE_CATEGORY_MARKENCLOSING)
        return U_ENCLOSING_MARK;
    if (cat&WKC_UNICODE_CATEGORY_MARKSPACINGCOMBINING)
        return U_COMBINING_SPACING_MARK;
    if (cat&WKC_UNICODE_CATEGORY_NUMBERDECIMALDIGIT)
        return U_DECIMAL_DIGIT_NUMBER;
    if (cat&WKC_UNICODE_CATEGORY_NUMBERLETTER)
        return U_LETTER_NUMBER;
    if (cat&WKC_UNICODE_CATEGORY_NUMBEROTHER)
        return U_OTHER_NUMBER;
    if (cat&WKC_UNICODE_CATEGORY_SEPARATORSPACE)
        return U_SPACE_SEPARATOR;
    if (cat&WKC_UNICODE_CATEGORY_SEPARATORLINE)
        return U_LINE_SEPARATOR;
    if (cat&WKC_UNICODE_CATEGORY_SEPARATORPARAGRAPH)
        return U_PARAGRAPH_SEPARATOR;
    if (cat&WKC_UNICODE_CATEGORY_OTHERCONTROL)
        return U_CONTROL_CHAR;
    if (cat&WKC_UNICODE_CATEGORY_OTHERFORMAT)
        return U_FORMAT_CHAR;
    if (cat&WKC_UNICODE_CATEGORY_OTHERPRIVATEUSE)
        return U_PRIVATE_USE_CHAR;
    if (cat&WKC_UNICODE_CATEGORY_OTHERSURROGATE)
        return U_SURROGATE;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONDASH)
        return U_DASH_PUNCTUATION;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONOPEN)
        return U_START_PUNCTUATION;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONCLOSE)
        return U_END_PUNCTUATION;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONCONNECTOR)
        return U_CONNECTOR_PUNCTUATION;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONOTHER)
        return U_OTHER_PUNCTUATION;
    if (cat&WKC_UNICODE_CATEGORY_SYMBOLMATH)
        return U_MATH_SYMBOL;
    if (cat&WKC_UNICODE_CATEGORY_SYMBOLCURRENCY)
        return U_CURRENCY_SYMBOL;
    if (cat&WKC_UNICODE_CATEGORY_SYMBOLMODIFIER)
        return U_MODIFIER_SYMBOL;
    if (cat&WKC_UNICODE_CATEGORY_SYMBOLOTHER)
        return U_OTHER_SYMBOL;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONINITIALQUOTE)
        return U_INITIAL_PUNCTUATION;
    if (cat&WKC_UNICODE_CATEGORY_PUNCTUATIONFINALQUOTE)
        return U_FINAL_PUNCTUATION;

    return U_GENERAL_OTHER_TYPES;
}

UChar32
u_toupper(UChar32 c)
{
    return wkcUnicodeToUpperPeer((int)c);
}

UChar32
u_tolower(UChar32 c)
{
    return wkcUnicodeToLowerPeer((int)c);
}

UChar32
u_totitle(UChar32 c)
{
    return wkcUnicodeToTitlePeer((int)c);
}

UChar32
u_charMirror(UChar32 c)
{
    int ret;
    if (wkcUnicodeGetMirrorCharPeer(c, &ret))
        return ret;
    else
        return c;
}

UBool
u_isprint(UChar32 c)
{
    return wkcUnicodeIsPrintPeer((int)c);
}

UBool
u_ispunct(UChar32 c)
{
    return wkcUnicodeIsPunctPeer((int)c);
}

UBool
u_isalnum(UChar32 c)
{
    return wkcUnicodeIsAlnumPeer((int)c);
}

int32_t
u_getIntPropertyValue(UChar32 c, UProperty which)
{
    if (which==UCHAR_DECOMPOSITION_TYPE) {
        // Ugh!: support it!
        // 150202 ACCESS Co.,Ltd.
        return U_DT_NONE;
    } else if (which==UCHAR_LINE_BREAK) {
        switch (wkcUnicodeLineBreakCategoryPeer((int)c)) {
        case WKC_UNICODE_LINEBREAKCATEGORY_SA:
            return U_LB_COMPLEX_CONTEXT;
        case WKC_UNICODE_LINEBREAKCATEGORY_CJ:
            return 37; //WK_U_LB_CONDITIONAL_JAPANESE_STARTER
        case WKC_UNICODE_LINEBREAKCATEGORY_ID:
            return U_LB_IDEOGRAPHIC;
        default:
            return U_LB_ALPHABETIC;
        }
    } else if (which==UCHAR_EAST_ASIAN_WIDTH) {
        // see UCD : EastAsianWidth
        // only supports part of list
        if ((c>=0xFF61 && c<=0xFFDC) || (c>=0xFFE9 && c<=0xFFEE)) {
            return U_EA_HALFWIDTH;
        } else if (c>=0xFFE0 && c<=0xFFE6) {
            return U_EA_FULLWIDTH;
        } else if ((c>=0x1100 && c<=0x115F) ||
            (c>=0x2329 && c<=0x232A) ||
            (c>=0x2E80 && c<=0x2FFB) ||
            (c>=0x3001 && c<=0xA4C6) ||
            (c>=0xF900 && c<=0xFAFF) ||
            (c>=0xFE10 && c<=0xFE19) ||
            (c>=0xFE30 && c<=0xFE6B)) {
            return U_EA_WIDE;
        }
        return U_EA_NEUTRAL;
    }
    return 0;
}

uint8_t
u_getCombiningClass(UChar32 c)
{
    switch (c) {
    case 0x3099:
    case 0x309A:
    case 0x309B:
    case 0x309C:
    case 0xFF9E:
    case 0xFF9F:
        return 8;
    }
    return 0;
}

UBlockCode
ublock_getCode(UChar32 c)
{
    if (c>=0x0000 && c<=0x007f)
        return UBLOCK_BASIC_LATIN;
    if (c>=0x0080 && c<=0x00ff)
        return UBLOCK_LATIN_1_SUPPLEMENT;
    if (c>=0x0100 && c<=0x017f)
        return UBLOCK_LATIN_EXTENDED_A;
    if (c>=0x0180 && c<=0x024f)
        return UBLOCK_LATIN_EXTENDED_B;
    if (c>=0x0250 && c<=0x02af)
        return UBLOCK_IPA_EXTENSIONS;
    if (c>=0x02b0 && c<=0x02ff)
        return UBLOCK_SPACING_MODIFIER_LETTERS;
    if (c>=0x0300 && c<=0x036f)
        return UBLOCK_COMBINING_DIACRITICAL_MARKS;
    if (c>=0x0370 && c<=0x03ff)
        return UBLOCK_GREEK;
    if (c>=0x0400 && c<=0x04ff)
        return UBLOCK_CYRILLIC;
    if (c>=0x0500 && c<=0x052f)
        return UBLOCK_CYRILLIC_SUPPLEMENTARY;
    if (c>=0x0530 && c<=0x058f)
        return UBLOCK_ARMENIAN;
    if (c>=0x0590 && c<=0x05ff)
        return UBLOCK_HEBREW;
    if (c>=0x0600 && c<=0x06ff)
        return UBLOCK_ARABIC;
    if (c>=0x0700 && c<=0x074f)
        return UBLOCK_SYRIAC;
    if (c>=0x0750 && c<=0x077f)
        return UBLOCK_ARABIC_SUPPLEMENT;
    if (c>=0x0780 && c<=0x07bf)
        return UBLOCK_THAANA;
    if (c>=0x07c0 && c<=0x07ff)
        return UBLOCK_NKO;
    if (c>=0x0800 && c<=0x083f)
        return UBLOCK_SAMARITAN;
    if (c>=0x0840 && c<=0x085f)
        return UBLOCK_MANDAIC;
    if (c>=0x0900 && c<=0x097f)
        return UBLOCK_DEVANAGARI;
    if (c>=0x0980 && c<=0x09ff)
        return UBLOCK_BENGALI;
    if (c>=0x0a00 && c<=0x0a7f)
        return UBLOCK_GURMUKHI;
    if (c>=0x0a80 && c<=0x0aff)
        return UBLOCK_GUJARATI;
    if (c>=0x0b00 && c<=0x0b7f)
        return UBLOCK_ORIYA;
    if (c>=0x0b80 && c<=0x0bff)
        return UBLOCK_TAMIL;
    if (c>=0x0c00 && c<=0x0c7f)
        return UBLOCK_TELUGU;
    if (c>=0x0c80 && c<=0x0cff)
        return UBLOCK_KANNADA;
    if (c>=0x0d00 && c<=0x0d7f)
        return UBLOCK_MALAYALAM;
    if (c>=0x0d80 && c<=0x0dff)
        return UBLOCK_SINHALA;
    if (c>=0x0e00 && c<=0x0e7f)
        return UBLOCK_THAI;
    if (c>=0x0e80 && c<=0x0eff)
        return UBLOCK_LAO;
    if (c>=0x0f00 && c<=0x0fff)
        return UBLOCK_TIBETAN;
    if (c>=0x1000 && c<=0x109f)
        return UBLOCK_MYANMAR;
    if (c>=0x10a0 && c<=0x10ff)
        return UBLOCK_GEORGIAN;
    if (c>=0x1100 && c<=0x11ff)
        return UBLOCK_HANGUL_JAMO;
    if (c>=0x1200 && c<=0x137f)
        return UBLOCK_ETHIOPIC;
    if (c>=0x1380 && c<=0x139f)
        return UBLOCK_ETHIOPIC_SUPPLEMENT;
    if (c>=0x13a0 && c<=0x13ff)
        return UBLOCK_CHEROKEE;
    if (c>=0x1400 && c<=0x167f)
        return UBLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS;
    if (c>=0x1680 && c<=0x169f)
        return UBLOCK_OGHAM;
    if (c>=0x16a0 && c<=0x16ff)
        return UBLOCK_RUNIC;
    if (c>=0x1700 && c<=0x171f)
        return UBLOCK_TAGALOG;
    if (c>=0x1720 && c<=0x173f)
        return UBLOCK_HANUNOO;
    if (c>=0x1740 && c<=0x175f)
        return UBLOCK_BUHID;
    if (c>=0x1760 && c<=0x177f)
        return UBLOCK_TAGBANWA;
    if (c>=0x1780 && c<=0x17ff)
        return UBLOCK_KHMER;
    if (c>=0x1800 && c<=0x18af)
        return UBLOCK_MONGOLIAN;
    if (c>=0x18b0 && c<=0x18ff)
        return UBLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS_EXTENDED;
    if (c>=0x1900 && c<=0x194f)
        return UBLOCK_LIMBU;
    if (c>=0x1950 && c<=0x197f)
        return UBLOCK_TAI_LE;
    if (c>=0x1980 && c<=0x19df)
        return UBLOCK_NEW_TAI_LUE;
    if (c>=0x19e0 && c<=0x19ff)
        return UBLOCK_KHMER_SYMBOLS;
    if (c>=0x1a00 && c<=0x1a1f)
        return UBLOCK_BUGINESE;
    if (c>=0x1a20 && c<=0x1aaf)
        return UBLOCK_TAI_THAM;
    if (c>=0x1b00 && c<=0x1b7f)
        return UBLOCK_BALINESE;
    if (c>=0x1b80 && c<=0x1bbf)
        return UBLOCK_SUNDANESE;
    if (c>=0x1bc0 && c<=0x1bff)
        return UBLOCK_BATAK;
    if (c>=0x1c00 && c<=0x1c4f)
        return UBLOCK_LEPCHA;
    if (c>=0x1c50 && c<=0x1c7f)
        return UBLOCK_OL_CHIKI;
    if (c>=0x1cd0 && c<=0x1cff)
        return UBLOCK_VEDIC_EXTENSIONS;
    if (c>=0x1d00 && c<=0x1d7f)
        return UBLOCK_PHONETIC_EXTENSIONS;
    if (c>=0x1d80 && c<=0x1dbf)
        return UBLOCK_PHONETIC_EXTENSIONS_SUPPLEMENT;
    if (c>=0x1dc0 && c<=0x1dff)
        return UBLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT;
    if (c>=0x1e00 && c<=0x1eff)
        return UBLOCK_LATIN_EXTENDED_ADDITIONAL;
    if (c>=0x1f00 && c<=0x1fff)
        return UBLOCK_GREEK_EXTENDED;
    if (c>=0x2000 && c<=0x206f)
        return UBLOCK_GENERAL_PUNCTUATION;
    if (c>=0x2070 && c<=0x209f)
        return UBLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS;
    if (c>=0x20a0 && c<=0x20cf)
        return UBLOCK_CURRENCY_SYMBOLS;
    if (c>=0x20d0 && c<=0x20ff)
        return UBLOCK_COMBINING_MARKS_FOR_SYMBOLS;
    if (c>=0x2100 && c<=0x214f)
        return UBLOCK_LETTERLIKE_SYMBOLS;
    if (c>=0x2150 && c<=0x218f)
        return UBLOCK_NUMBER_FORMS;
    if (c>=0x2190 && c<=0x21ff)
        return UBLOCK_ARROWS;
    if (c>=0x2200 && c<=0x22ff)
        return UBLOCK_MATHEMATICAL_OPERATORS;
    if (c>=0x2300 && c<=0x23ff)
        return UBLOCK_MISCELLANEOUS_TECHNICAL;
    if (c>=0x2400 && c<=0x243f)
        return UBLOCK_CONTROL_PICTURES;
    if (c>=0x2440 && c<=0x245f)
        return UBLOCK_OPTICAL_CHARACTER_RECOGNITION;
    if (c>=0x2460 && c<=0x24ff)
        return UBLOCK_ENCLOSED_ALPHANUMERICS;
    if (c>=0x2500 && c<=0x257f)
        return UBLOCK_BOX_DRAWING;
    if (c>=0x2580 && c<=0x259f)
        return UBLOCK_BLOCK_ELEMENTS;
    if (c>=0x25a0 && c<=0x25ff)
        return UBLOCK_GEOMETRIC_SHAPES;
    if (c>=0x2600 && c<=0x26ff)
        return UBLOCK_MISCELLANEOUS_SYMBOLS;
    if (c>=0x2700 && c<=0x27bf)
        return UBLOCK_DINGBATS;
    if (c>=0x27c0 && c<=0x27ef)
        return UBLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A;
    if (c>=0x27f0 && c<=0x27ff)
        return UBLOCK_SUPPLEMENTAL_ARROWS_A;
    if (c>=0x2800 && c<=0x28ff)
        return UBLOCK_BRAILLE_PATTERNS;
    if (c>=0x2900 && c<=0x297f)
        return UBLOCK_SUPPLEMENTAL_ARROWS_B;
    if (c>=0x2980 && c<=0x29ff)
        return UBLOCK_MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B;
    if (c>=0x2a00 && c<=0x2aff)
        return UBLOCK_SUPPLEMENTAL_MATHEMATICAL_OPERATORS;
    if (c>=0x2b00 && c<=0x2bff)
        return UBLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS;
    if (c>=0x2c00 && c<=0x2c5f)
        return UBLOCK_GLAGOLITIC;
    if (c>=0x2c60 && c<=0x2c7f)
        return UBLOCK_LATIN_EXTENDED_C;
    if (c>=0x2c80 && c<=0x2cff)
        return UBLOCK_COPTIC;
    if (c>=0x2d00 && c<=0x2d2f)
        return UBLOCK_GEORGIAN_SUPPLEMENT;
    if (c>=0x2d30 && c<=0x2d7f)
        return UBLOCK_TIFINAGH;
    if (c>=0x2d80 && c<=0x2ddf)
        return UBLOCK_ETHIOPIC_EXTENDED;
    if (c>=0x2de0 && c<=0x2dff)
        return UBLOCK_CYRILLIC_EXTENDED_A;
    if (c>=0x2e00 && c<=0x2e7f)
        return UBLOCK_SUPPLEMENTAL_PUNCTUATION;
    if (c>=0x2e80 && c<=0x2eff)
        return UBLOCK_CJK_RADICALS_SUPPLEMENT;
    if (c>=0x2f00 && c<=0x2fdf)
        return UBLOCK_KANGXI_RADICALS;
    if (c>=0x2fe0 && c<=0x2fff)
        return UBLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS;
    if (c>=0x3000 && c<=0x303f)
        return UBLOCK_CJK_SYMBOLS_AND_PUNCTUATION;
    if (c>=0x3040 && c<=0x309f)
        return UBLOCK_HIRAGANA;
    if (c>=0x30a0 && c<=0x30ff)
        return UBLOCK_KATAKANA;
    if (c>=0x3100 && c<=0x312f)
        return UBLOCK_BOPOMOFO;
    if (c>=0x3130 && c<=0x318f)
        return UBLOCK_HANGUL_COMPATIBILITY_JAMO;
    if (c>=0x3190 && c<=0x319f)
        return UBLOCK_KANBUN;
    if (c>=0x31a0 && c<=0x31bf)
        return UBLOCK_BOPOMOFO_EXTENDED;
    if (c>=0x31c0 && c<=0x31ef)
        return UBLOCK_CJK_STROKES;
    if (c>=0x31f0 && c<=0x31ff)
        return UBLOCK_KATAKANA_PHONETIC_EXTENSIONS;
    if (c>=0x3200 && c<=0x32ff)
        return UBLOCK_ENCLOSED_CJK_LETTERS_AND_MONTHS;
    if (c>=0x3300 && c<=0x33ff)
        return UBLOCK_CJK_COMPATIBILITY;
    if (c>=0x3400 && c<=0x4dbf)
        return UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A;
    if (c>=0x4dc0 && c<=0x4dff)
        return UBLOCK_YIJING_HEXAGRAM_SYMBOLS;
    if (c>=0x4e00 && c<=0x9fff)
        return UBLOCK_CJK_UNIFIED_IDEOGRAPHS;
    if (c>=0xa000 && c<=0xa48f)
        return UBLOCK_YI_SYLLABLES;
    if (c>=0xa490 && c<=0xa4cf)
        return UBLOCK_YI_RADICALS;
    if (c>=0xa4d0 && c<=0xa4ff)
        return UBLOCK_LISU;
    if (c>=0xa500 && c<=0xa63f)
        return UBLOCK_VAI;
    if (c>=0xa640 && c<=0xa69f)
        return UBLOCK_CYRILLIC_EXTENDED_B;
    if (c>=0xa6a0 && c<=0xa6ff)
        return UBLOCK_BAMUM;
    if (c>=0xa700 && c<=0xa71f)
        return UBLOCK_MODIFIER_TONE_LETTERS;
    if (c>=0xa720 && c<=0xa7ff)
        return UBLOCK_LATIN_EXTENDED_D;
    if (c>=0xa800 && c<=0xa82f)
        return UBLOCK_SYLOTI_NAGRI;
    if (c>=0xa830 && c<=0xa83f)
        return UBLOCK_COMMON_INDIC_NUMBER_FORMS;
    if (c>=0xa840 && c<=0xa87f)
        return UBLOCK_PHAGS_PA;
    if (c>=0xa880 && c<=0xa8df)
        return UBLOCK_SAURASHTRA;
    if (c>=0xa8e0 && c<=0xa8ff)
        return UBLOCK_DEVANAGARI_EXTENDED;
    if (c>=0xa900 && c<=0xa92f)
        return UBLOCK_KAYAH_LI;
    if (c>=0xa930 && c<=0xa95f)
        return UBLOCK_REJANG;
    if (c>=0xa960 && c<=0xa97f)
        return UBLOCK_HANGUL_JAMO_EXTENDED_A;
    if (c>=0xa980 && c<=0xa9df)
        return UBLOCK_JAVANESE;
    if (c>=0xaa00 && c<=0xaa5f)
        return UBLOCK_CHAM;
    if (c>=0xaa60 && c<=0xaa7f)
        return UBLOCK_MYANMAR_EXTENDED_A;
    if (c>=0xaa80 && c<=0xaadf)
        return UBLOCK_TAI_VIET;
    if (c>=0xab00 && c<=0xab2f)
        return UBLOCK_ETHIOPIC_EXTENDED_A;
    if (c>=0xabc0 && c<=0xabff)
        return UBLOCK_MEETEI_MAYEK;
    if (c>=0xac00 && c<=0xd7af)
        return UBLOCK_HANGUL_SYLLABLES;
    if (c>=0xd7b0 && c<=0xd7ff)
        return UBLOCK_HANGUL_JAMO_EXTENDED_B;
    if (c>=0xd800 && c<=0xdb7f)
        return UBLOCK_HIGH_SURROGATES;
    if (c>=0xdb80 && c<=0xdbff)
        return UBLOCK_HIGH_PRIVATE_USE_SURROGATES;
    if (c>=0xdc00 && c<=0xdfff)
        return UBLOCK_LOW_SURROGATES;
    if (c>=0xe000 && c<=0xf8ff)
        return UBLOCK_PRIVATE_USE_AREA;
    if (c>=0xf900 && c<=0xfaff)
        return UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS;
    if (c>=0xfb00 && c<=0xfb4f)
        return UBLOCK_ALPHABETIC_PRESENTATION_FORMS;
    if (c>=0xfb50 && c<=0xfdff)
        return UBLOCK_ARABIC_PRESENTATION_FORMS_A;
    if (c>=0xfe00 && c<=0xfe0f)
        return UBLOCK_VARIATION_SELECTORS;
    if (c>=0xfe10 && c<=0xfe1f)
        return UBLOCK_VERTICAL_FORMS;
    if (c>=0xfe20 && c<=0xfe2f)
        return UBLOCK_COMBINING_HALF_MARKS;
    if (c>=0xfe30 && c<=0xfe4f)
        return UBLOCK_CJK_COMPATIBILITY_FORMS;
    if (c>=0xfe50 && c<=0xfe6f)
        return UBLOCK_SMALL_FORM_VARIANTS;
    if (c>=0xfe70 && c<=0xfeff)
        return UBLOCK_ARABIC_PRESENTATION_FORMS_B;
    if (c>=0xff00 && c<=0xffef)
        return UBLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS;
    if (c>=0xfff0 && c<=0xffff)
        return UBLOCK_SPECIALS;

    if (c>=0x1F300 && c<=0x1F5FF)
        return UBLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS;
    if (c>=0x1F600 && c<=0x1F64F)
        return UBLOCK_EMOTICONS;
    if (c>=0x1F680 && c<=0x1F6FF)
        return UBLOCK_TRANSPORT_AND_MAP_SYMBOLS;

    if (c>=0x20000 && c<=0x2a6df)
        return UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B;
    if (c>=0x2a700 && c<=0x2b73f)
        return UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_C;
    if (c>=0x2b740 && c<=0x2b81f)
        return UBLOCK_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_D;
    if (c>=0x2f800 && c<=0x2fa1f)
        return UBLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT;

    if (c>=0xe0000 && c<=0xe007f)
        return UBLOCK_TAGS;

    return UBLOCK_NO_BLOCK;
}

// ustr

int32_t
u_strToLower(UChar *dest, int32_t destCapacity, 
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode)
{
    int len = srcLength <= destCapacity ? srcLength : destCapacity-1;
    for (int i=0; i<len; i++) {
        dest[i] = wkcUnicodeToLowerPeer((int)src[i]);
    }
    return len;
}

int32_t
u_strToUpper(UChar *dest, int32_t destCapacity, 
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode)
{
    int len = srcLength <= destCapacity ? srcLength : destCapacity-1;
    for (int i=0; i<len; i++) {
        dest[i] = wkcUnicodeToUpperPeer((int)src[i]);
    }
    return len;
}

int32_t
u_strFoldCase(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             uint32_t options,
             UErrorCode *pErrorCode)
{
    int len = srcLength <= destCapacity ? srcLength : destCapacity-1;
    for (int i=0; i<len; i++) {
        dest[i] = wkcUnicodeFoldCasePeer((int)src[i]);
    }
    return len;
}

int32_t
u_memcasecmp(const UChar *s1, const UChar *s2, int32_t length, uint32_t /*options*/)
{
    return wkcUnicodeUCharMemCaseCmpPeer(reinterpret_cast<const unsigned short*>(s1), reinterpret_cast<const unsigned short*>(s2), length);
}

// uset

USet*
uset_openPattern(const UChar* pattern, int32_t patternLength, UErrorCode* ec)
{
    notImplemented();
    return 0;
}

void
uset_close(USet* set)
{
    notImplemented();
}

void
uset_add(USet* set, UChar32 c)
{
    notImplemented();
}

void
uset_addRange(USet* set, UChar32 start, UChar32 end)
{
    notImplemented();
}

void
uset_addAll(USet* set, const USet *additionalSet)
{
    notImplemented();
}

UBool
uset_contains(const USet* set, UChar32 c)
{
    notImplemented();
    return FALSE;
}

// usearch

UStringSearch*
usearch_open(const UChar* pattern, int32_t patternlength, const UChar* text, int32_t textlength, const char* locale, UBreakIterator* breakiter, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
    return 0;
}

void
usearch_reset(UStringSearch* strsrch)
{
    notImplemented();
}

UCollator*
usearch_getCollator(const UStringSearch *strsrch)
{
    notImplemented();
    return 0;
}

void
usearch_setAttribute(UStringSearch* strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
}

void
usearch_setPattern(UStringSearch* strsrch, const UChar* pattern, int32_t patternlength, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
}

void
usearch_setText(UStringSearch* strsrch, const UChar* text, int32_t textlength, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
}

void
usearch_setOffset(UStringSearch* strsrch, int32_t position, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
}

int32_t
usearch_next(UStringSearch* strsrch, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
    return USEARCH_DONE;
}

int32_t
usearch_getMatchedLength(const UStringSearch *strsrch)
{
    return 0;
}

// ucol
UCollationStrength
ucol_getStrength(const UCollator* coll)
{
    notImplemented();
    return UCOL_DEFAULT;
}

void
ucol_setStrength(UCollator* coll, UCollationStrength strength)
{
    notImplemented();
}

// unorm

UNormalizationCheckResult
unorm_quickCheck(const UChar* source, int32_t sourcelength, UNormalizationMode mode, UErrorCode *status)
{
    // Ugh!: hard to implement...
    // 150501 ACCESS Co.,Ltd.
    return UNORM_YES;
}

int32_t
unorm_normalize(const UChar* source, int32_t sourceLength, UNormalizationMode mode, int32_t options, UChar* result, int32_t resultLength, UErrorCode *status)
{
    notImplemented();

    *status = U_ZERO_ERROR;
    if (result) {
        memcpy(result, source, sourceLength * sizeof(UChar));
    }
    return sourceLength;
}

// uidna

int32_t
uidna_IDNToASCII(const UChar* src, int32_t srcLength, UChar* dest, int32_t destCapacity, int32_t options, UParseError* parseError, UErrorCode* status)
{
    *status = U_ZERO_ERROR;
    int len = wkcI18NIDNfromUnicodePeer(reinterpret_cast<const unsigned short*>(src), srcLength, 0, 0);
    if (len<=0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    unsigned char* d = (unsigned char *)WTF::fastMalloc(len+1);
    ::memset(d, 0, len+1);
    len = wkcI18NIDNfromUnicodePeer(reinterpret_cast<const unsigned short*>(src), srcLength, d, len+1);
    if (len<0) {
        WTF::fastFree((void *)d);
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    len = std::min(len, destCapacity);
    if (dest) {
        for (int i=0; i<len; i++) {
            dest[i] = (UChar)d[i];
        }
    }
    WTF::fastFree((void *)d);
    return len;
}

// utext
UText*
utext_setup(UText *ut, int32_t extraSpace, UErrorCode *status)
{
    notImplemented();

    *status = U_ILLEGAL_ARGUMENT_ERROR;

    return 0;
}

UText*
utext_close(UText *ut)
{
    notImplemented();
    return 0;
}

// ubrk
UBreakIterator*
ubrk_open(UBreakIteratorType type, const char *locale, const UChar *text, int32_t textLength, UErrorCode *status)
{
    void* bi = wkcTextBreakIteratorNewPeer(WKC::toWKCTextBreakIteratorType(type));
    if (bi) {
        wkcTextBreakIteratorSetStringPeer(bi, text, textLength);
        *status = U_ZERO_ERROR;
    } else {
        *status = U_INTERNAL_PROGRAM_ERROR;
    }
    return reinterpret_cast<UBreakIterator*>(bi);
}

UBreakIterator*
ubrk_openRules(const UChar* rules, int32_t rulesLength, const UChar* text, int32_t textLength, UParseError* parseErr, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
    return (UBreakIterator *)1;
}

void
ubrk_setUText(UBreakIterator* bi, UText* text, UErrorCode* status)
{
    notImplemented();
    *status = U_ZERO_ERROR;
}

void
ubrk_setText(UBreakIterator* bi, const UChar* text, int32_t textLength, UErrorCode* status)
{
    if (wkcTextBreakIteratorSetStringPeer(bi, text, textLength)) {
        *status = U_ZERO_ERROR;
    } else {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

// utypes
const char *
u_errorName(UErrorCode code)
{
    return "Error";
}

U_STABLE UBool U_EXPORT2
u_hasBinaryProperty(UChar32 c, UProperty which)
{
    notImplemented();
    return false;
}

U_STABLE void U_EXPORT2
ubrk_close(UBreakIterator *bi)
{
    wkcTextBreakIteratorDeletePeer(bi);
}

U_STABLE int32_t U_EXPORT2
ubrk_current(const UBreakIterator *bi)
{
    return wkcTextBreakIteratorCurrentPeer(const_cast<UBreakIterator*>(bi));
}

U_STABLE int32_t U_EXPORT2
ubrk_first(UBreakIterator *bi)
{
    return wkcTextBreakIteratorFirstPeer(bi);
}

U_STABLE int32_t U_EXPORT2
ubrk_following(UBreakIterator *bi,
    int32_t offset)
{
    return wkcTextBreakIteratorFollowingPeer(bi, offset);
}

U_STABLE  UBool U_EXPORT2
ubrk_isBoundary(UBreakIterator *bi, int32_t offset)
{
    notImplemented();
    return false;
}

U_STABLE int32_t U_EXPORT2
ubrk_last(UBreakIterator *bi)
{
    return wkcTextBreakIteratorLastPeer(bi);
}

U_STABLE int32_t U_EXPORT2
ubrk_next(UBreakIterator *bi)
{
    return wkcTextBreakIteratorNextPeer(bi);
}

U_STABLE int32_t U_EXPORT2
ubrk_preceding(UBreakIterator *bi,
    int32_t offset)
{
    return wkcTextBreakIteratorPrecedingPeer(bi, offset);
}

U_STABLE int32_t U_EXPORT2
ubrk_previous(UBreakIterator *bi)
{
    return wkcTextBreakIteratorPreviousPeer(bi);
}

U_STABLE int32_t U_EXPORT2
uidna_nameToASCII(const UIDNA *idna,
    const UChar *name, int32_t length,
    UChar *dest, int32_t capacity,
    UIDNAInfo *pInfo, UErrorCode *pErrorCode)
{
    notImplemented();
    return 0;
}

U_STABLE UIDNA * U_EXPORT2
uidna_openUTS46(uint32_t options, UErrorCode *pErrorCode)
{
    notImplemented();
    return nullptr;
}

U_STABLE const UNormalizer2 * U_EXPORT2
unorm2_getNFCInstance(UErrorCode *pErrorCode)
{
    notImplemented();
    return nullptr;
}

U_STABLE const UNormalizer2 * U_EXPORT2
unorm2_getNFDInstance(UErrorCode *pErrorCode)
{
    notImplemented();
    return nullptr;
}

U_STABLE const UNormalizer2 * U_EXPORT2
unorm2_getNFKCInstance(UErrorCode *pErrorCode)
{
    notImplemented();
    return nullptr;
}

U_STABLE const UNormalizer2 * U_EXPORT2
unorm2_getNFKDInstance(UErrorCode *pErrorCode)
{
    notImplemented();
    return nullptr;
}

U_STABLE int32_t U_EXPORT2
unorm2_normalize(const UNormalizer2 *norm2,
    const UChar *src, int32_t length,
    UChar *dest, int32_t capacity,
    UErrorCode *pErrorCode)
{
    notImplemented();
    return 0;
}

U_STABLE int32_t U_EXPORT2
utf8_appendCharSafeBody(uint8_t *s, int32_t i, int32_t length, UChar32 c, UBool *pIsError)
{
    notImplemented();
    return 0;
}

} // extern "C"
