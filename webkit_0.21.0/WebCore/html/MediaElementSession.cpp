/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO)

#include "MediaElementSession.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#include "Logging.h"
#include "Page.h"
#include "PlatformMediaSessionManager.h"
#include "ScriptController.h"
#include "SourceBuffer.h"

#if PLATFORM(IOS)
#include "AudioSession.h"
#include "RuntimeApplicationChecksIOS.h"
#endif

namespace WebCore {

#if !LOG_DISABLED
static String restrictionName(MediaElementSession::BehaviorRestrictions restriction)
{
    StringBuilder restrictionBuilder;
#define CASE(restrictionType) \
    if (restriction & MediaElementSession::restrictionType) { \
        if (!restrictionBuilder.isEmpty()) \
            restrictionBuilder.append(", "); \
        restrictionBuilder.append(#restrictionType); \
    } \

    CASE(NoRestrictions);
    CASE(RequireUserGestureForLoad);
    CASE(RequireUserGestureForRateChange);
    CASE(RequireUserGestureForAudioRateChange);
    CASE(RequireUserGestureForFullscreen);
    CASE(RequirePageConsentToLoadMedia);
    CASE(RequirePageConsentToResumeMedia);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    CASE(RequireUserGestureToShowPlaybackTargetPicker);
    CASE(WirelessVideoPlaybackDisabled);
#endif
    CASE(RequireUserGestureForAudioRateChange);
    CASE(InvisibleAutoplayNotPermitted);

    return restrictionBuilder.toString();
}
#endif

static bool pageExplicitlyAllowsElementToAutoplayInline(const HTMLMediaElement& element)
{
    Document& document = element.document();
    Page* page = document.page();
    return document.isMediaDocument() && !document.ownerElement() && page && page->allowsMediaDocumentInlinePlayback();
}

MediaElementSession::MediaElementSession(PlatformMediaSessionClient& client)
    : PlatformMediaSession(client)
    , m_restrictions(NoRestrictions)
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , m_targetAvailabilityChangedTimer(*this, &MediaElementSession::targetAvailabilityChangedTimerFired)
#endif
{
}

void MediaElementSession::registerWithDocument(Document& document)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    document.addPlaybackTargetPickerClient(*this);
#else
    UNUSED_PARAM(document);
#endif
}

void MediaElementSession::unregisterWithDocument(Document& document)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    document.removePlaybackTargetPickerClient(*this);
#else
    UNUSED_PARAM(document);
#endif
}

void MediaElementSession::addBehaviorRestriction(BehaviorRestrictions restriction)
{
    LOG(Media, "MediaElementSession::addBehaviorRestriction - adding %s", restrictionName(restriction).utf8().data());
    m_restrictions |= restriction;
}

void MediaElementSession::removeBehaviorRestriction(BehaviorRestrictions restriction)
{
    LOG(Media, "MediaElementSession::removeBehaviorRestriction - removing %s", restrictionName(restriction).utf8().data());
    m_restrictions &= ~restriction;
}

#if PLATFORM(MAC)
static bool needsDocumentLevelMediaUserGestureQuirk(Document& document)
{
    if (!document.settings().needsSiteSpecificQuirks())
        return false;

    String host = document.topDocument().url().host();
    if (equalLettersIgnoringASCIICase(host, "slate.com") || host.endsWithIgnoringASCIICase(".slate.com"))
        return true;

    if (equalLettersIgnoringASCIICase(host, "ign.com") || host.endsWithIgnoringASCIICase(".ign.com"))
        return true;

    if (equalLettersIgnoringASCIICase(host, "washingtonpost.com") || host.endsWithIgnoringASCIICase(".washingtonpost.com"))
        return true;

    return false;
}
#endif // PLATFORM(MAC)

bool MediaElementSession::playbackPermitted(const HTMLMediaElement& element) const
{
    if (pageExplicitlyAllowsElementToAutoplayInline(element))
        return true;

#if PLATFORM(MAC)
    if (element.document().topDocument().mediaState() & MediaProducer::HasUserInteractedWithMediaElement && needsDocumentLevelMediaUserGestureQuirk(element.document()))
        return{};
#endif

    if (m_restrictions & RequireUserGestureForRateChange && !ScriptController::processingUserGesture()) {
        LOG(Media, "MediaElementSession::playbackPermitted - returning FALSE");
        return false;
    }

    if (m_restrictions & RequireUserGestureForAudioRateChange && element.hasAudio() && !ScriptController::processingUserGesture()) {
        LOG(Media, "MediaElementSession::playbackPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::dataLoadingPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForLoad && !ScriptController::processingUserGesture()) {
        LOG(Media, "MediaElementSession::dataLoadingPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::fullscreenPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForFullscreen && !ScriptController::processingUserGesture()) {
        LOG(Media, "MediaElementSession::fullscreenPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::pageAllowsDataLoading(const HTMLMediaElement& element) const
{
    Page* page = element.document().page();
    if (m_restrictions & RequirePageConsentToLoadMedia && page && !page->canStartMedia()) {
        LOG(Media, "MediaElementSession::pageAllowsDataLoading - returning FALSE");
        return false;
    }

    return true;
}

bool MediaElementSession::pageAllowsPlaybackAfterResuming(const HTMLMediaElement& element) const
{
    Page* page = element.document().page();
    if (m_restrictions & RequirePageConsentToResumeMedia && page && !page->canStartMedia()) {
        LOG(Media, "MediaElementSession::pageAllowsPlaybackAfterResuming - returning FALSE");
        return false;
    }

    return true;
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void MediaElementSession::showPlaybackTargetPicker(const HTMLMediaElement& element)
{
    LOG(Media, "MediaElementSession::showPlaybackTargetPicker");

    if (m_restrictions & RequireUserGestureToShowPlaybackTargetPicker && !ScriptController::processingUserGesture()) {
        LOG(Media, "MediaElementSession::showPlaybackTargetPicker - returning early because of permissions");
        return;
    }

    if (!element.document().page()) {
        LOG(Media, "MediaElementSession::showingPlaybackTargetPickerPermitted - returning early because page is NULL");
        return;
    }

#if !PLATFORM(IOS)
    if (element.readyState() < HTMLMediaElementEnums::HAVE_METADATA) {
        LOG(Media, "MediaElementSession::showPlaybackTargetPicker - returning early because element is not playable");
        return;
    }
#endif

    element.document().showPlaybackTargetPicker(*this, is<HTMLVideoElement>(element));
}

bool MediaElementSession::hasWirelessPlaybackTargets(const HTMLMediaElement&) const
{
#if PLATFORM(IOS)
    // FIXME: consolidate Mac and iOS implementations
    m_hasPlaybackTargets = PlatformMediaSessionManager::sharedManager().hasWirelessTargetsAvailable();
#endif

    LOG(Media, "MediaElementSession::hasWirelessPlaybackTargets - returning %s", m_hasPlaybackTargets ? "TRUE" : "FALSE");

    return m_hasPlaybackTargets;
}

bool MediaElementSession::wirelessVideoPlaybackDisabled(const HTMLMediaElement& element) const
{
    Settings* settings = element.document().settings();
    if (!settings || !settings->allowsAirPlayForMediaPlayback()) {
        LOG(Media, "MediaElementSession::wirelessVideoPlaybackDisabled - returning TRUE because of settings");
        return true;
    }

    if (element.fastHasAttribute(HTMLNames::webkitwirelessvideoplaybackdisabledAttr)) {
        LOG(Media, "MediaElementSession::wirelessVideoPlaybackDisabled - returning TRUE because of attribute");
        return true;
    }

#if PLATFORM(IOS)
    String legacyAirplayAttributeValue = element.fastGetAttribute(HTMLNames::webkitairplayAttr);
    if (equalIgnoringCase(legacyAirplayAttributeValue, "deny")) {
        LOG(Media, "MediaElementSession::wirelessVideoPlaybackDisabled - returning TRUE because of legacy attribute");
        return true;
    }
    if (equalIgnoringCase(legacyAirplayAttributeValue, "allow")) {
        LOG(Media, "MediaElementSession::wirelessVideoPlaybackDisabled - returning FALSE because of legacy attribute");
        return false;
    }
#endif

    MediaPlayer* player = element.player();
    if (!player)
        return true;

    bool disabled = player->wirelessVideoPlaybackDisabled();
    LOG(Media, "MediaElementSession::wirelessVideoPlaybackDisabled - returning %s because media engine says so", disabled ? "TRUE" : "FALSE");
    
    return disabled;
}

void MediaElementSession::setWirelessVideoPlaybackDisabled(const HTMLMediaElement& element, bool disabled)
{
    if (disabled)
        addBehaviorRestriction(WirelessVideoPlaybackDisabled);
    else
        removeBehaviorRestriction(WirelessVideoPlaybackDisabled);

    MediaPlayer* player = element.player();
    if (!player)
        return;

    LOG(Media, "MediaElementSession::setWirelessVideoPlaybackDisabled - disabled %s", disabled ? "TRUE" : "FALSE");
    player->setWirelessVideoPlaybackDisabled(disabled);
}

void MediaElementSession::setHasPlaybackTargetAvailabilityListeners(const HTMLMediaElement& element, bool hasListeners)
{
    LOG(Media, "MediaElementSession::setHasPlaybackTargetAvailabilityListeners - hasListeners %s", hasListeners ? "TRUE" : "FALSE");

#if PLATFORM(IOS)
    UNUSED_PARAM(element);
    m_hasPlaybackTargetAvailabilityListeners = hasListeners;
    PlatformMediaSessionManager::sharedManager().configureWireLessTargetMonitoring();
#else
    UNUSED_PARAM(hasListeners);
    element.document().playbackTargetPickerClientStateDidChange(*this, element.mediaState());
#endif
}

void MediaElementSession::setPlaybackTarget(Ref<MediaPlaybackTarget>&& device)
{
    m_playbackTarget = WTF::move(device);
    client().setWirelessPlaybackTarget(*m_playbackTarget.copyRef());
}

void MediaElementSession::targetAvailabilityChangedTimerFired()
{
    client().wirelessRoutesAvailableDidChange();
}

void MediaElementSession::externalOutputDeviceAvailableDidChange(bool hasTargets)
{
    if (m_hasPlaybackTargets == hasTargets)
        return;

    LOG(Media, "MediaElementSession::externalOutputDeviceAvailableDidChange(%p) - hasTargets %s", this, hasTargets ? "TRUE" : "FALSE");

    m_hasPlaybackTargets = hasTargets;
    m_targetAvailabilityChangedTimer.startOneShot(0);
}

bool MediaElementSession::canPlayToWirelessPlaybackTarget() const
{
    if (!m_playbackTarget || !m_playbackTarget->hasActiveRoute())
        return false;

    return client().canPlayToWirelessPlaybackTarget();
}

bool MediaElementSession::isPlayingToWirelessPlaybackTarget() const
{
    if (!m_playbackTarget || !m_playbackTarget->hasActiveRoute())
        return false;

    return client().isPlayingToWirelessPlaybackTarget();
}

void MediaElementSession::setShouldPlayToPlaybackTarget(bool shouldPlay)
{
    LOG(Media, "MediaElementSession::setShouldPlayToPlaybackTarget - shouldPlay %s", shouldPlay ? "TRUE" : "FALSE");
    m_shouldPlayToPlaybackTarget = shouldPlay;
    client().setShouldPlayToPlaybackTarget(shouldPlay);
}

void MediaElementSession::mediaStateDidChange(const HTMLMediaElement& element, MediaProducer::MediaStateFlags state)
{
    element.document().playbackTargetPickerClientStateDidChange(*this, state);
}
#endif

MediaPlayer::Preload MediaElementSession::effectivePreloadForElement(const HTMLMediaElement& element) const
{
    PlatformMediaSessionManager::SessionRestrictions restrictions = PlatformMediaSessionManager::sharedManager().restrictions(mediaType());
    MediaPlayer::Preload preload = element.preloadValue();

    if (pageExplicitlyAllowsElementToAutoplayInline(element))
        return preload;

    if ((restrictions & PlatformMediaSessionManager::MetadataPreloadingNotPermitted) == PlatformMediaSessionManager::MetadataPreloadingNotPermitted)
        return MediaPlayer::None;

    if ((restrictions & PlatformMediaSessionManager::AutoPreloadingNotPermitted) == PlatformMediaSessionManager::AutoPreloadingNotPermitted) {
        if (preload > MediaPlayer::MetaData)
            return MediaPlayer::MetaData;
    }

    return preload;
}

bool MediaElementSession::requiresFullscreenForVideoPlayback(const HTMLMediaElement& element) const
{
    if (pageExplicitlyAllowsElementToAutoplayInline(element))
        return false;

    if (!PlatformMediaSessionManager::sharedManager().sessionRestrictsInlineVideoPlayback(*this))
        return false;

    Settings* settings = element.document().settings();
    if (!settings || !settings->allowsInlineMediaPlayback())
        return true;

    if (element.fastHasAttribute(HTMLNames::webkit_playsinlineAttr))
        return false;

#if PLATFORM(IOS)
    if (applicationIsDumpRenderTree())
        return false;
#endif

    return true;
}

bool MediaElementSession::allowsAutomaticMediaDataLoading(const HTMLMediaElement& element) const
{
    if (pageExplicitlyAllowsElementToAutoplayInline(element))
        return true;

    Settings* settings = element.document().settings();
    if (settings && settings->mediaDataLoadsAutomatically())
        return true;

    return false;
}

void MediaElementSession::mediaEngineUpdated(const HTMLMediaElement& element)
{
    LOG(Media, "MediaElementSession::mediaEngineUpdated");

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_restrictions & WirelessVideoPlaybackDisabled)
        setWirelessVideoPlaybackDisabled(element, true);
    if (m_playbackTarget)
        client().setWirelessPlaybackTarget(*m_playbackTarget.copyRef());
    if (m_shouldPlayToPlaybackTarget)
        client().setShouldPlayToPlaybackTarget(true);
#else
    UNUSED_PARAM(element);
#endif
    
}

bool MediaElementSession::allowsPictureInPicture(const HTMLMediaElement& element) const
{
    Settings* settings = element.document().settings();
    return settings && settings->allowsPictureInPictureMediaPlayback() && !element.webkitCurrentPlaybackTargetIsWireless();
}

#if PLATFORM(IOS)
bool MediaElementSession::requiresPlaybackTargetRouteMonitoring() const
{
    return m_hasPlaybackTargetAvailabilityListeners && !client().elementIsHidden();
}
#endif

#if ENABLE(MEDIA_SOURCE)
const unsigned fiveMinutesOf1080PVideo = 290 * 1024 * 1024; // 290 MB is approximately 5 minutes of 8Mbps (1080p) content.
const unsigned fiveMinutesStereoAudio = 14 * 1024 * 1024; // 14 MB is approximately 5 minutes of 384kbps content.

size_t MediaElementSession::maximumMediaSourceBufferSize(const SourceBuffer& buffer) const
{
    // A good quality 1080p video uses 8,000 kbps and stereo audio uses 384 kbps, so assume 95% for video and 5% for audio.
    const float bufferBudgetPercentageForVideo = .95;
    const float bufferBudgetPercentageForAudio = .05;

    size_t maximum;
    Settings* settings = buffer.document().settings();
    if (settings)
        maximum = settings->maximumSourceBufferSize();
    else
        maximum = fiveMinutesOf1080PVideo + fiveMinutesStereoAudio;

    // Allow a SourceBuffer to buffer as though it is audio-only even if it doesn't have any active tracks (yet).
    size_t bufferSize = static_cast<size_t>(maximum * bufferBudgetPercentageForAudio);
    if (buffer.hasVideo())
        bufferSize += static_cast<size_t>(maximum * bufferBudgetPercentageForVideo);

    // FIXME: we might want to modify this algorithm to:
    // - decrease the maximum size for background tabs
    // - decrease the maximum size allowed for inactive elements when a process has more than one
    //   element, eg. so a page with many elements which are played one at a time doesn't keep
    //   everything buffered after an element has finished playing.

    return bufferSize;
}
#endif

}

#endif // ENABLE(VIDEO)
