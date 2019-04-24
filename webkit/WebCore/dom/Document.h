/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef Document_h
#define Document_h

#include "CollectionType.h"
#include "Color.h"
#include "ContainerNode.h"
#include "DocumentEventQueue.h"
#include "DocumentStyleSheetCollection.h"
#include "DocumentTiming.h"
#include "FocusDirection.h"
#include "FontSelector.h"
#include "MediaProducer.h"
#include "MutationObserver.h"
#include "PageVisibilityState.h"
#include "PlatformEvent.h"
#include "PlatformScreen.h"
#include "ReferrerPolicy.h"
#include "Region.h"
#include "RenderPtr.h"
#include "ScriptExecutionContext.h"
#include "StringWithDirection.h"
#include "StyleResolveTree.h"
#include "Timer.h"
#include "TreeScope.h"
#include "UserActionElementSet.h"
#include "ViewportArguments.h"
#if !PLATFORM(WKC)
#include <chrono>
#else
#include <wkcchrono.h>
#endif
#include <memory>
#include <wtf/Deque.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/WeakPtr.h>

namespace JSC {
class ExecState;
#if ENABLE(WEB_REPLAY)
class InputCursor;
#endif
}

namespace WebCore {

class AXObjectCache;
class Attr;
class CDATASection;
class CSSFontSelector;
class CSSStyleDeclaration;
class CSSStyleSheet;
class CachedCSSStyleSheet;
class CachedResourceLoader;
class CachedScript;
class CanvasRenderingContext;
class CharacterData;
class Comment;
class DOMImplementation;
class DOMNamedFlowCollection;
class DOMSelection;
class DOMWindow;
class DOMWrapperWorld;
class Database;
class DatabaseThread;
class DocumentFragment;
class DocumentLoader;
class DocumentMarkerController;
class DocumentParser;
class DocumentSharedObjectPool;
class DocumentType;
class Element;
class EntityReference;
class Event;
class EventListener;
class FloatRect;
class FloatQuad;
class FormController;
class Frame;
class FrameView;
class HTMLAllCollection;
class HTMLBodyElement;
class HTMLCanvasElement;
class HTMLCollection;
class HTMLDocument;
class HTMLElement;
class HTMLFrameOwnerElement;
class HTMLHeadElement;
class HTMLIFrameElement;
class HTMLImageElement;
class HTMLMapElement;
class HTMLMediaElement;
class HTMLNameCollection;
class HTMLPictureElement;
class HTMLScriptElement;
class HitTestRequest;
class HitTestResult;
class IntPoint;
class LayoutPoint;
class LayoutRect;
class LiveNodeList;
class JSNode;
class Locale;
class MediaCanStartListener;
class MediaPlaybackTarget;
class MediaPlaybackTargetClient;
class MediaQueryList;
class MediaQueryMatcher;
class MouseEventWithHitTestResults;
class NamedFlowCollection;
class NodeFilter;
class NodeIterator;
class Page;
class PlatformMouseEvent;
class ProcessingInstruction;
class QualifiedName;
class Range;
class RegisteredEventListener;
class RenderView;
class RenderFullScreen;
class ScriptableDocumentParser;
class ScriptElementData;
class ScriptRunner;
class SecurityOrigin;
class SelectorQuery;
class SelectorQueryCache;
class SerializedScriptValue;
class SegmentedString;
class Settings;
class StyleResolver;
class StyleSheet;
class StyleSheetContents;
class StyleSheetList;
class SVGDocumentExtensions;
class Text;
class TextResourceDecoder;
class TreeWalker;
class VisitedLinkState;
class WebKitNamedFlow;
class XMLHttpRequest;
class XPathEvaluator;
class XPathExpression;
class XPathNSResolver;
class XPathResult;

enum class ShouldOpenExternalURLsPolicy;

#if ENABLE(XSLT)
class TransformSource;
#endif

#if ENABLE(DASHBOARD_SUPPORT)
struct AnnotatedRegionValue;
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
#include <WebKitAdditions/DocumentIOSForward.h>
#endif

#if ENABLE(TOUCH_EVENTS) || ENABLE(IOS_TOUCH_EVENTS)
class Touch;
class TouchList;
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
class RequestAnimationFrameCallback;
class ScriptedAnimationController;
#endif

#if ENABLE(TEXT_AUTOSIZING)
class TextAutosizer;
#endif

#if ENABLE(CSP_NEXT)
class DOMSecurityPolicy;
#endif

#if ENABLE(FONT_LOAD_EVENTS)
class FontLoader;
#endif

typedef int ExceptionCode;

#if PLATFORM(IOS)
class DeviceMotionClient;
class DeviceMotionController;
class DeviceOrientationClient;
class DeviceOrientationController;
#endif

#if ENABLE(IOS_TEXT_AUTOSIZING)
struct TextAutoSizingHash;
class TextAutoSizingKey;
class TextAutoSizingValue;

struct TextAutoSizingTraits : WTF::GenericHashTraits<TextAutoSizingKey> {
    static const bool emptyValueIsZero = true;
    static void constructDeletedValue(TextAutoSizingKey& slot);
    static bool isDeletedValue(const TextAutoSizingKey& value);
};
#endif

#if ENABLE(MEDIA_SESSION)
class MediaSession;
#endif

enum PageshowEventPersistence {
    PageshowEventNotPersisted = 0,
    PageshowEventPersisted = 1
};

enum StyleResolverUpdateFlag { RecalcStyleImmediately, DeferRecalcStyle, RecalcStyleIfNeeded, DeferRecalcStyleIfNeeded };

enum NodeListInvalidationType {
    DoNotInvalidateOnAttributeChanges = 0,
    InvalidateOnClassAttrChange,
    InvalidateOnIdNameAttrChange,
    InvalidateOnNameAttrChange,
    InvalidateOnForAttrChange,
    InvalidateForFormControls,
    InvalidateOnHRefAttrChange,
    InvalidateOnAnyAttrChange,
};
const int numNodeListInvalidationTypes = InvalidateOnAnyAttrChange + 1;

enum class EventHandlerRemoval { One, All };
typedef HashCountedSet<Node*> EventTargetSet;

enum DocumentClass {
    DefaultDocumentClass = 0,
    HTMLDocumentClass = 1,
    XHTMLDocumentClass = 1 << 1,
    ImageDocumentClass = 1 << 2,
    PluginDocumentClass = 1 << 3,
    MediaDocumentClass = 1 << 4,
    SVGDocumentClass = 1 << 5,
    TextDocumentClass = 1 << 6
};

typedef unsigned char DocumentClassFlags;

enum class DocumentCompatibilityMode : unsigned char {
    NoQuirksMode = 1,
    QuirksMode = 1 << 1,
    LimitedQuirksMode = 1 << 2
};

enum DimensionsCheck { WidthDimensionsCheck = 1 << 0, HeightDimensionsCheck = 1 << 1, AllDimensionsCheck = 1 << 2 };

enum class HttpEquivPolicy {
    Enabled,
    DisabledBySettings,
    DisabledByContentDispositionAttachmentSandbox
};

class Document : public ContainerNode, public TreeScope, public ScriptExecutionContext, public FontSelectorClient {
public:
    static Ref<Document> create(Frame* frame, const URL& url)
    {
        return adoptRef(*new Document(frame, url));
    }
    static Ref<Document> createXHTML(Frame* frame, const URL& url)
    {
        return adoptRef(*new Document(frame, url, XHTMLDocumentClass));
    }
    static Ref<Document> createNonRenderedPlaceholder(Frame* frame, const URL& url)
    {
        return adoptRef(*new Document(frame, url, DefaultDocumentClass, NonRenderedPlaceholder));
    }
    static Ref<Document> create(ScriptExecutionContext&);

    virtual ~Document();

    // Nodes belonging to this document increase referencingNodeCount -
    // these are enough to keep the document from being destroyed, but
    // not enough to keep it from removing its children. This allows a
    // node that outlives its document to still have a valid document
    // pointer without introducing reference cycles.
    void incrementReferencingNodeCount()
    {
        ASSERT(!m_deletionHasBegun);
        ++m_referencingNodeCount;
    }

    void decrementReferencingNodeCount()
    {
        ASSERT(!m_deletionHasBegun || !m_referencingNodeCount);
        --m_referencingNodeCount;
        if (!m_referencingNodeCount && !refCount()) {
#if !ASSERT_DISABLED
            m_deletionHasBegun = true;
#endif
            delete this;
        }
    }

    unsigned referencingNodeCount() const { return m_referencingNodeCount; }

    void removedLastRef();

    WEBCORE_EXPORT static HashSet<Document*>& allDocuments();

    MediaQueryMatcher& mediaQueryMatcher();

    using ContainerNode::ref;
    using ContainerNode::deref;

    virtual bool canContainRangeEndPoint() const override final { return true; }

    Element* getElementByAccessKey(const String& key);
    void invalidateAccessKeyMap();

    void addImageElementByLowercasedUsemap(const AtomicStringImpl&, HTMLImageElement&);
    void removeImageElementByLowercasedUsemap(const AtomicStringImpl&, HTMLImageElement&);
    HTMLImageElement* imageElementByLowercasedUsemap(const AtomicStringImpl&) const;

    SelectorQuery* selectorQueryForString(const String&, ExceptionCode&);
    void clearSelectorQueryCache();

    // DOM methods & attributes for Document

    void setViewportArguments(const ViewportArguments& viewportArguments) { m_viewportArguments = viewportArguments; }
    ViewportArguments viewportArguments() const { return m_viewportArguments; }
#ifndef NDEBUG
    bool didDispatchViewportPropertiesChanged() const { return m_didDispatchViewportPropertiesChanged; }
#endif

    void setReferrerPolicy(ReferrerPolicy referrerPolicy) { m_referrerPolicy = referrerPolicy; }
    ReferrerPolicy referrerPolicy() const { return m_referrerPolicy; }

    DocumentType* doctype() const;

    DOMImplementation& implementation();
    
    Element* documentElement() const
    {
        return m_documentElement.get();
    }
    static ptrdiff_t documentElementMemoryOffset() { return OBJECT_OFFSETOF(Document, m_documentElement); }

    Element* activeElement();
    bool hasFocus() const;

    bool hasManifest() const;
    
    virtual RefPtr<Element> createElement(const AtomicString& tagName, ExceptionCode&);
    WEBCORE_EXPORT Ref<DocumentFragment> createDocumentFragment();
    WEBCORE_EXPORT Ref<Text> createTextNode(const String& data);
    Ref<Comment> createComment(const String& data);
    RefPtr<CDATASection> createCDATASection(const String& data, ExceptionCode&);
    RefPtr<ProcessingInstruction> createProcessingInstruction(const String& target, const String& data, ExceptionCode&);
    RefPtr<Attr> createAttribute(const String& name, ExceptionCode&);
    RefPtr<Attr> createAttributeNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode&, bool shouldIgnoreNamespaceChecks = false);
    RefPtr<EntityReference> createEntityReference(const String& name, ExceptionCode&);
    RefPtr<Node> importNode(Node* importedNode, ExceptionCode& ec) { return importNode(importedNode, true, ec); }
    RefPtr<Node> importNode(Node* importedNode, bool deep, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<Element> createElementNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode&);
    WEBCORE_EXPORT Ref<Element> createElement(const QualifiedName&, bool createdByParser);

    bool cssRegionsEnabled() const;
    bool cssCompositingEnabled() const;
#if ENABLE(CSS_REGIONS)
    RefPtr<DOMNamedFlowCollection> webkitGetNamedFlows();
#endif

    NamedFlowCollection& namedFlows();

    Element* elementFromPoint(int x, int y) { return elementFromPoint(LayoutPoint(x, y)); }
    Element* elementFromPoint(const LayoutPoint& clientPoint);

    RefPtr<Range> caretRangeFromPoint(int x, int y);
    RefPtr<Range> caretRangeFromPoint(const LayoutPoint& clientPoint);

    Element* scrollingElement();

    String readyState() const;

    String defaultCharset() const;

    String inputEncoding() const { return Document::encoding(); }
    String charset() const { return Document::encoding(); }
    String characterSet() const { return Document::encoding(); }

    AtomicString encoding() const;

    void setCharset(const String&);

    void setContent(const String&);

    String suggestedMIMEType() const;

    void overrideMIMEType(const String&);
    String contentType() const;

    String contentLanguage() const { return m_contentLanguage; }
    void setContentLanguage(const String&);

    String xmlEncoding() const { return m_xmlEncoding; }
    String xmlVersion() const { return m_xmlVersion; }
    enum StandaloneStatus { StandaloneUnspecified, Standalone, NotStandalone };
    bool xmlStandalone() const { return m_xmlStandalone == Standalone; }
    StandaloneStatus xmlStandaloneStatus() const { return static_cast<StandaloneStatus>(m_xmlStandalone); }
    bool hasXMLDeclaration() const { return m_hasXMLDeclaration; }

    void setXMLEncoding(const String& encoding) { m_xmlEncoding = encoding; } // read-only property, only to be set from XMLDocumentParser
    void setXMLVersion(const String&, ExceptionCode&);
    void setXMLStandalone(bool, ExceptionCode&);
    void setHasXMLDeclaration(bool hasXMLDeclaration) { m_hasXMLDeclaration = hasXMLDeclaration ? 1 : 0; }

    String documentURI() const { return m_documentURI; }
    void setDocumentURI(const String&);

    virtual URL baseURI() const override final;

#if ENABLE(WEB_REPLAY)
    JSC::InputCursor& inputCursor() const { return *m_inputCursor; }
    void setInputCursor(PassRefPtr<JSC::InputCursor>);
#endif

    void visibilityStateChanged();
    String visibilityState() const;
    bool hidden() const;

    void setTimerThrottlingEnabled(bool);
    bool isTimerThrottlingEnabled() const { return m_isTimerThrottlingEnabled; }

#if ENABLE(CSP_NEXT)
    DOMSecurityPolicy& securityPolicy();
#endif

    RefPtr<Node> adoptNode(PassRefPtr<Node> source, ExceptionCode&);

    Ref<HTMLCollection> images();
    Ref<HTMLCollection> embeds();
    Ref<HTMLCollection> plugins(); // an alias for embeds() required for the JS DOM bindings.
    Ref<HTMLCollection> applets();
    Ref<HTMLCollection> links();
    Ref<HTMLCollection> forms();
    Ref<HTMLCollection> anchors();
    Ref<HTMLCollection> scripts();
    Ref<HTMLCollection> all();

    Ref<HTMLCollection> windowNamedItems(const AtomicString& name);
    Ref<HTMLCollection> documentNamedItems(const AtomicString& name);

    // Other methods (not part of DOM)
    bool isSynthesized() const { return m_isSynthesized; }
    bool isHTMLDocument() const { return m_documentClasses & HTMLDocumentClass; }
    bool isXHTMLDocument() const { return m_documentClasses & XHTMLDocumentClass; }
    bool isImageDocument() const { return m_documentClasses & ImageDocumentClass; }
    bool isSVGDocument() const { return m_documentClasses & SVGDocumentClass; }
    bool isPluginDocument() const { return m_documentClasses & PluginDocumentClass; }
    bool isMediaDocument() const { return m_documentClasses & MediaDocumentClass; }
    bool isTextDocument() const { return m_documentClasses & TextDocumentClass; }
    bool hasSVGRootNode() const;
    virtual bool isFrameSet() const { return false; }

    static ptrdiff_t documentClassesMemoryOffset() { return OBJECT_OFFSETOF(Document, m_documentClasses); }
    static uint32_t isHTMLDocumentClassFlag() { return HTMLDocumentClass; }

    bool isSrcdocDocument() const { return m_isSrcdocDocument; }

    StyleResolver* styleResolverIfExists() const { return m_styleResolver.get(); }

    bool sawElementsInKnownNamespaces() const { return m_sawElementsInKnownNamespaces; }

    StyleResolver& ensureStyleResolver()
    { 
        if (!m_styleResolver)
            createStyleResolver();
        return *m_styleResolver;
    }

    CSSFontSelector& fontSelector();

    void notifyRemovePendingSheetIfNeeded();

    bool haveStylesheetsLoaded() const;

    // This is a DOM function.
    StyleSheetList& styleSheets();

    DocumentStyleSheetCollection& styleSheetCollection() { return m_styleSheetCollection; }

    bool gotoAnchorNeededAfterStylesheetsLoad() { return m_gotoAnchorNeededAfterStylesheetsLoad; }
    void setGotoAnchorNeededAfterStylesheetsLoad(bool b) { m_gotoAnchorNeededAfterStylesheetsLoad = b; }

    /**
     * Called when one or more stylesheets in the document may have been added, removed or changed.
     *
     * Creates a new style resolver and assign it to this document. This is done by iterating through all nodes in
     * document (or those before <BODY> in a HTML document), searching for stylesheets. Stylesheets can be contained in
     * <LINK>, <STYLE> or <BODY> elements, as well as processing instructions (XML documents only). A list is
     * constructed from these which is used to create the a new style selector which collates all of the stylesheets
     * found and is used to calculate the derived styles for all rendering objects.
     */
    WEBCORE_EXPORT void styleResolverChanged(StyleResolverUpdateFlag);

    void scheduleOptimizedStyleSheetUpdate();

    void evaluateMediaQueryList();

    FormController& formController();
    Vector<String> formElementsState() const;
    void setStateForNewFormElements(const Vector<String>&);

    WEBCORE_EXPORT FrameView* view() const; // can be NULL
    Frame* frame() const { return m_frame; } // can be NULL
    WEBCORE_EXPORT Page* page() const; // can be NULL
    WEBCORE_EXPORT Settings* settings() const; // can be NULL

    float deviceScaleFactor() const;

    WEBCORE_EXPORT Ref<Range> createRange();

    RefPtr<NodeIterator> createNodeIterator(Node* root, unsigned whatToShow,
        PassRefPtr<NodeFilter>, bool expandEntityReferences, ExceptionCode&);

    RefPtr<TreeWalker> createTreeWalker(Node* root, unsigned whatToShow,
        PassRefPtr<NodeFilter>, bool expandEntityReferences, ExceptionCode&);

    // Special support for editing
    Ref<CSSStyleDeclaration> createCSSStyleDeclaration();
    Ref<Text> createEditingTextNode(const String&);

    void recalcStyle(Style::Change = Style::NoChange);
    WEBCORE_EXPORT void updateStyleIfNeeded();
    bool needsStyleRecalc() const { return !inPageCache() && (m_pendingStyleRecalcShouldForce || childNeedsStyleRecalc() || m_optimizedStyleSheetUpdateTimer.isActive()); }

    WEBCORE_EXPORT void updateLayout();
    
    // updateLayoutIgnorePendingStylesheets() forces layout even if we are waiting for pending stylesheet loads,
    // so calling this may cause a flash of unstyled content (FOUC).
    enum class RunPostLayoutTasks {
        Asynchronously,
        Synchronously,
    };
    WEBCORE_EXPORT void updateLayoutIgnorePendingStylesheets(RunPostLayoutTasks = RunPostLayoutTasks::Asynchronously);

    Ref<RenderStyle> styleForElementIgnoringPendingStylesheets(Element*);

    // Returns true if page box (margin boxes and page borders) is visible.
    WEBCORE_EXPORT bool isPageBoxVisible(int pageIndex);

    // Returns the preferred page size and margins in pixels, assuming 96
    // pixels per inch. pageSize, marginTop, marginRight, marginBottom,
    // marginLeft must be initialized to the default values that are used if
    // auto is specified.
    WEBCORE_EXPORT void pageSizeAndMarginsInPixels(int pageIndex, IntSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft);

    CachedResourceLoader& cachedResourceLoader() { return m_cachedResourceLoader; }

    void didBecomeCurrentDocumentInFrame();
    void destroyRenderTree();
    void disconnectFromFrame();
    void prepareForDestruction();

    // Override ScriptExecutionContext methods to do additional work
    virtual void suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension) override final;
    virtual void resumeActiveDOMObjects(ActiveDOMObject::ReasonForSuspension) override final;
    virtual void stopActiveDOMObjects() override final;

    RenderView* renderView() const { return m_renderView.get(); }

    bool renderTreeBeingDestroyed() const { return m_renderTreeBeingDestroyed; }
    bool hasLivingRenderTree() const { return renderView() && !renderTreeBeingDestroyed(); }
    
    bool updateLayoutIfDimensionsOutOfDate(Element&, DimensionsCheck = AllDimensionsCheck);
    
    AXObjectCache* existingAXObjectCache() const;
    WEBCORE_EXPORT AXObjectCache* axObjectCache() const;
    void clearAXObjectCache();

    // to get visually ordered hebrew and arabic pages right
    void setVisuallyOrdered();
    bool visuallyOrdered() const { return m_visuallyOrdered; }
    
    WEBCORE_EXPORT DocumentLoader* loader() const;

    void open(Document* ownerDocument = 0);
    void implicitOpen();

    // close() is the DOM API document.close()
    void close();
    // In some situations (see the code), we ignore document.close().
    // explicitClose() bypass these checks and actually tries to close the
    // input stream.
    void explicitClose();
    // implicitClose() actually does the work of closing the input stream.
    void implicitClose();

    void cancelParsing();

    void write(const SegmentedString& text, Document* ownerDocument = 0);
    void write(const String& text, Document* ownerDocument = 0);
    void writeln(const String& text, Document* ownerDocument = 0);

    bool wellFormed() const { return m_wellFormed; }

    virtual const URL& url() const override final { return m_url; }
    void setURL(const URL&);

    // To understand how these concepts relate to one another, please see the
    // comments surrounding their declaration.
    const URL& baseURL() const { return m_baseURL; }
    void setBaseURLOverride(const URL&);
    const URL& baseURLOverride() const { return m_baseURLOverride; }
    const URL& baseElementURL() const { return m_baseElementURL; }
    const String& baseTarget() const { return m_baseTarget; }
    void processBaseElement();

    WEBCORE_EXPORT virtual URL completeURL(const String&) const override final;
    URL completeURL(const String&, const URL& baseURLOverride) const;

    virtual String userAgent(const URL&) const override final;

    virtual void disableEval(const String& errorMessage) override final;

    bool canNavigate(Frame* targetFrame);

    CSSStyleSheet& elementSheet();
    bool usesStyleBasedEditability() const;
    
    virtual Ref<DocumentParser> createParser();
    DocumentParser* parser() const { return m_parser.get(); }
    ScriptableDocumentParser* scriptableDocumentParser() const;
    
    bool printing() const { return m_printing; }
    void setPrinting(bool p) { m_printing = p; }

    bool paginatedForScreen() const { return m_paginatedForScreen; }
    void setPaginatedForScreen(bool p) { m_paginatedForScreen = p; }
    
    bool paginated() const { return printing() || paginatedForScreen(); }

    void setCompatibilityMode(DocumentCompatibilityMode);
    void lockCompatibilityMode() { m_compatibilityModeLocked = true; }
    static ptrdiff_t compatibilityModeMemoryOffset() { return OBJECT_OFFSETOF(Document, m_compatibilityMode); }

    String compatMode() const;

    bool inQuirksMode() const { return m_compatibilityMode == DocumentCompatibilityMode::QuirksMode; }
    bool inLimitedQuirksMode() const { return m_compatibilityMode == DocumentCompatibilityMode::LimitedQuirksMode; }
    bool inNoQuirksMode() const { return m_compatibilityMode == DocumentCompatibilityMode::NoQuirksMode; }

    enum ReadyState {
        Loading,
        Interactive,
        Complete
    };
    void setReadyState(ReadyState);
    void setParsing(bool);
    bool parsing() const { return m_bParsing; }
    std::chrono::milliseconds minimumLayoutDelay();

    bool shouldScheduleLayout();
    bool isLayoutTimerActive();
    std::chrono::milliseconds elapsedTime() const;
    
    void setTextColor(const Color& color) { m_textColor = color; }
    Color textColor() const { return m_textColor; }

    const Color& linkColor() const { return m_linkColor; }
    const Color& visitedLinkColor() const { return m_visitedLinkColor; }
    const Color& activeLinkColor() const { return m_activeLinkColor; }
    void setLinkColor(const Color& c) { m_linkColor = c; }
    void setVisitedLinkColor(const Color& c) { m_visitedLinkColor = c; }
    void setActiveLinkColor(const Color& c) { m_activeLinkColor = c; }
    void resetLinkColor();
    void resetVisitedLinkColor();
    void resetActiveLinkColor();
    VisitedLinkState& visitedLinkState() const { return *m_visitedLinkState; }

    MouseEventWithHitTestResults prepareMouseEvent(const HitTestRequest&, const LayoutPoint&, const PlatformMouseEvent&);

    /* Newly proposed CSS3 mechanism for selecting alternate
       stylesheets using the DOM. May be subject to change as
       spec matures. - dwh
    */
    String preferredStylesheetSet() const;
    String selectedStylesheetSet() const;
    void setSelectedStylesheetSet(const String&);

    enum class FocusRemovalEventsMode { Dispatch, DoNotDispatch };
    WEBCORE_EXPORT bool setFocusedElement(PassRefPtr<Element>, FocusDirection = FocusDirectionNone,
        FocusRemovalEventsMode = FocusRemovalEventsMode::Dispatch);
    Element* focusedElement() const { return m_focusedElement.get(); }
    UserActionElementSet& userActionElements()  { return m_userActionElements; }
    const UserActionElementSet& userActionElements() const { return m_userActionElements; }

    void removeFocusedNodeOfSubtree(Node*, bool amongChildrenOnly = false);
    void hoveredElementDidDetach(Element*);
    void elementInActiveChainDidDetach(Element*);

    void updateHoverActiveState(const HitTestRequest&, Element*, StyleResolverUpdateFlag = RecalcStyleIfNeeded);

    // Updates for :target (CSS3 selector).
    void setCSSTarget(Element*);
    Element* cssTarget() const { return m_cssTarget; }
    static ptrdiff_t cssTargetMemoryOffset() { return OBJECT_OFFSETOF(Document, m_cssTarget); }

    WEBCORE_EXPORT void scheduleForcedStyleRecalc();
    void scheduleStyleRecalc();
    void unscheduleStyleRecalc();
    bool hasPendingStyleRecalc() const;
    bool hasPendingForcedStyleRecalc() const;
    void styleRecalcTimerFired();
    void optimizedStyleSheetUpdateTimerFired();

    void registerNodeListForInvalidation(LiveNodeList&);
    void unregisterNodeListForInvalidation(LiveNodeList&);
    void registerCollection(HTMLCollection&);
    void unregisterCollection(HTMLCollection&);
    void collectionCachedIdNameMap(const HTMLCollection&);
    void collectionWillClearIdNameMap(const HTMLCollection&);
    bool shouldInvalidateNodeListAndCollectionCaches(const QualifiedName* attrName = nullptr) const;
    void invalidateNodeListAndCollectionCaches(const QualifiedName* attrName);

    void attachNodeIterator(NodeIterator*);
    void detachNodeIterator(NodeIterator*);
    void moveNodeIteratorsToNewDocument(Node*, Document*);

    void attachRange(Range*);
    void detachRange(Range*);

    void updateRangesAfterChildrenChanged(ContainerNode&);
    // nodeChildrenWillBeRemoved is used when removing all node children at once.
    void nodeChildrenWillBeRemoved(ContainerNode&);
    // nodeWillBeRemoved is only safe when removing one node at a time.
    void nodeWillBeRemoved(Node&);
    enum class AcceptChildOperation { Replace, InsertOrAdd };
    bool canAcceptChild(const Node& newChild, const Node* refChild, AcceptChildOperation) const;

    void textInserted(Node*, unsigned offset, unsigned length);
    void textRemoved(Node*, unsigned offset, unsigned length);
    void textNodesMerged(Text* oldNode, unsigned offset);
    void textNodeSplit(Text* oldNode);

    void createDOMWindow();
    void takeDOMWindowFrom(Document*);

    DOMWindow* domWindow() const { return m_domWindow.get(); }
    // In DOM Level 2, the Document's DOMWindow is called the defaultView.
    DOMWindow* defaultView() const { return domWindow(); } 

    // Helper functions for forwarding DOMWindow event related tasks to the DOMWindow if it exists.
    void setWindowAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value);
    void setWindowAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener>);
    EventListener* getWindowAttributeEventListener(const AtomicString& eventType);
    WEBCORE_EXPORT void dispatchWindowEvent(PassRefPtr<Event>, PassRefPtr<EventTarget> = 0);
    void dispatchWindowLoadEvent();

    RefPtr<Event> createEvent(const String& eventType, ExceptionCode&);

    // keep track of what types of event listeners are registered, so we don't
    // dispatch events unnecessarily
    enum ListenerType {
        DOMSUBTREEMODIFIED_LISTENER          = 1,
        DOMNODEINSERTED_LISTENER             = 1 << 1,
        DOMNODEREMOVED_LISTENER              = 1 << 2,
        DOMNODEREMOVEDFROMDOCUMENT_LISTENER  = 1 << 3,
        DOMNODEINSERTEDINTODOCUMENT_LISTENER = 1 << 4,
        DOMCHARACTERDATAMODIFIED_LISTENER    = 1 << 5,
        OVERFLOWCHANGED_LISTENER             = 1 << 6,
        ANIMATIONEND_LISTENER                = 1 << 7,
        ANIMATIONSTART_LISTENER              = 1 << 8,
        ANIMATIONITERATION_LISTENER          = 1 << 9,
        TRANSITIONEND_LISTENER               = 1 << 10,
        BEFORELOAD_LISTENER                  = 1 << 11,
        SCROLL_LISTENER                      = 1 << 12,
        FORCEWILLBEGIN_LISTENER              = 1 << 13,
        FORCECHANGED_LISTENER                = 1 << 14,
        FORCEDOWN_LISTENER                   = 1 << 15,
        FORCEUP_LISTENER                     = 1 << 16
    };

    bool hasListenerType(ListenerType listenerType) const { return (m_listenerTypes & listenerType); }
    bool hasListenerTypeForEventType(PlatformEvent::Type) const;
    void addListenerTypeIfNeeded(const AtomicString& eventType);

    bool hasMutationObserversOfType(MutationObserver::MutationType type) const
    {
        return m_mutationObserverTypes & type;
    }
    bool hasMutationObservers() const { return m_mutationObserverTypes; }
    void addMutationObserverTypes(MutationObserverOptions types) { m_mutationObserverTypes |= types; }

    CSSStyleDeclaration* getOverrideStyle(Element*, const String& pseudoElt);

    /**
     * Handles a HTTP header equivalent set by a meta tag using <meta http-equiv="..." content="...">. This is called
     * when a meta tag is encountered during document parsing, and also when a script dynamically changes or adds a meta
     * tag. This enables scripts to use meta tags to perform refreshes and set expiry dates in addition to them being
     * specified in a HTML file.
     *
     * @param equiv The http header name (value of the meta tag's "equiv" attribute)
     * @param content The header value (value of the meta tag's "content" attribute)
     */
    void processHttpEquiv(const String& equiv, const String& content);

#if PLATFORM(IOS)
    void processFormatDetection(const String&);

    // Called when <meta name="apple-mobile-web-app-orientations"> changes.
    void processWebAppOrientations();
#endif
    
    void processViewport(const String& features, ViewportArguments::Type origin);
    void updateViewportArguments();
    void processReferrerPolicy(const String& policy);
#if PLATFORM(WKC)
    void processFocusRingVisibility(const String& value);
#endif

    // Returns the owning element in the parent document.
    // Returns 0 if this is the top level document.
    HTMLFrameOwnerElement* ownerElement() const;

    // Used by DOM bindings; no direction known.
    String title() const { return m_title.string(); }
    void setTitle(const String&);

    void setTitleElement(const StringWithDirection&, Element* titleElement);
    void removeTitle(Element* titleElement);

    String cookie(ExceptionCode&);
    void setCookie(const String&, ExceptionCode&);

    String referrer() const;

    String origin() const;

    WEBCORE_EXPORT String domain() const;
    void setDomain(const String& newDomain, ExceptionCode&);

    String lastModified() const;

    // The cookieURL is used to query the cookie database for this document's
    // cookies. For example, if the cookie URL is http://example.com, we'll
    // use the non-Secure cookies for example.com when computing
    // document.cookie.
    //
    // Q: How is the cookieURL different from the document's URL?
    // A: The two URLs are the same almost all the time.  However, if one
    //    document inherits the security context of another document, it
    //    inherits its cookieURL but not its URL.
    //
    const URL& cookieURL() const { return m_cookieURL; }
    void setCookieURL(const URL&);

    // The firstPartyForCookies is used to compute whether this document
    // appears in a "third-party" context for the purpose of third-party
    // cookie blocking.  The document is in a third-party context if the
    // cookieURL and the firstPartyForCookies are from different hosts.
    //
    // Note: Some ports (including possibly Apple's) only consider the
    //       document in a third-party context if the cookieURL and the
    //       firstPartyForCookies have a different registry-controlled
    //       domain.
    //
    const URL& firstPartyForCookies() const { return m_firstPartyForCookies; }
    void setFirstPartyForCookies(const URL& url) { m_firstPartyForCookies = url; }
    
    // The following implements the rule from HTML 4 for what valid names are.
    // To get this right for all the XML cases, we probably have to improve this or move it
    // and make it sensitive to the type of document.
    static bool isValidName(const String&);

    // The following breaks a qualified name into a prefix and a local name.
    // It also does a validity check, and returns false if the qualified name
    // is invalid.  It also sets ExceptionCode when name is invalid.
    static bool parseQualifiedName(const String& qualifiedName, String& prefix, String& localName, ExceptionCode&);

    // Checks to make sure prefix and namespace do not conflict (per DOM Core 3)
    static bool hasValidNamespaceForElements(const QualifiedName&);
    static bool hasValidNamespaceForAttributes(const QualifiedName&);

    HTMLBodyElement* body() const;
    WEBCORE_EXPORT HTMLElement* bodyOrFrameset() const;
    void setBodyOrFrameset(PassRefPtr<HTMLElement>, ExceptionCode&);

    WEBCORE_EXPORT HTMLHeadElement* head();

    DocumentMarkerController& markers() const { return *m_markers; }

    bool directionSetOnDocumentElement() const { return m_directionSetOnDocumentElement; }
    bool writingModeSetOnDocumentElement() const { return m_writingModeSetOnDocumentElement; }
    void setDirectionSetOnDocumentElement(bool b) { m_directionSetOnDocumentElement = b; }
    void setWritingModeSetOnDocumentElement(bool b) { m_writingModeSetOnDocumentElement = b; }

    bool execCommand(const String& command, bool userInterface = false, const String& value = String());
    bool queryCommandEnabled(const String& command);
    bool queryCommandIndeterm(const String& command);
    bool queryCommandState(const String& command);
    bool queryCommandSupported(const String& command);
    String queryCommandValue(const String& command);

    // designMode support
    enum InheritedBool { off = false, on = true, inherit };    
    void setDesignMode(InheritedBool value);
    InheritedBool getDesignMode() const;
    bool inDesignMode() const;

    Document* parentDocument() const;
    Document& topDocument() const;
    
    ScriptRunner* scriptRunner() { return m_scriptRunner.get(); }

    HTMLScriptElement* currentScript() const { return !m_currentScriptStack.isEmpty() ? m_currentScriptStack.last().get() : 0; }
    void pushCurrentScript(PassRefPtr<HTMLScriptElement>);
    void popCurrentScript();

#if ENABLE(XSLT)
    void applyXSLTransform(ProcessingInstruction* pi);
    RefPtr<Document> transformSourceDocument() { return m_transformSourceDocument; }
    void setTransformSourceDocument(Document* doc) { m_transformSourceDocument = doc; }

    void setTransformSource(std::unique_ptr<TransformSource>);
    TransformSource* transformSource() const { return m_transformSource.get(); }
#endif

    void incDOMTreeVersion() { m_domTreeVersion = ++s_globalTreeVersion; }
    uint64_t domTreeVersion() const { return m_domTreeVersion; }

    // XPathEvaluator methods
    RefPtr<XPathExpression> createExpression(const String& expression, XPathNSResolver*, ExceptionCode&);
    RefPtr<XPathNSResolver> createNSResolver(Node* nodeResolver);
    RefPtr<XPathResult> evaluate(const String& expression, Node* contextNode, XPathNSResolver*, unsigned short type, XPathResult*, ExceptionCode&);

    enum PendingSheetLayout { NoLayoutWithPendingSheets, DidLayoutWithPendingSheets, IgnoreLayoutWithPendingSheets };

    bool didLayoutWithPendingStylesheets() const { return m_pendingSheetLayout == DidLayoutWithPendingSheets; }

    bool hasNodesWithPlaceholderStyle() const { return m_hasNodesWithPlaceholderStyle; }
    void setHasNodesWithPlaceholderStyle() { m_hasNodesWithPlaceholderStyle = true; }

    void updateFocusAppearanceSoon(bool restorePreviousSelection);
    void cancelFocusAppearanceUpdate();

    // Extension for manipulating canvas drawing contexts for use in CSS
    CanvasRenderingContext* getCSSCanvasContext(const String& type, const String& name, int width, int height);
    HTMLCanvasElement* getCSSCanvasElement(const String& name);

    bool isDNSPrefetchEnabled() const { return m_isDNSPrefetchEnabled; }
    void parseDNSPrefetchControlHeader(const String&);

#if !PLATFORM(WKC) || COMPILER(CLANG)
    virtual void postTask(Task) override final; // Executes the task on context's thread asynchronously.
#else
    virtual void postTask(Task&&) override final; // Executes the task on context's thread asynchronously.
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
    ScriptedAnimationController* scriptedAnimationController() { return m_scriptedAnimationController.get(); }
#endif
    void suspendScriptedAnimationControllerCallbacks();
    void resumeScriptedAnimationControllerCallbacks();
    void scriptedAnimationControllerSetThrottled(bool);
    
    void windowScreenDidChange(PlatformDisplayID);

    void finishedParsing();

    enum PageCacheState { NotInPageCache, AboutToEnterPageCache, InPageCache };

    PageCacheState pageCacheState() const { return m_pageCacheState; }
    void setPageCacheState(PageCacheState);

    // FIXME: Update callers to use pageCacheState() instead.
    bool inPageCache() const { return m_pageCacheState != NotInPageCache; }

    // Elements can register themselves for the "suspend()" and
    // "resume()" callbacks
    void registerForDocumentSuspensionCallbacks(Element*);
    void unregisterForDocumentSuspensionCallbacks(Element*);

    void documentWillBecomeInactive();
    void suspend(ActiveDOMObject::ReasonForSuspension);
    void resume(ActiveDOMObject::ReasonForSuspension);

    void registerForMediaVolumeCallbacks(Element*);
    void unregisterForMediaVolumeCallbacks(Element*);
    void mediaVolumeDidChange();

#if ENABLE(MEDIA_SESSION)
    MediaSession& defaultMediaSession();
#endif

    void registerForPrivateBrowsingStateChangedCallbacks(Element*);
    void unregisterForPrivateBrowsingStateChangedCallbacks(Element*);
    void storageBlockingStateDidChange();
    void privateBrowsingStateDidChange();

#if ENABLE(VIDEO_TRACK)
    void registerForCaptionPreferencesChangedCallbacks(Element*);
    void unregisterForCaptionPreferencesChangedCallbacks(Element*);
    void captionPreferencesChanged();
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    void registerForPageScaleFactorChangedCallbacks(HTMLMediaElement*);
    void unregisterForPageScaleFactorChangedCallbacks(HTMLMediaElement*);
    void pageScaleFactorChangedAndStable();
#endif

    void registerForVisibilityStateChangedCallbacks(Element*);
    void unregisterForVisibilityStateChangedCallbacks(Element*);

#if ENABLE(VIDEO)
    void registerForAllowsMediaDocumentInlinePlaybackChangedCallbacks(HTMLMediaElement&);
    void unregisterForAllowsMediaDocumentInlinePlaybackChangedCallbacks(HTMLMediaElement&);
    void allowsMediaDocumentInlinePlaybackChanged();
#endif

    WEBCORE_EXPORT void setShouldCreateRenderers(bool);
    bool shouldCreateRenderers();

    void setDecoder(PassRefPtr<TextResourceDecoder>);
    TextResourceDecoder* decoder() const { return m_decoder.get(); }

    WEBCORE_EXPORT String displayStringModifiedByEncoding(const String&) const;
    RefPtr<StringImpl> displayStringModifiedByEncoding(PassRefPtr<StringImpl>) const;
    void displayBufferModifiedByEncoding(LChar* buffer, unsigned len) const
    {
        displayBufferModifiedByEncodingInternal(buffer, len);
    }
    void displayBufferModifiedByEncoding(UChar* buffer, unsigned len) const
    {
        displayBufferModifiedByEncodingInternal(buffer, len);
    }

    // Quirk for the benefit of Apple's Dictionary application.
    void setFrameElementsShouldIgnoreScrolling(bool ignore) { m_frameElementsShouldIgnoreScrolling = ignore; }
    bool frameElementsShouldIgnoreScrolling() const { return m_frameElementsShouldIgnoreScrolling; }

#if ENABLE(DASHBOARD_SUPPORT)
    void setAnnotatedRegionsDirty(bool f) { m_annotatedRegionsDirty = f; }
    bool annotatedRegionsDirty() const { return m_annotatedRegionsDirty; }
    bool hasAnnotatedRegions () const { return m_hasAnnotatedRegions; }
    void setHasAnnotatedRegions(bool f) { m_hasAnnotatedRegions = f; }
    WEBCORE_EXPORT const Vector<AnnotatedRegionValue>& annotatedRegions() const;
    void setAnnotatedRegions(const Vector<AnnotatedRegionValue>&);
#endif

    virtual void removeAllEventListeners() override final;

    WEBCORE_EXPORT const SVGDocumentExtensions* svgExtensions();
    WEBCORE_EXPORT SVGDocumentExtensions& accessSVGExtensions();

    void initSecurityContext();
    void initContentSecurityPolicy();

    void updateURLForPushOrReplaceState(const URL&);
    void statePopped(PassRefPtr<SerializedScriptValue>);

    bool processingLoadEvent() const { return m_processingLoadEvent; }
    bool loadEventFinished() const { return m_loadEventFinished; }

    virtual bool isContextThread() const override final;
    virtual bool isJSExecutionForbidden() const override final { return false; }

    void enqueueWindowEvent(PassRefPtr<Event>);
    void enqueueDocumentEvent(PassRefPtr<Event>);
    void enqueueOverflowEvent(PassRefPtr<Event>);
    void dispatchPageshowEvent(PageshowEventPersistence);
    void enqueueHashchangeEvent(const String& oldURL, const String& newURL);
    void dispatchPopstateEvent(RefPtr<SerializedScriptValue>&& stateObject);
    virtual DocumentEventQueue& eventQueue() const override final { return m_eventQueue; }

    WEBCORE_EXPORT void addMediaCanStartListener(MediaCanStartListener*);
    WEBCORE_EXPORT void removeMediaCanStartListener(MediaCanStartListener*);
    MediaCanStartListener* takeAnyMediaCanStartListener();

#if ENABLE(FULLSCREEN_API)
    bool webkitIsFullScreen() const { return m_fullScreenElement.get(); }
    bool webkitFullScreenKeyboardInputAllowed() const { return m_fullScreenElement.get() && m_areKeysEnabledInFullScreen; }
    Element* webkitCurrentFullScreenElement() const { return m_fullScreenElement.get(); }
    
    enum FullScreenCheckType {
        EnforceIFrameAllowFullScreenRequirement,
        ExemptIFrameAllowFullScreenRequirement,
    };

    void requestFullScreenForElement(Element*, unsigned short flags, FullScreenCheckType);
    WEBCORE_EXPORT void webkitCancelFullScreen();
    
    WEBCORE_EXPORT void webkitWillEnterFullScreenForElement(Element*);
    WEBCORE_EXPORT void webkitDidEnterFullScreenForElement(Element*);
    WEBCORE_EXPORT void webkitWillExitFullScreenForElement(Element*);
    WEBCORE_EXPORT void webkitDidExitFullScreenForElement(Element*);
    
    void setFullScreenRenderer(RenderFullScreen*);
    RenderFullScreen* fullScreenRenderer() const { return m_fullScreenRenderer; }
    void fullScreenRendererDestroyed();
    
    void fullScreenChangeDelayTimerFired();
    bool fullScreenIsAllowedForElement(Element*) const;
    void fullScreenElementRemoved();
    void removeFullScreenElementOfSubtree(Node*, bool amongChildrenOnly = false);
    bool isAnimatingFullScreen() const;
    WEBCORE_EXPORT void setAnimatingFullScreen(bool);

    // W3C API
    bool webkitFullscreenEnabled() const;
    Element* webkitFullscreenElement() const { return !m_fullScreenElementStack.isEmpty() ? m_fullScreenElementStack.last().get() : 0; }
    void webkitExitFullscreen();
#endif

#if ENABLE(POINTER_LOCK)
    void exitPointerLock();
    Element* pointerLockElement() const;
#endif

    // Used to allow element that loads data without going through a FrameLoader to delay the 'load' event.
    void incrementLoadEventDelayCount() { ++m_loadEventDelayCount; }
    void decrementLoadEventDelayCount();
    bool isDelayingLoadEvent() const { return m_loadEventDelayCount; }

#if ENABLE(IOS_TOUCH_EVENTS)
#include <WebKitAdditions/DocumentIOS.h>
#elif ENABLE(TOUCH_EVENTS)
    RefPtr<Touch> createTouch(DOMWindow*, EventTarget*, int identifier, int pageX, int pageY, int screenX, int screenY, int radiusX, int radiusY, float rotationAngle, float force, ExceptionCode&) const;
#endif

#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(IOS)
    DeviceMotionController* deviceMotionController() const;
    DeviceOrientationController* deviceOrientationController() const;
#endif

#if ENABLE(WEB_TIMING)
    const DocumentTiming& timing() const { return m_documentTiming; }
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
    int requestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>);
    void cancelAnimationFrame(int id);
    void serviceScriptedAnimations(double monotonicAnimationStartTime);
#endif

    void sendWillRevealEdgeEventsIfNeeded(const IntPoint& oldPosition, const IntPoint& newPosition, const IntRect& visibleRect, const IntSize& contentsSize, Element* target = nullptr);

    virtual EventTarget* errorEventTarget() override final;
    virtual void logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, RefPtr<Inspector::ScriptCallStack>&&) override final;

    void initDNSPrefetch();

    void didAddWheelEventHandler(Node&);
    void didRemoveWheelEventHandler(Node&, EventHandlerRemoval = EventHandlerRemoval::One);

    double lastHandledUserGestureTimestamp() const { return m_lastHandledUserGestureTimestamp; }
    void updateLastHandledUserGestureTimestamp();

    // Used for testing. Count handlers in the main document, and one per frame which contains handlers.
    WEBCORE_EXPORT unsigned wheelEventHandlerCount() const;
    WEBCORE_EXPORT unsigned touchEventHandlerCount() const;

    WEBCORE_EXPORT void startTrackingStyleRecalcs();
    WEBCORE_EXPORT unsigned styleRecalcCount() const;

#if ENABLE(TOUCH_EVENTS)
    bool hasTouchEventHandlers() const { return (m_touchEventTargets.get()) ? m_touchEventTargets->size() : false; }
    bool touchEventTargetsContain(Node& node) const { return m_touchEventTargets ? m_touchEventTargets->contains(&node) : false; }
#else
    bool hasTouchEventHandlers() const { return false; }
    bool touchEventTargetsContain(Node&) const { return false; }
#endif

    void didAddTouchEventHandler(Node&);
    void didRemoveTouchEventHandler(Node&, EventHandlerRemoval = EventHandlerRemoval::One);

    void didRemoveEventTargetNode(Node&);

    const EventTargetSet* touchEventTargets() const
    {
#if ENABLE(TOUCH_EVENTS)
        return m_touchEventTargets.get();
#else
        return nullptr;
#endif
    }

    const EventTargetSet* wheelEventTargets() const { return m_wheelEventTargets.get(); }

    typedef std::pair<Region, bool> RegionFixedPair;
    RegionFixedPair absoluteRegionForEventTargets(const EventTargetSet*);

    LayoutRect absoluteEventHandlerBounds(bool&) override final;

    bool visualUpdatesAllowed() const { return m_visualUpdatesAllowed; }

    bool isInDocumentWrite() { return m_writeRecursionDepth > 0; }

    void suspendScheduledTasks(ActiveDOMObject::ReasonForSuspension);
    void resumeScheduledTasks(ActiveDOMObject::ReasonForSuspension);

#if ENABLE(CSS_DEVICE_ADAPTATION)
    IntSize initialViewportSize() const;
#endif

#if ENABLE(TEXT_AUTOSIZING)
    TextAutosizer* textAutosizer() { return m_textAutosizer.get(); }
#endif

    void adjustFloatQuadsForScrollAndAbsoluteZoomAndFrameScale(Vector<FloatQuad>&, const RenderStyle&);
    void adjustFloatRectForScrollAndAbsoluteZoomAndFrameScale(FloatRect&, const RenderStyle&);

    bool hasActiveParser();
    void incrementActiveParserCount() { ++m_activeParserCount; }
    void decrementActiveParserCount();

    DocumentSharedObjectPool* sharedObjectPool() { return m_sharedObjectPool.get(); }

    void didRemoveAllPendingStylesheet();
    void setNeedsNotifyRemoveAllPendingStylesheet() { m_needsNotifyRemoveAllPendingStylesheet = true; }
    void clearStyleResolver();

    bool inStyleRecalc() { return m_inStyleRecalc; }

    // Return a Locale for the default locale if the argument is null or empty.
    Locale& getCachedLocale(const AtomicString& locale = nullAtom);

#if ENABLE(TEMPLATE_ELEMENT)
    const Document* templateDocument() const;
    Document& ensureTemplateDocument();
    void setTemplateDocumentHost(Document* templateDocumentHost) { m_templateDocumentHost = templateDocumentHost; }
    Document* templateDocumentHost() { return m_templateDocumentHost; }
#endif

    void didAssociateFormControl(Element*);
    bool hasDisabledFieldsetElement() const { return m_disabledFieldsetElementsCount; }
    void addDisabledFieldsetElement() { m_disabledFieldsetElementsCount++; }
    void removeDisabledFieldsetElement() { ASSERT(m_disabledFieldsetElementsCount); m_disabledFieldsetElementsCount--; }

    WEBCORE_EXPORT virtual void addConsoleMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0) override final;

    SecurityOrigin& securityOrigin() const { return *SecurityContext::securityOrigin(); }

    WEBCORE_EXPORT virtual SecurityOrigin* topOrigin() const override final;

#if ENABLE(FONT_LOAD_EVENTS)
    RefPtr<FontLoader> fonts();
#endif

    void ensurePlugInsInjectedScript(DOMWrapperWorld&);

    void setVisualUpdatesAllowedByClient(bool);

#if ENABLE(SUBTLE_CRYPTO)
    virtual bool wrapCryptoKey(const Vector<uint8_t>& key, Vector<uint8_t>& wrappedKey) override final;
    virtual bool unwrapCryptoKey(const Vector<uint8_t>& wrappedKey, Vector<uint8_t>& key) override final;
#endif

    void setHasStyleWithViewportUnits() { m_hasStyleWithViewportUnits = true; }
    bool hasStyleWithViewportUnits() const { return m_hasStyleWithViewportUnits; }
    void updateViewportUnitsOnResize();

    WEBCORE_EXPORT void addAudioProducer(MediaProducer*);
    WEBCORE_EXPORT void removeAudioProducer(MediaProducer*);
    MediaProducer::MediaStateFlags mediaState() const { return m_mediaState; }
    void noteUserInteractionWithMediaElement();
    WEBCORE_EXPORT void updateIsPlayingMedia();
    void pageMutedStateDidChange();
    WeakPtr<Document> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void addPlaybackTargetPickerClient(MediaPlaybackTargetClient&);
    void removePlaybackTargetPickerClient(MediaPlaybackTargetClient&);
    void showPlaybackTargetPicker(MediaPlaybackTargetClient&, bool);
    void playbackTargetPickerClientStateDidChange(MediaPlaybackTargetClient&, MediaProducer::MediaStateFlags);

    void setPlaybackTarget(uint64_t, Ref<MediaPlaybackTarget>&&);
    void playbackTargetAvailabilityDidChange(uint64_t, bool);
    void setShouldPlayToPlaybackTarget(uint64_t, bool);
#endif

    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicyToPropagate() const;
    bool shouldEnforceContentDispositionAttachmentSandbox() const;

    void addViewportDependentPicture(HTMLPictureElement&);
    void removeViewportDependentPicture(HTMLPictureElement&);

    // Used in webarchive loading tests.
    void setAlwaysAllowLocalWebarchive() { m_alwaysAllowLocalWebarchive = true; }
    bool alwaysAllowLocalWebarchive() const { return m_alwaysAllowLocalWebarchive; }

protected:
    enum ConstructionFlags { Synthesized = 1, NonRenderedPlaceholder = 1 << 1 };
    Document(Frame*, const URL&, unsigned = DefaultDocumentClass, unsigned constructionFlags = 0);

    void clearXMLVersion() { m_xmlVersion = String(); }

    virtual Ref<Document> cloneDocumentWithoutChildren() const;

private:
    friend class Node;
    friend class IgnoreDestructiveWriteCountIncrementer;
    friend class IgnoreOpensDuringUnloadCountIncrementer;

    void commonTeardown();

    RenderObject* renderer() const  = delete;
    void setRenderer(RenderObject*)  = delete;

    void createRenderTree();
    void detachParser();

    typedef void (*ArgumentsCallback)(const String& keyString, const String& valueString, Document*, void* data);
    void processArguments(const String& features, void* data, ArgumentsCallback);

    // FontSelectorClient
    virtual void fontsNeedUpdate(FontSelector&) override final;

    virtual bool isDocument() const override final { return true; }

    virtual void childrenChanged(const ChildChange&) override final;

    virtual String nodeName() const override final;
    virtual NodeType nodeType() const override final;
    virtual bool childTypeAllowed(NodeType) const override final;
    virtual Ref<Node> cloneNodeInternal(Document&, CloningOperation) override final;
    void cloneDataFromDocument(const Document&);

    virtual void refScriptExecutionContext() override final { ref(); }
    virtual void derefScriptExecutionContext() override final { deref(); }

    virtual void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, RefPtr<Inspector::ScriptCallStack>&&, JSC::ExecState* = 0, unsigned long requestIdentifier = 0) override final;

    virtual double minimumTimerInterval() const override final;

    virtual double timerAlignmentInterval(bool hasReachedMaxNestingLevel) const override final;

    void updateTitle(const StringWithDirection&);
    void updateFocusAppearanceTimerFired();
    void updateBaseURL();

    void buildAccessKeyMap(TreeScope* root);

    void createStyleResolver();

    void loadEventDelayTimerFired();

    void pendingTasksTimerFired();
    bool isCookieAverse() const;

    template <typename CharacterType>
    void displayBufferModifiedByEncodingInternal(CharacterType*, unsigned) const;

    PageVisibilityState pageVisibilityState() const;

    Node* nodeFromPoint(const LayoutPoint& clientPoint, LayoutPoint* localPoint = nullptr);

    Ref<HTMLCollection> ensureCachedCollection(CollectionType);

#if ENABLE(FULLSCREEN_API)
    void dispatchFullScreenChangeOrErrorEvent(Deque<RefPtr<Node>>&, const AtomicString& eventName, bool shouldNotifyMediaElement);
    void clearFullscreenElementStack();
    void popFullscreenElementStack();
    void pushFullscreenElementStack(Element*);
    void addDocumentToFullScreenChangeEventQueue(Document*);
#endif

    void setVisualUpdatesAllowed(ReadyState);
    void setVisualUpdatesAllowed(bool);
    void visualUpdatesSuppressionTimerFired();

    void addListenerType(ListenerType listenerType) { m_listenerTypes |= listenerType; }

    void didAssociateFormControlsTimerFired();

    void wheelEventHandlersChanged();

    HttpEquivPolicy httpEquivPolicy() const;

    // DOM Cookies caching.
    const String& cachedDOMCookies() const { return m_cachedDOMCookies; }
    void setCachedDOMCookies(const String&);
    bool isDOMCookieCacheValid() const { return m_cookieCacheExpiryTimer.isActive(); }
    void invalidateDOMCookieCache();
    void domCookieCacheExpiryTimerFired();
    virtual void didLoadResourceSynchronously(const ResourceRequest&) override final;

    void checkViewportDependentPictures();

    unsigned m_referencingNodeCount;

    std::unique_ptr<StyleResolver> m_styleResolver;
    bool m_didCalculateStyleResolver;
    bool m_hasNodesWithPlaceholderStyle;
    bool m_needsNotifyRemoveAllPendingStylesheet;
    // But sometimes you need to ignore pending stylesheet count to
    // force an immediate layout when requested by JS.
    bool m_ignorePendingStylesheets;

    // If we do ignore the pending stylesheet count, then we need to add a boolean
    // to track that this happened so that we can do a full repaint when the stylesheets
    // do eventually load.
    PendingSheetLayout m_pendingSheetLayout;

    Frame* m_frame;
    RefPtr<DOMWindow> m_domWindow;

    Ref<CachedResourceLoader> m_cachedResourceLoader;
    RefPtr<DocumentParser> m_parser;
    unsigned m_activeParserCount;

    bool m_wellFormed;

    // Document URLs.
    URL m_url; // Document.URL: The URL from which this document was retrieved.
    URL m_baseURL; // Node.baseURI: The URL to use when resolving relative URLs.
    URL m_baseURLOverride; // An alternative base URL that takes precedence over m_baseURL (but not m_baseElementURL).
    URL m_baseElementURL; // The URL set by the <base> element.
    URL m_cookieURL; // The URL to use for cookie access.
    URL m_firstPartyForCookies; // The policy URL for third-party cookie blocking.

    // Document.documentURI:
    // Although URL-like, Document.documentURI can actually be set to any
    // string by content.  Document.documentURI affects m_baseURL unless the
    // document contains a <base> element, in which case the <base> element
    // takes precedence.
    //
    // This property is read-only from JavaScript, but writable from Objective C.
    String m_documentURI;

    String m_baseTarget;

    // MIME type of the document in case it was cloned or created by XHR.
    String m_overriddenMIMEType;

    std::unique_ptr<DOMImplementation> m_implementation;

    RefPtr<CSSStyleSheet> m_elementSheet;

    bool m_printing;
    bool m_paginatedForScreen;

    DocumentCompatibilityMode m_compatibilityMode;
    bool m_compatibilityModeLocked; // This is cheaper than making setCompatibilityMode virtual.

    Color m_textColor;

    RefPtr<Element> m_focusedElement;
    RefPtr<Element> m_hoveredElement;
    RefPtr<Element> m_activeElement;
    RefPtr<Element> m_documentElement;
    UserActionElementSet m_userActionElements;

    uint64_t m_domTreeVersion;
#if !PLATFORM(WKC)
    static uint64_t s_globalTreeVersion;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(uint64_t, s_globalTreeVersion);
#endif
    
    HashSet<NodeIterator*> m_nodeIterators;
    HashSet<Range*> m_ranges;

    unsigned m_listenerTypes;

    MutationObserverOptions m_mutationObserverTypes;

    DocumentStyleSheetCollection m_styleSheetCollection;
    RefPtr<StyleSheetList> m_styleSheetList;

    std::unique_ptr<FormController> m_formController;

    Color m_linkColor;
    Color m_visitedLinkColor;
    Color m_activeLinkColor;
    const std::unique_ptr<VisitedLinkState> m_visitedLinkState;

    bool m_visuallyOrdered;
    ReadyState m_readyState;
    bool m_bParsing;

    Timer m_optimizedStyleSheetUpdateTimer;
    Timer m_styleRecalcTimer;
    bool m_pendingStyleRecalcShouldForce;
    bool m_inStyleRecalc;
    bool m_closeAfterStyleRecalc;

    bool m_gotoAnchorNeededAfterStylesheetsLoad;
    bool m_isDNSPrefetchEnabled;
    bool m_haveExplicitlyDisabledDNSPrefetch;
    bool m_frameElementsShouldIgnoreScrolling;
    bool m_updateFocusAppearanceRestoresSelection;

    // https://html.spec.whatwg.org/multipage/webappapis.html#ignore-destructive-writes-counter
    unsigned m_ignoreDestructiveWriteCount { 0 };

    // https://html.spec.whatwg.org/multipage/webappapis.html#ignore-opens-during-unload-counter
    unsigned m_ignoreOpensDuringUnloadCount { 0 };

    unsigned m_styleRecalcCount { 0 };

    StringWithDirection m_title;
    StringWithDirection m_rawTitle;
    bool m_titleSetExplicitly;
    RefPtr<Element> m_titleElement;

    std::unique_ptr<AXObjectCache> m_axObjectCache;
    const std::unique_ptr<DocumentMarkerController> m_markers;
    
    Timer m_updateFocusAppearanceTimer;

    Element* m_cssTarget;

    // FIXME: Merge these 2 variables into an enum. Also, FrameLoader::m_didCallImplicitClose
    // is almost a duplication of this data, so that should probably get merged in too.
    // FIXME: Document::m_processingLoadEvent and DocumentLoader::m_wasOnloadHandled are roughly the same
    // and should be merged.
    bool m_processingLoadEvent;
    bool m_loadEventFinished;

    RefPtr<SerializedScriptValue> m_pendingStateObject;
    std::chrono::steady_clock::time_point m_startTime;
    bool m_overMinimumLayoutThreshold;
    
    std::unique_ptr<ScriptRunner> m_scriptRunner;

    Vector<RefPtr<HTMLScriptElement>> m_currentScriptStack;

#if ENABLE(XSLT)
    std::unique_ptr<TransformSource> m_transformSource;
    RefPtr<Document> m_transformSourceDocument;
#endif

    String m_xmlEncoding;
    String m_xmlVersion;
    unsigned m_xmlStandalone : 2;
    unsigned m_hasXMLDeclaration : 1;

    String m_contentLanguage;

    RefPtr<TextResourceDecoder> m_decoder;

    InheritedBool m_designMode;

    HashSet<LiveNodeList*> m_listsInvalidatedAtDocument;
    HashSet<HTMLCollection*> m_collectionsInvalidatedAtDocument;
    unsigned m_nodeListAndCollectionCounts[numNodeListInvalidationTypes];

    RefPtr<XPathEvaluator> m_xpathEvaluator;

    std::unique_ptr<SVGDocumentExtensions> m_svgExtensions;

#if ENABLE(DASHBOARD_SUPPORT)
    Vector<AnnotatedRegionValue> m_annotatedRegions;
    bool m_hasAnnotatedRegions;
    bool m_annotatedRegionsDirty;
#endif

    HashMap<String, RefPtr<HTMLCanvasElement>> m_cssCanvasElements;

    bool m_createRenderers;
    PageCacheState m_pageCacheState { NotInPageCache };

    HashSet<Element*> m_documentSuspensionCallbackElements;
    HashSet<Element*> m_mediaVolumeCallbackElements;
    HashSet<Element*> m_privateBrowsingStateChangedElements;
#if ENABLE(VIDEO_TRACK)
    HashSet<Element*> m_captionPreferencesChangedElements;
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    HashSet<HTMLMediaElement*> m_pageScaleFactorChangedElements;
#endif

    HashSet<Element*> m_visibilityStateCallbackElements;
#if ENABLE(VIDEO)
    HashSet<HTMLMediaElement*> m_allowsMediaDocumentInlinePlaybackElements;
#endif

    HashMap<StringImpl*, Element*, CaseFoldingHash> m_elementsByAccessKey;
    bool m_accessKeyMapValid;

    DocumentOrderedMap m_imagesByUsemap;

    std::unique_ptr<SelectorQueryCache> m_selectorQueryCache;

    DocumentClassFlags m_documentClasses;

    bool m_isSynthesized;
    bool m_isNonRenderedPlaceholder;

    bool m_sawElementsInKnownNamespaces;
    bool m_isSrcdocDocument;

    RenderPtr<RenderView> m_renderView;
    mutable DocumentEventQueue m_eventQueue;

    WeakPtrFactory<Document> m_weakFactory;

    HashSet<MediaCanStartListener*> m_mediaCanStartListeners;

#if ENABLE(FULLSCREEN_API)
    bool m_areKeysEnabledInFullScreen;
    RefPtr<Element> m_fullScreenElement;
    Vector<RefPtr<Element>> m_fullScreenElementStack;
    RenderFullScreen* m_fullScreenRenderer;
    Timer m_fullScreenChangeDelayTimer;
    Deque<RefPtr<Node>> m_fullScreenChangeEventTargetQueue;
    Deque<RefPtr<Node>> m_fullScreenErrorEventTargetQueue;
    bool m_isAnimatingFullScreen;
    LayoutRect m_savedPlaceholderFrameRect;
    RefPtr<RenderStyle> m_savedPlaceholderRenderStyle;
#endif

    HashSet<HTMLPictureElement*> m_viewportDependentPictures;

    int m_loadEventDelayCount;
    Timer m_loadEventDelayTimer;

    ViewportArguments m_viewportArguments;

    ReferrerPolicy m_referrerPolicy;

    bool m_directionSetOnDocumentElement;
    bool m_writingModeSetOnDocumentElement;

#if ENABLE(WEB_TIMING)
    DocumentTiming m_documentTiming;
#endif

    RefPtr<MediaQueryMatcher> m_mediaQueryMatcher;
    bool m_writeRecursionIsTooDeep;
    unsigned m_writeRecursionDepth;
    
#if ENABLE(TOUCH_EVENTS)
    std::unique_ptr<EventTargetSet> m_touchEventTargets;
#endif
    std::unique_ptr<EventTargetSet> m_wheelEventTargets;

    double m_lastHandledUserGestureTimestamp;

#if ENABLE(REQUEST_ANIMATION_FRAME)
    void clearScriptedAnimationController();
    RefPtr<ScriptedAnimationController> m_scriptedAnimationController;
#endif

#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(IOS)
    std::unique_ptr<DeviceMotionClient> m_deviceMotionClient;
    std::unique_ptr<DeviceMotionController> m_deviceMotionController;
    std::unique_ptr<DeviceOrientationClient> m_deviceOrientationClient;
    std::unique_ptr<DeviceOrientationController> m_deviceOrientationController;
#endif

// FIXME: Find a better place for this functionality.
#if ENABLE(TELEPHONE_NUMBER_DETECTION)
public:

    // These functions provide a two-level setting:
    //    - A user-settable wantsTelephoneNumberParsing (at the Page / WebView level)
    //    - A read-only telephoneNumberParsingAllowed which is set by the
    //      document if it has the appropriate meta tag.
    //    - isTelephoneNumberParsingEnabled() == isTelephoneNumberParsingAllowed() && page()->settings()->isTelephoneNumberParsingEnabled()

    WEBCORE_EXPORT bool isTelephoneNumberParsingAllowed() const;
    WEBCORE_EXPORT bool isTelephoneNumberParsingEnabled() const;

private:
    friend void setParserFeature(const String& key, const String& value, Document*, void* userData);
    void setIsTelephoneNumberParsingAllowed(bool);

    bool m_isTelephoneNumberParsingAllowed;
#endif

    Timer m_pendingTasksTimer;
    Vector<Task> m_pendingTasks;

#if ENABLE(IOS_TEXT_AUTOSIZING)
public:
    void addAutoSizingNode(Node*, float size);
    void validateAutoSizingNodes();
    void resetAutoSizingNodes();

private:
    typedef HashMap<TextAutoSizingKey, RefPtr<TextAutoSizingValue>, TextAutoSizingHash, TextAutoSizingTraits> TextAutoSizingMap;
    TextAutoSizingMap m_textAutoSizedNodes;
#endif

#if ENABLE(TEXT_AUTOSIZING)
    std::unique_ptr<TextAutosizer> m_textAutosizer;
#endif

    void platformSuspendOrStopActiveDOMObjects();

    bool m_scheduledTasksAreSuspended;
    
    bool m_visualUpdatesAllowed;
    Timer m_visualUpdatesSuppressionTimer;

    RefPtr<NamedFlowCollection> m_namedFlows;

#if ENABLE(CSP_NEXT)
    RefPtr<DOMSecurityPolicy> m_domSecurityPolicy;
#endif

    void sharedObjectPoolClearTimerFired();
    Timer m_sharedObjectPoolClearTimer;

    std::unique_ptr<DocumentSharedObjectPool> m_sharedObjectPool;

#ifndef NDEBUG
    bool m_didDispatchViewportPropertiesChanged;
#endif

    typedef HashMap<AtomicString, std::unique_ptr<Locale>> LocaleIdentifierToLocaleMap;
    LocaleIdentifierToLocaleMap m_localeCache;

#if ENABLE(TEMPLATE_ELEMENT)
    RefPtr<Document> m_templateDocument;
    Document* m_templateDocumentHost; // Manually managed weakref (backpointer from m_templateDocument).
#endif

    RefPtr<CSSFontSelector> m_fontSelector;

#if ENABLE(FONT_LOAD_EVENTS)
    RefPtr<FontLoader> m_fontloader;
#endif

#if ENABLE(WEB_REPLAY)
    RefPtr<JSC::InputCursor> m_inputCursor;
#endif

    Timer m_didAssociateFormControlsTimer;
    Timer m_cookieCacheExpiryTimer;
    String m_cachedDOMCookies;
    HashSet<RefPtr<Element>> m_associatedFormControls;
    unsigned m_disabledFieldsetElementsCount;

    bool m_hasInjectedPlugInsScript;
    bool m_renderTreeBeingDestroyed;
    bool m_hasPreparedForDestruction;

    bool m_hasStyleWithViewportUnits;
    bool m_isTimerThrottlingEnabled { false };
    bool m_isSuspended { false };

    HashSet<MediaProducer*> m_audioProducers;
    MediaProducer::MediaStateFlags m_mediaState { MediaProducer::IsNotPlaying };
    bool m_userHasInteractedWithMediaElement { false };

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    typedef HashMap<uint64_t, WebCore::MediaPlaybackTargetClient*> TargetIdToClientMap;
    TargetIdToClientMap m_idToClientMap;
    typedef HashMap<WebCore::MediaPlaybackTargetClient*, uint64_t> TargetClientToIdMap;
    TargetClientToIdMap m_clientToIDMap;
#endif

#if ENABLE(MEDIA_SESSION)
    RefPtr<MediaSession> m_defaultMediaSession;
#endif

    bool m_alwaysAllowLocalWebarchive { false };
};

inline void Document::notifyRemovePendingSheetIfNeeded()
{
    if (m_needsNotifyRemoveAllPendingStylesheet)
        didRemoveAllPendingStylesheet();
}

#if ENABLE(TEMPLATE_ELEMENT)
inline const Document* Document::templateDocument() const
{
    // If DOCUMENT does not have a browsing context, Let TEMPLATE CONTENTS OWNER be DOCUMENT and abort these steps.
    if (!m_frame)
        return this;

    return m_templateDocument.get();
}
#endif

// Put these methods here, because they require the Document definition, but we really want to inline them.

inline bool Node::isDocumentNode() const
{
    return this == &document();
}

inline ScriptExecutionContext* Node::scriptExecutionContext() const
{
    return &document();
}

Element* eventTargetElementForDocument(Document*);

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::Document)
    static bool isType(const WebCore::ScriptExecutionContext& context) { return context.isDocument(); }
    static bool isType(const WebCore::Node& node) { return node.isDocumentNode(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // Document_h
