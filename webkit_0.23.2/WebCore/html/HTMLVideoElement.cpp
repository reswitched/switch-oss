/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#if ENABLE(VIDEO)
#include "HTMLVideoElement.h"

#include "CSSPropertyNames.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "Frame.h"
#include "HTMLImageLoader.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "Logging.h"
#include "Page.h"
#include "RenderImage.h"
#include "RenderVideo.h"
#include "ScriptController.h"
#include "Settings.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

using namespace HTMLNames;

inline HTMLVideoElement::HTMLVideoElement(const QualifiedName& tagName, Document& document, bool createdByParser)
    : HTMLMediaElement(tagName, document, createdByParser)
{
    ASSERT(hasTagName(videoTag));
    setHasCustomStyleResolveCallbacks();
    if (document.settings())
        m_defaultPosterURL = document.settings()->defaultVideoPosterURL();
}

Ref<HTMLVideoElement> HTMLVideoElement::create(const QualifiedName& tagName, Document& document, bool createdByParser)
{
    Ref<HTMLVideoElement> videoElement = adoptRef(*new HTMLVideoElement(tagName, document, createdByParser));
    videoElement->suspendIfNeeded();
    return videoElement;
}

bool HTMLVideoElement::rendererIsNeeded(const RenderStyle& style) 
{
    return HTMLElement::rendererIsNeeded(style); 
}

RenderPtr<RenderElement> HTMLVideoElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderVideo>(*this, WTF::move(style));
}

void HTMLVideoElement::didAttachRenderers()
{
    HTMLMediaElement::didAttachRenderers();

    updateDisplayState();
    if (shouldDisplayPosterImage()) {
        if (!m_imageLoader)
            m_imageLoader = std::make_unique<HTMLImageLoader>(*this);
        m_imageLoader->updateFromElement();
        if (renderer())
            downcast<RenderImage>(*renderer()).imageResource().setCachedImage(m_imageLoader->image());
    }
}

void HTMLVideoElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStyleProperties& style)
{
    if (name == widthAttr)
        addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    else if (name == heightAttr)
        addHTMLLengthToStyle(style, CSSPropertyHeight, value);
    else
        HTMLMediaElement::collectStyleForPresentationAttribute(name, value, style);
}

bool HTMLVideoElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == widthAttr || name == heightAttr)
        return true;
    return HTMLMediaElement::isPresentationAttribute(name);
}

void HTMLVideoElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == posterAttr) {
        // Force a poster recalc by setting m_displayMode to Unknown directly before calling updateDisplayState.
        HTMLMediaElement::setDisplayMode(Unknown);
        updateDisplayState();

        if (shouldDisplayPosterImage()) {
            if (!m_imageLoader)
                m_imageLoader = std::make_unique<HTMLImageLoader>(*this);
            m_imageLoader->updateFromElementIgnoringPreviousError();
        } else {
            if (renderer())
                downcast<RenderImage>(*renderer()).imageResource().setCachedImage(nullptr);
        }
    }
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    else if (name == webkitwirelessvideoplaybackdisabledAttr)
        mediaSession().setWirelessVideoPlaybackDisabled(*this, true);
#endif
    else {
        HTMLMediaElement::parseAttribute(name, value);    

#if PLATFORM(IOS) && ENABLE(WIRELESS_PLAYBACK_TARGET)
        if (name == webkitairplayAttr) {
            bool disabled = false;
            if (equalIgnoringCase(fastGetAttribute(HTMLNames::webkitairplayAttr), "deny"))
                disabled = true;
            mediaSession().setWirelessVideoPlaybackDisabled(*this, disabled);
        }
#endif
    }

}

bool HTMLVideoElement::supportsFullscreen() const
{
    Page* page = document().page();
    if (!page) 
        return false;

    if (!player() || !player()->supportsFullscreen())
        return false;

#if PLATFORM(IOS)
    // Fullscreen implemented by player.
    return true;
#else
#if ENABLE(FULLSCREEN_API)
    // If the full screen API is enabled and is supported for the current element
    // do not require that the player has a video track to enter full screen.
    if (page->chrome().client().supportsFullScreenForElement(this, false))
        return true;
#endif

    if (!player()->hasVideo())
        return false;

    return page->chrome().client().supportsVideoFullscreen();
#endif // PLATFORM(IOS)
}

unsigned HTMLVideoElement::videoWidth() const
{
    if (!player())
        return 0;
    return clampToUnsigned(player()->naturalSize().width());
}

unsigned HTMLVideoElement::videoHeight() const
{
    if (!player())
        return 0;
    return clampToUnsigned(player()->naturalSize().height());
}

bool HTMLVideoElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == posterAttr || HTMLMediaElement::isURLAttribute(attribute);
}

const AtomicString& HTMLVideoElement::imageSourceURL() const
{
    const AtomicString& url = getAttribute(posterAttr);
    if (!stripLeadingAndTrailingHTMLSpaces(url).isEmpty())
        return url;
    return m_defaultPosterURL;
}

void HTMLVideoElement::setDisplayMode(DisplayMode mode)
{
    DisplayMode oldMode = displayMode();
    URL poster = posterImageURL();

#if PLATFORM(WKC)
    // Always display a poster also while video is playing.
    mode = Poster;
#endif

    if (!poster.isEmpty()) {
        // We have a poster path, but only show it until the user triggers display by playing or seeking and the
        // media engine has something to display.
        if (mode == Video) {
            if (oldMode != Video && player())
                player()->prepareForRendering();
            if (!hasAvailableVideoFrame())
                mode = PosterWaitingForVideo;
        }
    } else if (oldMode != Video && player())
        player()->prepareForRendering();

    HTMLMediaElement::setDisplayMode(mode);

    if (player() && player()->canLoadPoster()) {
        bool canLoad = true;
        if (!poster.isEmpty()) {
            if (Frame* frame = document().frame())
                canLoad = frame->loader().willLoadMediaElementURL(poster);
        }
        if (canLoad)
            player()->setPoster(poster);
    }

    if (renderer() && displayMode() != oldMode)
        renderer()->updateFromElement();
}

void HTMLVideoElement::updateDisplayState()
{
    if (posterImageURL().isEmpty())
        setDisplayMode(Video);
    else if (displayMode() < Poster)
        setDisplayMode(Poster);
}

void HTMLVideoElement::paintCurrentFrameInContext(GraphicsContext* context, const FloatRect& destRect)
{
    MediaPlayer* player = HTMLMediaElement::player();
    if (!player)
        return;
    
    player->setVisible(true); // Make player visible or it won't draw.
    player->paintCurrentFrameInContext(context, destRect);
}

bool HTMLVideoElement::copyVideoTextureToPlatformTexture(GraphicsContext3D* context, Platform3DObject texture, GC3Dint level, GC3Denum type, GC3Denum internalFormat, bool premultiplyAlpha, bool flipY)
{
    if (!player())
        return false;
    return player()->copyVideoTextureToPlatformTexture(context, texture, level, type, internalFormat, premultiplyAlpha, flipY);
}

bool HTMLVideoElement::hasAvailableVideoFrame() const
{
    if (!player())
        return false;
    
    return player()->hasVideo() && player()->hasAvailableVideoFrame();
}

PassNativeImagePtr HTMLVideoElement::nativeImageForCurrentTime()
{
    if (!player())
        return nullptr;

    return player()->nativeImageForCurrentTime();
}

void HTMLVideoElement::webkitEnterFullscreen(ExceptionCode& ec)
{
    if (isFullscreen())
        return;

    // Generate an exception if this isn't called in response to a user gesture, or if the 
    // element does not support fullscreen.
    if (!mediaSession().fullscreenPermitted(*this) || !supportsFullscreen()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    enterFullscreen();
}

void HTMLVideoElement::webkitExitFullscreen()
{
    if (isFullscreen())
        exitFullscreen();
}

bool HTMLVideoElement::webkitSupportsFullscreen()
{
    return supportsFullscreen();
}

bool HTMLVideoElement::webkitDisplayingFullscreen()
{
    return isFullscreen();
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
bool HTMLVideoElement::webkitWirelessVideoPlaybackDisabled() const
{
    return mediaSession().wirelessVideoPlaybackDisabled(*this);
}

void HTMLVideoElement::setWebkitWirelessVideoPlaybackDisabled(bool disabled)
{
    setBooleanAttribute(webkitwirelessvideoplaybackdisabledAttr, disabled);
}
#endif

void HTMLVideoElement::didMoveToNewDocument(Document* oldDocument)
{
    if (m_imageLoader)
        m_imageLoader->elementDidMoveToNewDocument();
    HTMLMediaElement::didMoveToNewDocument(oldDocument);
}

#if ENABLE(MEDIA_STATISTICS)
unsigned HTMLVideoElement::webkitDecodedFrameCount() const
{
    if (!player())
        return 0;

    return player()->decodedFrameCount();
}

unsigned HTMLVideoElement::webkitDroppedFrameCount() const
{
    if (!player())
        return 0;

    return player()->droppedFrameCount();
}
#endif

URL HTMLVideoElement::posterImageURL() const
{
    String url = stripLeadingAndTrailingHTMLSpaces(imageSourceURL());
    if (url.isEmpty())
        return URL();
    return document().completeURL(url);
}

#if ENABLE(VIDEO_PRESENTATION_MODE)

static const AtomicString& presentationModeFullscreen()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> fullscreen("fullscreen", AtomicString::ConstructFromLiteral);
    return fullscreen;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, fullscreen, new AtomicString("fullscreen", AtomicString::ConstructFromLiteral));
    return *fullscreen;
#endif
}

static const AtomicString& presentationModePictureInPicture()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> pictureInPicture("picture-in-picture", AtomicString::ConstructFromLiteral);
    return pictureInPicture;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, pictureInPicture, new AtomicString("picture-in-picture", AtomicString::ConstructFromLiteral));
    return *pictureInPicture;
#endif
}

static const AtomicString& presentationModeInline()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> inlineMode("inline", AtomicString::ConstructFromLiteral);
    return inlineMode;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, inlineMode, new AtomicString("inline", AtomicString::ConstructFromLiteral));
    return *inlineMode;
#endif
}

bool HTMLVideoElement::webkitSupportsPresentationMode(const String& mode) const
{
    if (mode == presentationModeFullscreen())
        return mediaSession().fullscreenPermitted(*this) && supportsFullscreen();

    if (mode == presentationModePictureInPicture())
        return wkIsOptimizedFullscreenSupported() && mediaSession().allowsPictureInPicture(*this) && supportsFullscreen();

    if (mode == presentationModeInline())
        return !mediaSession().requiresFullscreenForVideoPlayback(*this);

    return false;
}

void HTMLVideoElement::webkitSetPresentationMode(const String& mode)
{
    if (mode == presentationModeInline() && isFullscreen()) {
        exitFullscreen();
        return;
    }

    if (!mediaSession().fullscreenPermitted(*this) || !supportsFullscreen())
        return;

    LOG(Media, "HTMLVideoElement::webkitSetPresentationMode(%p) - setting to \"%s\"", this, mode.utf8().data());

    if (mode == presentationModeFullscreen())
        enterFullscreen(VideoFullscreenModeStandard);
    else if (mode == presentationModePictureInPicture())
        enterFullscreen(VideoFullscreenModePictureInPicture);
}

String HTMLVideoElement::webkitPresentationMode() const
{
    HTMLMediaElement::VideoFullscreenMode mode = fullscreenMode();

    if (mode == VideoFullscreenModeStandard)
        return presentationModeFullscreen();

    if (mode & VideoFullscreenModePictureInPicture)
        return presentationModePictureInPicture();

    if (mode == VideoFullscreenModeNone)
        return presentationModeInline();

    ASSERT_NOT_REACHED();
    return presentationModeInline();
}

void HTMLVideoElement::fullscreenModeChanged(VideoFullscreenMode mode)
{
    if (mode != fullscreenMode()) {
        LOG(Media, "HTMLVideoElement::fullscreenModeChanged(%p) - mode changed from %i to %i", this, fullscreenMode(), mode);
        scheduleEvent(eventNames().webkitpresentationmodechangedEvent);
    }

    if (player())
        player()->setVideoFullscreenMode(mode);

    HTMLMediaElement::fullscreenModeChanged(mode);
}

#endif

}

#endif
