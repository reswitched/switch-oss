/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile, Inc.
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
#include "CachedSVGFont.h"

#if ENABLE(SVG_FONTS)

#include "FontDescription.h"
#include "FontPlatformData.h"
#include "NoEventDispatchAssertion.h"
#include "SVGDocument.h"
#include "SVGFontData.h"
#include "SVGFontElement.h"
#include "SVGFontFaceElement.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include "TypedElementDescendantIterator.h"

#if ENABLE(SVG_OTF_CONVERTER)
#include "SVGToOTFFontConversion.h"
#endif

namespace WebCore {

CachedSVGFont::CachedSVGFont(const ResourceRequest& resourceRequest, SessionID sessionID)
    : CachedFont(resourceRequest, sessionID, SVGFontResource)
    , m_externalSVGFontElement(nullptr)
{
}

RefPtr<Font> CachedSVGFont::createFont(const FontDescription& fontDescription, const AtomicString& remoteURI, bool syntheticBold, bool syntheticItalic, bool externalSVG, const FontFeatureSettings& fontFaceFeatures, const FontVariantSettings& fontFaceVariantSettings)
{
#if ENABLE(SVG_OTF_CONVERTER)
    if (!externalSVG || firstFontFace(remoteURI))
        return CachedFont::createFont(fontDescription, remoteURI, syntheticBold, syntheticItalic, externalSVG, fontFaceFeatures, fontFaceVariantSettings);
#else
    if (!externalSVG)
        return CachedFont::createFont(fontDescription, remoteURI, syntheticBold, syntheticItalic, externalSVG, fontFaceFeatures, fontFaceVariantSettings);

    if (SVGFontFaceElement* firstFontFace = this->firstFontFace(remoteURI))
        return Font::create(std::make_unique<SVGFontData>(firstFontFace), fontDescription.computedPixelSize(), syntheticBold, syntheticItalic);
#endif
    return nullptr;
}

FontPlatformData CachedSVGFont::platformDataFromCustomData(const FontDescription& fontDescription, bool bold, bool italic, const FontFeatureSettings& fontFaceFeatures, const FontVariantSettings& fontFaceVariantSettings)
{
    if (m_externalSVGDocument)
        return FontPlatformData(fontDescription.computedPixelSize(), bold, italic);
    return CachedFont::platformDataFromCustomData(fontDescription, bold, italic, fontFaceFeatures, fontFaceVariantSettings);
}

bool CachedSVGFont::ensureCustomFontData(bool externalSVG, const AtomicString& remoteURI)
{
    if (!externalSVG)
        return CachedFont::ensureCustomFontData(externalSVG, remoteURI);

    if (!m_externalSVGDocument && !errorOccurred() && !isLoading() && m_data) {
        bool sawError = false;
        {
            // We may get here during render tree updates when events are forbidden.
            // Frameless document can't run scripts or call back to the client so this is safe.
            m_externalSVGDocument = SVGDocument::create(nullptr, URL());
            auto decoder = TextResourceDecoder::create("application/xml");

            NoEventDispatchAssertion::EventAllowedScope allowedScope(*m_externalSVGDocument);

            m_externalSVGDocument->setContent(decoder->decodeAndFlush(m_data->data(), m_data->size()));
            sawError = decoder->sawError();
        }
#if !ENABLE(SVG_OTF_CONVERTER)
        if (sawError)
            m_externalSVGDocument = nullptr;
#else
        if (decoder->sawError())
            m_externalSVGDocument = nullptr;
        else
            maybeInitializeExternalSVGFontElement(remoteURI);
        if (!m_externalSVGFontElement)
            return false;
        Vector<char> convertedFont = convertSVGToOTFFont(*m_externalSVGFontElement);
        m_convertedFont = SharedBuffer::adoptVector(convertedFont);
#endif
    }

#if !ENABLE(SVG_OTF_CONVERTER)
    return m_externalSVGDocument;
#else
    return m_externalSVGDocument && CachedFont::ensureCustomFontData(m_convertedFont.get());
#endif
}

SVGFontElement* CachedSVGFont::getSVGFontById(const String& fontName) const
{
    ASSERT(m_externalSVGDocument);
    auto elements = descendantsOfType<SVGFontElement>(*m_externalSVGDocument);

    if (fontName.isEmpty())
        return elements.first();

    for (auto& element : elements) {
        if (element.getIdAttribute() == fontName)
            return &element;
    }
    return nullptr;
}

SVGFontElement* CachedSVGFont::maybeInitializeExternalSVGFontElement(const AtomicString& remoteURI)
{
    if (m_externalSVGFontElement)
        return m_externalSVGFontElement;
    String fragmentIdentifier;
    size_t start = remoteURI.find('#');
    if (start != notFound)
        fragmentIdentifier = remoteURI.string().substring(start + 1);
    m_externalSVGFontElement = getSVGFontById(fragmentIdentifier);
    return m_externalSVGFontElement;
}

SVGFontFaceElement* CachedSVGFont::firstFontFace(const AtomicString& remoteURI)
{
    if (!maybeInitializeExternalSVGFontElement(remoteURI))
        return nullptr;

    if (auto* firstFontFace = childrenOfType<SVGFontFaceElement>(*m_externalSVGFontElement).first())
        return firstFontFace;
    return nullptr;
}

}

#endif
