/*
* Copyright (C) 2017 Apple Inc. All rights reserved.
* Copyright (c) 2018-2020 ACCESS CO., LTD. All rights reserved.
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
* THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "ComplexTextController.h"

#include "FontCascade.h"
#include "SurrogatePairAwareTextIterator.h"

#include "FontPlatformDataWKC.h"

#include <wkc/wkcgpeer.h>

namespace WebCore {

struct WKCRun {
    unsigned startIndex;
    unsigned endIndex;
};

static Optional<WKCRun>
findNextRun(const UChar* characters, unsigned length, unsigned offset)
{
    SurrogatePairAwareTextIterator textIterator(characters, offset, length, length);
    UChar32 character;
    unsigned clusterLength = 0;
    if (!textIterator.consume(character, clusterLength))
        return WTF::nullopt;

    unsigned startIndex = offset;
    for (textIterator.advance(clusterLength); textIterator.consume(character, clusterLength); textIterator.advance(clusterLength)) {
        if (FontCascade::treatAsZeroWidthSpace(character))
            continue;
    }

    return Optional<WKCRun>({ startIndex, textIterator.currentIndex() });
}

void
ComplexTextController::collectComplexTextRunsForCharacters(const UChar* characters, unsigned length, unsigned stringLocation, const Font* font)
{
    if (!font) {
        // Create a run of missing glyphs from the primary font.
        m_complexTextRuns.append(ComplexTextRun::create(m_font.primaryFont(), characters, stringLocation, length, 0, length, m_run.ltr()));
        return;
    }

    const auto pd = m_font.primaryFont().platformData();
    WKC::WKCFontInfo* info = pd.font();
    if (!info || !info->font()) {
        // Create a run of missing glyphs from the primary font.
        m_complexTextRuns.append(ComplexTextRun::create(m_font.primaryFont(), characters, stringLocation, length, 0, length, m_run.ltr()));
        return;
    }

    Vector<WKCRun> runList;
    unsigned offset = 0;
    while (offset < length) {
        auto run = findNextRun(characters, length, offset);
        if (!run)
            break;
        runList.append(run.value());
        offset = run->endIndex;
    }

    size_t runCount = runList.size();
    if (!runCount)
        return;

    const UChar* cp = characters;
    offset = stringLocation;
    for (unsigned i = 0; i < runCount; ++i) {
        auto& run = runList[m_run.rtl() ? runCount - i - 1 : i];
        size_t r = run.startIndex;

        bool needfree = false;
        int len = run.endIndex - r;
        UChar* gs = FontCascade::fixedGlyphs(info->specificUnicodeChar(), pd.isSupplemental(), &characters[r], len, needfree);

        Vector<FloatSize> advances;
        Vector<CGGlyph> glyphs;
        Vector<unsigned> stringIndices;
        FloatSize initialAdvance(0, 0);

        advances.reserveInitialCapacity(len);
        glyphs.append(gs, len);
        stringIndices.reserveInitialCapacity(len);

        if (needfree) {
            fastFree(gs);
        }

        r = 0;
        while (r < len) {
            stringIndices.uncheckedAppend(r);

            FloatSize advance;
            size_t emojilen = wkcDrawContextIsEmojiSequencePeer(&glyphs[r], len - r);
            if (emojilen) {
                advance.setWidth(wkcDrawContextGetEmojiWidthPeer(info->font(), &glyphs[r], emojilen, 0));
            } else {
                advance.setWidth(font->widthForGlyph(glyphs[r]));
                r++;
            }
            advance.setHeight(static_cast<float>(wkcFontGetSizePeer(info->font())));

            advances.uncheckedAppend(advance);

            if (emojilen) {
                for (size_t i = 1; i < emojilen; i++) {
                    stringIndices.uncheckedAppend(r + i);
                    advances.uncheckedAppend({0, 0});        
                }

                r += emojilen;
            }
        }

        m_complexTextRuns.append(ComplexTextRun::create(advances, {}, glyphs, stringIndices, initialAdvance, *font, cp, offset, len, 0, len, m_run.ltr()));
        cp += len;
        offset += len;
    }
}

}
