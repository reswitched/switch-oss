/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Holger Hans Peter Freyther <zecke@selfish.org>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "ImageBuffer.h"

#if USE(CAIRO)

#include "BitmapImage.h"
#include "CairoUtilities.h"
#include "Color.h"
#include "GraphicsContext.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Pattern.h"
#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <wtf/Vector.h>
#include <wtf/text/Base64.h>
#include <wtf/text/WTFString.h>

#if ENABLE(ACCELERATED_2D_CANVAS)
#include "GLContext.h"
#include "OpenGLShims.h"
#include "TextureMapperGL.h"
#include <cairo-gl.h>
#endif

using namespace std;

namespace WebCore {

ImageBufferData::ImageBufferData(const IntSize& size)
    : m_platformContext(0)
    , m_size(size)
#if ENABLE(ACCELERATED_2D_CANVAS)
    , m_texture(0)
#endif
{
}

#if ENABLE(ACCELERATED_2D_CANVAS)
PassRefPtr<cairo_surface_t> createCairoGLSurface(const FloatSize& size, uint32_t& texture)
{
    GLContext::sharingContext()->makeContextCurrent();

    // We must generate the texture ourselves, because there is no Cairo API for extracting it
    // from a pre-existing surface.
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA8, size.width(), size.height(), 0 /* border */, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    GLContext* context = GLContext::sharingContext();
    cairo_device_t* device = context->cairoDevice();

    // Thread-awareness is a huge performance hit on non-Intel drivers.
    cairo_gl_device_set_thread_aware(device, FALSE);

    return adoptRef(cairo_gl_surface_create_for_texture(device, CAIRO_CONTENT_COLOR_ALPHA, texture, size.width(), size.height()));
}
#endif

ImageBuffer::ImageBuffer(const FloatSize& size, float /* resolutionScale */, ColorSpace, RenderingMode renderingMode, bool& success)
    : m_data(IntSize(size))
    , m_size(size)
    , m_logicalSize(size)
{
    success = false;  // Make early return mean error.
    if (m_size.isEmpty())
        return;

#if ENABLE(ACCELERATED_2D_CANVAS)
    if (renderingMode == Accelerated)
        m_data.m_surface = createCairoGLSurface(size, m_data.m_texture);
    else
#else
    ASSERT_UNUSED(renderingMode, renderingMode != Accelerated);
#endif
        m_data.m_surface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height()));

    if (cairo_surface_status(m_data.m_surface.get()) != CAIRO_STATUS_SUCCESS)
        return;  // create will notice we didn't set m_initialized and fail.

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface.get()));
    m_data.m_platformContext.setCr(cr.get());
    m_data.m_context = std::make_unique<GraphicsContext>(&m_data.m_platformContext);
    success = true;
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext* ImageBuffer::context() const
{
    return m_data.m_context.get();
}

RefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, ScaleBehavior) const
{
    if (copyBehavior == CopyBackingStore)
        return BitmapImage::create(copyCairoImageSurface(m_data.m_surface.get()));

    // BitmapImage will release the passed in surface on destruction
    return BitmapImage::create(m_data.m_surface);
}

BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return DontCopyBackingStore;
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& maskRect) const
{
    context->platformContext()->pushImageMask(m_data.m_surface.get(), maskRect);
}

void ImageBuffer::draw(GraphicsContext* destinationContext, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
    CompositeOperator op, BlendMode blendMode, bool useLowQualityScale)
{
    BackingStoreCopy copyMode = destinationContext == context() ? CopyBackingStore : DontCopyBackingStore;
    RefPtr<Image> image = copyImage(copyMode);
    destinationContext->drawImage(image.get(), styleColorSpace, destRect, srcRect, ImagePaintingOptions(op, blendMode, ImageOrientationDescription(), useLowQualityScale));
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, const FloatSize& spacing, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode)
{
    if (RefPtr<Image> image = copyImage(DontCopyBackingStore))
        image->drawPattern(context, srcRect, patternTransform, phase, spacing, styleColorSpace, op, destRect);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    // FIXME: Enable color space conversions on accelerated canvases.
    if (cairo_surface_get_type(m_data.m_surface.get()) != CAIRO_SURFACE_TYPE_IMAGE)
        return;

    unsigned char* dataSrc = cairo_image_surface_get_data(m_data.m_surface.get());
    int stride = cairo_image_surface_get_stride(m_data.m_surface.get());
    for (int y = 0; y < m_size.height(); ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(dataSrc + stride * y);
        for (int x = 0; x < m_size.width(); x++) {
            unsigned* pixel = row + x;
            Color pixelColor = colorFromPremultipliedARGB(*pixel);
            pixelColor = Color(lookUpTable[pixelColor.red()],
                               lookUpTable[pixelColor.green()],
                               lookUpTable[pixelColor.blue()],
                               pixelColor.alpha());
            *pixel = premultipliedARGBFromColor(pixelColor);
        }
    }
    cairo_surface_mark_dirty_rectangle(m_data.m_surface.get(), 0, 0, m_size.width(), m_size.height());
}

PassRefPtr<cairo_surface_t> copySurfaceToImageAndAdjustRect(cairo_surface_t* surface, IntRect& rect)
{
    cairo_surface_type_t surfaceType = cairo_surface_get_type(surface);

    // If we already have an image, we write directly to the underlying data;
    // otherwise we create a temporary surface image
    if (surfaceType == CAIRO_SURFACE_TYPE_IMAGE)
        return surface;
    
    rect.setX(0);
    rect.setY(0);
    return adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rect.width(), rect.height()));
}

template <Multiply multiplied>
PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const ImageBufferData& data, const IntSize& size)
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);

    if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > size.width() || (rect.y() + rect.height()) > size.height())
        result->zeroFill();

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

    IntRect imageRect(originx, originy, numColumns, numRows);
    RefPtr<cairo_surface_t> imageSurface = copySurfaceToImageAndAdjustRect(data.m_surface.get(), imageRect);
    originx = imageRect.x();
    originy = imageRect.y();
    if (imageSurface != data.m_surface.get()) {
        IntRect area = intersection(rect, IntRect(0, 0, size.width(), size.height()));
        copyRectFromOneSurfaceToAnother(data.m_surface.get(), imageSurface.get(), IntSize(-area.x(), -area.y()), IntRect(IntPoint(), area.size()), IntSize(), CAIRO_OPERATOR_SOURCE);
    }

    unsigned char* dataSrc = cairo_image_surface_get_data(imageSurface.get());
    unsigned char* dataDst = result->data();
    int stride = cairo_image_surface_get_stride(imageSurface.get());
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;

            // Avoid calling Color::colorFromPremultipliedARGB() because one
            // function call per pixel is too expensive.
            unsigned alpha = (*pixel & 0xFF000000) >> 24;
            unsigned red = (*pixel & 0x00FF0000) >> 16;
            unsigned green = (*pixel & 0x0000FF00) >> 8;
            unsigned blue = (*pixel & 0x000000FF);

            if (multiplied == Unmultiplied) {
                if (alpha && alpha != 255) {
                    red = red * 255 / alpha;
                    green = green * 255 / alpha;
                    blue = blue * 255 / alpha;
                }
            }

            destRows[basex]     = red;
            destRows[basex + 1] = green;
            destRows[basex + 2] = blue;
            destRows[basex + 3] = alpha;
        }
        destRows += destBytesPerRow;
    }

    return result.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<Unmultiplied>(rect, m_data, m_size);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<Premultiplied>(rect, m_data, m_size);
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{

    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < m_size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.maxX());

    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= m_size.width());

    int numColumns = endx - destx;

    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < m_size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.maxY());

    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= m_size.height());
    int numRows = endy - desty;

    IntRect imageRect(destx, desty, numColumns, numRows);
    RefPtr<cairo_surface_t> imageSurface = copySurfaceToImageAndAdjustRect(m_data.m_surface.get(), imageRect);
    destx = imageRect.x();
    desty = imageRect.y();

    unsigned char* pixelData = cairo_image_surface_get_data(imageSurface.get());

    unsigned srcBytesPerRow = 4 * sourceSize.width();
    int stride = cairo_image_surface_get_stride(imageSurface.get());

    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(pixelData + stride * (y + desty));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + destx;

            // Avoid calling Color::premultipliedARGBFromColor() because one
            // function call per pixel is too expensive.
            unsigned red = srcRows[basex];
            unsigned green = srcRows[basex + 1];
            unsigned blue = srcRows[basex + 2];
            unsigned alpha = srcRows[basex + 3];

            if (multiplied == Unmultiplied) {
                if (alpha != 255) {
                    red = (red * alpha + 254) / 255;
                    green = (green * alpha + 254) / 255;
                    blue = (blue * alpha + 254) / 255;
                }
            }

            *pixel = (alpha << 24) | red  << 16 | green  << 8 | blue;
        }
        srcRows += srcBytesPerRow;
    }

    cairo_surface_mark_dirty_rectangle(imageSurface.get(), destx, desty, numColumns, numRows);

    if (imageSurface != m_data.m_surface.get())
        copyRectFromOneSurfaceToAnother(imageSurface.get(), m_data.m_surface.get(), IntSize(), IntRect(0, 0, numColumns, numRows), IntSize(destPoint.x() + sourceRect.x(), destPoint.y() + sourceRect.y()), CAIRO_OPERATOR_SOURCE);
}

#if !PLATFORM(GTK) && !PLATFORM(EFL)
static cairo_status_t writeFunction(void* output, const unsigned char* data, unsigned int length)
{
    if (!reinterpret_cast<Vector<unsigned char>*>(output)->tryAppend(data, length))
        return CAIRO_STATUS_WRITE_ERROR;
    return CAIRO_STATUS_SUCCESS;
}

static bool encodeImage(cairo_surface_t* image, const String& mimeType, Vector<char>* output)
{
    ASSERT_UNUSED(mimeType, mimeType == "image/png"); // Only PNG output is supported for now.

    return cairo_surface_write_to_png_stream(image, writeFunction, output) == CAIRO_STATUS_SUCCESS;
}

String ImageBuffer::toDataURL(const String& mimeType, const double*, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    cairo_surface_t* image = cairo_get_target(context()->platformContext()->cr());

    Vector<char> encodedImage;
    if (!image || !encodeImage(image, mimeType, &encodedImage))
        return "data:,";

    Vector<char> base64Data;
    base64Encode(encodedImage, base64Data);

    return "data:" + mimeType + ";base64," + base64Data;
}
#endif

#if ENABLE(ACCELERATED_2D_CANVAS)
void ImageBufferData::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    ASSERT(m_texture);

    // Cairo may change the active context, so we make sure to change it back after flushing.
    GLContext* previousActiveContext = GLContext::getCurrent();
    cairo_surface_flush(m_surface.get());
    previousActiveContext->makeContextCurrent();

    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(m_texture, TextureMapperGL::ShouldBlend, m_size, targetRect, matrix, opacity);
}
#endif

PlatformLayer* ImageBuffer::platformLayer() const
{
#if ENABLE(ACCELERATED_2D_CANVAS)
    if (m_data.m_texture)
        return const_cast<ImageBufferData*>(&m_data);
#endif
    return 0;
}

} // namespace WebCore

#endif // USE(CAIRO)
