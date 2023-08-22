/*
 * Copyright (C) 2016 Apple Inc.  All rights reserved.
 * Copyright (c) 2017-2021 ACCESS CO., LTD. All rights reserved.
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
#include "NativeImage.h"

#if USE(WKC_CAIRO)

#include "CairoOperations.h"
#include "PlatformContextCairo.h"
#include <cairo.h>
#include "NotImplemented.h"
#include "ImageWKC.h"

namespace WebCore {

IntSize nativeImageSize(const NativeImagePtr& image)
{
    return image ? image->size() : IntSize();
}

bool nativeImageHasAlpha(const NativeImagePtr& image)
{
    return !image || image->hasAlpha();
}

Color nativeImageSinglePixelSolidColor(const NativeImagePtr& image)
{
    if (!image || nativeImageSize(image) != IntSize(1, 1))
        return Color();

#if 0 // TODO:
    if (cairo_surface_get_type(image) != CAIRO_SURFACE_TYPE_IMAGE)
        return Color();
#endif

    char* const bitmap = reinterpret_cast<char*>(image->bitmap());
    const int type = image->type();

    if (type==ImageWKC::EColorARGB8888) {
        RGBA32* pixel = reinterpret_cast<RGBA32*>(bitmap);
        return colorFromPremultipliedARGB(*pixel);
    } else {
        // TODO:
        notImplemented();
        return Color();
    }
}

static bool
isSafeToRoundImagePosition(GraphicsContext& context)
{
    bool isDrawingOnOffscreen = wkcDrawContextIsForOffscreenPeer(context.platformContext());
    if (!isDrawingOnOffscreen)
        return false;

    const AffineTransform t = context.getCTM();
    if ((floor(t.a()) == t.a()) && t.a() > 0 && t.b() == 0 && t.c() == 0 && t.d() == t.a())
        return true;
    return false;
}

void drawNativeImage(const NativeImagePtr& image, GraphicsContext& context, const FloatRect& destRect, const FloatRect& srcRect, const IntSize&, const WebCore::ImagePaintingOptions& options)
{
    context.save();

    CompositeOperator op = options.compositeOperator();
    BlendMode mode = options.blendMode();
    ImageOrientation orientation = options.orientation();

    // Set the compositing operation.
    if (op == CompositeOperator::SourceOver && mode == BlendMode::Normal && !nativeImageHasAlpha(image))
        context.setCompositeOperation(CompositeOperator::Copy);
    else
        context.setCompositeOperation(op, mode);

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    IntSize scaledSize = nativeImageSize(image);
    FloatRect adjustedSrcRect = adjustSourceRectForDownSampling(srcRect, scaledSize);
#else
    FloatRect adjustedSrcRect(srcRect);
#endif

    FloatRect adjustedDestRect = destRect;

    if (orientation != ImageOrientation::Orientation::None) {
        // ImageOrientation expects the origin to be at (0, 0).
        context.translate(destRect.x(), destRect.y());
        adjustedDestRect.setLocation(FloatPoint());
        context.concatCTM(orientation.transformFromDefault(adjustedDestRect.size()));
        if (orientation.usesWidthAsHeight()) {
            // The destination rectangle will have it's width and height already reversed for the orientation of
            // the image, as it was needed for page layout, so we need to reverse it back here.
            adjustedDestRect.setSize(adjustedDestRect.size().transposedSize());
        }
    }

    if (isSafeToRoundImagePosition(context))
        adjustedDestRect = context.roundToDevicePixels(adjustedDestRect);

    cairo_format_t format;

    if (image->type() == ImageWKC::EColorARGB8888) {
        format = CAIRO_FORMAT_ARGB32;
    } else if (image->type() == ImageWKC::EColorRGB565) {
        format = CAIRO_FORMAT_RGB16_565;
    } else {
        ASSERT_NOT_REACHED();
    }

    cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*)image->bitmap(), format, image->size().width(), image->size().height(), image->rowbytes());
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(surface);
        context.restore();
        return;
    }

    Cairo::drawSurface(*context.platformContext(), surface, adjustedDestRect, adjustedSrcRect, context.imageInterpolationQuality(), context.alpha(), Cairo::ShadowState(context.state()));

    cairo_surface_destroy(surface);

    context.restore();
}

void clearNativeImageSubimages(const NativeImagePtr&)
{
}

} // namespace WebCore

#endif // USE(WKC_CAIRO)
