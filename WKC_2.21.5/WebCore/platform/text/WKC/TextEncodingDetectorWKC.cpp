/*
 * Copyright (C) 2008, 2009 Google Inc. All rights reserved.
 * Copyright (c) 2010 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextEncodingDetector.h"

#include "TextEncoding.h"

#include <wkc/wkcpeer.h>

namespace WebCore {

bool detectTextEncoding(const char* data, size_t len,
                        const char* hintEncodingName,
                        TextEncoding* detectedEncoding)
{
    int encoding = WKC_I18N_CODEC_UNKNOWN;
    const char* type = "";

    encoding = wkcI18NDetectEncodingPeer(data, len, hintEncodingName);
    switch (encoding) {
    case WKC_I18N_CODEC_UTF8:
        type = "UTF-8";
        break;
    case WKC_I18N_CODEC_ISO_8859_1:
        type = "ISO-8859-1";
        break;
    case WKC_I18N_CODEC_ISO_8859_2:
        type = "ISO-8859-2";
        break;
    case WKC_I18N_CODEC_ISO_8859_3:
        type = "ISO-8859-3";
        break;
    case WKC_I18N_CODEC_ISO_8859_4:
        type = "ISO-8859-4";
        break;
    case WKC_I18N_CODEC_ISO_8859_5:
        type = "ISO-8859-5";
        break;
    case WKC_I18N_CODEC_ISO_8859_6:
        type = "ISO-8859-6";
        break;
    case WKC_I18N_CODEC_ISO_8859_7:
        type = "ISO-8859-7";
        break;
    case WKC_I18N_CODEC_ISO_8859_8:
        type = "ISO-8859-8";
        break;
    case WKC_I18N_CODEC_ISO_8859_9:
        type = "ISO-8859-9";
        break;
    case WKC_I18N_CODEC_ISO_8859_10:
        type = "ISO-8859-10";
        break;
    case WKC_I18N_CODEC_ISO_8859_13:
        type = "ISO-8859-13";
        break;
    case WKC_I18N_CODEC_ISO_8859_14:
        type = "ISO-8859-14";
        break;
    case WKC_I18N_CODEC_ISO_8859_15:
        type = "ISO-8859-15";
        break;
    case WKC_I18N_CODEC_ISO_8859_16:
        type = "ISO-8859-16";
        break;
    case WKC_I18N_CODEC_CP437:
        type = "CP437";
        break;
    case WKC_I18N_CODEC_CP737:
        type = "CP737";
        break;
    case WKC_I18N_CODEC_CP850:
        type = "CP850";
        break;
    case WKC_I18N_CODEC_CP852:
        type = "CP852";
        break;
    case WKC_I18N_CODEC_CP855:
        type = "CP855";
        break;
    case WKC_I18N_CODEC_CP857:
        type = "CP857";
        break;
    case WKC_I18N_CODEC_CP860:
        type = "CP860";
        break;
    case WKC_I18N_CODEC_CP861:
        type = "CP861";
        break;
    case WKC_I18N_CODEC_CP863:
        type = "CP863";
        break;
    case WKC_I18N_CODEC_CP865:
        type = "CP865";
        break;
    case WKC_I18N_CODEC_CP866:
        type = "CP866";
        break;
    case WKC_I18N_CODEC_CP869:
        type = "CP869";
        break;
    case WKC_I18N_CODEC_CP874:
        type = "CP874";
        break;
    case WKC_I18N_CODEC_CP949:
        type = "CP949";
        break;
    case WKC_I18N_CODEC_CP950:
        type = "CP950";
        break;
    case WKC_I18N_CODEC_CP1250:
        type = "CP1250";
        break;
    case WKC_I18N_CODEC_CP1251:
        type = "CP1251";
        break;
    case WKC_I18N_CODEC_CP1252:
        type = "CP1252";
        break;
    case WKC_I18N_CODEC_CP1253:
        type = "CP1253";
        break;
    case WKC_I18N_CODEC_CP1254:
        type = "CP1254";
        break;
    case WKC_I18N_CODEC_CP1255:
        type = "CP1255";
        break;
    case WKC_I18N_CODEC_CP1256:
        type = "CP1256";
        break;
    case WKC_I18N_CODEC_CP1257:
        type = "CP1257";
        break;
    case WKC_I18N_CODEC_CP1258:
        type = "CP1258";
        break;
    case WKC_I18N_CODEC_KOI8_R:
        type = "KOI8-R";
        break;
    case WKC_I18N_CODEC_KOI8_U:
        type = "KOI8-U";
        break;
    case WKC_I18N_CODEC_MIK:
        type = "MIK";
        break;
    case WKC_I18N_CODEC_TIS_620:
        type = "TIS-620";
        break;
    case WKC_I18N_CODEC_MAC_ROMAN:
        type = "mac-roman";
        break;
    case WKC_I18N_CODEC_MAC_CYRILLIC:
        type = "mac-cyrillic";
        break;
    case WKC_I18N_CODEC_MAC_CENTRALEUROPE:
        type = "MAC-CENTRALEUROPE";
        break;
    case WKC_I18N_CODEC_ISCII:
        type = "ISCII";
        break;
    case WKC_I18N_CODEC_TSCII:
        type = "TSCII";
        break;
    case WKC_I18N_CODEC_VISCII:
        type = "VISCII";
        break;
    case WKC_I18N_CODEC_SHIFT_JIS:
        type = "Shift_JIS";
        break;
    case WKC_I18N_CODEC_EUC_JP:
        type = "EUC-JP";
        break;
    case WKC_I18N_CODEC_ISO_2022_JP:
        type = "ISO-2022-JP";
        break;
    case WKC_I18N_CODEC_BIG5:
        type = "BIG5";
        break;
    case WKC_I18N_CODEC_BIG5_HKSCS:
        type = "BIG5-HKSCS";
        break;
    case WKC_I18N_CODEC_ISO_2022_KR:
        type = "ISO-2022-KR";
        break;
    case WKC_I18N_CODEC_EUC_KR:
        type = "EUC-KR";
        break;
    case WKC_I18N_CODEC_HZ:
        type = "HZ";
        break;
    case WKC_I18N_CODEC_GB18030:
        type = "GB18030";
        break;
    case WKC_I18N_CODEC_EUC_CN:
        type = "EUC-CN";
        break;
    case WKC_I18N_CODEC_GB_2312_80:
        type = "GB_2312-80";
        break;
    default:
        *detectedEncoding = TextEncoding();
        return false;
    }
    *detectedEncoding = TextEncoding(type);
    return true;
}

}
