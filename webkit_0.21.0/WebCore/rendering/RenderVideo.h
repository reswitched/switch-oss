/*
 * Copyright (C) 2007, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef RenderVideo_h
#define RenderVideo_h

#if ENABLE(VIDEO)

#include "RenderMedia.h"

namespace WebCore {

class HTMLVideoElement;

class RenderVideo final : public RenderMedia {
public:
    RenderVideo(HTMLVideoElement&, Ref<RenderStyle>&&);
    virtual ~RenderVideo();

    HTMLVideoElement& videoElement() const;

    IntRect videoBox() const;

    static IntSize defaultSize();

    bool supportsAcceleratedRendering() const;
    void acceleratedRenderingStateChanged();

    bool requiresImmediateCompositing() const;

    bool shouldDisplayVideo() const;

private:
    void mediaElement() const  = delete;

    virtual void updateFromElement() override;

    virtual void intrinsicSizeChanged() override;
    LayoutSize calculateIntrinsicSize();
    void updateIntrinsicSize();

    virtual void imageChanged(WrappedImagePtr, const IntRect*) override;

    virtual const char* renderName() const override { return "RenderVideo"; }

    virtual bool requiresLayer() const override { return true; }
    virtual bool isVideo() const override { return true; }

    virtual void paintReplaced(PaintInfo&, const LayoutPoint&) override;

    virtual void layout() override;

    virtual LayoutUnit computeReplacedLogicalWidth(ShouldComputePreferred  = ComputeActual) const override;
    virtual LayoutUnit computeReplacedLogicalHeight() const override;
    virtual LayoutUnit minimumReplacedHeight() const override;

#if ENABLE(FULLSCREEN_API)
    virtual LayoutUnit offsetLeft() const override;
    virtual LayoutUnit offsetTop() const override;
    virtual LayoutUnit offsetWidth() const override;
    virtual LayoutUnit offsetHeight() const override;
#endif

    void updatePlayer();

    virtual bool foregroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect, unsigned maxDepthToTest) const override;

    LayoutSize m_cachedImageSize;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderVideo, isVideo())

#endif // ENABLE(VIDEO)
#endif // RenderVideo_h
