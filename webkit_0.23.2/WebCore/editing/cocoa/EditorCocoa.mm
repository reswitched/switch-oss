/*
 * Copyright (C) 2006, 2007, 2008, 2013, 2015 Apple Inc. All rights reserved.
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

#import "config.h"
#import "Editor.h"

#import "CSSValueList.h"
#import "CSSValuePool.h"
#import "EditingStyle.h"
#import "Frame.h"
#import "FrameSelection.h"
#import "NSAttributedStringSPI.h"
#import "RenderElement.h"
#import "RenderStyle.h"
#import "SoftLinking.h"
#import "Text.h"
#import "htmlediting.h"

namespace WebCore {

// FIXME: This figures out the current style by inserting a <span>!
RenderStyle* Editor::styleForSelectionStart(Frame* frame, Node *&nodeToRemove)
{
    nodeToRemove = nullptr;
    
    if (frame->selection().isNone())
        return nullptr;

    Position position = adjustedSelectionStartForStyleComputation(frame->selection().selection());
    if (!position.isCandidate() || position.isNull())
        return nullptr;

    RefPtr<EditingStyle> typingStyle = frame->selection().typingStyle();
    if (!typingStyle || !typingStyle->style())
        return &position.deprecatedNode()->renderer()->style();

    Ref<Element> styleElement = frame->document()->createElement(HTMLNames::spanTag, false);

    String styleText = typingStyle->style()->asText() + " display: inline";
    styleElement->setAttribute(HTMLNames::styleAttr, styleText);

    styleElement->appendChild(frame->document()->createEditingTextNode(""), ASSERT_NO_EXCEPTION);

    ContainerNode* parentNode = position.deprecatedNode()->parentNode();

    if (!parentNode->ensurePreInsertionValidity(*styleElement, nullptr, IGNORE_EXCEPTION))
        return nullptr;

    parentNode->appendChild(styleElement.copyRef(), ASSERT_NO_EXCEPTION);

    nodeToRemove = styleElement.ptr();

    frame->document()->updateStyleIfNeeded();
    return styleElement->renderer() ? &styleElement->renderer()->style() : nullptr;
}

void Editor::getTextDecorationAttributesRespectingTypingStyle(RenderStyle& style, NSMutableDictionary* result) const
{
    RefPtr<EditingStyle> typingStyle = m_frame.selection().typingStyle();
    if (typingStyle && typingStyle->style()) {
        RefPtr<CSSValue> value = typingStyle->style()->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
        if (value && value->isValueList()) {
            CSSValueList& valueList = downcast<CSSValueList>(*value);
            if (valueList.hasValue(cssValuePool().createIdentifierValue(CSSValueLineThrough).ptr()))
                [result setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSStrikethroughStyleAttributeName];
            if (valueList.hasValue(cssValuePool().createIdentifierValue(CSSValueUnderline).ptr()))
                [result setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName];
        }
    } else {
        int decoration = style.textDecorationsInEffect();
        if (decoration & TextDecorationLineThrough)
            [result setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSStrikethroughStyleAttributeName];
        if (decoration & TextDecorationUnderline)
            [result setObject:[NSNumber numberWithInt:NSUnderlineStyleSingle] forKey:NSUnderlineStyleAttributeName];
    }
}

}
