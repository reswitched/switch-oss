/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "HTMLAttachmentElement.h"

#if ENABLE(ATTACHMENT_ELEMENT)

#include "File.h"
#include "HTMLNames.h"
#include "RenderAttachment.h"

namespace WebCore {

using namespace HTMLNames;

HTMLAttachmentElement::HTMLAttachmentElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(attachmentTag));
}

HTMLAttachmentElement::~HTMLAttachmentElement()
{
}

Ref<HTMLAttachmentElement> HTMLAttachmentElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new HTMLAttachmentElement(tagName, document));
}

RenderPtr<RenderElement> HTMLAttachmentElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderAttachment>(*this, WTF::move(style));
}

File* HTMLAttachmentElement::file() const
{
    return m_file.get();
}

void HTMLAttachmentElement::setFile(File* file)
{
    m_file = file;

    auto* renderer = this->renderer();
    if (!is<RenderAttachment>(renderer))
        return;

    downcast<RenderAttachment>(*renderer).invalidate();
}

void HTMLAttachmentElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if ((name == progressAttr || name == titleAttr || name == subtitleAttr) && is<RenderAttachment>(renderer())) {
        downcast<RenderAttachment>(*renderer()).invalidate();
        return;
    }

    HTMLElement::parseAttribute(name, value);
}

String HTMLAttachmentElement::attachmentTitle() const
{
    String title = fastGetAttribute(titleAttr);
    if (!title.isEmpty())
        return title;
    return m_file ? m_file->name() : String();
}

} // namespace WebCore

#endif // ENABLE(ATTACHMENT_ELEMENT)
