/*
 * Copyright (C) 2004, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 * Copyright (C) 2008 Jurg Billeter <j@bitron.ch>
 * Copyright (C) 2009 Dominik Rottsches <dominik.roettsches@access-company.com>
 * Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "TextCodecWKC.h"

#include "CString.h"
#include "WTFString.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include "Logging.h"

#include <wkc/wkcpeer.h>

using std::min;

namespace WebCore {

typedef const char* const codecAliasList[];

// We're specifying the list of text codecs and their aliases here. 
// For each codec the first entry is the canonical name, remaining ones are used as aliases.
// Each alias list must be terminated by a 0.

// Unicode
static codecAliasList m_codecAliases_UTF_8            = { "UTF-8", 0 }; 

// Western
static codecAliasList m_codecAliases_ISO_8859_1       = { "ISO-8859-1", "CP819", "IBM819", "ISO-IR-100", "ISO8859-1", "ISO_8859-1", "ISO_8859-1:1987",  "L1", "LATIN1", "CSISOLATIN1", 0 };
static codecAliasList m_codecAliases_CP1252           = { "CP1252", "ISO-8859-1-Windows-3.0-Latin-1",  "csWindows30Latin1", "ISO-8859-1-Windows-3.1-Latin-1", "csWindows31Latin1", 0 };
static codecAliasList m_codecAliases_MACROMAN         = { "MACROMAN", "MAC", "MACINTOSH", "CSMACINTOSH", 0 };

// Japanese
static codecAliasList m_codecAliases_SHIFT_JIS        = { "Shift_JIS", "MS_KANJI", "SHIFT-JIS", "SJIS", "CSSHIFTJIS", "X-SJIS", "Windows-31J", "CP932", "MS932", 0 };
static codecAliasList m_codecAliases_EUC_JP           = { "EUC-JP", "EUC_JP", "EUCJP", "EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE", "CSEUCPKDFMTJAPANESE", "X-EUC-JP", 0 };
static codecAliasList m_codecAliases_ISO_2022_JP      = { "ISO-2022-JP", "csISO2022JP", "ISO-2022-JP-2", "csISO2022JP2", 0 };
static codecAliasList m_codecAliases_SHIFT_JIS_2004        = { "Shift_JIS-2004", "SHIFT-JIS-2004", "X-Shift_JIS-2004", 0 };
static codecAliasList m_codecAliases_EUC_JIS_2004           = { "EUC-JIS-2004", "X-EUC-JIS-2004", 0 };
static codecAliasList m_codecAliases_ISO_2022_JP_2004      = { "ISO-2022-JP-2004", "X-ISO-2022-JP-2004", 0 };

// Traditional Chinese
static codecAliasList m_codecAliases_BIG5             = { "BIG5", "BIG-5", "BIG-FIVE", "BIGFIVE", "CN-BIG5", "CSBIG5", 0 };
static codecAliasList m_codecAliases_BIG5_HKSCS       = { "BIG5-HKSCS", "BIG5-HKSCS:2004", "BIG5HKSCS", 0 };
static codecAliasList m_codecAliases_CP950            = { "CP950", 0 };

// Korean
static codecAliasList m_codecAliases_ISO_2022_KR      = { "ISO-2022-KR", "CSISO2022KR", 0 };
static codecAliasList m_codecAliases_CP949            = { "CP949", "UHC", "windows-949", "KS_C_5601-1987", "KSC5601", 0 };
static codecAliasList m_codecAliases_EUC_KR           = { "EUC-KR", "CSEUCKR", 0 };

// Arabic
static codecAliasList m_codecAliases_ISO_8859_6       = { "ISO-8859-6", "ISO8859-6", "ISO_8859-6:1987", "iso-ir-127", "ISO_8859-6", "ECMA-114", "ASMO-708", "arabic", "csISOLatinArabic", "ISO-8859-6-E", "ISO_8859-6-E", "csISO88596E", "ISO-8859-6-I", "ISO_8859-6-I", "csISO88596I", 0 };
static codecAliasList m_codecAliases_CP1256           = { "windows-1256", "CP1256", "MS-ARAB", 0 }; // rearranged, windows-1256 now declared the canonical name and put to lowercase to fix /fast/encoding/ahram-org-eg.html test case

// Hebrew
static codecAliasList m_codecAliases_ISO_8859_8       = { "ISO-8859-8", "ISO8859-8", "ISO_8859-8:1988", "iso-ir-138", "ISO_8859-8", "hebrew", "csISOLatinHebrew", "ISO-8859-8-E", "ISO_8859-8-E", "csISO88598E", "ISO-8859-8-I", "ISO_8859-8-I", "csISO88598I", 0 };
static codecAliasList m_codecAliases_CP1255           = { "windows-1255", "CP1255", "MS-HEBR", 0 }; // rearranged, moved windows-1255 as canonical and lowercased, fixing /fast/encoding/meta-charset.html

// Greek
static codecAliasList m_codecAliases_ISO_8859_7       = { "ISO-8859-7", "ISO8859-7", "ISO_8859-7:1987", "ISO_8859-7:2003", "iso-ir-126", "ISO_8859-7", "ELOT_928", "ECMA-118", "greek", "greek8", "csISOLatinGreek", 0 };
static codecAliasList m_codecAliases_CP869            = { "CP869", "869", "CP-GR", "IBM869", "CSIBM869", 0 };
static codecAliasList m_codecAliases_CP1253           = { "WINDOWS-1253", "CP1253", 0 };

// Cyrillic
static codecAliasList m_codecAliases_ISO_8859_5       = { "ISO-8859-5", "CYRILLIC", "ISO-IR-144", "ISO8859-5", "ISO_8859-5", "ISO_8859-5:1988", "CSISOLATINCYRILLIC", 0 };
static codecAliasList m_codecAliases_KOI8_R           = { "KOI8-R", "CSKOI8R", 0 };
static codecAliasList m_codecAliases_CP866            = { "CP866", "866", "IBM866", "CSIBM866", 0 };
static codecAliasList m_codecAliases_KOI8_U           = { "KOI8-U", 0 };
static codecAliasList m_codecAliases_CP1251           = { "windows-1251", "CP1251", 0 }; // CP1251 added to pass /fast/encoding/charset-cp1251.html
static codecAliasList m_codecAliases_MACCYRILLIC      = { "mac-cyrillic", "MACCYRILLIC", "x-mac-cyrillic", 0 }; 

// Thai
static codecAliasList m_codecAliases_CP874            = { "CP874", "WINDOWS-874", 0 };
static codecAliasList m_codecAliases_TIS_620          = { "TIS-620", "ISO-8859-11", "ISO-IR-166", 0 };

// Simplified Chinese
static codecAliasList m_codecAliases_GBK              = { "GBK", "CP936", "MS936", "windows-936", 0 };
static codecAliasList m_codecAliases_HZ               = { "HZ", "HZ-GB-2312", 0 };
static codecAliasList m_codecAliases_GB18030          = { "GB18030", 0 };
static codecAliasList m_codecAliases_EUC_CN           = { "EUC-CN", "EUCCN", "GB2312", "CN-GB", "CSGB2312", "EUC_CN", 0 };
static codecAliasList m_codecAliases_2312_80          = { "GB_2312-80", "CHINESE", "csISO58GB231280", "GB2312.1980-0", "ISO-IR-58", 0 };

// Central European
static codecAliasList m_codecAliases_ISO_8859_2       = { "ISO-8859-2", "ISO-IR-101", "ISO8859-2", "ISO_8859-2", "ISO_8859-2:1987", "L2", "LATIN2", "CSISOLATIN2", 0 };
static codecAliasList m_codecAliases_CP1250           = { "CP1250", "MS-EE", "WINDOWS-1250", "ISO-8859-2-Windows-Latin-2", "csWindows31Latin2", 0 };
static codecAliasList m_codecAliases_MACCENTRALEUROPE = { "MAC-CENTRALEUROPE", "x-mac-centraleurroman", 0 };

// Vietnamese
static codecAliasList m_codecAliases_CP1258           = { "CP1258", "WINDOWS-1258", 0 };

// Turkish
static codecAliasList m_codecAliases_CP1254           = { "CP1254", "MS-TURK", "WINDOWS-1254", "ISO-8859-9-Windows-Latin-5", "csWindows31Latin5", 0 };
static codecAliasList m_codecAliases_ISO_8859_9       = { "ISO-8859-9", "ISO-IR-148", "ISO8859-9", "ISO_8859-9", "ISO_8859-9:1989", "L5", "LATIN5", "CSISOLATIN5", 0 };

// Baltic
static codecAliasList m_codecAliases_CP1257           = { "CP1257", "WINBALTRIM", "WINDOWS-1257", 0 };
static codecAliasList m_codecAliases_ISO_8859_4       = { "ISO-8859-4", "ISO-IR-110", "ISO8859-4", "ISO_8859-4", "ISO_8859-4:1988", "L4", "LATIN4", "CSISOLATIN4", 0 };

// misc
static codecAliasList m_codecAliases_ISO_8859_3       = { "ISO-8859-3", "ISO-IR-109", "ISO8859-3", "ISO_8859-3", "ISO_8859-3:1988", "L3", "LATIN3", "csISOLatin3", 0 };
static codecAliasList m_codecAliases_ISO_8859_10      = { "ISO-8859-10", "ISO-IR-157", "ISO8859-10", "ISO_8859-10", "ISO_8859-10:1992", "L6", "LATIN6", "csISOLatin6", 0 };
static codecAliasList m_codecAliases_ISO_8859_13      = { "ISO-8859-13", "ISO8859-13", 0 };
static codecAliasList m_codecAliases_ISO_8859_14      = { "ISO-8859-14", "ISO8859-14" "ISO-IR-199", "ISO8859-14", "ISO_8859-14", "ISO_8859-14:1998", "L8", "LATIN8", "iso-celtic", 0 };
static codecAliasList m_codecAliases_ISO_8859_15      = { "ISO-8859-15", "ISO8859-15", "ISO_8859-15", "LATIN-9", 0 };
static codecAliasList m_codecAliases_ISO_8859_16      = { "ISO-8859-16", "ISO8859-16", "ISO_8859-16", "ISO_8859-16:2001", "LATIN10", "ISO-IR-226", "L10", 0 };

static codecAliasList m_codecAliases_MIK              = { "MIK", 0 };

static codecAliasList m_codecAliases_ISCII            = { "ISCII", 0 };
static codecAliasList m_codecAliases_TSCII            = { "TSCII", "csTSCII", 0 };
static codecAliasList m_codecAliases_VISCII           = { "VISCII", "csVISCII", 0 };

static codecAliasList m_codecAliases_CP437            = { "CP437", "IBM437", "437", "csPC8CodePage437", 0 };
static codecAliasList m_codecAliases_CP737            = { "CP737", "IBM737", 0 };
static codecAliasList m_codecAliases_CP850            = { "CP850", "IBM850", "850", "csPC850Multilingual", 0 };
static codecAliasList m_codecAliases_CP852            = { "CP852", "IBM852", "852", "csPCp852", 0 };
static codecAliasList m_codecAliases_CP855            = { "CP855", "IBM855", "855", "csIBM855", 0  };
static codecAliasList m_codecAliases_CP857            = { "CP857", "IBM857", "857", "csIBM857", 0  };
static codecAliasList m_codecAliases_CP860            = { "CP860", "IBM860", "860", "csIBM860", 0  };
static codecAliasList m_codecAliases_CP861            = { "CP861", "IBM861", "861", "csIBM861", 0  };
static codecAliasList m_codecAliases_CP863            = { "CP863", "IBM863", "863", "csIBM863", 0  };
static codecAliasList m_codecAliases_CP865            = { "CP865", "IBM865", "865", "csIBM865", 0  };


static void
registerEncodingNames(EncodingNameRegistrar registrar, const void* obj)
{
    int i=0;
    codecAliasList* list = static_cast<codecAliasList*>(obj);
    do {
        registrar((*list)[i], (*list)[0]);
        i++;
    } while ((*list)[i]);
}

void
TextCodecWKC::registerBaseEncodingNames(EncodingNameRegistrar registrar)
{
    registerEncodingNames(registrar, &m_codecAliases_UTF_8);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_1);
}

void
TextCodecWKC::registerExtendedEncodingNames(EncodingNameRegistrar registrar)
{
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_2);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_3);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_4);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_5);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_6);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_7);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_8);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_9);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_10);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_13);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_14);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_15);
    registerEncodingNames(registrar, &m_codecAliases_ISO_8859_16);

    registerEncodingNames(registrar, &m_codecAliases_CP437);
    registerEncodingNames(registrar, &m_codecAliases_CP737);
    registerEncodingNames(registrar, &m_codecAliases_CP850);
    registerEncodingNames(registrar, &m_codecAliases_CP852);
    registerEncodingNames(registrar, &m_codecAliases_CP855);
    registerEncodingNames(registrar, &m_codecAliases_CP857);
    registerEncodingNames(registrar, &m_codecAliases_CP860);
    registerEncodingNames(registrar, &m_codecAliases_CP861);
    registerEncodingNames(registrar, &m_codecAliases_CP863);
    registerEncodingNames(registrar, &m_codecAliases_CP865);
    registerEncodingNames(registrar, &m_codecAliases_CP866);
    registerEncodingNames(registrar, &m_codecAliases_CP869);
    registerEncodingNames(registrar, &m_codecAliases_CP874);

    registerEncodingNames(registrar, &m_codecAliases_CP1250);
    registerEncodingNames(registrar, &m_codecAliases_CP1251);
    registerEncodingNames(registrar, &m_codecAliases_CP1252);
    registerEncodingNames(registrar, &m_codecAliases_CP1253);
    registerEncodingNames(registrar, &m_codecAliases_CP1254);
    registerEncodingNames(registrar, &m_codecAliases_CP1255);
    registerEncodingNames(registrar, &m_codecAliases_CP1256);
    registerEncodingNames(registrar, &m_codecAliases_CP1257);
    registerEncodingNames(registrar, &m_codecAliases_CP1258);

    registerEncodingNames(registrar, &m_codecAliases_MACROMAN);
    registerEncodingNames(registrar, &m_codecAliases_MACCENTRALEUROPE);
    registerEncodingNames(registrar, &m_codecAliases_MACCYRILLIC);

    registerEncodingNames(registrar, &m_codecAliases_KOI8_R);
    registerEncodingNames(registrar, &m_codecAliases_KOI8_U);
    registerEncodingNames(registrar, &m_codecAliases_MIK);

    registerEncodingNames(registrar, &m_codecAliases_TIS_620);

    registerEncodingNames(registrar, &m_codecAliases_ISCII);
    registerEncodingNames(registrar, &m_codecAliases_TSCII);
    registerEncodingNames(registrar, &m_codecAliases_VISCII);

    registerEncodingNames(registrar, &m_codecAliases_SHIFT_JIS);
    registerEncodingNames(registrar, &m_codecAliases_EUC_JP);
    registerEncodingNames(registrar, &m_codecAliases_ISO_2022_JP);
    registerEncodingNames(registrar, &m_codecAliases_SHIFT_JIS_2004);
    registerEncodingNames(registrar, &m_codecAliases_EUC_JIS_2004);
    registerEncodingNames(registrar, &m_codecAliases_ISO_2022_JP_2004);

    registerEncodingNames(registrar, &m_codecAliases_BIG5);
    registerEncodingNames(registrar, &m_codecAliases_BIG5_HKSCS);
    registerEncodingNames(registrar, &m_codecAliases_CP950);

    registerEncodingNames(registrar, &m_codecAliases_ISO_2022_KR);
    registerEncodingNames(registrar, &m_codecAliases_CP949);
    registerEncodingNames(registrar, &m_codecAliases_EUC_KR);

    registerEncodingNames(registrar, &m_codecAliases_GBK);
    registerEncodingNames(registrar, &m_codecAliases_HZ);
    registerEncodingNames(registrar, &m_codecAliases_GB18030);
    registerEncodingNames(registrar, &m_codecAliases_EUC_CN);
    registerEncodingNames(registrar, &m_codecAliases_2312_80);

}

static std::unique_ptr<TextCodec> newTextCodecWKC(const TextEncoding& encoding, const void* in_codecid)
{
    int id = (int)reinterpret_cast<intptr_t>(in_codecid);
    return std::make_unique<TextCodecWKC>(encoding, id);
}

void
TextCodecWKC::registerBaseCodecs(TextCodecRegistrar registrar)
{
    registrar(m_codecAliases_UTF_8[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_UTF8);
    registrar(m_codecAliases_ISO_8859_1[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_1);
}

void
TextCodecWKC::registerExtendedCodecs(TextCodecRegistrar registrar)
{
    registrar(m_codecAliases_ISO_8859_2[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_2);
    registrar(m_codecAliases_ISO_8859_3[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_3);
    registrar(m_codecAliases_ISO_8859_4[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_4);
    registrar(m_codecAliases_ISO_8859_5[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_5);
    registrar(m_codecAliases_ISO_8859_6[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_6);
    registrar(m_codecAliases_ISO_8859_7[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_7);
    registrar(m_codecAliases_ISO_8859_8[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_8);
    registrar(m_codecAliases_ISO_8859_9[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_9);
    registrar(m_codecAliases_ISO_8859_10[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_10);
    registrar(m_codecAliases_ISO_8859_13[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_13);
    registrar(m_codecAliases_ISO_8859_14[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_14);
    registrar(m_codecAliases_ISO_8859_15[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_15);
    registrar(m_codecAliases_ISO_8859_16[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_8859_16);

    registrar(m_codecAliases_CP437[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP437);
    registrar(m_codecAliases_CP737[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP737);
    registrar(m_codecAliases_CP850[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP850);
    registrar(m_codecAliases_CP852[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP852);
    registrar(m_codecAliases_CP855[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP855);
    registrar(m_codecAliases_CP857[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP857);
    registrar(m_codecAliases_CP860[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP860);
    registrar(m_codecAliases_CP861[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP861);
    registrar(m_codecAliases_CP863[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP863);
    registrar(m_codecAliases_CP865[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP865);
    registrar(m_codecAliases_CP866[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP866);
    registrar(m_codecAliases_CP869[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP869);
    registrar(m_codecAliases_CP874[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP874);

    registrar(m_codecAliases_CP1250[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1250);
    registrar(m_codecAliases_CP1251[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1251);
    registrar(m_codecAliases_CP1252[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1252);
    registrar(m_codecAliases_CP1253[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1253);
    registrar(m_codecAliases_CP1254[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1254);
    registrar(m_codecAliases_CP1255[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1255);
    registrar(m_codecAliases_CP1256[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1256);
    registrar(m_codecAliases_CP1257[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1257);
    registrar(m_codecAliases_CP1258[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP1258);
    
    registrar(m_codecAliases_MACROMAN[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_MAC_ROMAN);
    registrar(m_codecAliases_MACCENTRALEUROPE[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_MAC_CENTRALEUROPE);
    registrar(m_codecAliases_MACCYRILLIC[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_MAC_CYRILLIC);

    registrar(m_codecAliases_KOI8_R[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_KOI8_R);
    registrar(m_codecAliases_KOI8_U[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_KOI8_U);
    registrar(m_codecAliases_MIK[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_MIK);

    registrar(m_codecAliases_TIS_620[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_TIS_620);

    registrar(m_codecAliases_ISCII[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISCII);
    registrar(m_codecAliases_TSCII[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_TSCII);
    registrar(m_codecAliases_VISCII[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_VISCII);

    registrar(m_codecAliases_SHIFT_JIS[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_SHIFT_JIS);
    registrar(m_codecAliases_EUC_JP[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_EUC_JP);
    registrar(m_codecAliases_ISO_2022_JP[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_2022_JP);
    registrar(m_codecAliases_SHIFT_JIS_2004[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_SHIFT_JIS_2004);
    registrar(m_codecAliases_EUC_JIS_2004[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_EUC_JIS_2004);
    registrar(m_codecAliases_ISO_2022_JP_2004[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_2022_JP_2004);

    registrar(m_codecAliases_BIG5[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_BIG5);
    registrar(m_codecAliases_BIG5_HKSCS[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_BIG5_HKSCS);
    registrar(m_codecAliases_CP950[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP950);
    registrar(m_codecAliases_ISO_2022_KR[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_ISO_2022_KR);
    registrar(m_codecAliases_CP949[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_CP949);
    registrar(m_codecAliases_EUC_KR[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_EUC_KR);
    registrar(m_codecAliases_GBK[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_GBK);
    registrar(m_codecAliases_HZ[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_HZ);
    registrar(m_codecAliases_GB18030[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_GB18030);
    registrar(m_codecAliases_EUC_CN[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_EUC_CN);
    registrar(m_codecAliases_2312_80[0], newTextCodecWKC, (void *)WKC_I18N_CODEC_GB_2312_80);
}

TextCodecWKC::TextCodecWKC(const TextEncoding& in_encoding, int codecId)
     : m_codecId(codecId)
{
    m_remains8[0] = 0;
    m_remains16[0] = 0;
    m_remainsLen = 0;
    m_decoder = 0;
    m_encoder = 0;
}

TextCodecWKC::~TextCodecWKC()
{
    if (m_decoder) {
        wkcI18NEndDecodePeer(m_decoder);
    }
    if (m_encoder) {
        wkcI18NEndEncodePeer(m_encoder);
    }
}

int
TextCodecWKC::decode(const char* str, size_t length, unsigned short* outbuf, size_t outbuf_length, bool stopOnError, bool& sawError)
{
    char buf[256];
    int ulen = 0;
    int len = 0;
    int remains = 0;
    int olen = 0;

    if (m_remainsLen > 0) {
        memcpy(buf, m_remains8, m_remainsLen);
        len = min(length, sizeof(buf)-m_remainsLen);
        memcpy(buf+m_remainsLen, str, len);
        str += len;
        length -= len;
        len += m_remainsLen;
    } else {
        len = min(length, sizeof(buf));
        memcpy(buf, str, len);
        str += len;
        length -= len;
    }

    ulen = 0;
    while (len>0) {
        int error = 0;
        remains = 0;
        if (outbuf) {
            olen = wkcI18NDecodePeer(m_decoder, buf, len, outbuf, outbuf_length, &remains, (stopOnError?1:0), &error);
            outbuf += olen;
            outbuf_length -= olen;
        } else {
            olen = wkcI18NDecodePeer(m_decoder, buf, len, NULL, 0, &remains, (stopOnError?1:0), &error);
        }
        if (error)
          sawError = true;
        if (olen<=0) {
            break;
        }
        ulen += olen;

        if (remains>0) {
            memcpy(buf, buf+len-remains, remains);
            len = min(length, sizeof(buf)-remains);
            memcpy(buf+remains, str, len);
            str += len;
            length -= len;
            len += remains;
        } else {
            len = min(length, sizeof(buf));
            memcpy(buf, str, len);
            str += len;
            length -= len;
        }
    
    }

    if (outbuf) {
        if (remains>0 && remains<sizeof(m_remains8)) {
            memcpy(m_remains8, buf, remains);
            m_remainsLen = remains;
        } else {
            m_remainsLen = 0;
        }
    }

    return ulen;
}

String
TextCodecWKC::decode(const char* str, size_t length, bool flush, bool stopOnError, bool& sawError)
{
    int ulen=0;
    UChar* ubuf = NULL;

    if (!m_decoder) {
        m_decoder = wkcI18NBeginDecodePeer(m_codecId);
        if (!m_decoder) {
            return String();
        }
    }

    if (flush) {
        m_remainsLen = 0;
        m_remains8[0] = 0;
        wkcI18NFlushDecodeStatePeer(m_decoder);
    }

    if (!str || !length) {
        return String();
    }

    wkcI18NSaveDecodeStatePeer(m_decoder);
    ulen = decode(str, length, 0, 0, stopOnError, sawError);
    wkcI18NRestoreDecodeStatePeer(m_decoder);
    if (ulen<0) return String();
    String ret = String::createUninitialized(ulen, ubuf);
    ulen = decode(str, length, ubuf, ulen, stopOnError, sawError);
    if (ulen<=0) {
        return String();
    }
    return ret;
}

CString
TextCodecWKC::encode(const UChar* str, size_t length, UnencodableHandling handling)
{
    int len, len2;
    char* buf = NULL;
    int remains = 0;
    CString ret;
    int fallback = WKC_I18N_ENCODEERRORFALLBACK_NONE;

    if (!str || !length) {
        goto error_end;
    }

    switch (handling) {
    case QuestionMarksForUnencodables:
        fallback = WKC_I18N_ENCODEERRORFALLBACK_QUESTION;
        break;
    case EntitiesForUnencodables:
        fallback = WKC_I18N_ENCODEERRORFALLBACK_ESCAPE_XML_DECIMAL;
        break;
    case URLEncodedEntitiesForUnencodables:
        fallback = WKC_I18N_ENCODEERRORFALLBACK_ESCAPE_URLENCODE;
        break;
    default:
        break;
    }

    // should we consider dividing between surrogate pair ?
    // 100331 ACCESS Co.,Ltd.

    if (!m_encoder) {
        m_encoder = wkcI18NBeginEncodePeer(m_codecId);
        if (!m_encoder) {
            goto error_end;
        }
    }
    len = wkcI18NEncodePeer(m_encoder, str, length, NULL, 0, &remains, fallback);
    if (len<=0) {
        goto error_end;
    }
    len += wkcI18NFlushEncodeStatePeer(m_encoder, false, NULL, 0);
    ret = CString::newUninitialized(len, buf);

    len2 = wkcI18NEncodePeer(m_encoder, str, length, buf, len, &remains, fallback);
    wkcI18NFlushEncodeStatePeer(m_encoder, false, buf + len2, len - len2);

    return ret;

error_end:
    return CString("");
}

// returned value: 0 or more if succeeded, -1 if failed.
int
TextCodecWKC::getDecodedTextLength(const char* str, size_t length)
{
    int ulen = 0;
    bool error = false;

    if (!m_decoder) {
        m_decoder = wkcI18NBeginDecodePeer(m_codecId);
        if (!m_decoder) {
            return -1;
        }
    }

    if (!str || !length) {
        return 0;
    }

    wkcI18NSaveDecodeStatePeer(m_decoder);
    ulen = decode(str, length, 0, 0, false, error);
    wkcI18NRestoreDecodeStatePeer(m_decoder);
    if (ulen < 0) {
        return -1;
    }

    return ulen;
}

} // namespace WebCore
