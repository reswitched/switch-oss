/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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

#if !PLATFORM(WKC)
#if ENABLE(VIDEO)
#include "MediaControlsApple.h"

#include "CSSValueKeywords.h"
#include "ExceptionCodePlaceholder.h"
#include "HTMLNames.h"
#include "WheelEvent.h"

namespace WebCore {

MediaControlsApple::MediaControlsApple(Document& document)
    : MediaControls(document)
    , m_rewindButton(0)
    , m_returnToRealTimeButton(0)
    , m_statusDisplay(0)
    , m_timeRemainingDisplay(0)
    , m_timelineContainer(0)
    , m_seekBackButton(0)
    , m_seekForwardButton(0)
    , m_closedCaptionsTrackList(0)
    , m_closedCaptionsContainer(0)
    , m_volumeSliderMuteButton(0)
    , m_volumeSliderContainer(0)
    , m_fullScreenMinVolumeButton(0)
    , m_fullScreenVolumeSlider(0)
    , m_fullScreenMaxVolumeButton(0)
{
}

PassRefPtr<MediaControls> MediaControls::create(Document& document)
{
    return MediaControlsApple::createControls(document);
}

PassRefPtr<MediaControlsApple> MediaControlsApple::createControls(Document& document)
{
    if (!document.page())
        return nullptr;

    RefPtr<MediaControlsApple> controls = adoptRef(new MediaControlsApple(document));

    Ref<MediaControlPanelElement> panel = MediaControlPanelElement::create(document);

    ExceptionCode ec;

    Ref<MediaControlRewindButtonElement> rewindButton = MediaControlRewindButtonElement::create(document);
    controls->m_rewindButton = rewindButton.ptr();
    panel->appendChild(WTF::move(rewindButton), ec);
    if (ec)
        return nullptr;

    Ref<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(document);
    controls->m_playButton = playButton.ptr();
    panel->appendChild(WTF::move(playButton), ec);
    if (ec)
        return nullptr;

    Ref<MediaControlReturnToRealtimeButtonElement> returnToRealtimeButton = MediaControlReturnToRealtimeButtonElement::create(document);
    controls->m_returnToRealTimeButton = returnToRealtimeButton.ptr();
    panel->appendChild(WTF::move(returnToRealtimeButton), ec);
    if (ec)
        return nullptr;

    if (document.page()->theme().usesMediaControlStatusDisplay()) {
        Ref<MediaControlStatusDisplayElement> statusDisplay = MediaControlStatusDisplayElement::create(document);
        controls->m_statusDisplay = statusDisplay.ptr();
        panel->appendChild(WTF::move(statusDisplay), ec);
        if (ec)
            return nullptr;
    }

    Ref<MediaControlTimelineContainerElement> timelineContainer = MediaControlTimelineContainerElement::create(document);

    Ref<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(document);
    controls->m_currentTimeDisplay = currentTimeDisplay.ptr();
    timelineContainer->appendChild(WTF::move(currentTimeDisplay), ec);
    if (ec)
        return nullptr;

    Ref<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(document, controls.get());
    controls->m_timeline = timeline.ptr();
    timelineContainer->appendChild(WTF::move(timeline), ec);
    if (ec)
        return nullptr;

    Ref<MediaControlTimeRemainingDisplayElement> timeRemainingDisplay = MediaControlTimeRemainingDisplayElement::create(document);
    controls->m_timeRemainingDisplay = timeRemainingDisplay.ptr();
    timelineContainer->appendChild(WTF::move(timeRemainingDisplay), ec);
    if (ec)
        return nullptr;

    controls->m_timelineContainer = timelineContainer.ptr();
    panel->appendChild(WTF::move(timelineContainer), ec);
    if (ec)
        return nullptr;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    Ref<MediaControlSeekBackButtonElement> seekBackButton = MediaControlSeekBackButtonElement::create(document);
    controls->m_seekBackButton = seekBackButton.ptr();
    panel->appendChild(WTF::move(seekBackButton), ec);
    if (ec)
        return nullptr;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    Ref<MediaControlSeekForwardButtonElement> seekForwardButton = MediaControlSeekForwardButtonElement::create(document);
    controls->m_seekForwardButton = seekForwardButton.ptr();
    panel->appendChild(WTF::move(seekForwardButton), ec);
    if (ec)
        return nullptr;

    if (document.page()->theme().supportsClosedCaptioning()) {
        Ref<MediaControlClosedCaptionsContainerElement> closedCaptionsContainer = MediaControlClosedCaptionsContainerElement::create(document);

        Ref<MediaControlClosedCaptionsTrackListElement> closedCaptionsTrackList = MediaControlClosedCaptionsTrackListElement::create(document, controls.get());
        controls->m_closedCaptionsTrackList = closedCaptionsTrackList.ptr();
        closedCaptionsContainer->appendChild(WTF::move(closedCaptionsTrackList), ec);
        if (ec)
            return nullptr;

        Ref<MediaControlToggleClosedCaptionsButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsButtonElement::create(document, controls.get());
        controls->m_toggleClosedCaptionsButton = toggleClosedCaptionsButton.ptr();
        panel->appendChild(WTF::move(toggleClosedCaptionsButton), ec);
        if (ec)
            return nullptr;

        controls->m_closedCaptionsContainer = closedCaptionsContainer.ptr();
        controls->appendChild(WTF::move(closedCaptionsContainer), ec);
        if (ec)
            return nullptr;
    }

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    Ref<MediaControlFullscreenButtonElement> fullScreenButton = MediaControlFullscreenButtonElement::create(document);
    controls->m_fullScreenButton = fullScreenButton.ptr();
    panel->appendChild(WTF::move(fullScreenButton), ec);

    // The mute button and the slider element should be in the same div.
    Ref<HTMLDivElement> panelVolumeControlContainer = HTMLDivElement::create(document);

    if (document.page()->theme().usesMediaControlVolumeSlider()) {
        Ref<MediaControlVolumeSliderContainerElement> volumeSliderContainer = MediaControlVolumeSliderContainerElement::create(document);

        Ref<MediaControlPanelVolumeSliderElement> slider = MediaControlPanelVolumeSliderElement::create(document);
        controls->m_volumeSlider = slider.ptr();
        volumeSliderContainer->appendChild(WTF::move(slider), ec);
        if (ec)
            return nullptr;

        // This is a duplicate mute button, which is visible in some ports at the bottom of the volume bar.
        // It's important only when the volume bar is displayed below the controls.
        Ref<MediaControlVolumeSliderMuteButtonElement> volumeSliderMuteButton = MediaControlVolumeSliderMuteButtonElement::create(document);
        controls->m_volumeSliderMuteButton = volumeSliderMuteButton.ptr();
        volumeSliderContainer->appendChild(WTF::move(volumeSliderMuteButton), ec);

        if (ec)
            return nullptr;

        controls->m_volumeSliderContainer = volumeSliderContainer.ptr();
        panelVolumeControlContainer->appendChild(WTF::move(volumeSliderContainer), ec);
        if (ec)
            return nullptr;
    }

    Ref<MediaControlPanelMuteButtonElement> panelMuteButton = MediaControlPanelMuteButtonElement::create(document, controls.get());
    controls->m_panelMuteButton = panelMuteButton.ptr();
    panelVolumeControlContainer->appendChild(WTF::move(panelMuteButton), ec);
    if (ec)
        return nullptr;

    panel->appendChild(WTF::move(panelVolumeControlContainer), ec);
    if (ec)
        return nullptr;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    Ref<MediaControlFullscreenVolumeMinButtonElement> fullScreenMinVolumeButton = MediaControlFullscreenVolumeMinButtonElement::create(document);
    controls->m_fullScreenMinVolumeButton = fullScreenMinVolumeButton.ptr();
    panel->appendChild(WTF::move(fullScreenMinVolumeButton), ec);
    if (ec)
        return nullptr;

    Ref<MediaControlFullscreenVolumeSliderElement> fullScreenVolumeSlider = MediaControlFullscreenVolumeSliderElement::create(document);
    controls->m_fullScreenVolumeSlider = fullScreenVolumeSlider.ptr();
    panel->appendChild(WTF::move(fullScreenVolumeSlider), ec);
    if (ec)
        return nullptr;

    Ref<MediaControlFullscreenVolumeMaxButtonElement> fullScreenMaxVolumeButton = MediaControlFullscreenVolumeMaxButtonElement::create(document);
    controls->m_fullScreenMaxVolumeButton = fullScreenMaxVolumeButton.ptr();
    panel->appendChild(WTF::move(fullScreenMaxVolumeButton), ec);
    if (ec)
        return nullptr;

    controls->m_panel = panel.ptr();
    controls->appendChild(WTF::move(panel), ec);
    if (ec)
        return nullptr;

    return controls.release();
}

void MediaControlsApple::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;

    MediaControls::setMediaController(controller);

    if (m_rewindButton)
        m_rewindButton->setMediaController(controller);
    if (m_returnToRealTimeButton)
        m_returnToRealTimeButton->setMediaController(controller);
    if (m_statusDisplay)
        m_statusDisplay->setMediaController(controller);
    if (m_timeRemainingDisplay)
        m_timeRemainingDisplay->setMediaController(controller);
    if (m_timelineContainer)
        m_timelineContainer->setMediaController(controller);
    if (m_seekBackButton)
        m_seekBackButton->setMediaController(controller);
    if (m_seekForwardButton)
        m_seekForwardButton->setMediaController(controller);
    if (m_volumeSliderMuteButton)
        m_volumeSliderMuteButton->setMediaController(controller);
    if (m_volumeSliderContainer)
        m_volumeSliderContainer->setMediaController(controller);
    if (m_fullScreenMinVolumeButton)
        m_fullScreenMinVolumeButton->setMediaController(controller);
    if (m_fullScreenVolumeSlider)
        m_fullScreenVolumeSlider->setMediaController(controller);
    if (m_fullScreenMaxVolumeButton)
        m_fullScreenMaxVolumeButton->setMediaController(controller);
    if (m_closedCaptionsTrackList)
        m_closedCaptionsTrackList->setMediaController(controller);
    if (m_closedCaptionsContainer)
        m_closedCaptionsContainer->setMediaController(controller);
}

void MediaControlsApple::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        if (m_closedCaptionsContainer && m_closedCaptionsContainer->isShowing()) {
            hideClosedCaptionTrackList();
            event->setDefaultHandled();
        }
    }

    MediaControls::defaultEventHandler(event);
}

void MediaControlsApple::hide()
{
    MediaControls::hide();
    m_volumeSliderContainer->hide();
    if (m_closedCaptionsContainer)
        hideClosedCaptionTrackList();
}

void MediaControlsApple::makeTransparent()
{
    MediaControls::makeTransparent();
    m_volumeSliderContainer->hide();
    if (m_closedCaptionsContainer)
        hideClosedCaptionTrackList();
}

void MediaControlsApple::changedClosedCaptionsVisibility()
{
    MediaControls::changedClosedCaptionsVisibility();
    if (m_closedCaptionsContainer && m_closedCaptionsContainer->isShowing())
        hideClosedCaptionTrackList();

}

void MediaControlsApple::reset()
{
    Page* page = document().page();
    if (!page)
        return;

    updateStatusDisplay();

    if (m_mediaController->supportsFullscreen())
        m_fullScreenButton->show();
    else
        m_fullScreenButton->hide();

    double duration = m_mediaController->duration();
    if (std::isfinite(duration) || page->theme().hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        m_timeline->setDuration(duration);
        m_timelineContainer->show();
        m_timeline->setPosition(m_mediaController->currentTime());
        updateCurrentTimeDisplay();
    } else
        m_timelineContainer->hide();

    if (m_mediaController->hasAudio() || page->theme().hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->show();
    else
        m_panelMuteButton->hide();

    if (m_volumeSlider)
        setSliderVolume();

    if (m_toggleClosedCaptionsButton) {
        if (m_mediaController->hasClosedCaptions())
            m_toggleClosedCaptionsButton->show();
        else
            m_toggleClosedCaptionsButton->hide();
    }

    if (m_playButton)
        m_playButton->updateDisplayType();

#if ENABLE(FULLSCREEN_API)
    if (m_fullScreenVolumeSlider)
        setFullscreenSliderVolume();

    if (m_isFullscreen) {
        if (m_mediaController->isLiveStream()) {
            m_seekBackButton->hide();
            m_seekForwardButton->hide();
            m_rewindButton->show();
            m_returnToRealTimeButton->show();
        } else {
            m_seekBackButton->show();
            m_seekForwardButton->show();
            m_rewindButton->hide();
            m_returnToRealTimeButton->hide();
        }
    } else
#endif
    if (!m_mediaController->isLiveStream()) {
        m_returnToRealTimeButton->hide();
        m_rewindButton->show();
    } else {
        m_returnToRealTimeButton->show();
        m_rewindButton->hide();
    }

    makeOpaque();
}

void MediaControlsApple::updateCurrentTimeDisplay()
{
    double now = m_mediaController->currentTime();
    double duration = m_mediaController->duration();

    Page* page = document().page();
    if (!page)
        return;

    // Allow the theme to format the time.
    m_currentTimeDisplay->setInnerText(page->theme().formatMediaControlsCurrentTime(now, duration), IGNORE_EXCEPTION);
    m_currentTimeDisplay->setCurrentValue(now);
    m_timeRemainingDisplay->setInnerText(page->theme().formatMediaControlsRemainingTime(now, duration), IGNORE_EXCEPTION);
    m_timeRemainingDisplay->setCurrentValue(now - duration);
}

void MediaControlsApple::reportedError()
{
    Page* page = document().page();
    if (!page)
        return;

    if (!page->theme().hasOwnDisabledStateHandlingFor(MediaSliderPart))
        m_timelineContainer->hide();

    if (!page->theme().hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->hide();

    m_fullScreenButton->hide();

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->hide();
    if (m_toggleClosedCaptionsButton && !page->theme().hasOwnDisabledStateHandlingFor(MediaToggleClosedCaptionsButtonPart))
        m_toggleClosedCaptionsButton->hide();
    if (m_closedCaptionsContainer)
        hideClosedCaptionTrackList();
}

void MediaControlsApple::updateStatusDisplay()
{
    if (m_statusDisplay)
        m_statusDisplay->update();
}

void MediaControlsApple::loadedMetadata()
{
    if (m_statusDisplay && !m_mediaController->isLiveStream())
        m_statusDisplay->hide();

    MediaControls::loadedMetadata();
}

void MediaControlsApple::changedMute()
{
    MediaControls::changedMute();

    if (m_volumeSliderMuteButton)
        m_volumeSliderMuteButton->changedMute();
}

void MediaControlsApple::changedVolume()
{
    MediaControls::changedVolume();

    if (m_fullScreenVolumeSlider)
        setFullscreenSliderVolume();
}

void MediaControlsApple::enteredFullscreen()
{
    MediaControls::enteredFullscreen();
    m_panel->setCanBeDragged(true);

    if (m_mediaController->isLiveStream()) {
        m_seekBackButton->hide();
        m_seekForwardButton->hide();
        m_rewindButton->show();
        m_returnToRealTimeButton->show();
    } else {
        m_seekBackButton->show();
        m_seekForwardButton->show();
        m_rewindButton->hide();
        m_returnToRealTimeButton->hide();
    }
}

void MediaControlsApple::exitedFullscreen()
{
    m_rewindButton->show();
    m_seekBackButton->show();
    m_seekForwardButton->show();
    m_returnToRealTimeButton->show();

    m_panel->setCanBeDragged(false);

    // We will keep using the panel, but we want it to go back to the standard position.
    // This will matter right away because we use the panel even when not fullscreen.
    // And if we reenter fullscreen we also want the panel in the standard position.
    m_panel->resetPosition();

    MediaControls::exitedFullscreen();
}

void MediaControlsApple::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->show();
}

void MediaControlsApple::toggleClosedCaptionTrackList()
{
    if (!m_mediaController->hasClosedCaptions())
        return;

    if (m_closedCaptionsContainer) {
        if (m_closedCaptionsContainer->isShowing())
            hideClosedCaptionTrackList();
        else {
            if (m_closedCaptionsTrackList)
                m_closedCaptionsTrackList->updateDisplay();
            showClosedCaptionTrackList();
        }
    }
}

void MediaControlsApple::showClosedCaptionTrackList()
{
    if (!m_closedCaptionsContainer || m_closedCaptionsContainer->isShowing())
        return;

    m_closedCaptionsContainer->show();

    // Ensure the controls panel does not receive any events while the captions
    // track list is visible as all events now need to be captured by the
    // track list.
    m_panel->setInlineStyleProperty(CSSPropertyPointerEvents, CSSValueNone);

    RefPtr<EventListener> listener = eventListener();
    m_closedCaptionsContainer->addEventListener(eventNames().wheelEvent, listener, true);

    // Track click events in the capture phase at two levels, first at the document level
    // such that a click outside of the <video> may dismiss the track list, second at the
    // media controls level such that a click anywhere outside of the track list hides the
    // track list. These two levels are necessary since it would not be possible to get a
    // reference to the track list when handling the event outside of the shadow tree.
    document().addEventListener(eventNames().clickEvent, listener, true);
    addEventListener(eventNames().clickEvent, listener, true);
}

void MediaControlsApple::hideClosedCaptionTrackList()
{
    if (!m_closedCaptionsContainer || !m_closedCaptionsContainer->isShowing())
        return;

    m_closedCaptionsContainer->hide();

    // Buttons in the controls panel may now be interactive.
    m_panel->removeInlineStyleProperty(CSSPropertyPointerEvents);

    EventListener* listener = eventListener().get();
    m_closedCaptionsContainer->removeEventListener(eventNames().wheelEvent, listener, true);
    document().removeEventListener(eventNames().clickEvent, listener, true);
    removeEventListener(eventNames().clickEvent, listener, true);
}

void MediaControlsApple::setFullscreenSliderVolume()
{
    m_fullScreenVolumeSlider->setVolume(m_mediaController->muted() ? 0.0 : m_mediaController->volume());
}

bool MediaControlsApple::shouldClosedCaptionsContainerPreventPageScrolling(int wheelDeltaY)
{
    int scrollTop = m_closedCaptionsContainer->scrollTop();
    // Scrolling down.
    if (wheelDeltaY < 0 && (scrollTop + m_closedCaptionsContainer->offsetHeight()) >= m_closedCaptionsContainer->scrollHeight())
        return true;
    // Scrolling up.
    if (wheelDeltaY > 0 && scrollTop <= 0)
        return true;
    return false;
}

void MediaControlsApple::handleClickEvent(Event* event)
{
    Node* currentTarget = event->currentTarget()->toNode();
    Node* target = event->target()->toNode();

    if ((currentTarget == &document() && !shadowHost()->contains(target)) || (currentTarget == this && !m_closedCaptionsContainer->contains(target))) {
        hideClosedCaptionTrackList();
        event->stopImmediatePropagation();
        event->setDefaultHandled();
    }
}

void MediaControlsApple::closedCaptionTracksChanged()
{
    if (m_toggleClosedCaptionsButton) {
        if (m_mediaController->hasClosedCaptions())
            m_toggleClosedCaptionsButton->show();
        else
            m_toggleClosedCaptionsButton->hide();
    }
}

PassRefPtr<MediaControlsAppleEventListener> MediaControlsApple::eventListener()
{
    if (!m_eventListener)
        m_eventListener = MediaControlsAppleEventListener::create(this);
    return m_eventListener;
}

// --------

void MediaControlsAppleEventListener::handleEvent(ScriptExecutionContext*, Event* event)
{
    if (event->type() == eventNames().clickEvent)
        m_mediaControls->handleClickEvent(event);
    else if (eventNames().isWheelEventType(event->type()) && is<WheelEvent>(*event)) {
        WheelEvent& wheelEvent = downcast<WheelEvent>(*event);
        if (m_mediaControls->shouldClosedCaptionsContainerPreventPageScrolling(wheelEvent.wheelDeltaY()))
            wheelEvent.preventDefault();
    }
}

bool MediaControlsAppleEventListener::operator==(const EventListener& listener)
{
    if (const MediaControlsAppleEventListener* mediaControlsAppleEventListener = MediaControlsAppleEventListener::cast(&listener))
        return m_mediaControls == mediaControlsAppleEventListener->m_mediaControls;
    return false;
}

}

#endif
#endif
