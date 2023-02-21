/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "PageOverlay.h"

#include "FrameView.h"
#include "GraphicsContext.h"
#include "MainFrame.h"
#include "Page.h"
#include "PageOverlayController.h"
#include "PlatformMouseEvent.h"
#include "ScrollbarTheme.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

static const double fadeAnimationDuration = 0.2;
static const double fadeAnimationFrameRate = 30;

static PageOverlay::PageOverlayID generatePageOverlayID()
{
#if !PLATFORM(WKC)
    static PageOverlay::PageOverlayID pageOverlayID;
#else
    WKC_DEFINE_STATIC_TYPE(PageOverlay::PageOverlayID, pageOverlayID, 0);
#endif
    return ++pageOverlayID;
}

Ref<PageOverlay> PageOverlay::create(Client& client, OverlayType overlayType)
{
    return adoptRef(*new PageOverlay(client, overlayType));
}

PageOverlay::PageOverlay(Client& client, OverlayType overlayType)
    : m_client(client)
    , m_page(nullptr)
    , m_fadeAnimationTimer(*this, &PageOverlay::fadeAnimationTimerFired)
    , m_fadeAnimationStartTime(0)
    , m_fadeAnimationDuration(fadeAnimationDuration)
    , m_fadeAnimationType(NoAnimation)
    , m_fractionFadedIn(1)
    , m_overlayType(overlayType)
    , m_backgroundColor(Color::transparent)
    , m_pageOverlayID(generatePageOverlayID())
{
}

PageOverlay::~PageOverlay()
{
}

PageOverlayController* PageOverlay::controller() const
{
    if (!m_page)
        return nullptr;
    return &m_page->mainFrame().pageOverlayController();
}

IntRect PageOverlay::bounds() const
{
    if (!m_overrideFrame.isEmpty())
        return IntRect(IntPoint(), m_overrideFrame.size());

    FrameView* frameView = m_page->mainFrame().view();

    if (!frameView)
        return IntRect();

    switch (m_overlayType) {
    case OverlayType::View: {
        int width = frameView->width();
        int height = frameView->height();

        if (!ScrollbarTheme::theme()->usesOverlayScrollbars()) {
            if (frameView->verticalScrollbar())
                width -= frameView->verticalScrollbar()->width();
            if (frameView->horizontalScrollbar())
                height -= frameView->horizontalScrollbar()->height();
        }
        return IntRect(0, 0, width, height);
    }
    case OverlayType::Document:
        return IntRect(IntPoint(), frameView->contentsSize());
    }

    ASSERT_NOT_REACHED();
    return IntRect(IntPoint(), frameView->contentsSize());
}

IntRect PageOverlay::frame() const
{
    if (!m_overrideFrame.isEmpty())
        return m_overrideFrame;

    return bounds();
}

void PageOverlay::setFrame(IntRect frame)
{
    if (m_overrideFrame == frame)
        return;

    m_overrideFrame = frame;

    if (auto pageOverlayController = controller())
        pageOverlayController->didChangeOverlayFrame(*this);
}

IntSize PageOverlay::viewToOverlayOffset() const
{
    switch (m_overlayType) {
    case OverlayType::View:
        return IntSize();

    case OverlayType::Document: {
        FrameView* frameView = m_page->mainFrame().view();
        return frameView ? toIntSize(frameView->viewToContents(IntPoint())) : IntSize();
    }
    }
    return IntSize();
}

void PageOverlay::setBackgroundColor(RGBA32 backgroundColor)
{
    if (m_backgroundColor == backgroundColor)
        return;

    m_backgroundColor = backgroundColor;

    if (auto pageOverlayController = controller())
        pageOverlayController->didChangeOverlayBackgroundColor(*this);
}

void PageOverlay::setPage(Page* page)
{
    m_client.willMoveToPage(*this, page);
    m_page = page;
    m_client.didMoveToPage(*this, page);

    m_fadeAnimationTimer.stop();
}

void PageOverlay::setNeedsDisplay(const IntRect& dirtyRect)
{
    if (auto pageOverlayController = controller()) {
        pageOverlayController->setPageOverlayOpacity(*this, m_fractionFadedIn);
        pageOverlayController->setPageOverlayNeedsDisplay(*this, dirtyRect);
    }
}

void PageOverlay::setNeedsDisplay()
{
    setNeedsDisplay(bounds());
}

void PageOverlay::drawRect(GraphicsContext& graphicsContext, const IntRect& dirtyRect)
{
    // If the dirty rect is outside the bounds, ignore it.
    IntRect paintRect = intersection(dirtyRect, bounds());
    if (paintRect.isEmpty())
        return;

    GraphicsContextStateSaver stateSaver(graphicsContext);
    m_client.drawRect(*this, graphicsContext, paintRect);
}
    
bool PageOverlay::mouseEvent(const PlatformMouseEvent& mouseEvent)
{
    IntPoint mousePositionInOverlayCoordinates(mouseEvent.position());

    if (m_overlayType == PageOverlay::OverlayType::Document)
        mousePositionInOverlayCoordinates = m_page->mainFrame().view()->windowToContents(mousePositionInOverlayCoordinates);

    // Ignore events outside the bounds.
    if (!bounds().contains(mousePositionInOverlayCoordinates))
        return false;

    return m_client.mouseEvent(*this, mouseEvent);
}

void PageOverlay::didScrollFrame(Frame& frame)
{
    m_client.didScrollFrame(*this, frame);
}

bool PageOverlay::copyAccessibilityAttributeStringValueForPoint(String attribute, FloatPoint parameter, String& value)
{
    return m_client.copyAccessibilityAttributeStringValueForPoint(*this, attribute, parameter, value);
}

bool PageOverlay::copyAccessibilityAttributeBoolValueForPoint(String attribute, FloatPoint parameter, bool& value)
{
    return m_client.copyAccessibilityAttributeBoolValueForPoint(*this, attribute, parameter, value);
}

Vector<String> PageOverlay::copyAccessibilityAttributeNames(bool parameterizedNames)
{
    return m_client.copyAccessibilityAttributeNames(*this, parameterizedNames);
}

void PageOverlay::startFadeInAnimation()
{
    m_fractionFadedIn = 0;
    m_fadeAnimationType = FadeInAnimation;

    startFadeAnimation();
}

void PageOverlay::startFadeOutAnimation()
{
    m_fractionFadedIn = 1;
    m_fadeAnimationType = FadeOutAnimation;

    startFadeAnimation();
}

void PageOverlay::stopFadeOutAnimation()
{
    m_fractionFadedIn = 1.0;
    m_fadeAnimationTimer.stop();
}

void PageOverlay::startFadeAnimation()
{
    m_fadeAnimationStartTime = currentTime();
    m_fadeAnimationTimer.startRepeating(1 / fadeAnimationFrameRate);
}

void PageOverlay::fadeAnimationTimerFired()
{
    float animationProgress = (currentTime() - m_fadeAnimationStartTime) / m_fadeAnimationDuration;

    if (animationProgress >= 1.0)
        animationProgress = 1.0;

    double sine = sin(piOverTwoFloat * animationProgress);
    float fadeAnimationValue = sine * sine;

    m_fractionFadedIn = (m_fadeAnimationType == FadeInAnimation) ? fadeAnimationValue : 1 - fadeAnimationValue;
    controller()->setPageOverlayOpacity(*this, m_fractionFadedIn);

    if (animationProgress == 1.0) {
        m_fadeAnimationTimer.stop();

        bool wasFadingOut = m_fadeAnimationType == FadeOutAnimation;
        m_fadeAnimationType = NoAnimation;

        // If this was a fade out, uninstall the page overlay.
        if (wasFadingOut)
            controller()->uninstallPageOverlay(this, PageOverlay::FadeMode::DoNotFade);
    }
}

void PageOverlay::clear()
{
    if (auto pageOverlayController = controller())
        pageOverlayController->clearPageOverlay(*this);
}

GraphicsLayer& PageOverlay::layer()
{
    return controller()->layerForOverlay(*this);
}

} // namespace WebKit
