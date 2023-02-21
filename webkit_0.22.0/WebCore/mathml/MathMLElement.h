/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Fran�Fois Sausset (sausset@gmail.com). All rights reserved.
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

#ifndef MathMLElement_h
#define MathMLElement_h

#if ENABLE(MATHML)

#include "MathMLNames.h"
#include "StyledElement.h"

namespace WebCore {

class MathMLElement : public StyledElement {
public:
    static Ref<MathMLElement> create(const QualifiedName& tagName, Document&);

    int colSpan() const;
    int rowSpan() const;

    bool isMathMLToken() const
    {
        return hasTagName(MathMLNames::miTag) || hasTagName(MathMLNames::mnTag) || hasTagName(MathMLNames::moTag) || hasTagName(MathMLNames::msTag) || hasTagName(MathMLNames::mtextTag);
    }

    bool isSemanticAnnotation() const
    {
        return hasTagName(MathMLNames::annotationTag) || hasTagName(MathMLNames::annotation_xmlTag);
    }

    virtual bool isPresentationMathML() const;

    bool hasTagName(const MathMLQualifiedName& name) const { return hasLocalName(name.localName()); }

protected:
    MathMLElement(const QualifiedName& tagName, Document&);

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual bool childShouldCreateRenderer(const Node&) const override;
    virtual void attributeChanged(const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue, AttributeModificationReason) override;

    virtual bool isPresentationAttribute(const QualifiedName&) const override;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStyleProperties&) override;

    bool isPhrasingContent(const Node&) const;
    bool isFlowContent(const Node&) const;

private:
    virtual void updateSelectedChild() { }
};

inline bool Node::hasTagName(const MathMLQualifiedName& name) const
{
    return isMathMLElement() && downcast<MathMLElement>(*this).hasTagName(name);
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::MathMLElement)
    static bool isType(const WebCore::Node& node) { return node.isMathMLElement(); }
SPECIALIZE_TYPE_TRAITS_END()

#include "MathMLElementTypeHelpers.h"

#endif // ENABLE(MATHML)

#endif // MathMLElement_h
