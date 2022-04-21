/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WKCSetting_h
#define WKCSetting_h

#include <stddef.h>
#include "helpers/WKCString.h"

#include "helpers/WKCHelpersEnums.h"

namespace WKC {

class WKCWebViewPrivate;

class WKC_API WKCSettings {
public:
    // same as WebCore::Settings
    enum EditableLinkBehavior {
        EditableLinkDefaultBehavior,
        EditableLinkAlwaysLive,
        EditableLinkOnlyLiveWithShiftKey,
        EditableLinkLiveWhenNotFocused,
        EditableLinkNeverLive
    };
    enum TextDirectionSubmenuInclusionBehavior {
        TextDirectionSubmenuNeverIncluded,
        TextDirectionSubmenuAutomaticallyIncluded,
        TextDirectionSubmenuAlwaysIncluded
    };
    enum EditingBehaviorType { EditingMacBehavior, EditingWindowsBehavior, EditingUnixBehavior };

    // from FontRenderingMode.h
    enum FontRenderingMode { NormalRenderingMode, AlternateRenderingMode };

    void setStandardFontFamily(const char*);
    const char* standardFontFamily();

    void setFixedFontFamily(const char*);
    const char* fixedFontFamily();

    void setSerifFontFamily(const char*);
    const char* serifFontFamily();

    void setSansSerifFontFamily(const char*);
    const char* sansSerifFontFamily();

    void setCursiveFontFamily(const char*);
    const char* cursiveFontFamily();

    void setFantasyFontFamily(const char*);
    const char* fantasyFontFamily();

    void setMinimumFontSize(int);
    int minimumFontSize() const;

    void setMinimumLogicalFontSize(int);
    int minimumLogicalFontSize() const;

    void setDefaultFontSize(int);
    int defaultFontSize() const;

    void setDefaultFixedFontSize(int);
    int defaultFixedFontSize() const;

    void setLoadsImagesAutomatically(bool);
    bool loadsImagesAutomatically() const;

    void setScriptEnabled(bool);
    bool isScriptEnabled() const;

    void setWebSecurityEnabled(bool);
    bool isWebSecurityEnabled() const;

    void setAllowUniversalAccessFromFileURLs(bool);
    bool allowUniversalAccessFromFileURLs() const;

    void setJavaScriptCanOpenWindowsAutomatically(bool);
    bool javaScriptCanOpenWindowsAutomatically() const;

    void setJavaScriptCanAccessClipboard(bool);
    bool javaScriptCanAccessClipboard() const;

    void setJavaEnabled(bool);
    bool isJavaEnabled() const;

    void setPluginsEnabled(bool);
    bool arePluginsEnabled() const;

    void setLocalStorageEnabled(bool);
    bool localStorageEnabled() const;

    void setSessionStorageQuota(unsigned);
    unsigned sessionStorageQuota() const;

    void setPrivateBrowsingEnabled(bool);
    bool privateBrowsingEnabled() const;

    void setCaretBrowsingEnabled(bool);
    bool caretBrowsingEnabled() const;

    void setDefaultTextEncodingName(const char*);
    const char* defaultTextEncodingName();
        
    void setUsesEncodingDetector(bool);
    bool usesEncodingDetector() const;

    void setUserStyleSheetLocation(const char*);
    const char* userStyleSheetLocation();

    void setShouldPrintBackgrounds(bool);
    bool shouldPrintBackgrounds() const;

    void setTextAreasAreResizable(bool);
    bool textAreasAreResizable() const;

    void setEditableLinkBehavior(EditableLinkBehavior);
    EditableLinkBehavior editableLinkBehavior();

    void setTextDirectionSubmenuInclusionBehavior(TextDirectionSubmenuInclusionBehavior);
    TextDirectionSubmenuInclusionBehavior textDirectionSubmenuInclusionBehavior() const;
        
    void setNeedsAdobeFrameReloadingQuirk(bool);
    bool needsAcrobatFrameReloadingQuirk() const;

    void setNeedsKeyboardEventDisambiguationQuirks(bool);
    bool needsKeyboardEventDisambiguationQuirks() const;

    void setTreatsAnyTextCSSLinkAsStylesheet(bool);
    bool treatsAnyTextCSSLinkAsStylesheet() const;

    void setDOMPasteAllowed(bool);
    bool isDOMPasteAllowed() const;
        
    void setUsesBackForwardCache(bool);
    bool usesBackForwardCache() const;

    void setShrinksStandaloneImagesToFit(bool);
    bool shrinksStandaloneImagesToFit() const;

    void setShowsURLsInToolTips(bool);
    bool showsURLsInToolTips() const;

    void setFTPDirectoryTemplatePath(const char*);
    const char* ftpDirectoryTemplatePath();
        
    void setForceFTPDirectoryListings(bool);
    bool forceFTPDirectoryListings() const;
        
    void setDeveloperExtrasEnabled(bool);
    bool developerExtrasEnabled() const;

    void setFrameFlattening(FrameFlattening);
    FrameFlattening frameFlattening() const;

    void setAuthorAndUserStylesEnabled(bool);
    bool authorAndUserStylesEnabled() const;
        
    void setFontRenderingMode(FontRenderingMode mode);
    FontRenderingMode fontRenderingMode() const;

    void setNeedsSiteSpecificQuirks(bool);
    bool needsSiteSpecificQuirks() const;
        
    void setWebArchiveDebugModeEnabled(bool);
    bool webArchiveDebugModeEnabled() const;

    void setLocalFileContentSniffingEnabled(bool);
    bool localFileContentSniffingEnabled() const;

    void setLocalStorageDatabasePath(const char*);
    const char* localStorageDatabasePath();
        
    void setApplicationChromeMode(bool);
    bool inApplicationChromeMode() const;

    void setOfflineWebApplicationCacheEnabled(bool);
    bool offlineWebApplicationCacheEnabled() const;

    void setEnforceCSSMIMETypeInNoQuirksMode(bool);
    bool enforceCSSMIMETypeInNoQuirksMode();

    void setMaximumDecodedImageSize(size_t size);
    size_t maximumDecodedImageSize() const;

    void setEditingBehaviorType(EditingBehaviorType behavior);
    EditingBehaviorType editingBehaviorType() const;
        
    void setDownloadableBinaryFontsEnabled(bool);
    bool downloadableBinaryFontsEnabled() const;

    void setXSSAuditorEnabled(bool);
    bool xssAuditorEnabled() const;

    void setCanvasUsesAcceleratedDrawing(bool);
    bool canvasUsesAcceleratedDrawing() const;

    void setAcceleratedDrawingEnabled(bool);
    bool acceleratedDrawingEnabled() const;

    void setAcceleratedCompositingEnabled(bool);
    bool acceleratedCompositingEnabled() const;

    void setAcceleratedCompositingForFixedPositionEnabled(bool);
    bool acceleratedCompositingForFixedPositionEnabled() const;

    void setShowDebugBorders(bool);
    bool showDebugBorders() const;

    void setShowRepaintCounter(bool);
    bool showRepaintCounter() const;

    void setExperimentalNotificationsEnabled(bool);
    bool experimentalNotificationsEnabled() const;

    void setWebAudioEnabled(bool);
    bool webAudioEnabled() const;

    void setWebGLEnabled(bool);
    bool webGLEnabled() const;

    void setAccelerated2dCanvasEnabled(bool);
    bool accelerated2dCanvasEnabled() const;

    void setFullScreenEnabled(bool);
    bool fullScreenEnabled() const;

    void setForceCompositingMode(bool);
    bool forceCompositingMode();

    void setShouldDisplaySubtitles(bool);
    bool shouldDisplaySubtitles() const;

    void setShouldDisplayCaptions(bool);
    bool shouldDisplayCaptions() const;

    void setShouldDisplayTextDescriptions(bool);
    bool shouldDisplayTextDescriptions() const;

    void setInteractiveFormValidationEnabled(bool);
    bool interactiveFormValidationEnabled() const;

    void setHiddenPageDOMTimerThrottlingEnabled(bool);
    bool hiddenPageDOMTimerThrottlingEnabled() const;

    void setAllowDisplayOfInsecureContent(bool);
    bool allowDisplayOfInsecureContent() const;

    void setAllowRunningOfInsecureContent(bool);
    bool allowRunningOfInsecureContent() const;

    void setVideoPlaybackRequiresUserGesture(bool);
    bool videoPlaybackRequiresUserGesture() const;

    void setAnimatedImageAsyncDecodingEnabled(bool);
    bool animatedImageAsyncDecodingEnabled() const;

    void setLargeImageAsyncDecodingEnabled(bool);
    bool largeImageAsyncDecodingEnabled() const;

    void setMaximumSourceBufferSize(int);
    int maximumSourceBufferSize() const;
    void setMediaSourceEnabled(bool);
    bool mediaSourceEnabled() const;

private:
    friend class WKCWebViewPrivate;
    WKCSettings(WKC::WKCWebViewPrivate*);
    ~WKCSettings();

    WKCSettings(const WKCSettings&);
    WKCSettings& operator=(const WKCSettings&);

    void* m_private;
    WKC::WKCWebViewPrivate* m_parent;

    String m_standardFontFamily;
    String m_fixedFontFamily;
    String m_serifFontFamily;
    String m_sansSerifFontFamily;
    String m_cursiveFontFamily;
    String m_fantasyFontFamily;
    String m_defaultTextEncodingName;
    String m_userStyleSheetLocation;
    String m_ftpDirectoryTemplatePath;
    String m_localStorageDatabasePath;
};

class WKC_API WKCGlobalSettings {
private:
    WKCGlobalSettings();
    ~WKCGlobalSettings();
    static WKCGlobalSettings* create();
    bool construct();

    WKCGlobalSettings(const WKCGlobalSettings&);
    WKCGlobalSettings& operator=(const WKCGlobalSettings&);

public:
    static bool createSharedInstance(bool autoflag = false);
    static void deleteSharedInstance();
    static bool isExistSharedInstance();
    static bool isAutomatic();
};

}   // namespace

#endif  /* WKCSetting_h */
