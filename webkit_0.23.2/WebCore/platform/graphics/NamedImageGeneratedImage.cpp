/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "NamedImageGeneratedImage.h"

#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "Theme.h"

namespace WebCore {

NamedImageGeneratedImage::NamedImageGeneratedImage(String name, const FloatSize& size)
    : m_name(name)
{
    setContainerSize(size);
}

void NamedImageGeneratedImage::draw(GraphicsContext* context, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace, CompositeOperator compositeOp, BlendMode blendMode, ImageOrientationDescription)
{
#if USE(NEW_THEME)
    GraphicsContextStateSaver stateSaver(*context);
    context->setCompositeOperation(compositeOp, blendMode);
    context->clip(dstRect);
    context->translate(dstRect.x(), dstRect.y());
    if (dstRect.size() != srcRect.size())
        context->scale(FloatSize(dstRect.width() / srcRect.width(), dstRect.height() / srcRect.height()));
    context->translate(-srcRect.x(), -srcRect.y());

    platformTheme()->drawNamedImage(m_name, context, dstRect);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(dstRect);
    UNUSED_PARAM(srcRect);
    UNUSED_PARAM(compositeOp);
    UNUSED_PARAM(blendMode);
#endif
}

void NamedImageGeneratedImage::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, const FloatSize& spacing, ColorSpace styleColorSpace, CompositeOperator compositeOp, const FloatRect& dstRect, BlendMode blendMode)
{
#if USE(NEW_THEME)
//    std::unique_ptr<ImageBuffer> imageBuffer = ImageBuffer::create(size(), 1, ColorSpaceDeviceRGB, context->isAcceleratedContext() ? Accelerated : Unaccelerated);
    std::unique_ptr<ImageBuffer> imageBuffer = context->createCompatibleBuffer(size(), true);
    if (!imageBuffer)
        return;

    GraphicsContext* graphicsContext = imageBuffer->context();
    platformTheme()->drawNamedImage(m_name, graphicsContext, FloatRect(0, 0, size().width(), size().height()));

    // Tile the image buffer into the context.
    imageBuffer->drawPattern(context, srcRect, patternTransform, phase, spacing, styleColorSpace, compositeOp, dstRect, blendMode);
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(srcRect);
    UNUSED_PARAM(patternTransform);
    UNUSED_PARAM(phase);
    UNUSED_PARAM(spacing);
    UNUSED_PARAM(styleColorSpace);
    UNUSED_PARAM(dstRect);
    UNUSED_PARAM(compositeOp);
    UNUSED_PARAM(blendMode);
#endif
}

}
