/*
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef XMLDocumentParser_h
#define XMLDocumentParser_h

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "FragmentScriptingPermission.h"
#include "ScriptableDocumentParser.h"
#include "SegmentedString.h"
#include "XMLErrors.h"
#include <wtf/HashMap.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/CString.h>

#include <libxml/tree.h>
#include <libxml/xmlstring.h>

namespace WebCore {

class ContainerNode;
class CachedScript;
class CachedResourceLoader;
class DocumentFragment;
class Document;
class Element;
class FrameView;
class PendingCallbacks;
class Text;

    class XMLParserContext : public RefCounted<XMLParserContext> {
    public:
        static RefPtr<XMLParserContext> createMemoryParser(xmlSAXHandlerPtr, void* userData, const CString& chunk);
        static Ref<XMLParserContext> createStringParser(xmlSAXHandlerPtr, void* userData);
        ~XMLParserContext();
        xmlParserCtxtPtr context() const { return m_context; }

    private:
        XMLParserContext(xmlParserCtxtPtr context)
            : m_context(context)
        {
        }
        xmlParserCtxtPtr m_context;
    };

    class XMLDocumentParser final : public ScriptableDocumentParser, public CachedResourceClient {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static Ref<XMLDocumentParser> create(Document& document, FrameView* view)
        {
            return adoptRef(*new XMLDocumentParser(document, view));
        }
        static Ref<XMLDocumentParser> create(DocumentFragment& fragment, Element* element, ParserContentPolicy parserContentPolicy)
        {
            return adoptRef(*new XMLDocumentParser(fragment, element, parserContentPolicy));
        }

        ~XMLDocumentParser();

        // Exposed for callbacks:
        void handleError(XMLErrors::ErrorType, const char* message, TextPosition);

        void setIsXHTMLDocument(bool isXHTML) { m_isXHTMLDocument = isXHTML; }
        bool isXHTMLDocument() const { return m_isXHTMLDocument; }

        static bool parseDocumentFragment(const String&, DocumentFragment&, Element* parent = nullptr, ParserContentPolicy = AllowScriptingContent);

        // Used by the XMLHttpRequest to check if the responseXML was well formed.
        virtual bool wellFormed() const override { return !m_sawError; }

        static bool supportsXMLVersion(const String&);

    private:
        XMLDocumentParser(Document&, FrameView* = nullptr);
        XMLDocumentParser(DocumentFragment&, Element*, ParserContentPolicy);

        // From DocumentParser
        virtual void insert(const SegmentedString&) override;
        virtual void append(PassRefPtr<StringImpl>) override;
        virtual void finish() override;
        virtual bool isWaitingForScripts() const override;
        virtual void stopParsing() override;
        virtual void detach() override;

        virtual TextPosition textPosition() const override;
        virtual bool shouldAssociateConsoleMessagesWithTextPosition() const override;

        // from CachedResourceClient
        virtual void notifyFinished(CachedResource*) override;

        void end();

        void pauseParsing();
        void resumeParsing();

        bool appendFragmentSource(const String&);

    public:
        // callbacks from parser SAX
        void error(XMLErrors::ErrorType, const char* message, va_list args) WTF_ATTRIBUTE_PRINTF(3, 0);
        void startElementNs(const xmlChar* xmlLocalName, const xmlChar* xmlPrefix, const xmlChar* xmlURI, int nb_namespaces,
                            const xmlChar** namespaces, int nb_attributes, int nb_defaulted, const xmlChar** libxmlAttributes);
        void endElementNs();
        void characters(const xmlChar* s, int len);
        void processingInstruction(const xmlChar* target, const xmlChar* data);
        void cdataBlock(const xmlChar* s, int len);
        void comment(const xmlChar* s);
        void startDocument(const xmlChar* version, const xmlChar* encoding, int standalone);
        void internalSubset(const xmlChar* name, const xmlChar* externalID, const xmlChar* systemID);
        void endDocument();

        bool isParsingEntityDeclaration() const { return m_isParsingEntityDeclaration; }
        void setIsParsingEntityDeclaration(bool value) { m_isParsingEntityDeclaration = value; }

        int depthTriggeringEntityExpansion() const { return m_depthTriggeringEntityExpansion; }
        void setDepthTriggeringEntityExpansion(int depth) { m_depthTriggeringEntityExpansion = depth; }

    private:
        void initializeParserContext(const CString& chunk = CString());

        void pushCurrentNode(ContainerNode*);
        void popCurrentNode();
        void clearCurrentNodeStack();

        void insertErrorMessageBlock();

        void createLeafTextNode();
        bool updateLeafTextNode();

        void doWrite(const String&);
        void doEnd();

        FrameView* m_view;

        SegmentedString m_originalSourceForTransform;

        xmlParserCtxtPtr context() const { return m_context ? m_context->context() : nullptr; };
        RefPtr<XMLParserContext> m_context;
        std::unique_ptr<PendingCallbacks> m_pendingCallbacks;
        Vector<xmlChar> m_bufferedText;
        int m_depthTriggeringEntityExpansion;
        bool m_isParsingEntityDeclaration;

        ContainerNode* m_currentNode;
        Vector<ContainerNode*> m_currentNodeStack;

        RefPtr<Text> m_leafTextNode;

        bool m_sawError;
        bool m_sawCSS;
        bool m_sawXSLTransform;
        bool m_sawFirstElement;
        bool m_isXHTMLDocument;
        bool m_parserPaused;
        bool m_requestingScript;
        bool m_finishCalled;

        std::unique_ptr<XMLErrors> m_xmlErrors;

        CachedResourceHandle<CachedScript> m_pendingScript;
        RefPtr<Element> m_scriptElement;
        TextPosition m_scriptStartPosition;

        bool m_parsingFragment;
        AtomicString m_defaultNamespaceURI;

        typedef HashMap<AtomicString, AtomicString> PrefixForNamespaceMap;
        PrefixForNamespaceMap m_prefixToNamespaceMap;
        SegmentedString m_pendingSrc;
    };

#if ENABLE(XSLT)
void* xmlDocPtrForString(CachedResourceLoader&, const String& source, const String& url);
#endif

HashMap<String, String> parseAttributes(const String&, bool& attrsOK);

} // namespace WebCore

#endif // XMLDocumentParser_h
