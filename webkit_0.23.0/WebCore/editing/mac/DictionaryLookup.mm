/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "DictionaryLookup.h"

#if PLATFORM(MAC)

#import "BlockExceptions.h"
#import "Document.h"
#import "FocusController.h"
#import "Frame.h"
#import "FrameSelection.h"
#import "HTMLConverter.h"
#import "HitTestResult.h"
#import "LookupSPI.h"
#import "Page.h"
#import "Range.h"
#import "RenderObject.h"
#import "TextIterator.h"
#import "VisiblePosition.h"
#import "VisibleSelection.h"
#import "VisibleUnits.h"
#import "WebCoreSystemInterface.h"
#import "htmlediting.h"
#import <PDFKit/PDFKit.h>
#import <wtf/RefPtr.h>

namespace WebCore {

bool isPositionInRange(const VisiblePosition& position, Range* range)
{
    RefPtr<Range> positionRange = makeRange(position, position);

    ExceptionCode ec = 0;
    range->compareBoundaryPoints(Range::START_TO_START, positionRange.get(), ec);
    if (ec)
        return false;

    if (!range->isPointInRange(positionRange->startContainer(), positionRange->startOffset(), ec))
        return false;
    if (ec)
        return false;

    return true;
}

bool shouldUseSelection(const VisiblePosition& position, const VisibleSelection& selection)
{
    if (!selection.isRange())
        return false;

    RefPtr<Range> selectedRange = selection.toNormalizedRange();
    if (!selectedRange)
        return false;

    return isPositionInRange(position, selectedRange.get());
}

PassRefPtr<Range> rangeForDictionaryLookupForSelection(const VisibleSelection& selection, NSDictionary **options)
{
    RefPtr<Range> selectedRange = selection.toNormalizedRange();
    if (!selectedRange)
        return nullptr;

    VisiblePosition selectionStart = selection.visibleStart();
    VisiblePosition selectionEnd = selection.visibleEnd();

    // As context, we are going to use the surrounding paragraphs of text.
    VisiblePosition paragraphStart = startOfParagraph(selectionStart);
    VisiblePosition paragraphEnd = endOfParagraph(selectionEnd);

    int lengthToSelectionStart = TextIterator::rangeLength(makeRange(paragraphStart, selectionStart).get());
    int lengthToSelectionEnd = TextIterator::rangeLength(makeRange(paragraphStart, selectionEnd).get());
    NSRange rangeToPass = NSMakeRange(lengthToSelectionStart, lengthToSelectionEnd - lengthToSelectionStart);

    String fullPlainTextString = plainText(makeRange(paragraphStart, paragraphEnd).get());

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    // Since we already have the range we want, we just need to grab the returned options.
    if (Class luLookupDefinitionModule = getLULookupDefinitionModuleClass())
        [luLookupDefinitionModule tokenRangeForString:fullPlainTextString range:rangeToPass options:options];
    END_BLOCK_OBJC_EXCEPTIONS;

    return selectedRange.release();
}

PassRefPtr<Range> rangeForDictionaryLookupAtHitTestResult(const HitTestResult& hitTestResult, NSDictionary **options)
{
    Node* node = hitTestResult.innerNonSharedNode();
    if (!node)
        return nullptr;

    auto renderer = node->renderer();
    if (!renderer)
        return nullptr;

    Frame* frame = node->document().frame();
    if (!frame)
        return nullptr;

    // Don't do anything if there is no character at the point.
    IntPoint framePoint = hitTestResult.roundedPointInInnerNodeFrame();
    if (!frame->rangeForPoint(framePoint))
        return nullptr;

    VisiblePosition position = frame->visiblePositionForPoint(framePoint);
    if (position.isNull())
        position = firstPositionInOrBeforeNode(node);

    VisibleSelection selection = frame->page()->focusController().focusedOrMainFrame().selection().selection();
    if (shouldUseSelection(position, selection))
        return rangeForDictionaryLookupForSelection(selection, options);

    VisibleSelection selectionAccountingForLineRules = VisibleSelection(position);
    selectionAccountingForLineRules.expandUsingGranularity(WordGranularity);
    position = selectionAccountingForLineRules.start();

    // As context, we are going to use 250 characters of text before and after the point.
    RefPtr<Range> fullCharacterRange = rangeExpandedAroundPositionByCharacters(position, 250);
    if (!fullCharacterRange)
        return nullptr;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSRange rangeToPass = NSMakeRange(TextIterator::rangeLength(makeRange(fullCharacterRange->startPosition(), position).get()), 0);

    String fullPlainTextString = plainText(fullCharacterRange.get());

    NSRange extractedRange = NSMakeRange(rangeToPass.location, 0);
    if (Class luLookupDefinitionModule = getLULookupDefinitionModuleClass())
        extractedRange = [luLookupDefinitionModule tokenRangeForString:fullPlainTextString range:rangeToPass options:options];

    // This function sometimes returns {NSNotFound, 0} if it was unable to determine a good string.
    if (extractedRange.location == NSNotFound)
        return nullptr;

    return TextIterator::subrange(fullCharacterRange.get(), extractedRange.location, extractedRange.length);

    END_BLOCK_OBJC_EXCEPTIONS;
    return nullptr;
}

static void expandSelectionByCharacters(PDFSelection *selection, NSInteger numberOfCharactersToExpand, NSInteger& charactersAddedBeforeStart, NSInteger& charactersAddedAfterEnd)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    size_t originalLength = selection.string.length;
    [selection extendSelectionAtStart:numberOfCharactersToExpand];
    
    charactersAddedBeforeStart = selection.string.length - originalLength;
    
    [selection extendSelectionAtEnd:numberOfCharactersToExpand];
    charactersAddedAfterEnd = selection.string.length - originalLength - charactersAddedBeforeStart;

    END_BLOCK_OBJC_EXCEPTIONS;
}

NSString *dictionaryLookupForPDFSelection(PDFSelection *selection, NSDictionary **options)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    // Don't do anything if there is no character at the point.
    if (!selection || !selection.string.length)
        return @"";
    
    RetainPtr<PDFSelection> selectionForLookup = adoptNS([selection copy]);
    
    // As context, we are going to use 250 characters of text before and after the point.
    NSInteger originalLength = [selectionForLookup string].length;
    NSInteger charactersAddedBeforeStart = 0;
    NSInteger charactersAddedAfterEnd = 0;
    expandSelectionByCharacters(selectionForLookup.get(), 250, charactersAddedBeforeStart, charactersAddedAfterEnd);
    
    NSString *fullPlainTextString = [selectionForLookup string];
    NSRange rangeToPass = NSMakeRange(charactersAddedBeforeStart, 0);
    
    NSRange extractedRange = NSMakeRange(rangeToPass.location, 0);
    if (Class luLookupDefinitionModule = getLULookupDefinitionModuleClass())
        extractedRange = [luLookupDefinitionModule tokenRangeForString:fullPlainTextString range:rangeToPass options:options];
    
    // This function sometimes returns {NSNotFound, 0} if it was unable to determine a good string.
    if (extractedRange.location == NSNotFound)
        return selection.string;
    
    NSInteger lookupAddedBefore = rangeToPass.location - extractedRange.location;
    NSInteger lookupAddedAfter = (extractedRange.location + extractedRange.length) - (rangeToPass.location + originalLength);
    
    [selection extendSelectionAtStart:lookupAddedBefore];
    [selection extendSelectionAtEnd:lookupAddedAfter];
    
    ASSERT([selection.string isEqualToString:[fullPlainTextString substringWithRange:extractedRange]]);
    return selection.string;

    END_BLOCK_OBJC_EXCEPTIONS;
    return nil;
}

} // namespace WebCore

#endif // PLATFORM(MAC)

