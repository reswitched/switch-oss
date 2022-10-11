/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef InternalSettings_h
#define InternalSettings_h

// FIXME (121927): This include should not be needed.
#include <wtf/text/AtomicStringHash.h>

#include "EditingBehaviorTypes.h"
#include "FontGenericFamilies.h"
#include "IntSize.h"
#include "InternalSettingsGenerated.h"
#include "SecurityOrigin.h"

namespace WebCore {

typedef int ExceptionCode;

class Frame;
class Document;
class Page;
class Settings;

class InternalSettings : public InternalSettingsGenerated {
public:
    class Backup {
    public:
        explicit Backup(Settings&);
        void restoreTo(Settings&);

        bool m_originalCSSShapesEnabled;
        EditingBehaviorType m_originalEditingBehavior;

        // Initially empty, only used if changed by a test.
        ScriptFontFamilyMap m_standardFontFamilies;
        ScriptFontFamilyMap m_fixedFontFamilies;
        ScriptFontFamilyMap m_serifFontFamilies;
        ScriptFontFamilyMap m_sansSerifFontFamilies;
        ScriptFontFamilyMap m_cursiveFontFamilies;
        ScriptFontFamilyMap m_fantasyFontFamilies;
        ScriptFontFamilyMap m_pictographFontFamilies;

#if ENABLE(TEXT_AUTOSIZING)
        bool m_originalTextAutosizingEnabled;
        IntSize m_originalTextAutosizingWindowSizeOverride;
        float m_originalTextAutosizingFontScaleFactor;
#endif
        String m_originalMediaTypeOverride;
        bool m_originalCanvasUsesAcceleratedDrawing;
        bool m_originalMockScrollbarsEnabled;
        bool m_originalUsesOverlayScrollbars;
        bool m_langAttributeAwareFormControlUIEnabled;
        bool m_imagesEnabled;
        double m_minimumTimerInterval;
#if ENABLE(VIDEO_TRACK)
        bool m_shouldDisplaySubtitles;
        bool m_shouldDisplayCaptions;
        bool m_shouldDisplayTextDescriptions;
#endif
        String m_defaultVideoPosterURL;
        bool m_forcePendingWebGLPolicy;
        bool m_originalTimeWithoutMouseMovementBeforeHidingControls;
        bool m_useLegacyBackgroundSizeShorthandBehavior;
        bool m_autoscrollForDragAndDropEnabled;
        bool m_quickTimePluginReplacementEnabled;
        bool m_youTubeFlashPluginReplacementEnabled;
        bool m_shouldConvertPositionStyleOnCopy;
        bool m_fontFallbackPrefersPictographs;
        bool m_backgroundShouldExtendBeyondPage;
        SecurityOrigin::StorageBlockingPolicy m_storageBlockingPolicy;
        bool m_scrollingTreeIncludesFrames;
#if ENABLE(TOUCH_EVENTS)
        bool m_touchEventEmulationEnabled;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        bool m_allowsAirPlayForMediaPlayback;
#endif
    };

    static Ref<InternalSettings> create(Page* page)
    {
        return adoptRef(*new InternalSettings(page));
    }
    static InternalSettings* from(Page*);
    void hostDestroyed() { m_page = 0; }

    virtual ~InternalSettings();
    void resetToConsistentState();

    void setUsesOverlayScrollbars(bool, ExceptionCode&);
    void setTouchEventEmulationEnabled(bool, ExceptionCode&);
    void setStandardFontFamily(const String& family, const String& script, ExceptionCode&);
    void setSerifFontFamily(const String& family, const String& script, ExceptionCode&);
    void setSansSerifFontFamily(const String& family, const String& script, ExceptionCode&);
    void setFixedFontFamily(const String& family, const String& script, ExceptionCode&);
    void setCursiveFontFamily(const String& family, const String& script, ExceptionCode&);
    void setFantasyFontFamily(const String& family, const String& script, ExceptionCode&);
    void setPictographFontFamily(const String& family, const String& script, ExceptionCode&);
    void setTextAutosizingEnabled(bool enabled, ExceptionCode&);
    void setTextAutosizingWindowSizeOverride(int width, int height, ExceptionCode&);
    void setTextAutosizingFontScaleFactor(float fontScaleFactor, ExceptionCode&);
    void setMediaTypeOverride(const String& mediaType, ExceptionCode&);
    void setCSSShapesEnabled(bool, ExceptionCode&);
    void setCanStartMedia(bool, ExceptionCode&);
    void setWirelessPlaybackDisabled(bool);
    void setEditingBehavior(const String&, ExceptionCode&);
    void setShouldDisplayTrackKind(const String& kind, bool enabled, ExceptionCode&);
    bool shouldDisplayTrackKind(const String& kind, ExceptionCode&);
    void setStorageBlockingPolicy(const String&, ExceptionCode&);
    void setLangAttributeAwareFormControlUIEnabled(bool);
    void setImagesEnabled(bool, ExceptionCode&);
    void setMinimumTimerInterval(double intervalInSeconds, ExceptionCode&);
    void setDefaultVideoPosterURL(const String& url, ExceptionCode&);
    void setForcePendingWebGLPolicy(bool, ExceptionCode&);
    void setTimeWithoutMouseMovementBeforeHidingControls(double time, ExceptionCode&);
    void setUseLegacyBackgroundSizeShorthandBehavior(bool, ExceptionCode&);
    void setAutoscrollForDragAndDropEnabled(bool, ExceptionCode&);
    void setFontFallbackPrefersPictographs(bool, ExceptionCode&);
    void setQuickTimePluginReplacementEnabled(bool, ExceptionCode&);
    void setYouTubeFlashPluginReplacementEnabled(bool, ExceptionCode&);
    void setBackgroundShouldExtendBeyondPage(bool, ExceptionCode&);
    void setShouldConvertPositionStyleOnCopy(bool, ExceptionCode&);
    void setScrollingTreeIncludesFrames(bool, ExceptionCode&);

private:
    explicit InternalSettings(Page*);

    Settings* settings() const;
    Page* page() const { return m_page; }
    static const char* supplementName();

    Page* m_page;
    Backup m_backup;
};

} // namespace WebCore

#endif
