/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * All rights reserved.
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#include "FontCascade.h"

#include "FloatRect.h"
#include "Font.h"
#include "FontCache.h"
#include "FontDescription.h"
#include "FontPlatformDataWKC.h"
#include "GlyphBuffer.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>

namespace WebCore {

static int averageGlyph(int glyph=0, bool set=false)
{
    WKC_DEFINE_STATIC_INT(gGlyph, 0);
    if (set) {
        gGlyph = glyph;
    }
    return gGlyph;
}

void Font::platformInit()
{
    WKC::WKCFontInfo* info = m_platformData.font();
    if (!info)
        return;

    void* font = info->font();
    float scale = info->scale();
    if (!font) return;

    m_fontMetrics.setAscent(info->ascent() * scale);
    m_fontMetrics.setDescent(info->descent() * scale);
    m_fontMetrics.setCapHeight(info->requestSize());
    m_fontMetrics.setLineSpacing(info->lineSpacing() * scale);
    m_fontMetrics.setXHeight((float)wkcFontGetXHeightPeer(font) * scale);
    m_fontMetrics.setUnitsPerEm(1);
    m_fontMetrics.setLineGap((float)wkcFontGetLineGapPeer(font) * scale);
}

void Font::platformCharWidthInit()
{
    WKC::WKCFontInfo* info = m_platformData.font();
    if (!info)
        return;

    void* font = info->font();
    float scale = info->scale();

    if (!font) return;
    if (!averageGlyph()) {
        m_avgCharWidth = (float)wkcFontGetAverageCharWidthPeer(font) * scale;
    } else {
        unsigned short buf[2] = {0};
        buf[0] = (unsigned short)averageGlyph();
        m_avgCharWidth = FontCascade::getTextWidth(m_platformData, WKC_FONT_FLAG_NONE, buf, 1, 0) * scale;
    }
    m_maxCharWidth = (float)wkcFontGetMaxCharWidthPeer(font) * scale;
    initCharWidths();
}

void Font::platformDestroy()
{
}

FloatRect Font::platformBoundsForGlyph(Glyph glyph) const
{
    if (!wkcFontEngineIsReadyToDrawPeer())
        return FloatRect(0, 0, cGlyphSizeUnknown, cGlyphSizeUnknown);

    WKC::WKCFontInfo* info = m_platformData.font();
    if (!info || !info->font()) return FloatRect();

    const float scale = info->scale();
    UChar32 c = glyph;
    int u = info->specificUnicodeChar();

    if (m_platformData.isEmoji()) {
    } else if (u==0) {
        if (FontCascade::treatAsSpace(c)) {
            c = 0x20;
        } else if ((FontCascade::treatAsZeroWidthSpace(c) || c==0xfeff)) {
            return FloatRect(0,0,0,0);
        }
    } else {
        c = u;
    }

    unsigned short buf[2] = {0};
    int len = 1;
    if (c<0x10000) {
        buf[0] = c;
        len = 1;
    } else {
        buf[0] = U16_LEAD(c);
        buf[1] = U16_TRAIL(c);
        len = 2;
    }
    float w = 0;
    /*const float tw = */FontCascade::getTextWidth(m_platformData, WKC_FONT_FLAG_NONE, buf, len, &w);

    if (scale!=1.f) {
        w = w * scale;
    }

    const float h = wkcFontGetSizePeer(info->font());
    return FloatRect(0, 0, w, h);
}


void Font::determinePitch()
{
    WKC::WKCFontInfo* info = m_platformData.font();
    if (!info || !info->font()) return;

    m_treatAsFixedPitch = wkcFontIsFixedFontPeer(info->font());
}

float Font::platformWidthForGlyph(Glyph glyph) const
{
    if (!wkcFontEngineIsReadyToDrawPeer())
        return cGlyphSizeUnknown;

    WKC::WKCFontInfo* info = m_platformData.font();
    if (!info || !info->font()) return 0;

    const float scale = info->scale();
    UChar32 c = glyph;
    int u = info->specificUnicodeChar();

    if (m_platformData.isEmoji()) {
    } else if (u==0) {
        if (FontCascade::treatAsSpace(c)) {
            c = 0x20;
        } else if ((FontCascade::treatAsZeroWidthSpace(c) || c==0xfeff)) {
            return 0.f;
        }
    } else {
        c = u;
    }

    unsigned short buf[2] = {0};
    int len = 1;
    if (c<0x10000) {
        buf[0] = c;
        len = 1;
    } else {
        buf[0] = U16_LEAD(c);
        buf[1] = U16_TRAIL(c);
        len = 2;
    }
    const float w = FontCascade::getTextWidth(m_platformData, WKC_FONT_FLAG_NONE, buf, len, 0);

    if (scale==1.f) {
        return w;
    } else {
        return w * scale;
    }
}

void
SimpleFontData_SetAverageFontGlyph(const unsigned int in_glyph)
{
    averageGlyph(in_glyph, true);
}

}
