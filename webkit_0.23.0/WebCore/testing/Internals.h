/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Internals_h
#define Internals_h

#include "CSSComputedStyleDeclaration.h"
#include "ContextDestructionObserver.h"
#include "ExceptionCodePlaceholder.h"
#include "NodeList.h"
#include "PageConsoleClient.h"
#include <bindings/ScriptValue.h>
#include <runtime/ArrayBuffer.h>
#include <runtime/Float32Array.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AudioContext;
class ClientRect;
class ClientRectList;
class DOMPath;
class DOMStringList;
class DOMWindow;
class Document;
class Element;
class File;
class Frame;
class HTMLMediaElement;
class InspectorFrontendChannelDummy;
class InspectorFrontendClientDummy;
class InternalSettings;
class MallocStatistics;
class MemoryInfo;
class Node;
class Page;
class Range;
class RenderedDocumentMarker;
class ScriptExecutionContext;
class SerializedScriptValue;
class SourceBuffer;
class TimeRanges;
class TypeConversions;
class XMLHttpRequest;

#if ENABLE(CONTENT_FILTERING)
class MockContentFilterSettings;
#endif

typedef int ExceptionCode;

class Internals : public RefCounted<Internals>
                , public ContextDestructionObserver {
public:
    static Ref<Internals> create(Document*);
    virtual ~Internals();

    static void resetToConsistentState(Page*);

    String elementRenderTreeAsText(Element*, ExceptionCode&);
    bool hasPausedImageAnimations(Element*, ExceptionCode&);

    String address(Node*);
    bool nodeNeedsStyleRecalc(Node*, ExceptionCode&);
    String description(Deprecated::ScriptValue);

    bool isPreloaded(const String& url);
    bool isLoadingFromMemoryCache(const String& url);
    String xhrResponseSource(XMLHttpRequest*);
    bool isSharingStyleSheetContents(Element* linkA, Element* linkB);
    bool isStyleSheetLoadingSubresources(Element* link);
    void setOverrideCachePolicy(const String&);
    void setOverrideResourceLoadPriority(const String&);

    void clearMemoryCache();
    void pruneMemoryCacheToSize(unsigned size);
    unsigned memoryCacheSize() const;

    void clearPageCache();
    unsigned pageCacheSize() const;

    RefPtr<CSSComputedStyleDeclaration> computedStyleIncludingVisitedInfo(Node*, ExceptionCode&) const;

    Node* ensureShadowRoot(Element* host, ExceptionCode&);
    Node* ensureUserAgentShadowRoot(Element* host, ExceptionCode&);
    Node* createShadowRoot(Element* host, ExceptionCode&);
    Node* shadowRoot(Element* host, ExceptionCode&);
    String shadowRootType(const Node*, ExceptionCode&) const;
    Element* includerFor(Node*, ExceptionCode&);
    String shadowPseudoId(Element*, ExceptionCode&);
    void setShadowPseudoId(Element*, const String&, ExceptionCode&);

    // DOMTimers throttling testing.
    bool isTimerThrottled(int timeoutId, ExceptionCode&);
    bool isRequestAnimationFrameThrottled() const;
    bool areTimersThrottled() const;

    // Spatial Navigation testing.
    unsigned lastSpatialNavigationCandidateCount(ExceptionCode&) const;

    // CSS Animation testing.
    unsigned numberOfActiveAnimations() const;
    bool animationsAreSuspended(ExceptionCode&) const;
    void suspendAnimations(ExceptionCode&) const;
    void resumeAnimations(ExceptionCode&) const;
    bool pauseAnimationAtTimeOnElement(const String& animationName, double pauseTime, Element*, ExceptionCode&);
    bool pauseAnimationAtTimeOnPseudoElement(const String& animationName, double pauseTime, Element*, const String& pseudoId, ExceptionCode&);

    // CSS Transition testing.
    bool pauseTransitionAtTimeOnElement(const String& propertyName, double pauseTime, Element*, ExceptionCode&);
    bool pauseTransitionAtTimeOnPseudoElement(const String& property, double pauseTime, Element*, const String& pseudoId, ExceptionCode&);

    Node* treeScopeRootNode(Node*, ExceptionCode&);
    Node* parentTreeScope(Node*, ExceptionCode&);

    bool attached(Node*, ExceptionCode&);

    String visiblePlaceholder(Element*);
#if ENABLE(INPUT_TYPE_COLOR)
    void selectColorInColorChooser(Element*, const String& colorValue);
#endif
    Vector<String> formControlStateOfPreviousHistoryItem(ExceptionCode&);
    void setFormControlStateOfPreviousHistoryItem(const Vector<String>&, ExceptionCode&);

    Ref<ClientRect> absoluteCaretBounds(ExceptionCode&);

    Ref<ClientRect> boundingBox(Element*, ExceptionCode&);

    Ref<ClientRectList> inspectorHighlightRects(ExceptionCode&);
    String inspectorHighlightObject(ExceptionCode&);

    unsigned markerCountForNode(Node*, const String&, ExceptionCode&);
    RefPtr<Range> markerRangeForNode(Node*, const String& markerType, unsigned index, ExceptionCode&);
    String markerDescriptionForNode(Node*, const String& markerType, unsigned index, ExceptionCode&);
    void addTextMatchMarker(const Range*, bool isActive);
    void setMarkedTextMatchesAreHighlighted(bool, ExceptionCode&);

    void invalidateFontCache();

    void setScrollViewPosition(long x, long y, ExceptionCode&);
    void setViewBaseBackgroundColor(const String& colorValue, ExceptionCode&);

    void setPagination(const String& mode, int gap, ExceptionCode& ec) { setPagination(mode, gap, 0, ec); }
    void setPagination(const String& mode, int gap, int pageLength, ExceptionCode&);
    String configurationForViewport(float devicePixelRatio, int deviceWidth, int deviceHeight, int availableWidth, int availableHeight, ExceptionCode&);

    bool wasLastChangeUserEdit(Element* textField, ExceptionCode&);
    bool elementShouldAutoComplete(Element* inputElement, ExceptionCode&);
    void setEditingValue(Element* inputElement, const String&, ExceptionCode&);
    void setAutofilled(Element*, bool enabled, ExceptionCode&);
    void setShowAutoFillButton(Element*, bool enabled, ExceptionCode&);
    void scrollElementToRect(Element*, long x, long y, long w, long h, ExceptionCode&);

    void paintControlTints(ExceptionCode&);

    RefPtr<Range> rangeFromLocationAndLength(Element* scope, int rangeLocation, int rangeLength, ExceptionCode&);
    unsigned locationFromRange(Element* scope, const Range*, ExceptionCode&);
    unsigned lengthFromRange(Element* scope, const Range*, ExceptionCode&);
    String rangeAsText(const Range*, ExceptionCode&);
    RefPtr<Range> subrange(Range*, int rangeLocation, int rangeLength, ExceptionCode&);
    RefPtr<Range> rangeForDictionaryLookupAtLocation(int x, int y, ExceptionCode&);

    void setDelegatesScrolling(bool enabled, ExceptionCode&);

    int lastSpellCheckRequestSequence(ExceptionCode&);
    int lastSpellCheckProcessedSequence(ExceptionCode&);

    Vector<String> userPreferredLanguages() const;
    void setUserPreferredLanguages(const Vector<String>&);

    Vector<String> userPreferredAudioCharacteristics() const;
    void setUserPreferredAudioCharacteristic(const String&);

    unsigned wheelEventHandlerCount(ExceptionCode&);
    unsigned touchEventHandlerCount(ExceptionCode&);

    RefPtr<NodeList> nodesFromRect(Document*, int x, int y, unsigned topPadding, unsigned rightPadding,
        unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping, bool allowShadowContent, bool allowChildFrameContent, ExceptionCode&) const;

    String parserMetaData(Deprecated::ScriptValue = Deprecated::ScriptValue());

    void updateEditorUINowIfScheduled();

    bool hasSpellingMarker(int from, int length, ExceptionCode&);
    bool hasGrammarMarker(int from, int length, ExceptionCode&);
    bool hasAutocorrectedMarker(int from, int length, ExceptionCode&);
    void setContinuousSpellCheckingEnabled(bool enabled, ExceptionCode&);
    void setAutomaticQuoteSubstitutionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticLinkDetectionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticDashSubstitutionEnabled(bool enabled, ExceptionCode&);
    void setAutomaticTextReplacementEnabled(bool enabled, ExceptionCode&);
    void setAutomaticSpellingCorrectionEnabled(bool enabled, ExceptionCode&);

    bool isOverwriteModeEnabled(ExceptionCode&);
    void toggleOverwriteModeEnabled(ExceptionCode&);

    unsigned countMatchesForText(const String&, unsigned findOptions, const String& markMatches, ExceptionCode&);

    unsigned numberOfScrollableAreas(ExceptionCode&);

    bool isPageBoxVisible(int pageNumber, ExceptionCode&);

    static const char* internalsId;

    InternalSettings* settings() const;
    unsigned workerThreadCount() const;

    void setBatteryStatus(const String& eventType, bool charging, double chargingTime, double dischargingTime, double level, ExceptionCode&);

    void setDeviceProximity(const String& eventType, double value, double min, double max, ExceptionCode&);

    enum {
        // Values need to be kept in sync with Internals.idl.
        LAYER_TREE_INCLUDES_VISIBLE_RECTS = 1,
        LAYER_TREE_INCLUDES_TILE_CACHES = 2,
        LAYER_TREE_INCLUDES_REPAINT_RECTS = 4,
        LAYER_TREE_INCLUDES_PAINTING_PHASES = 8,
        LAYER_TREE_INCLUDES_CONTENT_LAYERS = 16
    };
    String layerTreeAsText(Document*, unsigned flags, ExceptionCode&) const;
    String layerTreeAsText(Document*, ExceptionCode&) const;
    String repaintRectsAsText(ExceptionCode&) const;
    String scrollingStateTreeAsText(ExceptionCode&) const;
    String mainThreadScrollingReasons(ExceptionCode&) const;
    RefPtr<ClientRectList> nonFastScrollableRects(ExceptionCode&) const;

    void garbageCollectDocumentResources(ExceptionCode&) const;

    void allowRoundingHacks() const;

    void insertAuthorCSS(const String&, ExceptionCode&) const;
    void insertUserCSS(const String&, ExceptionCode&) const;

    unsigned numberOfLiveNodes() const;
    unsigned numberOfLiveDocuments() const;

    Vector<String> consoleMessageArgumentCounts() const;
    PassRefPtr<DOMWindow> openDummyInspectorFrontend(const String& url);
    void closeDummyInspectorFrontend();
    void setInspectorIsUnderTest(bool isUnderTest, ExceptionCode&);

    String counterValue(Element*);

    int pageNumber(Element*, float pageWidth = 800, float pageHeight = 600);
    Vector<String> shortcutIconURLs() const;

    int numberOfPages(float pageWidthInPixels = 800, float pageHeightInPixels = 600);
    String pageProperty(String, int, ExceptionCode& = ASSERT_NO_EXCEPTION) const;
    String pageSizeAndMarginsInPixels(int, int, int, int, int, int, int, ExceptionCode& = ASSERT_NO_EXCEPTION) const;

    void setPageScaleFactor(float scaleFactor, int x, int y, ExceptionCode&);
    void setPageZoomFactor(float zoomFactor, ExceptionCode&);

    void setUseFixedLayout(bool useFixedLayout, ExceptionCode&);
    void setFixedLayoutSize(int width, int height, ExceptionCode&);

    void setHeaderHeight(float);
    void setFooterHeight(float);

    void setTopContentInset(float);

#if ENABLE(FULLSCREEN_API)
    void webkitWillEnterFullScreenForElement(Element*);
    void webkitDidEnterFullScreenForElement(Element*);
    void webkitWillExitFullScreenForElement(Element*);
    void webkitDidExitFullScreenForElement(Element*);
#endif

    WEBCORE_TESTSUPPORT_EXPORT void setApplicationCacheOriginQuota(unsigned long long);

    void registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme);
    void removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme);

    Ref<MallocStatistics> mallocStatistics() const;
    Ref<TypeConversions> typeConversions() const;
    Ref<MemoryInfo> memoryInfo() const;

    Vector<String> getReferencedFilePaths() const;

    void startTrackingRepaints(ExceptionCode&);
    void stopTrackingRepaints(ExceptionCode&);

    void startTrackingLayerFlushes(ExceptionCode&);
    unsigned long layerFlushCount(ExceptionCode&);
    
    void startTrackingStyleRecalcs(ExceptionCode&);
    unsigned long styleRecalcCount(ExceptionCode&);

    void setPrinting(int width, int height);
    void startTrackingCompositingUpdates(ExceptionCode&);
    unsigned long compositingUpdateCount(ExceptionCode&);

    void updateLayoutIgnorePendingStylesheetsAndRunPostLayoutTasks(ExceptionCode&);
    void updateLayoutIgnorePendingStylesheetsAndRunPostLayoutTasks(Node*, ExceptionCode&);
    unsigned layoutCount() const;

    RefPtr<ArrayBuffer> serializeObject(PassRefPtr<SerializedScriptValue>) const;
    RefPtr<SerializedScriptValue> deserializeBuffer(PassRefPtr<ArrayBuffer>) const;

    bool isFromCurrentWorld(Deprecated::ScriptValue) const;

    void setUsesOverlayScrollbars(bool enabled);

    String getCurrentCursorInfo(ExceptionCode&);

    String markerTextForListItem(Element*, ExceptionCode&);

    String toolTipFromElement(Element*, ExceptionCode&) const;

    void forceReload(bool endToEnd);

#if ENABLE(ENCRYPTED_MEDIA_V2)
    void initializeMockCDM();
#endif

#if ENABLE(SPEECH_SYNTHESIS)
    void enableMockSpeechSynthesizer();
#endif

    void notifyResourceLoadObserver();

#if ENABLE(MEDIA_STREAM)
    void enableMockRTCPeerConnectionHandler();
#endif

    String getImageSourceURL(Element*, ExceptionCode&);

#if ENABLE(VIDEO)
    void simulateAudioInterruption(Node*);
    bool mediaElementHasCharacteristic(Node*, const String&, ExceptionCode&);
#endif

    bool isSelectPopupVisible(Node*);

    String captionsStyleSheetOverride(ExceptionCode&);
    void setCaptionsStyleSheetOverride(const String&, ExceptionCode&);
    void setPrimaryAudioTrackLanguageOverride(const String&, ExceptionCode&);
    void setCaptionDisplayMode(const String&, ExceptionCode&);

#if ENABLE(VIDEO)
    Ref<TimeRanges> createTimeRanges(Float32Array* startTimes, Float32Array* endTimes);
    double closestTimeToTimeRanges(double time, TimeRanges*);
#endif

    Ref<ClientRect> selectionBounds(ExceptionCode&);

#if ENABLE(VIBRATION)
    bool isVibrating();
#endif

    bool isPluginUnavailabilityIndicatorObscured(Element*, ExceptionCode&);
    bool isPluginSnapshotted(Element*, ExceptionCode&);

#if ENABLE(MEDIA_SOURCE)
    WEBCORE_TESTSUPPORT_EXPORT void initializeMockMediaSource();
    Vector<String> bufferedSamplesForTrackID(SourceBuffer*, const AtomicString&);
#endif

#if ENABLE(VIDEO)
    void beginMediaSessionInterruption(const String&, ExceptionCode&);
    void endMediaSessionInterruption(const String&);
    void applicationWillEnterForeground() const;
    void applicationWillEnterBackground() const;
    void setMediaSessionRestrictions(const String& mediaType, const String& restrictions, ExceptionCode&);
    void setMediaElementRestrictions(HTMLMediaElement*, const String& restrictions, ExceptionCode&);
    void postRemoteControlCommand(const String&, ExceptionCode&);
    void postRemoteControlCommand(const String&, float argument, ExceptionCode&);
    bool elementIsBlockingDisplaySleep(Element*) const;
#endif

#if ENABLE(WEB_AUDIO)
    void setAudioContextRestrictions(AudioContext*, const String& restrictions, ExceptionCode&);
#endif

    void simulateSystemSleep() const;
    void simulateSystemWake() const;

    void installMockPageOverlay(const String& overlayType, ExceptionCode&);
    String pageOverlayLayerTreeAsText(ExceptionCode&) const;

    void setPageMuted(bool);
    bool isPagePlayingAudio();

    RefPtr<File> createFile(const String&);
    void queueMicroTask(int);
    bool testPreloaderSettingViewport();

#if ENABLE(CONTENT_FILTERING)
    MockContentFilterSettings& mockContentFilterSettings();
#endif

#if ENABLE(CSS_SCROLL_SNAP)
    String scrollSnapOffsets(Element*, ExceptionCode&);
#endif

    PassRefPtr<DOMPath> pathWithShrinkWrappedRects(Vector<double> rectComponents, double radius, ExceptionCode&);

private:
    explicit Internals(Document*);
    Document* contextDocument() const;
    Frame* frame() const;

    RenderedDocumentMarker* markerAt(Node*, const String& markerType, unsigned index, ExceptionCode&);

    RefPtr<DOMWindow> m_frontendWindow;
    std::unique_ptr<InspectorFrontendClientDummy> m_frontendClient;
    std::unique_ptr<InspectorFrontendChannelDummy> m_frontendChannel;
};

} // namespace WebCore

#endif
