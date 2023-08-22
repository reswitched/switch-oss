/*
 * Copyright (C) 2013 Apple Inc.  All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "WebVTTElement.h"

#include "HTMLElementFactory.h"
#include "TextTrack.h"

namespace WebCore {

static const QualifiedName& nodeTypeToTagName(WebVTTNodeType nodeType)
{
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, cTag, (nullAtom, "c", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, vTag, (nullAtom, "v", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, langTag, (nullAtom, "lang", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, bTag, (nullAtom, "b", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, uTag, (nullAtom, "u", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, iTag, (nullAtom, "i", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, rubyTag, (nullAtom, "ruby", nullAtom));
    DEPRECATED_DEFINE_STATIC_LOCAL(QualifiedName, rtTag, (nullAtom, "rt", nullAtom));
    switch (nodeType) {
    case WebVTTNodeTypeClass:
        return cTag;
    case WebVTTNodeTypeItalic:
        return iTag;
    case WebVTTNodeTypeLanguage:
        return langTag;
    case WebVTTNodeTypeBold:
        return bTag;
    case WebVTTNodeTypeUnderline:
        return uTag;
    case WebVTTNodeTypeRuby:
        return rubyTag;
    case WebVTTNodeTypeRubyText:
        return rtTag;
    case WebVTTNodeTypeVoice:
        return vTag;
    case WebVTTNodeTypeNone:
    default:
        ASSERT_NOT_REACHED();
        return cTag; // Make the compiler happy.
    }
}

WebVTTElement::WebVTTElement(WebVTTNodeType nodeType, Document& document)
    : Element(nodeTypeToTagName(nodeType), document, CreateElement)
    , m_isPastNode(0)
    , m_webVTTNodeType(nodeType)
{
}

Ref<WebVTTElement> WebVTTElement::create(WebVTTNodeType nodeType, Document& document)
{
    return adoptRef(*new WebVTTElement(nodeType, document));
}

Ref<Element> WebVTTElement::cloneElementWithoutAttributesAndChildren(Document& targetDocument)
{
    Ref<WebVTTElement> clone = create(static_cast<WebVTTNodeType>(m_webVTTNodeType), targetDocument);
    clone->setLanguage(m_language);
    return WTF::move(clone);
}

PassRefPtr<HTMLElement> WebVTTElement::createEquivalentHTMLElement(Document& document)
{
    RefPtr<HTMLElement> htmlElement;

    switch (m_webVTTNodeType) {
    case WebVTTNodeTypeClass:
    case WebVTTNodeTypeLanguage:
    case WebVTTNodeTypeVoice:
        htmlElement = HTMLElementFactory::createElement(HTMLNames::spanTag, document);
        htmlElement->setAttribute(HTMLNames::titleAttr, getAttribute(voiceAttributeName()));
        htmlElement->setAttribute(HTMLNames::langAttr, getAttribute(langAttributeName()));
        break;
    case WebVTTNodeTypeItalic:
        htmlElement = HTMLElementFactory::createElement(HTMLNames::iTag, document);
        break;
    case WebVTTNodeTypeBold:
        htmlElement = HTMLElementFactory::createElement(HTMLNames::bTag, document);
        break;
    case WebVTTNodeTypeUnderline:
        htmlElement = HTMLElementFactory::createElement(HTMLNames::uTag, document);
        break;
    case WebVTTNodeTypeRuby:
        htmlElement = HTMLElementFactory::createElement(HTMLNames::rubyTag, document);
        break;
    case WebVTTNodeTypeRubyText:
        htmlElement = HTMLElementFactory::createElement(HTMLNames::rtTag, document);
        break;
    }

    ASSERT(htmlElement);
    if (htmlElement)
        htmlElement->setAttribute(HTMLNames::classAttr, fastGetAttribute(HTMLNames::classAttr));
    return htmlElement.release();
}

} // namespace WebCore

#endif
