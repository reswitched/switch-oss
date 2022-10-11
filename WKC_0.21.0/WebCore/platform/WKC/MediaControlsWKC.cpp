/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2012 Igalia S.L.
 * Copyright (c) 2013-2015 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "MediaControls.h"
#include "NotImplemented.h"

namespace WebCore {

class MediaControlsWKC : public MediaControls
{
public:
    virtual ~MediaControlsWKC();

public:
    static PassRefPtr<MediaControlsWKC> createControls(Document&);

    virtual void setMediaController(MediaControllerInterface*) override;
    virtual void reset() override;
    virtual void playbackStarted() override;
    virtual void updateCurrentTimeDisplay() override;
    virtual void showVolumeSlider() override;
    void changedMute() override;

#if ENABLE(VIDEO_TRACK)
    void createTextTrackDisplay() override;
#endif

private:
    explicit MediaControlsWKC(Document& document);

    bool initializeControls(Document& document);

private:
    MediaControlTimeRemainingDisplayElement* m_durationDisplay;
    MediaControlPanelEnclosureElement* m_enclosure;
    MediaControlVolumeSliderContainerElement* m_volumeSliderContainer;
};

MediaControlsWKC::MediaControlsWKC(Document& document)
    : MediaControls(document)
{
}

MediaControlsWKC::~MediaControlsWKC()
{
}

PassRefPtr<MediaControlsWKC>
MediaControlsWKC::createControls(Document& document)
{
    if (!document.page())
        return 0;
    RefPtr<MediaControlsWKC> r = adoptRef(new MediaControlsWKC(document));
    if (r->initializeControls(document))
        return r.release();

    return 0;
}

bool MediaControlsWKC::initializeControls(Document& document)
{
    // Create an enclosing element for the panel so we can visually offset the controls correctly.
    RefPtr<MediaControlPanelEnclosureElement> enclosure = MediaControlPanelEnclosureElement::create(document);
    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(document);
    ExceptionCode exceptionCode;

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(document);
    m_playButton = playButton.get();
    panel->appendChild(playButton.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    RefPtr<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(document, this);
    m_timeline = timeline.get();
    panel->appendChild(timeline.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(document);
    m_currentTimeDisplay = currentTimeDisplay.get();
    m_currentTimeDisplay->hide();
    panel->appendChild(currentTimeDisplay.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    RefPtr<MediaControlTimeRemainingDisplayElement> durationDisplay = MediaControlTimeRemainingDisplayElement::create(document);
    m_durationDisplay = durationDisplay.get();
    panel->appendChild(durationDisplay.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    if (document.page()->theme().supportsClosedCaptioning()) {
        RefPtr<MediaControlToggleClosedCaptionsButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsButtonElement::create(document, this);
        m_toggleClosedCaptionsButton = toggleClosedCaptionsButton.get();
        panel->appendChild(toggleClosedCaptionsButton.releaseNonNull(), exceptionCode);
        if (exceptionCode)
            return false;
    }

    RefPtr<MediaControlFullscreenButtonElement> fullscreenButton = MediaControlFullscreenButtonElement::create(document);
    m_fullScreenButton = fullscreenButton.get();
    panel->appendChild(fullscreenButton.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    RefPtr<MediaControlPanelMuteButtonElement> panelMuteButton = MediaControlPanelMuteButtonElement::create(document, this);
    m_panelMuteButton = panelMuteButton.get();
    panel->appendChild(panelMuteButton.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    RefPtr<MediaControlVolumeSliderContainerElement> sliderContainer = MediaControlVolumeSliderContainerElement::create(document);
    m_volumeSliderContainer = sliderContainer.get();
    panel->appendChild(sliderContainer.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    RefPtr<MediaControlPanelVolumeSliderElement> slider = MediaControlPanelVolumeSliderElement::create(document);
    m_volumeSlider = slider.get();
    m_volumeSlider->setClearMutedOnUserInteraction(true);
    m_volumeSliderContainer->appendChild(slider.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    m_panel = panel.get();
    enclosure->appendChild(panel.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    m_enclosure = enclosure.get();
    appendChild(enclosure.releaseNonNull(), exceptionCode);
    if (exceptionCode)
        return false;

    return true;
}

void MediaControlsWKC::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;

    MediaControls::setMediaController(controller);

    if (m_durationDisplay)
        m_durationDisplay->setMediaController(controller);
    if (m_enclosure)
        m_enclosure->setMediaController(controller);
}

void MediaControlsWKC::reset()
{
    Page* page = document().page();
    if (!page)
        return;

    double duration = m_mediaController->duration();
    m_durationDisplay->setInnerText(page->theme().formatMediaControlsTime(duration), ASSERT_NO_EXCEPTION);
    m_durationDisplay->setCurrentValue(duration);
    m_timeline->setDuration(duration);
    m_timeline->setPosition(0);

    MediaControls::reset();
}

void MediaControlsWKC::playbackStarted()
{
    m_currentTimeDisplay->show();
    m_durationDisplay->hide();

    MediaControls::playbackStarted();
}

void MediaControlsWKC::updateCurrentTimeDisplay()
{
    double now = m_mediaController->currentTime();
    double duration = m_mediaController->duration();

    Page* page = document().page();
    if (!page)
        return;

    // After seek, hide duration display and show current time.
    if (now > 0) {
        m_currentTimeDisplay->show();
        m_durationDisplay->hide();
    }

    // Allow the theme to format the time.
    ExceptionCode exceptionCode;
    m_currentTimeDisplay->setInnerText(page->theme().formatMediaControlsCurrentTime(now, duration), exceptionCode);
    m_currentTimeDisplay->setCurrentValue(now);
}

void MediaControlsWKC::changedMute()
{
    MediaControls::changedMute();

    if (m_mediaController->muted())
        m_volumeSlider->setVolume(0);
    else
        m_volumeSlider->setVolume(m_mediaController->volume());
}

void MediaControlsWKC::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->show();
}

#if ENABLE(VIDEO_TRACK)
void MediaControlsWKC::createTextTrackDisplay()
{
    if (m_textDisplayContainer)
        return;

    RefPtr<MediaControlTextTrackContainerElement> textDisplayContainer = MediaControlTextTrackContainerElement::create(document());
    m_textDisplayContainer = textDisplayContainer.get();

    if (m_mediaController)
        m_textDisplayContainer->setMediaController(m_mediaController);

    // Insert it before the first controller element so it always displays behind the controls.
    insertBefore(textDisplayContainer.releaseNonNull(), m_enclosure, ASSERT_NO_EXCEPTION);
}
#endif

PassRefPtr<MediaControls> MediaControls::create(Document& document)
{
    return MediaControlsWKC::createControls(document);
}

} // namespace
