/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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

#include "GlyphPage.h"
#include "FontCache.h"
#include "Font.h"
#include "FontPlatformDataWKC.h"

#include <wkc/wkcgpeer.h>

namespace WebCore
{

bool GlyphPage::fill( UChar* buffer, unsigned bufferLength)
{
    if (GlyphPage::size == bufferLength) {
        bool haveGlyphs = false;
        unsigned short localGlyphBuffer[GlyphPage::size * 2];

        const Font& fontData = this->font();
        bool result = wkcFontGetGlyphsForCharactersPeer(fontData.platformData().font()->font(), buffer, bufferLength, localGlyphBuffer);
        if (!result)
            return  false;

        for (unsigned i = 0; i < GlyphPage::size; i++) {
            Glyph glyph = localGlyphBuffer[i];
            if (!glyph)
                setGlyphForIndex(i, 0);
            else {
                setGlyphForIndex(i, glyph);
                haveGlyphs = true;
            }
        }

        return haveGlyphs;
    } else {
        for (unsigned i = 0; i < GlyphPage::size; i++) {
            UChar c0 = buffer[i*2+0];
            UChar c1 = buffer[i*2+1];
            UChar32 v = U16_GET_SUPPLEMENTARY(c0, c1);
            Glyph glyph = v&0xffff;
            setGlyphForIndex(i, glyph);
        }
        return true;
    }
}

}
