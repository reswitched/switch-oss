/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Holger Hans Peter Freyther <zecke@selfish.org>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (c) 2010-2014 ACCESS CO., LTD. All rights reserved.
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

#if !USE(WKC_CAIRO)

#include "ImageBuffer.h"

#include "Base64.h"
#include "BitmapImage.h"
#include "Color.h"
#include "GraphicsContext.h"
#include "ImageData.h"
#include "ImageWKC.h"
#include "MIMETypeRegistry.h"
#include "Pattern.h"
#include "WTFString.h"
#include "FastMalloc.h"

#include "NotImplemented.h"

#include <wkc/wkcgpeer.h>
#include <wkc/wkcmpeer.h>

#ifdef WKC_USE_OFFSCREEN_HW_ACCELERATION
# include "MainThread.h"
# include "GCController.h"
#endif

using namespace std;

namespace WebCore {

static bool
gUseBilinear(bool flag=false, bool set=false)
{
    WKC_DEFINE_STATIC_BOOL(gBilinear, false);
    if (set) {
        gBilinear = flag;
    }
    return (bool)gBilinear;
}

static bool
gUseAA(bool flag=false, bool set=false)
{
    WKC_DEFINE_STATIC_BOOL(gAA, false);
    if (set) {
        gAA = flag;
    }
    return (bool)gAA;
}

void
ImageBufferData::setUseBilinear(bool flag)
{
    gUseBilinear(flag, true);
}

void
ImageBufferData::setUseAA(bool flag)
{
    gUseAA(flag, true);
}

ImageBufferData::ImageBufferData(const IntSize& size)
    : m_image(0)
    , m_accelerateRendering(false)
{
}


ImageBuffer::ImageBuffer(const IntSize& size, float resolutionScale, ColorSpace colorSpace, RenderingMode renderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
    , m_resolutionScale(1.f)
{
    success = false;
    void* bitmap = 0;
    int rowbytes = 0;
    void* offscreen = 0;
    void* dc = 0;
    WKCSize osize;

    osize.fWidth = size.width();
    osize.fHeight = size.height();
    offscreen = wkcOffscreenNewPeer(WKC_OFFSCREEN_TYPE_IMAGEBUF, 0, 0, &osize);

#ifdef WKC_USE_OFFSCREEN_HW_ACCELERATION
    if (!offscreen) {
        if (WTF::isMainThread()) {
            WebCore::gcController().garbageCollectNow();
            offscreen = wkcOffscreenNewPeer(WKC_OFFSCREEN_TYPE_IMAGEBUF, 0, 0, &osize);
        }
    }
#endif

    if (!offscreen) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(osize.fWidth*osize.fHeight * 4, WKC_MEMORYALLOC_TYPE_IMAGE);
        return;
    }
    m_data.m_accelerateRendering = wkcOffscreenIsAcceleratedPeer(offscreen);

    bitmap = wkcOffscreenBitmapPeer(offscreen, &rowbytes);
    if (!bitmap && !m_data.m_accelerateRendering) {
        wkcOffscreenDeletePeer(offscreen);
        return;
    }

    dc = wkcDrawContextNewPeer(offscreen);
    if (!dc) {
        wkcOffscreenDeletePeer(offscreen);
        return;
    }

    if (bitmap && !m_data.m_accelerateRendering) {
        m_data.m_image = ImageWKC::create(bitmap, rowbytes, size, false);
        if (!m_data.m_image) {
            wkcMemoryNotifyMemoryAllocationErrorPeer(sizeof(ImageWKC), WKC_MEMORYALLOC_TYPE_IMAGE);
            wkcDrawContextDeletePeer(dc);
            wkcOffscreenDeletePeer(offscreen);
            return;
        }
    }

    m_context = adoptPtr(new GraphicsContext(dc));

    wkcOffscreenSetUseInterpolationForImagePeer(offscreen, gUseBilinear());
    wkcOffscreenSetUseAntiAliasForPolygonPeer(offscreen, gUseAA());

    FloatRect fr(0, 0, m_size.width(), m_size.height());
    m_context->clip(fr);

    if (m_data.m_accelerateRendering)
        wkcDrawContextClearPeer(dc);

    success = true;
}

ImageBuffer::~ImageBuffer()
{
    if (m_data.m_image) {
        delete m_data.m_image;
    }

    if (m_context) {
        void* dc = m_context->platformContext();
        void* offscreen = wkcDrawContextGetOffscreenPeer(dc);

        m_context.clear();

        wkcDrawContextDeletePeer(dc);
        wkcOffscreenDeletePeer(offscreen);
    }
}

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy, ScaleBehavior) const
{
    void* newsurface = 0;
    ImageWKC* img = 0;

    if (!this) {
        // tiny guard for HTMLCanvasElement::makePresentationCopy().
        return 0;
    }

    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);

    if (!m_data.m_image) {
        if (m_data.m_accelerateRendering)
        {
            // consider divide memory area for allocating image / others
            wkcMemorySetAllocatingForImagesPeer(true);
            WTF::TryMallocReturnValue rv = tryFastMalloc(m_size.width() * m_size.height() * 4);
            wkcMemorySetAllocatingForImagesPeer(false);
            if (!rv.getValue(newsurface)) {
                wkcMemoryNotifyMemoryAllocationErrorPeer(m_size.width() * m_size.height() * 4, WKC_MEMORYALLOC_TYPE_IMAGE);
                newsurface = 0;
            } else {
                WKCRect wkc_rect;
                WKCRect_SetXYWH(&wkc_rect, 0, 0, m_size.width(), m_size.height());
                wkcOffscreenGetPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                        newsurface,
                        &wkc_rect);
            }

            img = ImageWKC::create(newsurface, m_size.width()*4, m_size, true);
            if (!img) {
                wkcMemoryNotifyMemoryAllocationErrorPeer(sizeof(ImageWKC), WKC_MEMORYALLOC_TYPE_IMAGE);
                if (newsurface) {
                    fastFree(newsurface);
                }
                return 0;
            }

            return BitmapImage::create(img);
        }
        else {
            return NULL;
        }
    }

    // consider divide memory area for allocating image / others
    wkcMemorySetAllocatingForImagesPeer(true);
    WTF::TryMallocReturnValue rv = tryFastMalloc(m_data.m_image->rowbytes() * m_data.m_image->size().height());
    wkcMemorySetAllocatingForImagesPeer(false);
    if (!rv.getValue(newsurface)) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(m_data.m_image->rowbytes() * m_data.m_image->size().height(), WKC_MEMORYALLOC_TYPE_IMAGE);
        newsurface = 0;
    } else {
        memcpy(newsurface, m_data.m_image->bitmap(), m_data.m_image->rowbytes() * m_data.m_image->size().height());
    }

    img = ImageWKC::create(newsurface, m_data.m_image->rowbytes(), m_data.m_image->size(), true);
    if (!img) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(sizeof(ImageWKC), WKC_MEMORYALLOC_TYPE_IMAGE);
        if (newsurface) {
            fastFree(newsurface);
        }
        return 0;
    }

    return BitmapImage::create(img);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);

    if (!m_data.m_image) {
        if (m_data.m_accelerateRendering)
        {
            void* newsurface = 0;
            WKCRect wkc_rect;
            WKCRect_SetXYWH(&wkc_rect, 0, 0, m_size.width(), m_size.height());

            // consider divide memory area for allocating image / others
            wkcMemorySetAllocatingForImagesPeer(true);
            WTF::TryMallocReturnValue rv = tryFastMalloc(m_size.width() * m_size.height() * 4);
            wkcMemorySetAllocatingForImagesPeer(false);
            if (!rv.getValue(newsurface)) {
                wkcMemoryNotifyMemoryAllocationErrorPeer(m_size.width() * m_size.height() * 4, WKC_MEMORYALLOC_TYPE_IMAGE);
                newsurface = 0;
            } else {
                wkcOffscreenGetPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                        newsurface,
                        &wkc_rect);
            }

            if (newsurface) {
                unsigned char* dataSrc = (unsigned char *)newsurface;
                int stride = m_size.width() * 4;
                for (int y = 0; y < m_size.height(); ++y) {
                    unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * y);
                    for (int x = 0; x < m_size.width(); x++) {
                        unsigned* pixel = row + x;
                        Color pixelColor = colorFromPremultipliedARGB(*pixel);
//                       Color pixelColor(*pixel);
                        pixelColor = Color(lookUpTable[pixelColor.red()],
                                           lookUpTable[pixelColor.green()],
                                           lookUpTable[pixelColor.blue()],
                                           pixelColor.alpha());
                        *pixel = premultipliedARGBFromColor(pixelColor);
//                     *pixel = pixelColor.rgb();
                    }
                }

                wkcOffscreenPutPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                        newsurface,
                        &wkc_rect);
                fastFree(newsurface);
            }
            return;
        }
        else
        {
            return;
        }
    }

    unsigned char* dataSrc = (unsigned char *)m_data.m_image->bitmap();
    if (!dataSrc) return;
    int stride = m_data.m_image->rowbytes();
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
}

template <Multiply multiplied>
PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const ImageBufferData& data, const IntSize& size)
{
    PassRefPtr<Uint8ClampedArray> result = Uint8ClampedArray::create(rect.size().width()*rect.size().height()*4);
    if (!result) return result;
    if (!data.m_image) return result;
    unsigned char* dataSrc = (unsigned char *)data.m_image->bitmap();
    unsigned char* dataDst = result->data();
    if (!dataSrc) return result;
    if (!dataDst) return result;
    if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > size.width() || (rect.y() + rect.height()) > size.height())
        memset(dataDst, 0, result->length());

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.x() + rect.width();
    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.y() + rect.height();
    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

    int stride = data.m_image->rowbytes();
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;
            Color pixelColor;
            if (multiplied == Unmultiplied)
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

    return result;
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);
    if (!m_data.m_image) {
        if (m_data.m_accelerateRendering)
        {
            // read pixel data directly from the offscreen
            WKCRect wkc_rect;
            RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::create(rect.size().width()*rect.size().height()*4);

            unsigned char* dataDst = result->data();
            if (!dataDst)
                return result.release();

            if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > m_size.width() || (rect.y() + rect.height()) > m_size.height())
                memset(dataDst, 0, result->length());

            int originx = rect.x();
            int destx = 0;
            if (originx < 0) {
                destx = -originx;
                originx = 0;
            }
            int endx = rect.x() + rect.width();
            if (endx > m_size.width())
                endx = m_size.width();

            int originy = rect.y();
            int desty = 0;
            if (originy < 0) {
                desty = -originy;
                originy = 0;
            }
            int endy = rect.y() + rect.height();
            if (endy > m_size.height())
                endy = m_size.height();

            unsigned destBytesPerRow = 4 * rect.width();

            dataDst += desty * destBytesPerRow + destx * 4;

            WKCRect_SetXYWH(&wkc_rect, originx, originy, endx-originx, endy-originy);

            wkcOffscreenGetPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                    dataDst,
                    &wkc_rect);

            return result.release();
        }
           //else return an empty buffer later when getImageData gets called
    }
    return getImageData<Unmultiplied>(rect, m_data, m_size);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);
    if (!m_data.m_image) {
        if (m_data.m_accelerateRendering)
        {
            // read pixel data directly from the offscreen
            WKCRect wkc_rect;
            RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::create(rect.size().width()*rect.size().height()*4);

            unsigned char* dataDst = result->data();
            if (!dataDst)
                return result.release();

            if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > m_size.width() || (rect.y() + rect.height()) > m_size.height())
                memset(dataDst, 0, result->length());

            int originx = rect.x();
            int destx = 0;
            if (originx < 0) {
                destx = -originx;
                originx = 0;
            }
            int endx = rect.x() + rect.width();
            if (endx > m_size.width())
                endx = m_size.width();
            int numColumns = endx - originx;

            int originy = rect.y();
            int desty = 0;
            if (originy < 0) {
                desty = -originy;
                originy = 0;
            }
            int endy = rect.y() + rect.height();
            if (endy > m_size.height())
                endy = m_size.height();
            int numRows = endy - originy;

            int stride = rect.size().width()*4;         //probably the same with destBytesPerRow
            unsigned destBytesPerRow = 4 * rect.width();
            unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;

            dataDst += desty * destBytesPerRow + destx * 4;

            WKCRect_SetXYWH(&wkc_rect, originx, originy, endx-originx, endy-originy);

            wkcOffscreenGetPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                    dataDst,
                    &wkc_rect);

            //compute premultiplied data for internal usage - the data read from offscreen is not premultiplied
            unsigned char* dataSrc = dataDst;
            for (int y = 0; y < numRows; ++y) {
                unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * (y + originy));
                for (int x = 0; x < numColumns; x++) {
                    int basex = x * 4;
                    unsigned* pixel = row + x + originx;
                    Color pixelColor;
                       pixelColor = Color(*pixel);
                    *((unsigned*)(&destRows[basex])) = premultipliedARGBFromColor(pixelColor);
                }
                destRows += destBytesPerRow;
            }

            return result.release();
        }
        //else return an empty buffer later when getImageData gets called
    }
    return getImageData<Premultiplied>(rect, m_data, m_size);
}

template <Multiply multiplied>
void putImageData(Uint8ClampedArray* source, const IntRect& sourceRect, const IntPoint& destPoint, ImageBufferData& data, const IntSize& size)
{
    if (!sourceRect.width() || !sourceRect.height() || !size.width() || !size.height())
        return;

    if (!data.m_image) return;
    unsigned char* dataDst = (unsigned char *)data.m_image->bitmap();
    if (!dataDst) return;

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

    unsigned srcBytesPerRow = 4 * sourceRect.width();
    int stride = data.m_image->rowbytes();

    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataDst + stride * (y + desty));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + destx;
            Color pixelColor(srcRows[basex],
                    srcRows[basex + 1],
                    srcRows[basex + 2],
                    srcRows[basex + 3]);
            if (multiplied == Unmultiplied)
                *pixel = premultipliedARGBFromColor(pixelColor);
            else
                *pixel = pixelColor.rgb();
        }
        srcRows += srcBytesPerRow;
    }
}

void ImageBuffer::putByteArray(Multiply mult, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem coord)
{
    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);
    if (mult==Premultiplied) {
        if (!m_data.m_image) {
            if (m_data.m_accelerateRendering)
            {
                WKCRect wkc_rect;
                WKCRect_SetXYWH(&wkc_rect, destPoint.x(), destPoint.y(), sourceSize.width(), sourceSize.height());
                
                if (   (sourceRect.x() == 0)
                       && (sourceRect.y() == 0)
                       && (sourceRect.width() == sourceSize.width())
                       && (sourceRect.height() == sourceSize.height()) )
                {
                    void* dataSrc = source->data();
                    //FIXME: compute premultiplied RGBA
                    wkcOffscreenPutPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                                              dataSrc,
                                              &wkc_rect);
                }
                else // the src buffer must not include other pixels outside of the region that must be copied
                {
                    //this can be skipped for platforms that can handle different rowbyte values in the source
                    int pixelsNo = sourceRect.width()*sourceRect.height();
                    if (pixelsNo <= 0)
                        return;
                    
                    unsigned char* pixelsToCopy = NULL;
                    wkcMemorySetAllocatingForImagesPeer(true);
                    WTF::TryMallocReturnValue rv = tryFastMalloc(pixelsNo * 4);
                    wkcMemorySetAllocatingForImagesPeer(false);
                    if (!rv.getValue(pixelsToCopy)) {
                        wkcMemoryNotifyMemoryAllocationErrorPeer(pixelsNo * 4, WKC_MEMORYALLOC_TYPE_IMAGE);
                        return;
                    }
                    
//                memset(pixelsToCopy, 0, pixelsNo * 4);
                    
                    int originx = sourceRect.x();
                    int destx = 0;
                    ASSERT(originx >= 0);
                    ASSERT(originx <= sourceRect.maxX());
                    
//                int endx = sourceRect.width();
                    int numColumns = sourceRect.width();
                    
                    int originy = sourceRect.y();
                    int desty = 0;
                    ASSERT(originy >= 0);
                    ASSERT(originy <= sourceRect.maxY());
                    
//                int endy = sourceRect.height();
                    int numRows = sourceRect.height();
                    
                    unsigned srcBytesPerRow = 4 * sourceRect.width(); // isn't it sourceSize.width() ?
                    int stride = 4 * sourceRect.width();
                    
                    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
                    for (int y = 0; y < numRows; ++y) {
                        unsigned* row = reinterpret_cast<unsigned*>(pixelsToCopy + stride * (y + desty));
                        for (int x = 0; x < numColumns; x++) {
                            int basex = x * 4;
                            unsigned* pixel = row + x + destx;
//                        Color pixelColor(srcRows[basex],
//                                srcRows[basex + 1],
//                                srcRows[basex + 2],
//                                srcRows[basex + 3]);
                            Color pixelColor = colorFromPremultipliedARGB(*((unsigned*)(&srcRows[basex])));
                            *pixel = pixelColor.rgb();
                        }
                        srcRows += srcBytesPerRow;
                    }
                    
                    // now we have in dataDst exactly the pixels that have to be uploaded to the offscreen
                    wkcOffscreenPutPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                                              pixelsToCopy,
                                              &wkc_rect);
                    if (pixelsToCopy) {
                        fastFree(pixelsToCopy);
                    }
                }
                return;
            }
        }
        putImageData<Premultiplied>(source, sourceRect, destPoint, m_data, m_size);
    } else {
        if (!m_data.m_image) {
            if (m_data.m_accelerateRendering)
            {
                WKCRect wkc_rect;
                WKCRect_SetXYWH(&wkc_rect, destPoint.x(), destPoint.y(), sourceSize.width(), sourceSize.height());
                
                if (   (sourceRect.x() == 0)
                       && (sourceRect.y() == 0)
                       && (sourceRect.width() == sourceSize.width())
                       && (sourceRect.height() == sourceSize.height()) )
                {
                    void* dataSrc = source->data();
                    wkcOffscreenPutPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                                              dataSrc,
                                              &wkc_rect);
                }
                else // the src buffer must not include other pixels outside of the region that must be copied
                {
                    //this can be skipped for platforms that can handle different rowbyte values in the source
                    int pixelsNo = sourceRect.width()*sourceRect.height();
                    if (pixelsNo <= 0)
                        return;
                    
                    unsigned char* pixelsToCopy = NULL;
                    wkcMemorySetAllocatingForImagesPeer(true);
                    WTF::TryMallocReturnValue rv = tryFastMalloc(pixelsNo * 4);
                    wkcMemorySetAllocatingForImagesPeer(false);
                    if (!rv.getValue(pixelsToCopy)) {
                        wkcMemoryNotifyMemoryAllocationErrorPeer(pixelsNo * 4, WKC_MEMORYALLOC_TYPE_IMAGE);
                        return;
                    }
                    
//                memset(pixelsToCopy, 0, pixelsNo * 4);
                    
                    int originx = sourceRect.x();
                    int destx = 0;
                    ASSERT(originx >= 0);
                    ASSERT(originx <= sourceRect.maxX());
                    
//                int endx = sourceRect.width();
                    int numColumns = sourceRect.width();
                    
                    int originy = sourceRect.y();
                    int desty = 0;
                    ASSERT(originy >= 0);
                    ASSERT(originy <= sourceRect.maxY());
                    
//                int endy = sourceRect.height();
                    int numRows = sourceRect.height();
                    
                    unsigned srcBytesPerRow = 4 * sourceRect.width(); // isn't it sourceSize.width() ?
                    int stride = 4 * sourceRect.width();
                    
                    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
                    for (int y = 0; y < numRows; ++y) {
                        unsigned* row = reinterpret_cast<unsigned*>(pixelsToCopy + stride * (y + desty));
                        for (int x = 0; x < numColumns; x++) {
                            int basex = x * 4;
                            unsigned* pixel = row + x + destx;
                            Color pixelColor(srcRows[basex],
                                             srcRows[basex + 1],
                                             srcRows[basex + 2],
                                             srcRows[basex + 3]);
                            *pixel = pixelColor.rgb();
                        }
                        srcRows += srcBytesPerRow;
                    }
                    
                    // now we have in dataDst exactly the pixels that have to be uploaded to the offscreen
                    wkcOffscreenPutPixelsPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()),
                                              pixelsToCopy,
                                              &wkc_rect);
                    
                    if (pixelsToCopy) {
                        fastFree(pixelsToCopy);
                    }
                }
                return;
            }
        }
        putImageData<Unmultiplied>(source, sourceRect, destPoint, m_data, m_size);
    }
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    notImplemented();
    return "data:,";
}

void
ImageBuffer::clip(GraphicsContext*, const FloatRect&) const
{
    // Ugh!: implement it!
    // 110621 ACCESS Co.,Ltd.
    notImplemented();
}

void
ImageBuffer::draw(GraphicsContext* ctx, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
                  CompositeOperator op, BlendMode mode, bool useLowQualityScale)
{
    void* const dc = (void *)ctx->platformContext();

    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);

    void* offscreen = wkcDrawContextGetOffscreenPeer(context()->platformContext());
    ImageWKC* bitmap = m_data.m_image;
    if (!bitmap) {
        if (m_data.m_accelerateRendering) {
            if (!offscreen) {
                return;
            }
        } else {
            return;
        }
    }

    WKCPeerImage img = {0};
    img.fType = WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASALPHA | WKC_IMAGETYPE_FLAG_HASTRUEALPHA;
    if (bitmap) {
        img.fBitmap = bitmap->bitmap();
        img.fRowBytes = bitmap->rowbytes();
        img.fMask = bitmap->mask();
        img.fMaskRowBytes = bitmap->maskrowbytes();
    }
    if (m_data.m_accelerateRendering) {
        img.fOffscreen = offscreen;
        img.fTexture = NULL;
    }
    WKCFloatRect_SetXYWH(&img.fSrcRect, srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height());
    const WKCFloatRect dest = { destRect.x(), destRect.y(), destRect.width(), destRect.height() };
    WKCFloatSize_Set(&img.fScale, 1.f, 1.f);
    WKCFloatSize_Set(&img.fiScale, 1.f, 1.f);
    WKCFloatPoint_Set(&img.fPhase, 0, 0);
    WKCFloatSize_Set(&img.fiTransform, 1, 1);
    if (m_context->platformContext() != dc) {
        wkcDrawContextBitBltPeer(dc, &img, &dest, ImageWKC::platformOperator(op));
    } else {
        // draw to itself.
        if (m_data.m_accelerateRendering)
        {
            img.fOffscreen = offscreen;
            img.fTexture = NULL;
            wkcDrawContextBitBltPeer(dc, &img, &dest, ImageWKC::platformOperator(op));
        }
        else
        {
            RefPtr<Image> cimg = copyImage();
            ImageWKC* cbitmap = (ImageWKC *)cimg->nativeImageForCurrentFrame();
            if (cbitmap) {
                img.fBitmap = cbitmap->bitmap();
                img.fRowBytes = cbitmap->rowbytes();
                img.fMask = cbitmap->mask();
                img.fMaskRowBytes = cbitmap->maskrowbytes();
                wkcDrawContextBitBltPeer(dc, &img, &dest, ImageWKC::platformOperator(op));
            }
        }
    }
}

void
ImageBuffer::drawPattern(GraphicsContext* ctx, const FloatRect& srcRect, const AffineTransform& patternTransform,
                         const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    void* const dc = (void *)ctx->platformContext();

    wkcOffscreenFlushPeer(wkcDrawContextGetOffscreenPeer(context()->platformContext()), WKC_OFFSCREEN_FLUSH_FOR_DRAW);

    void* offscreen = wkcDrawContextGetOffscreenPeer(context()->platformContext());
    ImageWKC* bitmap = m_data.m_image;
    if (!bitmap) {
        if (m_data.m_accelerateRendering) {
            if (!offscreen) {
                return;
            }
        } else {
            return;
        }
    }


    WKCPeerImage img = {0};
    img.fType = WKC_IMAGETYPE_ARGB8888 | WKC_IMAGETYPE_FLAG_HASALPHA | WKC_IMAGETYPE_FLAG_HASTRUEALPHA;
    if (bitmap) {
        img.fBitmap = bitmap->bitmap();
        img.fRowBytes = bitmap->rowbytes();
        img.fMask = bitmap->mask();
        img.fMaskRowBytes = bitmap->maskrowbytes();
    }
    if (m_data.m_accelerateRendering) {
        img.fOffscreen = wkcDrawContextGetOffscreenPeer(context()->platformContext());
        img.fTexture = NULL;
    }
    WKCFloatRect_SetXYWH(&img.fSrcRect, srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height());
    const WKCFloatRect dest = { destRect.x(), destRect.y(), destRect.width(), destRect.height() };
    WKCFloatSize_Set(&img.fScale, 1.f, 1.f);
    WKCFloatSize_Set(&img.fiScale, 1.f, 1.f);
    WKCFloatPoint_Set(&img.fPhase, phase.x(), phase.y());
    WKCFloatSize_Set(&img.fiTransform, patternTransform.a(), patternTransform.d());
    if (m_context->platformContext() != dc) {
        wkcDrawContextBlitPatternPeer(dc, &img, &dest, ImageWKC::platformOperator(op));
    } else {
        if (m_data.m_accelerateRendering)
        {
            img.fOffscreen = offscreen;
            img.fTexture = NULL;
            wkcDrawContextBlitPatternPeer(dc, &img, &dest, ImageWKC::platformOperator(op));
        }
        else
        {
            RefPtr<Image> cimg = copyImage();
            ImageWKC* cbitmap = (ImageWKC *)cimg->nativeImageForCurrentFrame();
            if (cbitmap) {
                img.fBitmap = cbitmap->bitmap();
                img.fRowBytes = cbitmap->rowbytes();
                img.fMask = cbitmap->mask();
                img.fMaskRowBytes = cbitmap->maskrowbytes();
                wkcDrawContextBlitPatternPeer(dc, &img, &dest, ImageWKC::platformOperator(op));
            }
        }
    }
}


BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return DontCopyBackingStore;
}

} // namespace WebCore

#endif // !USE(WKC_CAIRO)
