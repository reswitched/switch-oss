/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#if ENABLE(MATHML)

#include "MathMLTextElement.h"

#include "MathMLNames.h"
#include "RenderMathMLOperator.h"
#include "RenderMathMLSpace.h"
#include "RenderMathMLToken.h"

namespace WebCore {
    
using namespace MathMLNames;

inline MathMLTextElement::MathMLTextElement(const QualifiedName& tagName, Document& document)
    : MathMLElement(tagName, document)
{
    setHasCustomStyleResolveCallbacks();
}

Ref<MathMLTextElement> MathMLTextElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new MathMLTextElement(tagName, document));
}

void MathMLTextElement::didAttachRenderers()
{
    MathMLElement::didAttachRenderers();
    if (is<RenderMathMLToken>(renderer()))
        downcast<RenderMathMLToken>(*renderer()).updateTokenContent();
}

void MathMLTextElement::childrenChanged(const ChildChange& change)
{
    MathMLElement::childrenChanged(change);
    if (is<RenderMathMLToken>(renderer()))
        downcast<RenderMathMLToken>(*renderer()).updateTokenContent();
}

void MathMLTextElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == stretchyAttr) {
        if (is<RenderMathMLOperator>(renderer()))
            downcast<RenderMathMLOperator>(*renderer()).setOperatorFlagAndScheduleLayoutIfNeeded(MathMLOperatorDictionary::Stretchy, value);
        return;
    }

    MathMLElement::parseAttribute(name, value);
}

RenderPtr<RenderElement> MathMLTextElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition& insertionPosition)
{
    if (hasTagName(MathMLNames::moTag))
        return createRenderer<RenderMathMLOperator>(*this, WTF::move(style));
    if (hasTagName(MathMLNames::mspaceTag))
        return createRenderer<RenderMathMLSpace>(*this, WTF::move(style));
    if (hasTagName(MathMLNames::annotationTag))
        return MathMLElement::createElementRenderer(WTF::move(style), insertionPosition);

    ASSERT(hasTagName(MathMLNames::miTag) || hasTagName(MathMLNames::mnTag) || hasTagName(MathMLNames::msTag) || hasTagName(MathMLNames::mtextTag));

    // FIXME: why do we have to set the alignment here ? It seems needed to make the
    // style-changed.htmt test to pass, since mathml renders expect Stretch as default.
    style.get().setAlignItemsPosition(ItemPositionStretch);

    return createRenderer<RenderMathMLToken>(*this, WTF::move(style));
}

bool MathMLTextElement::childShouldCreateRenderer(const Node& child) const
{
    if (hasTagName(MathMLNames::mspaceTag))
        return false;

    // FIXME: phrasing content should be accepted in <mo> elements too (https://bugs.webkit.org/show_bug.cgi?id=130245).
    if (hasTagName(MathMLNames::annotationTag) || hasTagName(MathMLNames::moTag))
        return child.isTextNode();

    // The HTML specification defines <mi>, <mo>, <mn>, <ms> and <mtext> as insertion points.
    return isPhrasingContent(child) && StyledElement::childShouldCreateRenderer(child);
}

}

#endif // ENABLE(MATHML)
