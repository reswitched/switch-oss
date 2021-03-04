/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "SurroundingText.h"

#include "Document.h"
#include "TextIterator.h"
#include "VisibleSelection.h"
#include "VisibleUnits.h"

namespace WebCore {

SurroundingText::SurroundingText(const VisiblePosition& visiblePosition, unsigned maxLength)
    : m_positionOffsetInContent(0)
{
    if (visiblePosition.isNull())
        return;

    const unsigned halfMaxLength = maxLength / 2;
#if !PLATFORM(WKC)
    CharacterIterator forwardIterator(makeRange(visiblePosition, endOfDocument(visiblePosition)).get(), TextIteratorStopsOnFormControls);
#else
    Range& r = *makeRange(visiblePosition, endOfDocument(visiblePosition));
    CharacterIterator forwardIterator(r, TextIteratorStopsOnFormControls);
#endif
    if (!forwardIterator.atEnd())
        forwardIterator.advance(maxLength - halfMaxLength);

    Position position = visiblePosition.deepEquivalent().parentAnchoredEquivalent();
    Document* document = position.document();
    RefPtr<Range> forwardRange = forwardIterator.range();
#if !PLATFORM(WKC)
    if (!forwardRange || !Range::create(document, position, forwardRange->startPosition())->text().length()) {
#else
    if (!forwardRange || !Range::create(*document, position, forwardRange->startPosition())->text().length()) {
#endif
        ASSERT(forwardRange);
        return;
    }

#if !PLATFORM(WKC)
    BackwardsCharacterIterator backwardsIterator(makeRange(startOfDocument(visiblePosition), visiblePosition).get(), TextIteratorStopsOnFormControls);
#else
    Range& r2 = *makeRange(startOfDocument(visiblePosition), visiblePosition);
    BackwardsCharacterIterator backwardsIterator(r2);
#endif
    if (!backwardsIterator.atEnd())
        backwardsIterator.advance(halfMaxLength);

    RefPtr<Range> backwardsRange = backwardsIterator.range();
    if (!backwardsRange) {
        ASSERT(backwardsRange);
        return;
    }

#if !PLATFORM(WKC)
    m_positionOffsetInContent = Range::create(document, backwardsRange->endPosition(), position)->text().length();
    m_contentRange = Range::create(document, backwardsRange->endPosition(), forwardRange->startPosition());
#else
    m_positionOffsetInContent = Range::create(*document, backwardsRange->endPosition(), position)->text().length();
    m_contentRange = Range::create(*document, backwardsRange->endPosition(), forwardRange->startPosition());
#endif
    ASSERT(m_contentRange);
}

PassRefPtr<Range> SurroundingText::rangeFromContentOffsets(unsigned startOffsetInContent, unsigned endOffsetInContent)
{
    if (startOffsetInContent >= endOffsetInContent || endOffsetInContent > content().length())
        return 0;

#if !PLATFORM(WKC)
    CharacterIterator iterator(m_contentRange.get());
#else
    CharacterIterator iterator(*m_contentRange);
#endif

    ASSERT(!iterator.atEnd());
    iterator.advance(startOffsetInContent);

#if !PLATFORM(WKC)
    ASSERT(iterator.range());
#endif
    Position start = iterator.range()->startPosition();

    ASSERT(!iterator.atEnd());
    iterator.advance(endOffsetInContent - startOffsetInContent);

#if !PLATFORM(WKC)
    ASSERT(iterator.range());
#endif
    Position end = iterator.range()->startPosition();

#if !PLATFORM(WKC)
    return Range::create(start.document(), start, end);
#else
    return Range::create(*start.document(), start, end);
#endif
}

String SurroundingText::content() const
{
    if (m_contentRange)
        return m_contentRange->text();
    return String();
}

unsigned SurroundingText::positionOffsetInContent() const
{
    return m_positionOffsetInContent;
}

} // namespace WebCore
