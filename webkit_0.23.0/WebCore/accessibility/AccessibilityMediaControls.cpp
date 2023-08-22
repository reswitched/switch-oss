/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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


#include "config.h"

#if ENABLE(VIDEO)

#include "AccessibilityMediaControls.h"

#include "AXObjectCache.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "MediaControlElements.h"
#include "RenderObject.h"
#include "RenderSlider.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

using namespace HTMLNames;


AccessibilityMediaControl::AccessibilityMediaControl(RenderObject* renderer)
    : AccessibilityRenderObject(renderer)
{
}

Ref<AccessibilityObject> AccessibilityMediaControl::create(RenderObject* renderer)
{
    ASSERT(renderer->node());

    switch (mediaControlElementType(renderer->node())) {
    case MediaSlider:
        return AccessibilityMediaTimeline::create(renderer);

    case MediaCurrentTimeDisplay:
    case MediaTimeRemainingDisplay:
        return AccessibilityMediaTimeDisplay::create(renderer);

    case MediaControlsPanel:
        return AccessibilityMediaControlsContainer::create(renderer);

    default:
        return adoptRef(*new AccessibilityMediaControl(renderer));
    }
}

MediaControlElementType AccessibilityMediaControl::controlType() const
{
    if (!renderer() || !renderer()->node())
        return MediaTimelineContainer; // Timeline container is not accessible.

    return mediaControlElementType(renderer()->node());
}

const String& AccessibilityMediaControl::controlTypeName() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const String> mediaEnterFullscreenButtonName(ASCIILiteral("EnterFullscreenButton"));
    static NeverDestroyed<const String> mediaExitFullscreenButtonName(ASCIILiteral("ExitFullscreenButton"));
    static NeverDestroyed<const String> mediaMuteButtonName(ASCIILiteral("MuteButton"));
    static NeverDestroyed<const String> mediaPlayButtonName(ASCIILiteral("PlayButton"));
    static NeverDestroyed<const String> mediaSeekBackButtonName(ASCIILiteral("SeekBackButton"));
    static NeverDestroyed<const String> mediaSeekForwardButtonName(ASCIILiteral("SeekForwardButton"));
    static NeverDestroyed<const String> mediaRewindButtonName(ASCIILiteral("RewindButton"));
    static NeverDestroyed<const String> mediaReturnToRealtimeButtonName(ASCIILiteral("ReturnToRealtimeButton"));
    static NeverDestroyed<const String> mediaUnMuteButtonName(ASCIILiteral("UnMuteButton"));
    static NeverDestroyed<const String> mediaPauseButtonName(ASCIILiteral("PauseButton"));
    static NeverDestroyed<const String> mediaStatusDisplayName(ASCIILiteral("StatusDisplay"));
    static NeverDestroyed<const String> mediaCurrentTimeDisplay(ASCIILiteral("CurrentTimeDisplay"));
    static NeverDestroyed<const String> mediaTimeRemainingDisplay(ASCIILiteral("TimeRemainingDisplay"));
    static NeverDestroyed<const String> mediaShowClosedCaptionsButtonName(ASCIILiteral("ShowClosedCaptionsButton"));
    static NeverDestroyed<const String> mediaHideClosedCaptionsButtonName(ASCIILiteral("HideClosedCaptionsButton"));

    switch (controlType()) {
    case MediaEnterFullscreenButton:
        return mediaEnterFullscreenButtonName;
    case MediaExitFullscreenButton:
        return mediaExitFullscreenButtonName;
    case MediaMuteButton:
        return mediaMuteButtonName;
    case MediaPlayButton:
        return mediaPlayButtonName;
    case MediaSeekBackButton:
        return mediaSeekBackButtonName;
    case MediaSeekForwardButton:
        return mediaSeekForwardButtonName;
    case MediaRewindButton:
        return mediaRewindButtonName;
    case MediaReturnToRealtimeButton:
        return mediaReturnToRealtimeButtonName;
    case MediaUnMuteButton:
        return mediaUnMuteButtonName;
    case MediaPauseButton:
        return mediaPauseButtonName;
    case MediaStatusDisplay:
        return mediaStatusDisplayName;
    case MediaCurrentTimeDisplay:
        return mediaCurrentTimeDisplay;
    case MediaTimeRemainingDisplay:
        return mediaTimeRemainingDisplay;
    case MediaShowClosedCaptionsButton:
        return mediaShowClosedCaptionsButtonName;
    case MediaHideClosedCaptionsButton:
        return mediaHideClosedCaptionsButtonName;

    default:
        break;
    }

    return nullAtom;
#else
    WKC_DEFINE_STATIC_PTR(const String*, mediaEnterFullscreenButtonName, new String(ASCIILiteral("EnterFullscreenButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaExitFullscreenButtonName, new String(ASCIILiteral("ExitFullscreenButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaMuteButtonName, new String(ASCIILiteral("MuteButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaPlayButtonName, new String(ASCIILiteral("PlayButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaSeekBackButtonName, new String(ASCIILiteral("SeekBackButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaSeekForwardButtonName, new String(ASCIILiteral("SeekForwardButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaRewindButtonName, new String(ASCIILiteral("RewindButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaReturnToRealtimeButtonName, new String(ASCIILiteral("ReturnToRealtimeButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaUnMuteButtonName, new String(ASCIILiteral("UnMuteButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaPauseButtonName, new String(ASCIILiteral("PauseButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaStatusDisplayName, new String(ASCIILiteral("StatusDisplay")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaCurrentTimeDisplay, new String(ASCIILiteral("CurrentTimeDisplay")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaTimeRemainingDisplay, new String(ASCIILiteral("TimeRemainingDisplay")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaShowClosedCaptionsButtonName, new String(ASCIILiteral("ShowClosedCaptionsButton")));
    WKC_DEFINE_STATIC_PTR(const String*, mediaHideClosedCaptionsButtonName, new String(ASCIILiteral("HideClosedCaptionsButton")));

    switch (controlType()) {
    case MediaEnterFullscreenButton:
        return *mediaEnterFullscreenButtonName;
    case MediaExitFullscreenButton:
        return *mediaExitFullscreenButtonName;
    case MediaMuteButton:
        return *mediaMuteButtonName;
    case MediaPlayButton:
        return *mediaPlayButtonName;
    case MediaSeekBackButton:
        return *mediaSeekBackButtonName;
    case MediaSeekForwardButton:
        return *mediaSeekForwardButtonName;
    case MediaRewindButton:
        return *mediaRewindButtonName;
    case MediaReturnToRealtimeButton:
        return *mediaReturnToRealtimeButtonName;
    case MediaUnMuteButton:
        return *mediaUnMuteButtonName;
    case MediaPauseButton:
        return *mediaPauseButtonName;
    case MediaStatusDisplay:
        return *mediaStatusDisplayName;
    case MediaCurrentTimeDisplay:
        return *mediaCurrentTimeDisplay;
    case MediaTimeRemainingDisplay:
        return *mediaTimeRemainingDisplay;
    case MediaShowClosedCaptionsButton:
        return *mediaShowClosedCaptionsButtonName;
    case MediaHideClosedCaptionsButton:
        return *mediaHideClosedCaptionsButtonName;

    default:
        break;
    }

    return nullAtom;
#endif
}

void AccessibilityMediaControl::accessibilityText(Vector<AccessibilityText>& textOrder)
{
    String description = accessibilityDescription();
    if (!description.isEmpty())
        textOrder.append(AccessibilityText(description, AlternativeText));

    String title = this->title();
    if (!title.isEmpty())
        textOrder.append(AccessibilityText(title, AlternativeText));

    String helptext = helpText();
    if (!helptext.isEmpty())
        textOrder.append(AccessibilityText(helptext, HelpText));
}
    

String AccessibilityMediaControl::title() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const String> controlsPanel(ASCIILiteral("ControlsPanel"));

    if (controlType() == MediaControlsPanel)
        return localizedMediaControlElementString(controlsPanel);
#else
    WKC_DEFINE_STATIC_PTR(const String*, controlsPanel, 0);
    if (!controlsPanel)
        controlsPanel = new String(ASCIILiteral("ControlsPanel"));

    if (controlType() == MediaControlsPanel)
        return localizedMediaControlElementString(*controlsPanel);
#endif


    return AccessibilityRenderObject::title();
}

String AccessibilityMediaControl::accessibilityDescription() const
{
    return localizedMediaControlElementString(controlTypeName());
}

String AccessibilityMediaControl::helpText() const
{
    return localizedMediaControlElementHelpText(controlTypeName());
}

bool AccessibilityMediaControl::computeAccessibilityIsIgnored() const
{
    if (!m_renderer || m_renderer->style().visibility() != VISIBLE || controlType() == MediaTimelineContainer)
        return true;

    return accessibilityIsIgnoredByDefault();
}

AccessibilityRole AccessibilityMediaControl::roleValue() const
{
    switch (controlType()) {
    case MediaEnterFullscreenButton:
    case MediaExitFullscreenButton:
    case MediaMuteButton:
    case MediaPlayButton:
    case MediaSeekBackButton:
    case MediaSeekForwardButton:
    case MediaRewindButton:
    case MediaReturnToRealtimeButton:
    case MediaUnMuteButton:
    case MediaPauseButton:
    case MediaShowClosedCaptionsButton:
    case MediaHideClosedCaptionsButton:
        return ButtonRole;

    case MediaStatusDisplay:
        return StaticTextRole;

    case MediaTimelineContainer:
        return GroupRole;

    default:
        break;
    }

    return UnknownRole;
}



//
// AccessibilityMediaControlsContainer

AccessibilityMediaControlsContainer::AccessibilityMediaControlsContainer(RenderObject* renderer)
    : AccessibilityMediaControl(renderer)
{
}

Ref<AccessibilityObject> AccessibilityMediaControlsContainer::create(RenderObject* renderer)
{
    return adoptRef(*new AccessibilityMediaControlsContainer(renderer));
}

String AccessibilityMediaControlsContainer::accessibilityDescription() const
{
    return localizedMediaControlElementString(elementTypeName());
}

String AccessibilityMediaControlsContainer::helpText() const
{
    return localizedMediaControlElementHelpText(elementTypeName());
}

bool AccessibilityMediaControlsContainer::controllingVideoElement() const
{
    auto element = parentMediaElement(*m_renderer);
    return !element || element->isVideo();
}

const String& AccessibilityMediaControlsContainer::elementTypeName() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const String> videoElement(ASCIILiteral("VideoElement"));
    static NeverDestroyed<const String> audioElement(ASCIILiteral("AudioElement"));

    if (controllingVideoElement())
        return videoElement;
    return audioElement;
#else
    WKC_DEFINE_STATIC_PTR(const String*, videoElement, 0);
    WKC_DEFINE_STATIC_PTR(const String*, audioElement, 0);
    if (!videoElement) {
        videoElement = new String(ASCIILiteral("VideoElement"));
        audioElement = new String(ASCIILiteral("AudioElement"));
    }
    if (controllingVideoElement())
        return *videoElement;
    return *audioElement;
#endif
}

bool AccessibilityMediaControlsContainer::computeAccessibilityIsIgnored() const
{
    return accessibilityIsIgnoredByDefault();
}

//
// AccessibilityMediaTimeline

AccessibilityMediaTimeline::AccessibilityMediaTimeline(RenderObject* renderer)
    : AccessibilitySlider(renderer)
{
}

Ref<AccessibilityObject> AccessibilityMediaTimeline::create(RenderObject* renderer)
{
    return adoptRef(*new AccessibilityMediaTimeline(renderer));
}

String AccessibilityMediaTimeline::valueDescription() const
{
    Node* node = m_renderer->node();
    if (!is<HTMLInputElement>(*node))
        return String();

    float time = downcast<HTMLInputElement>(*node).value().toFloat();
    return localizedMediaTimeDescription(time);
}

String AccessibilityMediaTimeline::helpText() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const String> slider(ASCIILiteral("Slider"));
    return localizedMediaControlElementHelpText(slider);
#else
    WKC_DEFINE_STATIC_PTR(const String*, slider, 0);
    if (!slider)
        slider = new String(ASCIILiteral("Slider"));
    return localizedMediaControlElementHelpText(*slider);
#endif
}


//
// AccessibilityMediaTimeDisplay

AccessibilityMediaTimeDisplay::AccessibilityMediaTimeDisplay(RenderObject* renderer)
    : AccessibilityMediaControl(renderer)
{
}

Ref<AccessibilityObject> AccessibilityMediaTimeDisplay::create(RenderObject* renderer)
{
    return adoptRef(*new AccessibilityMediaTimeDisplay(renderer));
}

bool AccessibilityMediaTimeDisplay::computeAccessibilityIsIgnored() const
{
    if (!m_renderer || m_renderer->style().visibility() != VISIBLE)
        return true;

    if (!m_renderer->style().width().value())
        return true;
    
    return accessibilityIsIgnoredByDefault();
}

String AccessibilityMediaTimeDisplay::accessibilityDescription() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<const String> currentTimeDisplay(ASCIILiteral("CurrentTimeDisplay"));
    static NeverDestroyed<const String> timeRemainingDisplay(ASCIILiteral("TimeRemainingDisplay"));

    if (controlType() == MediaCurrentTimeDisplay)
        return localizedMediaControlElementString(currentTimeDisplay);

    return localizedMediaControlElementString(timeRemainingDisplay);
#else
    WKC_DEFINE_STATIC_PTR(const String*, currentTimeDisplay, 0);
    WKC_DEFINE_STATIC_PTR(const String*, timeRemainingDisplay, 0);
    if (!currentTimeDisplay) {
        currentTimeDisplay = new String(ASCIILiteral("CurrentTimeDisplay"));
        timeRemainingDisplay = new String(ASCIILiteral("TimeRemainingDisplay"));
    }

    if (controlType() == MediaCurrentTimeDisplay)
        return localizedMediaControlElementString(*currentTimeDisplay);

    return localizedMediaControlElementString(*timeRemainingDisplay);
#endif
}

String AccessibilityMediaTimeDisplay::stringValue() const
{
    if (!m_renderer || !m_renderer->node())
        return String();

    MediaControlTimeDisplayElement* element = static_cast<MediaControlTimeDisplayElement*>(m_renderer->node());
    float time = element->currentValue();
    return localizedMediaTimeDescription(fabsf(time));
}

} // namespace WebCore

#endif // ENABLE(VIDEO)
