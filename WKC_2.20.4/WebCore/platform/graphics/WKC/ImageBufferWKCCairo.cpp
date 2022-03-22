/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Holger Hans Peter Freyther <zecke@selfish.org>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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

#if USE(WKC_CAIRO)

#include "ImageBuffer.h"

#include "Base64.h"
#include "BitmapImage.h"
#include "CairoUtilities.h"
#include "Color.h"
#include "GenericTypedArrayViewInlines.h"
#include "GraphicsContext.h"
#include "ImageBufferDataCairo.h"
#include "ImageData.h"
#include "ImageWKC.h"
#include "image-encoders/JPEGImageEncoder.h"
#include "JSGenericTypedArrayView.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Pattern.h"
#include "PlatformContextCairo.h"
#include "WTFString.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/Vector.h>

#if USE(WEBP)
#include <webp/encode.h>
#endif

#include <wkc/wkcgpeer.h>

// followings are originally from WebCore/platform/graphics/cairo/* .

using namespace std;

namespace WebCore {

ImageBufferData::ImageBufferData(const IntSize& size, RenderingMode renderingMode)
    :
#if ENABLE(ACCELERATED_2D_CANVAS)
      PlatformLayer(),
#endif
      m_surface(0)
    , m_platformContext(0)
    , m_size(size)
    , m_renderingMode(renderingMode)
#if ENABLE(ACCELERATED_2D_CANVAS)
#if USE(COORDINATED_GRAPHICS_THREADED)
    , m_compositorTexture(0)
#endif
    , m_texture(0)
#endif
{
#if ENABLE(ACCELERATED_2D_CANVAS) && USE(COORDINATED_GRAPHICS_THREADED)
    if (m_renderingMode == RenderingMode::Accelerated)
        m_platformLayerProxy = adoptRef(new TextureMapperPlatformLayerProxy);
#endif
}

#if ENABLE(ACCELERATED_2D_CANVAS) && !USE(COORDINATED_GRAPHICS_THREADED)
void ImageBufferData::paintToTextureMapper(TextureMapper*, const FloatRect& target, const TransformationMatrix&, float opacity)
{
    notImplemented();
}
#endif

ImageBufferData::~ImageBufferData()
{
}

ImageBuffer::ImageBuffer(const FloatSize& size, float resolutionScale, ColorSpace colorSpace, RenderingMode renderingMode, const HostWindow*, bool& success)
    : m_data(IntSize(size), renderingMode)
    , m_size(size)
    , m_logicalSize(size)
    , m_resolutionScale(1.f)
{
    success = false;  // Make early return mean error.

    m_data.m_surface = (cairo_surface_t *)wkcDrawContextCairoSurfaceNewPeer(size.width(), size.height());
    if (!m_data.m_surface.get() || (cairo_surface_status(m_data.m_surface.get()) != CAIRO_STATUS_SUCCESS)) {
        // Commented out the call to wkcMemoryNotifyNoMemoryPeer because we want to continue (skipping painting the error surface) without restart.
        // wkcMemoryNotifyNoMemoryPeer(size.width() * size.height() * 4);
        return ;  // create will notice we didn't set m_initialized and fail.
    }
    WKC_CAIRO_ADD_OBJECT(m_data.m_surface.get(), cairo_surface);

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface.get()));
    WKC_CAIRO_ADD_OBJECT(cr.get(), cairo);
    m_data.m_platformContext.setCr(cr.get());
    m_data.m_context = std::make_unique<GraphicsContext>(&m_data.m_platformContext);
    success = true;
}

ImageBuffer::~ImageBuffer()
{
    wkcDrawContextCairoSurfaceDeletePeer(m_data.m_surface.get());
}

GraphicsContext& ImageBuffer::context() const
{
    return *m_data.m_context;
}

RefPtr<Image> ImageBuffer::sinkIntoImage(std::unique_ptr<ImageBuffer> imageBuffer, PreserveResolution preserveResolution)
{
    return imageBuffer->copyImage(DontCopyBackingStore, preserveResolution);
}

RefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy, PreserveResolution) const
{
    cairo_surface_t* surface = m_data.m_surface.get();
    cairo_surface_type_t type = cairo_surface_get_type(surface);
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        surface = cairo_surface_map_to_image(surface, NULL);
    }

    void* newsurface = 0;
    cairo_surface_t* src = copyCairoImageSurface(surface).leakRef();
    if (!src)
        return nullptr;
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        cairo_surface_unmap_image(m_data.m_surface.get(), surface);
    }

    int format = cairo_image_surface_get_format(src) == CAIRO_FORMAT_RGB16_565 ? ImageWKC::EColorRGB565 : ImageWKC::EColorARGB8888;
    int src_rowbytes = cairo_image_surface_get_stride(src);
    int src_width = cairo_image_surface_get_width(src);
    int src_height = cairo_image_surface_get_height(src);


    WTF::TryMallocReturnValue rv = tryFastMalloc(src_rowbytes * src_height);
    if (!rv.getValue(newsurface)) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(src_rowbytes *src_height, WKC_MEMORYALLOC_TYPE_IMAGE);
        newsurface = 0;
    } else {
        memcpy(newsurface, cairo_image_surface_get_data(src), src_rowbytes *src_height);
    }

    WKC_CAIRO_REMOVE_OBJECT(src);
    cairo_surface_destroy(src);

    const IntSize size(src_width, src_height);
    Ref<ImageWKC> img = ImageWKC::create(format, newsurface, src_rowbytes, size, true);
    // BitmapImage will release the passed in surface on destruction
    return BitmapImage::create(WTFMove(img));
}

void ImageBuffer::clip(GraphicsContext& context, const FloatRect& maskRect) const
{
    context.platformContext()->pushImageMask(m_data.m_surface.get(), maskRect);
}

static RefPtr<ImageWKC>
createImageForSurface(cairo_surface_t* surface)
{
    int width = 0, height = 0;

    wkcDrawContextCairoSurfaceGetSizePeer((void *)surface, &width, &height);
    if (!width || !height)
        return nullptr;
    const IntSize size(width, height);

    bool isimage = wkcDrawContextCairoSurfaceIsImagePeer((void *)surface);
    if (!isimage)
        return nullptr;

    int rowbytes = cairo_image_surface_get_stride(surface);
    void *data = cairo_image_surface_get_data(surface);

    return ImageWKC::create(ImageWKC::EColorARGB8888, data, rowbytes, size, false);
}

void ImageBuffer::draw(GraphicsContext& in_context, const FloatRect& destRect, const FloatRect& srcRect, const ImagePaintingOptions& options)
{
    RefPtr<ImageWKC> img = createImageForSurface(m_data.m_surface.get());
    if (!img)
        return;

    // BitmapImage will release the passed in surface on destruction
    RefPtr<Image> image;
    if (&context() != &in_context) {
        image = BitmapImage::create(WTFMove(img));
    } else {
        image = copyImage();
    }
    if (!image)
        return;
    in_context.drawImage(*image, destRect, srcRect, options);
}

void ImageBuffer::drawPattern(GraphicsContext& in_context, const FloatRect& destRect, const FloatRect& srcRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, const FloatSize& spacing, const ImagePaintingOptions& options)
{
    RefPtr<ImageWKC> img = createImageForSurface(m_data.m_surface.get());
    if (!img)
        return;

    // BitmapImage will release the passed in surface on destruction
    RefPtr<Image> image;
    if (&context() != &in_context) {
        image = BitmapImage::create(WTFMove(img));
    } else {
        image = copyImage();
    }
    if (!image)
        return;
    image->drawPattern(in_context, destRect, srcRect, patternTransform, phase, spacing, options);
}

void ImageBuffer::platformTransformColorSpace(const std::array<uint8_t, 256>& lookUpTable)
{
    cairo_surface_type_t type = cairo_surface_get_type(m_data.m_surface.get());
    cairo_surface_t* surface = m_data.m_surface.get();
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        surface = cairo_surface_map_to_image(surface, NULL);
    }

    unsigned char* dataSrc = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    for (int y = 0; y < m_size.height(); ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * y);
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
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        cairo_surface_unmap_image(m_data.m_surface.get(), surface);
    }
    cairo_surface_mark_dirty_rectangle (m_data.m_surface.get(), 0, 0, m_size.width(), m_size.height());
}

template <AlphaPremultiplication multiplied>
RefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const ImageBufferData& data, const IntSize& size)
{
    cairo_surface_type_t type = cairo_surface_get_type(data.m_surface.get());
    cairo_surface_t* surface = data.m_surface.get();
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        surface = cairo_surface_map_to_image(surface, NULL);
    }

    // The area can overflow if the rect is too big.
    Checked<unsigned, RecordOverflow> area = 4;
    area *= rect.width();
    area *= rect.height();
    if (area.hasOverflowed())
        return nullptr;

    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(area.unsafeGet());
    if (!result || !result->data()) {
        wkcMemoryNotifyNoMemoryPeer(rect.width() * rect.height() * 4);
    }
    unsigned char* dataSrc = cairo_image_surface_get_data(surface);
    unsigned char* dataDst = result->data();

    if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > size.width() || (rect.y() + rect.height()) > size.height())
        memset(dataDst, 0, result->length());

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

    int stride = cairo_image_surface_get_stride(surface);
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;
            Color pixelColor;
            if (multiplied == AlphaPremultiplication::Unpremultiplied)
                pixelColor = colorFromPremultipliedARGB(*pixel);
            else
                pixelColor = Color(*pixel);
            destRows[basex]     = pixelColor.red();
            destRows[basex + 1] = pixelColor.green();
            destRows[basex + 2] = pixelColor.blue();
            destRows[basex + 3] = pixelColor.alpha();
        }
        destRows += destBytesPerRow;
    }

    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        cairo_surface_unmap_image(data.m_surface.get(), surface);
    }
    return result;
}

RefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, IntSize* pixelArrayDimensions, CoordinateSystem) const
{
    if (pixelArrayDimensions)
        *pixelArrayDimensions = rect.size();
    return getImageData<AlphaPremultiplication::Unpremultiplied>(rect, m_data, m_size);
}

RefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, IntSize* pixelArrayDimensions, CoordinateSystem) const
{
    if (pixelArrayDimensions)
        *pixelArrayDimensions = rect.size();
    return getImageData<AlphaPremultiplication::Premultiplied>(rect, m_data, m_size);
}

template <AlphaPremultiplication multiplied>
void putImageData(const Uint8ClampedArray& source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, ImageBufferData& data, const IntSize& size)
{
    cairo_surface_t* surface = data.m_surface.get();
    cairo_surface_type_t type = cairo_surface_get_type(surface);
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        surface = cairo_surface_map_to_image(surface, NULL);
    }

    unsigned char* dataDst = cairo_image_surface_get_data(surface);

    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.maxX());

    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= size.width());

    int numColumns = endx - destx;

    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.maxY());

    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= size.height());
    int numRows = endy - desty;

    unsigned srcBytesPerRow = 4 * sourceSize.width();
    int stride = cairo_image_surface_get_stride(surface);

    unsigned char* srcRows = source.data() + originy * srcBytesPerRow + originx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataDst + stride * (y + desty));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + destx;
            Color pixelColor(srcRows[basex],
                    srcRows[basex + 1],
                    srcRows[basex + 2],
                    srcRows[basex + 3]);
            if (multiplied == AlphaPremultiplication::Unpremultiplied)
                *pixel = premultipliedARGBFromColor(pixelColor);
            else
                *pixel = pixelColor.rgb();
        }
        srcRows += srcBytesPerRow;
    }
    if (type!=CAIRO_SURFACE_TYPE_IMAGE) {
        cairo_surface_unmap_image(data.m_surface.get(), surface);
    }
    cairo_surface_mark_dirty_rectangle (data.m_surface.get(),
                                        destx, desty,
                                        numColumns, numRows);
}

void ImageBuffer::putByteArray(const Uint8ClampedArray& source, AlphaPremultiplication bufferFormat, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem coord)
{
    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context().platformContext()), WKC_OFFSCREEN_FLUSH_FOR_READPIXELS);
    if (bufferFormat==AlphaPremultiplication::Premultiplied)
        putImageData<AlphaPremultiplication::Premultiplied>(source, sourceSize, sourceRect, destPoint, m_data, m_size);
    else
        putImageData<AlphaPremultiplication::Unpremultiplied>(source, sourceSize, sourceRect, destPoint, m_data, m_size);
}

static cairo_status_t writeFunction(void* closure, const unsigned char* data, unsigned int length)
{
    Vector<char>* in = reinterpret_cast<Vector<char>*>(closure);
    in->append(data, length);
    return CAIRO_STATUS_SUCCESS;
}

static bool encodeImage(cairo_surface_t* image, const String& mimeType, Vector<uint8_t>* output)
{
    ASSERT_UNUSED(mimeType, mimeType == "image/png"); // Only PNG output is supported for now.

    return cairo_surface_write_to_png_stream(image, writeFunction, output) == CAIRO_STATUS_SUCCESS;
}

String ImageBuffer::toDataURL(const String& mimeType, Optional<double> quality, PreserveResolution) const
{
#if CAIRO_HAS_PNG_FUNCTIONS
    cairo_surface_t* image = cairo_get_target(context().platformContext()->cr());
    if (!image)
        return "data:,";

    String actualMimeType("image/png");
    if (MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType))
        actualMimeType = mimeType;

    Vector<char> in;
    Vector<char> out;
    if (actualMimeType=="image/png") {
        if (cairo_surface_write_to_png_stream(image, writeFunction, &in)!=CAIRO_STATUS_SUCCESS)
            goto error_end;
    } else if (actualMimeType=="image/jpeg") {
        const IntSize size(cairo_image_surface_get_width(image), cairo_image_surface_get_height(image));
        unsigned char* data = cairo_image_surface_get_data(image);
        if (!data)
            goto error_end;
        WTF::TryMallocReturnValue ret = WTF::tryFastMalloc(size.width() * size.height() * 4);
        unsigned char* buf = 0;
        if (!ret.getValue(buf))
            goto error_end;
        const unsigned char* s = data;
        unsigned char* d = buf;
        for (int i=0; i<size.width()*size.height(); i++, s+=4, d+=4) {
#if CPU(BIG_ENDIAN)
            d[0] = s[1];
            d[1] = s[2];
            d[2] = s[3];
            d[3] = s[0];
#else
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
            d[3] = s[3];
#endif
        }
        bool result = compressRGBABigEndianToJPEG(buf, size, in, quality);
        fastFree(buf);
        if (!result)
            goto error_end;
#if USE(WEBP)
    } else if (actualMimeType=="image/webp") {
        unsigned char* data = cairo_image_surface_get_data(image);
        if (!data)
            goto error_end;
        int width = cairo_image_surface_get_width(image);
        int height = cairo_image_surface_get_height(image);
        size_t stride = cairo_image_surface_get_stride(image);
        float q = 80;
        if (quality)
            q = 100 * (*quality);
        if (q>100) q = 100;
        if (q<0) q = 10;
        uint8_t* buf = 0;
        size_t ret = WebPEncodeBGRA(data, width, height, stride, q, &buf);
        if (!ret)
            goto error_end;
        in.append(buf, ret);
        wkc_free(buf);
#endif
    } else {
        goto error_end;
    }

    base64Encode(in, out);
    return "data:" + actualMimeType + ";base64," + String(out.data(), out.size());

error_end:
#endif
    return "data:,";
}

Vector<uint8_t>
ImageBuffer::toData(const String& mimeType, Optional<double>) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    cairo_surface_t* image = cairo_get_target(context().platformContext()->cr());

    Vector<uint8_t> encodedImage;
    if (!image || !encodeImage(image, mimeType, &encodedImage))
        return{};

    return encodedImage;
}

BackingStoreCopy
ImageBuffer::fastCopyImageMode()
{
    return CopyBackingStore;
}

void ImageBuffer::drawConsuming(std::unique_ptr<ImageBuffer> imageBuffer, GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, const ImagePaintingOptions& options)
{
    imageBuffer->draw(destContext, destRect, srcRect, options);
}

NativeImagePtr ImageBuffer::nativeImage() const
{
    return createImageForSurface(m_data.m_surface.get());
}

} // namespace WebCore

#endif // USE(WKC_CAIRO)
