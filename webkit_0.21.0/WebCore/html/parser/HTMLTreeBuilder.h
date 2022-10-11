/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLTreeBuilder_h
#define HTMLTreeBuilder_h

#include "HTMLConstructionSite.h"
#include "HTMLParserOptions.h"

namespace WebCore {

class HTMLDocumentParser;

class HTMLTreeBuilder {
    WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLTreeBuilder(HTMLDocumentParser&, HTMLDocument&, ParserContentPolicy, const HTMLParserOptions&);
    HTMLTreeBuilder(HTMLDocumentParser&, DocumentFragment&, Element& contextElement, ParserContentPolicy, const HTMLParserOptions&);
    void setShouldSkipLeadingNewline(bool);

    ~HTMLTreeBuilder();

    bool isParsingFragment() const;

    void constructTree(AtomicHTMLToken&);

    bool hasParserBlockingScript() const;

    // Must be called to take the parser-blocking script before calling the parser again.
    RefPtr<Element> takeScriptToProcess(TextPosition& scriptStartPosition);

    // Done, close any open tags, etc.
    void finished();

private:
    class ExternalCharacterTokenBuffer;

    // Represents HTML5 "insertion mode"
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#insertion-mode
    enum class InsertionMode {
        Initial,
        BeforeHTML,
        BeforeHead,
        InHead,
        InHeadNoscript,
        AfterHead,
#if ENABLE(TEMPLATE_ELEMENT)
        TemplateContents,
#endif
        InBody,
        Text,
        InTable,
        InTableText,
        InCaption,
        InColumnGroup,
        InTableBody,
        InRow,
        InCell,
        InSelect,
        InSelectInTable,
        AfterBody,
        InFrameset,
        AfterFrameset,
        AfterAfterBody,
        AfterAfterFrameset,
    };

    bool isParsingTemplateContents() const;
    bool isParsingFragmentOrTemplateContents() const;

#if ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(IOS)
    void insertPhoneNumberLink(const String&);
    void linkifyPhoneNumbers(const String&);
#endif

    void processToken(AtomicHTMLToken&);

    void processDoctypeToken(AtomicHTMLToken&);
    void processStartTag(AtomicHTMLToken&);
    void processEndTag(AtomicHTMLToken&);
    void processComment(AtomicHTMLToken&);
    void processCharacter(AtomicHTMLToken&);
    void processEndOfFile(AtomicHTMLToken&);

    bool processStartTagForInHead(AtomicHTMLToken&);
    void processStartTagForInBody(AtomicHTMLToken&);
    void processStartTagForInTable(AtomicHTMLToken&);
    void processEndTagForInBody(AtomicHTMLToken&);
    void processEndTagForInTable(AtomicHTMLToken&);
    void processEndTagForInTableBody(AtomicHTMLToken&);
    void processEndTagForInRow(AtomicHTMLToken&);
    void processEndTagForInCell(AtomicHTMLToken&);

    void processIsindexStartTagForInBody(AtomicHTMLToken&);
    void processHtmlStartTagForInBody(AtomicHTMLToken&);
    bool processBodyEndTagForInBody(AtomicHTMLToken&);
    bool processTableEndTagForInTable();
    bool processCaptionEndTagForInCaption();
    bool processColgroupEndTagForInColumnGroup();
    bool processTrEndTagForInRow();

    void processAnyOtherEndTagForInBody(AtomicHTMLToken&);

    void processCharacterBuffer(ExternalCharacterTokenBuffer&);
    inline void processCharacterBufferForInBody(ExternalCharacterTokenBuffer&);

    void processFakeStartTag(const QualifiedName&, Vector<Attribute>&& attributes = Vector<Attribute>());
    void processFakeEndTag(const QualifiedName&);
    void processFakeEndTag(const AtomicString&);
    void processFakeCharacters(const String&);
    void processFakePEndTagIfPInButtonScope();

    void processGenericRCDATAStartTag(AtomicHTMLToken&);
    void processGenericRawTextStartTag(AtomicHTMLToken&);
    void processScriptStartTag(AtomicHTMLToken&);

    // Default processing for the different insertion modes.
    void defaultForInitial();
    void defaultForBeforeHTML();
    void defaultForBeforeHead();
    void defaultForInHead();
    void defaultForInHeadNoscript();
    void defaultForAfterHead();
    void defaultForInTableText();

    bool shouldProcessTokenInForeignContent(AtomicHTMLToken&);
    void processTokenInForeignContent(AtomicHTMLToken&);
    
    HTMLStackItem& adjustedCurrentStackItem() const;

    Vector<Attribute> attributesForIsindexInput(AtomicHTMLToken&);

    void callTheAdoptionAgency(AtomicHTMLToken&);

    void closeTheCell();

    template <bool shouldClose(const HTMLStackItem&)> void processCloseWhenNestedTag(AtomicHTMLToken&);

    void parseError(AtomicHTMLToken&);

    void resetInsertionModeAppropriately();

#if ENABLE(TEMPLATE_ELEMENT)
    void processTemplateStartTag(AtomicHTMLToken&);
    bool processTemplateEndTag(AtomicHTMLToken&);
    bool processEndOfFileForInTemplateContents(AtomicHTMLToken&);
#endif

    class FragmentParsingContext {
    public:
        FragmentParsingContext();
        FragmentParsingContext(DocumentFragment&, Element& contextElement);

        DocumentFragment* fragment() const;
        Element& contextElement() const;
        HTMLStackItem& contextElementStackItem() const;

    private:
        DocumentFragment* m_fragment { nullptr };
        RefPtr<HTMLStackItem> m_contextElementStackItem;
    };

    HTMLDocumentParser& m_parser;
    const HTMLParserOptions m_options;
    const FragmentParsingContext m_fragmentContext;

    HTMLConstructionSite m_tree;

    // https://html.spec.whatwg.org/multipage/syntax.html#the-insertion-mode
    InsertionMode m_insertionMode { InsertionMode::Initial };
    InsertionMode m_originalInsertionMode { InsertionMode::Initial };
#if ENABLE(TEMPLATE_ELEMENT)
    Vector<InsertionMode, 1> m_templateInsertionModes;
#endif

    // https://html.spec.whatwg.org/multipage/syntax.html#concept-pending-table-char-tokens
    StringBuilder m_pendingTableCharacters;

    RefPtr<Element> m_scriptToProcess; // <script> tag which needs processing before resuming the parser.
    TextPosition m_scriptToProcessStartPosition; // Starting line number of the script tag needing processing.

    bool m_shouldSkipLeadingNewline { false };

    bool m_framesetOk { true };

#if !ASSERT_DISABLED
    bool m_destroyed { false };
    bool m_destructionProhibited { true };
#endif
};

inline HTMLTreeBuilder::~HTMLTreeBuilder()
{
#if !ASSERT_DISABLED
    ASSERT(!m_destroyed);
    ASSERT(!m_destructionProhibited);
    m_destroyed = true;
#endif
}

inline void HTMLTreeBuilder::setShouldSkipLeadingNewline(bool shouldSkip)
{
    ASSERT(!m_destroyed);
    m_shouldSkipLeadingNewline = shouldSkip;
}

inline bool HTMLTreeBuilder::isParsingFragment() const
{
    ASSERT(!m_destroyed);
    return !!m_fragmentContext.fragment();
}

inline bool HTMLTreeBuilder::hasParserBlockingScript() const
{
    ASSERT(!m_destroyed);
    return !!m_scriptToProcess;
}

inline DocumentFragment* HTMLTreeBuilder::FragmentParsingContext::fragment() const
{
    return m_fragment;
}

}

#endif
