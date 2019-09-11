/*
* Copyright (C) 2017 Apple Inc. All rights reserved.
* Copyright (c) 2018-2019 ACCESS CO., LTD. All rights reserved.
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

static std::optional<WKCRun>
findNextRun(const UChar* characters, unsigned length, unsigned offset)
{
    SurrogatePairAwareTextIterator textIterator(characters + offset, offset, length, length);
    UChar32 character;
    unsigned clusterLength = 0;
    if (!textIterator.consume(character, clusterLength))
        return std::nullopt;

    unsigned startIndex = offset;
    for (textIterator.advance(clusterLength); textIterator.consume(character, clusterLength); textIterator.advance(clusterLength)) {
        if (FontCascade::treatAsZeroWidthSpace(character))
            continue;
    }

    return std::optional<WKCRun>({ startIndex, textIterator.currentIndex() });
}

void
ComplexTextController::collectComplexTextRunsForCharacters(const UChar* characters, unsigned length, unsigned stringLocation, const Font* font)
{
    if (!font) {
        // Create a run of missing glyphs from the primary font.
        m_complexTextRuns.append(ComplexTextRun::create(m_font.primaryFont(), characters, stringLocation, length, 0, length, m_run.ltr()));
        return;
    }

    WKC::WKCFontInfo* info = font->platformData().font();
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

    for (unsigned i = 0; i < runCount; ++i) {
        auto& run = runList[m_run.rtl() ? runCount - i - 1 : i];

        Vector<FloatSize> advances;
        Vector<CGGlyph> glyphs;
        Vector<unsigned> stringIndices;
        FloatSize initialAdvance(0, 0);

        auto runLengthInCodeUnits = run.endIndex - run.startIndex;
        advances.reserveInitialCapacity(runLengthInCodeUnits);
        glyphs.reserveInitialCapacity(runLengthInCodeUnits);
        stringIndices.reserveInitialCapacity(runLengthInCodeUnits);

        unsigned r = run.startIndex;
        while (r < run.endIndex) {
            stringIndices.uncheckedAppend(r);
            UChar32 character;
            U16_NEXT(characters, r, length, character);

            Glyph glyph = font->glyphForCharacter(character);
            glyphs.uncheckedAppend(glyph);
            FloatSize advance = { font->widthForGlyph(glyph), static_cast<float>(wkcFontGetSizePeer(info->font())) };
            advances.uncheckedAppend(advance);
        }

        m_complexTextRuns.append(ComplexTextRun::create(advances, {}, glyphs, stringIndices, initialAdvance, *font, characters, stringLocation, length, run.startIndex, run.endIndex, m_run.ltr()));
    }
}

}
