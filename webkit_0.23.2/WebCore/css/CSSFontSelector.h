/*
 * Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
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

#ifndef CSSFontSelector_h
#define CSSFontSelector_h

#include "CachedResourceHandle.h"
#include "Font.h"
#include "FontSelector.h"
#include "Timer.h"
#include <memory>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CSSFontFace;
class CSSFontFaceRule;
class CSSSegmentedFontFace;
class CachedFont;
class Document;
class FontDescription;
class StyleRuleFontFace;

class CSSFontSelector final : public FontSelector {
public:
    static Ref<CSSFontSelector> create(Document& document)
    {
        return adoptRef(*new CSSFontSelector(document));
    }
    virtual ~CSSFontSelector();
    
    virtual unsigned version() const override { return m_version; }
    virtual unsigned uniqueId() const override { return m_uniqueId; }

    virtual FontRanges fontRangesForFamily(const FontDescription&, const AtomicString&) override;
    virtual size_t fallbackFontCount() override;
    virtual PassRefPtr<Font> fallbackFontAt(const FontDescription&, size_t) override;
    CSSSegmentedFontFace* getFontFace(const FontDescription&, const AtomicString& family);

    virtual bool resolvesFamilyFor(const FontDescription&) const override;

    void clearDocument();

    void addFontFaceRule(const StyleRuleFontFace*, bool isInitiatingElementInUserAgentShadowTree);

    void fontLoaded();
    virtual void fontCacheInvalidated() override;

    bool isEmpty() const;

    virtual void registerForInvalidationCallbacks(FontSelectorClient&) override;
    virtual void unregisterForInvalidationCallbacks(FontSelectorClient&) override;

    Document* document() const { return m_document; }

    void beginLoadingFontSoon(CachedFont*);

private:
    explicit CSSFontSelector(Document&);

    void dispatchInvalidationCallbacks();

    void beginLoadTimerFired();

    Document* m_document;
    HashMap<String, std::unique_ptr<Vector<RefPtr<CSSFontFace>>>, CaseFoldingHash> m_fontFaces;
    HashMap<String, std::unique_ptr<Vector<RefPtr<CSSFontFace>>>, CaseFoldingHash> m_locallyInstalledFontFaces;
    HashMap<String, std::unique_ptr<HashMap<unsigned, RefPtr<CSSSegmentedFontFace>>>, CaseFoldingHash> m_fonts;
    HashSet<FontSelectorClient*> m_clients;

    Vector<CachedResourceHandle<CachedFont>> m_fontsToBeginLoading;
    Timer m_beginLoadingTimer;

    unsigned m_uniqueId;
    unsigned m_version;
};

} // namespace WebCore

#endif // CSSFontSelector_h
