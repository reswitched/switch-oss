/*
 * Copyright (C) 2007 Apple Computer, Kevin Ollivier.  All rights reserved.
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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
#include "ImageWKC.h"

#include "BitmapImage.h"
#include "GraphicsContext.h"
#include "ImageDecoder.h"
#include "ImageObserver.h"
#include "FastMalloc.h"
#include "TransformationMatrix.h"
#include "FloatConversion.h"
#include "ImageSource.h"
#include "Timer.h"

#include "NotImplemented.h"

#if USE(WKC_CAIRO)
#include "PlatformContextCairo.h"
#include "CairoUtilities.h"
#endif

#include <wkc/wkcgpeer.h>

using namespace std;

namespace WebCore {

static int gInternalFormat = ImageWKC::EColorRGB565;
static bool gReduceTo565IfPossible = false;

void
ImageWKC::setInternalColorFormatARGB8888(bool reduceifpossible)
{
    gInternalFormat = EColorARGB8888;
    gReduceTo565IfPossible = reduceifpossible;
}

void
ImageWKC::setInternalColorFormatRGB565()
{
    gInternalFormat = EColorRGB565;
    gReduceTo565IfPossible = false;
}


static inline void platformRect(const FloatRect& in, WKCFloatRect& out)
{
    out.fX = in.x();
    out.fY = in.y();
    out.fWidth = in.width();
    out.fHeight = in.height();
}

int
ImageWKC::platformOperator(CompositeOperator op)
{
    switch (op) {
    case CompositeClear:
        return WKC_COMPOSITEOPERATION_CLEAR;
    case CompositeCopy:
        return WKC_COMPOSITEOPERATION_COPY;
    case CompositeSourceOver:
        return WKC_COMPOSITEOPERATION_SOURCEOVER;
    case CompositeSourceIn:
        return WKC_COMPOSITEOPERATION_SOURCEIN;
    case CompositeSourceOut:
        return WKC_COMPOSITEOPERATION_SOURCEOUT;
    case CompositeSourceAtop:
        return WKC_COMPOSITEOPERATION_SOURCEATOP;
    case CompositeDestinationOver:
        return WKC_COMPOSITEOPERATION_DESTINATIONOVER;
    case CompositeDestinationIn:
        return WKC_COMPOSITEOPERATION_DESTINATIONIN;
    case CompositeDestinationOut:
        return WKC_COMPOSITEOPERATION_DESTINATIONOUT;
    case CompositeDestinationAtop:
        return WKC_COMPOSITEOPERATION_DESTINATIONATOP;
    case CompositeXOR:
        return WKC_COMPOSITEOPERATION_XOR;
    case CompositePlusDarker:
        return WKC_COMPOSITEOPERATION_PLUSDARKER;
    case CompositePlusLighter:
        return WKC_COMPOSITEOPERATION_PLUSLIGHTER;
    default:
        return 0;
    }
}

ImageWKC::ImageWKC(int type, void* bitmap, int rowbytes, const IntSize& size, bool ownbitmap, bool onstack)
    : m_refcount(1)
    , m_type(type)
    , m_bitmap(bitmap)
    , m_rowbytes(rowbytes)
    , m_size(size)
    , m_ownbitmap(ownbitmap)
    , m_onstack(onstack)
    , m_hasAlpha(false)
    , m_scalex(1)
    , m_scaley(1)
    , m_allowReduceColor(gReduceTo565IfPossible)
    , m_offscreen(0)
{
    switch (type) {
    case EColorARGB8888:
        m_bpp = 4;
        break;
    case EColorRGB565:
        m_bpp = 2;
        m_allowReduceColor = false;
        break;
    default:
        ASSERT_NOT_REACHED();
        m_bpp = 4;
        break;
    }
}

ImageWKC*
ImageWKC::create(int type, void* bitmap, int rowbytes, const IntSize& size, bool ownbitmap)
{
    return new ImageWKC(type, bitmap, rowbytes, size, ownbitmap);
}

ImageWKC::~ImageWKC()
{
    if (!m_ownbitmap)
        return;
    if (m_bitmap) {
        WTF::fastFree(m_bitmap);
        m_bitmap = 0;
    }
#if !USE(WKC_CAIRO)
    if (m_offscreen) {
        delete m_offscreen;
        m_offscreen = 0;
    }
#endif
}

void
ImageWKC::ref()
{
    m_refcount++;
}

void
ImageWKC::unref()
{
    m_refcount--;
    if (!m_refcount & !m_onstack)
        delete this;
}

bool
ImageWKC::resize(const IntSize& size)
{
    if (!m_ownbitmap)
        return false;

    int width = size.width();
    if ((m_bpp == 2) && (width & 1))
        width++; // rowbytes must be 4-byte align.

    const int len = width * size.height();
    void* newbitmap = 0;
    bool allocSucceeded = false;

    if (wkcMemoryCheckMemoryAllocatablePeer(len * m_bpp, wkcMemoryGetAllocationStatePeer())) {
        wkcMemorySetAllocatingForImagesPeer(true);
        WTF::TryMallocReturnValue rv = WTF::tryFastMalloc(len * m_bpp);
        wkcMemorySetAllocatingForImagesPeer(false);
        allocSucceeded = rv.getValue(newbitmap);
    }
    if (!allocSucceeded) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(len * m_bpp, wkcMemoryGetAllocationStatePeer());
        return false;
    }
    if (m_bitmap)
        WTF::fastFree(m_bitmap);
    m_bitmap = newbitmap;
    m_rowbytes = width * m_bpp;
    m_size = size;

#if !USE(WKC_CAIRO)
    if (m_offscreen) {
        delete m_offscreen;
        m_offscreen = 0;
    }

    WKCSize osize = {m_size.width(), m_size.height()};
    //calculate the optimal size tile according with the width and height
    //FIXME: replace 2048 with a value read from the peer
    unsigned int max_tile_width = 2048;
    unsigned int max_tile_height = 2048;
    if (m_size.width() < max_tile_width)
        max_tile_width = m_size.width();
    if (m_size.height() < max_tile_height)
        max_tile_height = m_size.height();

    WKCSize tile_size = {max_tile_width, max_tile_height};

    m_offscreen = new ImageTilesWKC(osize, tile_size);
#endif
    zeroFill();

    return true;
}

void
ImageWKC::clear()
{
    if (!m_ownbitmap)
        return;
    if (m_bitmap) {
        WTF::fastFree(m_bitmap);
        m_bitmap = 0;
    }
#if !USE(WKC_CAIRO)
    if (m_offscreen) {
        delete m_offscreen;
        m_offscreen = 0;
    }
#endif
}

void
ImageWKC::zeroFill()
{
    const int len = m_rowbytes * m_size.height();

    if (m_bitmap) {
        ::memset(m_bitmap, 0, len);
    }

#if !USE(WKC_CAIRO)
    if (m_offscreen) {
        const WKCFloatRect idst = {0,0, m_size.width(), m_size.height()};
        m_offscreen->ClearRect(idst);
    }
#endif
}

void
ImageWKC::zeroFillRect(const IntRect& r)
{
    if (!m_bitmap)
        return;
    int x0 = WKC_MAX(0, r.x());
    int x1 = WKC_MIN(m_size.width(), r.maxX());
    int len = (x1 - x0) * m_bpp;
    if (len<=0)
        return;

    int y0 = WKC_MAX(0, r.y());
    int y1 = WKC_MIN(m_size.height(), r.maxY());
    if (m_bpp==4) {
        unsigned int* dst = (unsigned int *)m_bitmap + y0*m_size.width() + x0;
        for (int y=y0; y<y1; y++, dst+=m_size.width()) {
            memset(dst, 0, len);
        }
    } else if (m_bpp==2) {
        unsigned short* dst = (unsigned short *)m_bitmap + y0*m_size.width() + x0;
        for (int y=y0; y<y1; y++, dst+=m_size.width()) {
            memset(dst, 0, len);
        }
    }

#if !USE(WKC_CAIRO)
    if (m_offscreen) {
        const WKCFloatRect idst = {r.x(),r.y(), r.width(), r.height()};
        m_offscreen->ClearRect(idst);
    }
#endif
}

void
ImageWKC::notifyStatus(int status, bool hasduration)
{
#if !USE(WKC_CAIRO)
    if (status!=ImageFrame::FrameComplete)
        return;

    if (!m_bitmap)
        return;

    if (m_offscreen) {
        //blit the bitmap to the external surface/texture/etc.
        WKCPeerImage img = {0};
        img.fType = WKC_IMAGETYPE_ARGB8888;
        img.fBitmap = m_bitmap;
        img.fRowBytes = m_rowbytes;
        img.fMask = m_mask;
        img.fMaskRowBytes = m_maskrowbytes;
        WKCFloatRect_SetXYWH(&img.fSrcRect, 0, 0, m_size.width(), m_size.height());
        WKCFloatSize_Set(&img.fScale, 1.f, 1.f);
        WKCFloatSize_Set(&img.fiScale, 1.f, 1.f);
        WKCFloatPoint_Set(&img.fPhase, 0, 0);
        WKCFloatSize_Set(&img.fiTransform, 1, 1);
        img.fOffscreen = 0;
        const WKCFloatRect idst = {0,0, m_size.width(), m_size.height()};
        m_offscreen->BitBlt(&img, &idst);
    }
#else
    if (!m_allowReduceColor)
        return;
    if (!m_ownbitmap)
        return;
    if (status!=ImageFrame::FrameComplete)
        return;
    if (m_hasAlpha)
        return;
    if (m_type!=EColorARGB8888)
        return;
    if (hasduration)
        return;

    int w = m_size.width();
    if (w&1)
        w++; // rowbytes must be 4-byte align.

    WTF::TryMallocReturnValue rv = WTF::tryFastMalloc(w * m_size.height() * 2);
    unsigned short* newimg = 0;
    if (!rv.getValue(newimg))
        return;

    const unsigned int* src = (const unsigned int *)m_bitmap;
    for (int y=0; y<m_size.height(); y++) {
        unsigned short* dest = (unsigned short *)newimg + y*w;
        for (int x=0; x<m_size.width(); x++) {
            const unsigned int v = *src++;
            *dest++ = (unsigned short)(((v>>8)&0xf800) | ((v>>5)&0x07e0) | ((v>>3)&0x1f));
        }
    }

    fastFree(m_bitmap);

    m_bitmap = newimg;
    m_rowbytes = w * 2;
    m_bpp = 2;
    m_type = EColorRGB565;
#endif
}

bool
ImageWKC::copyImage(const ImageWKC* other, bool hasduration)
{
    if (!resize(IntSize(other->size())))
        return false;

    m_rowbytes = other->rowbytes();                               

    m_hasAlpha = other->hasAlpha();
    m_scalex = other->scalex();
    m_scaley = other->scaley();
    m_allowReduceColor = other->allowReduceColor();

    if (!other->bitmap()) {
        ::memset(m_bitmap, 0, m_rowbytes*m_size.height());
        notifyStatus(ImageFrame::FrameComplete);
        return true;
    }

    ::memcpy(m_bitmap, other->bitmap(), m_rowbytes*m_size.height());
    notifyStatus(ImageFrame::FrameComplete, hasduration);

    return true;
}

void*
ImageWKC::bitmap(bool /*retain*/) const
{
    return m_bitmap;
}

void
ImageWKC::setAllowReduceColor(bool flag)
{
    m_allowReduceColor = flag;

    if (!gReduceTo565IfPossible)
        m_allowReduceColor = false;
}

BitmapImage::BitmapImage(PassNativeImagePtr in_image, ImageObserver* in_observer)
    : Image(in_observer)
    , m_minimumSubsamplingLevel(0)
    , m_imageOrientation(OriginTopLeft)
    , m_shouldRespectImageOrientation(false)
    , m_currentFrame(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_desiredFrameStartTime(0)
    , m_decodedSize(0)
    , m_decodedPropertiesSize(0)
    , m_frameCount(1)
    , m_allowSubsampling(false)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_hasUniformFrameSize(false)
    , m_haveFrameCount(true)
    , m_animationFinishedWhenCatchingUp(false)
    , m_frames(0)
    , m_frameTimer(nullptr)
{
    ImageWKC* img = (ImageWKC *)in_image;

    int width = img->size().width();
    int height = img->size().height();
    bool hasalpha = false;
    switch (img->type()) {
    case ImageWKC::EColorARGB8888:
        hasalpha = true;
        break;
    case ImageWKC::EColorRGB565:
        hasalpha = false;
        break;
    default:
        hasalpha = false;
        break;
    }

    checkForSolidColor();

    m_decodedSize = img->rowbytes() * height;

    m_size = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_frame = in_image;
    m_frames[0].m_hasAlpha = hasalpha;
    m_frames[0].m_haveMetadata = true;
}

void BitmapImage::invalidatePlatformData()
{
}

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata) {
        m_haveMetadata = false;
    }

    if (m_frame) {
        ImageWKC* img = reinterpret_cast<ImageWKC*>(m_frame);
        img->unref();
        m_frame = 0;
        return true;
    }
    return false;
}

#if USE(WKC_CAIRO)
static bool
isSafeToRoundImagePosition(GraphicsContext* context)
{
    bool isDrawingOnOffscreen = wkcDrawContextIsForOffscreenPeer(context->platformContext());
    if (!isDrawingOnOffscreen)
        return false;

    const AffineTransform t = context->getCTM();
    if ((floor(t.a()) == t.a()) && t.a() > 0 && t.b() == 0 && t.c() == 0 && t.d() == t.a())
        return true;
    return false;
}
#endif

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode, ImageOrientationDescription)
{
    if (dst.width() == 0.0f || dst.height() == 0.0f || src.width() == 0.0f || src.height() == 0.0f)
        return;

    // Spin the animation to the correct frame before we try to draw it, so we
    // don't draw an old frame and then immediately need to draw a newer one,
    // causing flicker and wasting CPU.
    startAnimation();

    ImageWKC* bitmap = (ImageWKC *)frameAtIndex(m_currentFrame);

    if (!bitmap) // If it's too early we won't have an image yet.
        return;
#if USE(WKC_CAIRO)
    if (!bitmap->isBitmapAvail())
        return;
    if (bitmap->type() != ImageWKC::EColorARGB8888 && bitmap->type() != ImageWKC::EColorRGB565)
        return; // Not supported.

    FloatRect srcRect(src);
    FloatRect dstRect(dst);

    if (isSafeToRoundImagePosition(context))
        dstRect = context->roundToDevicePixels(dstRect);

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(context, dstRect, solidColor(), styleColorSpace, op);
        return;
    }

    if (bitmap->type()==ImageWKC::EColorARGB8888) {
        cairo_surface_t* image = cairo_image_surface_create_for_data((unsigned char*)bitmap->bitmap(), CAIRO_FORMAT_ARGB32, bitmap->size().width(), bitmap->size().height(), bitmap->rowbytes());
        if (cairo_surface_status( image ) != CAIRO_STATUS_SUCCESS) {
            cairo_surface_destroy(image);
            return;
        }
        WKC_CAIRO_ADD_OBJECT(image, cairo_surface);

        context->save();

        if (op==CompositeSourceOver && blendMode==BlendModeNormal && (!frameHasAlphaAtIndex(m_currentFrame)/* || wkcDrawContextIsForLayerPeer(context->platformContext())*/))
            context->setCompositeOperation(CompositeCopy);
        else
            context->setCompositeOperation(op, blendMode);
        context->platformContext()->drawSurfaceToContext(image, dstRect, srcRect, context);
        WKC_CAIRO_REMOVE_OBJECT(image);
        cairo_surface_destroy(image);

        context->restore();
    } else if (bitmap->type()==ImageWKC::EColorRGB565) {
        cairo_surface_t* image = cairo_image_surface_create_for_data((unsigned char*)bitmap->bitmap(), CAIRO_FORMAT_RGB16_565, bitmap->size().width(), bitmap->size().height(), bitmap->rowbytes());
        if (cairo_surface_status( image ) != CAIRO_STATUS_SUCCESS) {
            cairo_surface_destroy(image);
            return;
        }
        WKC_CAIRO_ADD_OBJECT(image, cairo_surface);

        context->save();

        context->setCompositeOperation(op, blendMode);
        context->platformContext()->drawSurfaceToContext(image, dstRect, srcRect, context);
        WKC_CAIRO_REMOVE_OBJECT(image);
        cairo_surface_destroy(image);

        context->restore();
    }
#else
    void* dc = (void *)context->platformContext();
    int type = WKC_IMAGETYPE_ARGB8888;
    WKCFloatRect idst;
    WKCFloatRect isrc;

    double scalex = 1.f;
    double scaley = 1.f;

    switch (bitmap->type()) {
    case ImageWKC::EColorARGB8888:
        type = WKC_IMAGETYPE_ARGB8888;
        break;
    case ImageWKC::EColorRGB565:
        type = WKC_IMAGETYPE_RGB565;
        break;
    default:
        goto end;
    }
    if (frameHasAlphaAtIndex(m_currentFrame)) {
        type |= WKC_IMAGETYPE_FLAG_HASALPHA;
        if (bitmap->hasAlpha()) {
            type |= WKC_IMAGETYPE_FLAG_HASTRUEALPHA;
        }
    }

    platformRect(dst, idst);
    platformRect(src, isrc);

    scalex = bitmap->scalex();
    scaley = bitmap->scaley();
    isrc.fX = ceil((double)isrc.fX * scalex);
    isrc.fY = ceil((double)isrc.fY * scaley);
    isrc.fWidth = floor((double)isrc.fWidth * scalex);
    isrc.fHeight = floor((double)isrc.fHeight * scaley);

    if (isrc.fWidth && isrc.fHeight) {
        if (bitmap->bitmap()) {
            WKCPeerImage img = {0};
            img.fType = type;
            img.fBitmap = bitmap->bitmap();
            img.fRowBytes = bitmap->rowbytes();
            img.fMask = bitmap->mask();
            img.fMaskRowBytes = bitmap->maskrowbytes();
            WKCFloatRect_SetXYWH(&img.fSrcRect, isrc.fX, isrc.fY, isrc.fWidth, isrc.fHeight);
            WKCFloatSize_Set(&img.fScale, scalex, scaley);
            img.fiScale.fWidth = ((img.fScale.fWidth!= 0) ? 1.f / img.fScale.fWidth : 0);
            img.fiScale.fHeight = ((img.fScale.fHeight!= 0) ? 1.f / img.fScale.fHeight : 0);
            WKCFloatPoint_Set(&img.fPhase, 0, 0);
            WKCFloatSize_Set(&img.fiTransform, 1, 1);
            if (bitmap->offscreen())
                bitmap->offscreen()->BitBltToDC(dc, &img, idst, ImageWKC::platformOperator(op));
            else
                wkcDrawContextBitBltPeer(dc, &img, &idst, ImageWKC::platformOperator(op));
        } else {
            context->save();
            context->setStrokeThickness(1.0);
            context->setFillColor(Color(0xffffffff), context->fillColorSpace());
            context->drawRect(IntRect(dst));
            context->restore();
        }
    }

end:
#endif
    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

void Image::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, const FloatSize& spacing, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode blendMode)
{
    void* dc = (void *)context->platformContext();
    ImageWKC* bitmap = (ImageWKC *)nativeImageForCurrentFrame();

    // Avoid NaN
    if (!isfinite(phase.x()) || !isfinite(phase.y()))
        return;
    if (!patternTransform.a() || !patternTransform.d())
        return;
    if (!bitmap) // If it's too early we won't have an image yet.
        return;
#if USE(WKC_CAIRO)
    if (!bitmap->isBitmapAvail())
        return;

    cairo_surface_t* image = 0;
    cairo_t* cr = ((PlatformContextCairo*)dc)->cr();

    cairo_operator_t cop;
    if (blendMode == BlendModeNormal)
        cop = toCairoOperator(op);
    else
        cop = toCairoOperator(blendMode);

    IntSize isize(size());
    if (bitmap->type() == ImageWKC::EColorARGB8888) {
        image = cairo_image_surface_create_for_data((unsigned char*)bitmap->bitmap(), CAIRO_FORMAT_ARGB32, bitmap->size().width(), bitmap->size().height(), bitmap->rowbytes());
        WKC_CAIRO_ADD_OBJECT(image, cairo_surface);
        drawPatternToCairoContext(cr, image, isize, srcRect, patternTransform, phase, cop, destRect);
        WKC_CAIRO_REMOVE_OBJECT(image);
        cairo_surface_destroy(image);
    } else if (bitmap->type() == ImageWKC::EColorRGB565) {
        image = cairo_image_surface_create_for_data((unsigned char*)bitmap->bitmap(), CAIRO_FORMAT_RGB16_565, bitmap->size().width(), bitmap->size().height(), bitmap->rowbytes());
        WKC_CAIRO_ADD_OBJECT(image, cairo_surface);
        drawPatternToCairoContext(cr, image, isize, srcRect, patternTransform, phase, cop, destRect);
        WKC_CAIRO_REMOVE_OBJECT(image);
        cairo_surface_destroy(image);
    }
#else
    int type = WKC_IMAGETYPE_ARGB8888;
    WKCFloatRect isrc;
    WKCFloatRect idst;
    WKCFloatPoint iphase;
    WKCFloatSize itransform;
    WKCFloatSize srcscale;

    switch (bitmap->type()) {
    case ImageWKC::EColorARGB8888:
        type = WKC_IMAGETYPE_ARGB8888;
        break;
    case ImageWKC::EColorRGB565:
        type = WKC_IMAGETYPE_RGB565;
        break;
    default:
        goto end;
    }
    if (bitmap->hasAlpha()) {
        type |= WKC_IMAGETYPE_FLAG_HASALPHA;
        if (bitmap->hasAlpha()) {
            type |= WKC_IMAGETYPE_FLAG_HASTRUEALPHA;
        }
    }

    context->save();
    context->clip(IntRect(destRect.x(), destRect.y(), destRect.width(), destRect.height()));

    platformRect(srcRect, isrc);
    platformRect(destRect, idst);
    iphase.fX = phase.x();
    iphase.fY = phase.y();
    itransform.fWidth = 1.0/patternTransform.a();
    itransform.fHeight = 1.0/patternTransform.d();

    srcscale.fWidth = (float)bitmap->scalex();
    srcscale.fHeight = (float)bitmap->scaley();
    isrc.fX = isrc.fX * srcscale.fWidth;
    isrc.fY = isrc.fY * srcscale.fHeight;
    isrc.fWidth = isrc.fWidth * srcscale.fWidth;
    isrc.fHeight = isrc.fHeight * srcscale.fHeight;
    iphase.fX = iphase.fX * srcscale.fWidth;
    iphase.fY = iphase.fY * srcscale.fHeight;

    if (isrc.fWidth && isrc.fHeight && itransform.fWidth && itransform.fHeight) {
        if (bitmap->bitmap()) {
            WKCPeerImage img = {0};
            img.fType = type;
            img.fBitmap = bitmap->bitmap();
            img.fRowBytes = bitmap->rowbytes();
            img.fMask = bitmap->mask();
            img.fMaskRowBytes = bitmap->maskrowbytes();
            WKCFloatRect_SetXYWH(&img.fSrcRect, isrc.fX, isrc.fY, isrc.fWidth, isrc.fHeight);
            WKCFloatSize_SetSize(&img.fiTransform, &itransform);
            WKCFloatPoint_SetPoint(&img.fPhase, &iphase);
            WKCFloatSize_SetSize(&img.fScale, &srcscale);
            img.fiScale.fWidth = ((img.fScale.fWidth!= 0) ? 1.f / img.fScale.fWidth : 0);
            img.fiScale.fHeight = ((img.fScale.fHeight!= 0) ? 1.f / img.fScale.fHeight : 0);
            img.fOffscreen = 0;
            if (bitmap->offscreen())
                bitmap->offscreen()->BitBltPatternToDC(dc, &img, idst, ImageWKC::platformOperator(op));
            else
               wkcDrawContextBlitPatternPeer(dc, &img, &idst, ImageWKC::platformOperator(op));
        } else {
            context->setStrokeThickness(1.0);
            context->setFillColor(Color(0xffffffff), context->fillColorSpace());
            context->drawRect(IntRect(destRect));
        }
    }

    context->restore();

    startAnimation();

end:
#endif
    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = false;
    m_checkedForSolidColor = true;

    if (frameCount() > 1)
        return;

    ImageWKC* img = reinterpret_cast<ImageWKC*>(frameAtIndex(0));
    if (!img) {
        m_checkedForSolidColor = true;
        return;
    }
    const int w = img->size().width();
    const int h = img->size().height();
    if (w*h>256)
        return;

    const char* const bitmap = reinterpret_cast<const char*>(img->bitmap());
    if (!bitmap)
        return;
    const unsigned int br = img->rowbytes();
    const int type = img->type();

    int r=0,g=0,b=0;

    if (type==ImageWKC::EColorARGB8888) {
        const unsigned int v=*(reinterpret_cast<const unsigned int *>(bitmap));
        if (w>1||h>1)
            for (int y=0; y<h; y++)
                for (int x=0; x<w; x++)
                    if (v!=*(reinterpret_cast<const unsigned int *>(bitmap + y*br + x*4)))
                        return;

        m_solidColor = colorFromPremultipliedARGB(v);
    } else if (type==ImageWKC::EColorRGB565) {
        const unsigned short v = *(reinterpret_cast<const unsigned short *>(bitmap));
        if (w>1||h>1) {
            for (int y=0; y<h; y++)
                for (int x=0; x<w; x++)
                    if (v!=*(reinterpret_cast<const unsigned short *>(bitmap + y*br + x*2)))
                        return;
        }

        r = ((v>>8)&0xf8);
        g = ((v>>3)&0xf8);
        b = ((v<<3)&0xf8);
        if (r) r|=0x1f;
        if (g) g|=0x1f;
        if (b) b|=0x1f;
        m_solidColor = Color(r,g,b,255);
    } else {
        return;
    }

    m_isSolidColor = true;
}

float Image::platformResourceScale()
{
    return wkcStockImageGetImageScalePeer();
}

PassRefPtr<Image> Image::loadPlatformResource(const char* name)
{
    const unsigned char* bitmap = 0;
    unsigned int width=0, height=0;

    bitmap = wkcStockImageGetPlatformResourceImagePeer(name, &width, &height);
    if (bitmap) {
        ImageWKC* wimg = ImageWKC::create();
        if (wimg->resize(IntSize(width, height))) {
            for (int y=0; y<height; y++) {
                const unsigned int* p = (unsigned int *)bitmap + y*width;
                unsigned int* d = (unsigned int *)wimg->decodebuffer() + width*y;
                for (int x=0; x<width; x++) {
                    unsigned int v = *p++;
                    unsigned int a = (v>>24)&0xff;
                    unsigned int r = (v>>16)&0xff;
                    unsigned int g = (v>>8)&0xff;
                    unsigned int b = (v)&0xff;
                    *d++ = (a<<24) | (r<<16) | (g<<8) | b;
                }
            }
            RefPtr<Image> img = BitmapImage::create(wimg);
            return img.release();
        }
        wimg->unref();
    }

last:
    RefPtr<Image> img = BitmapImage::create();
    Vector<char> arr;
    RefPtr<SharedBuffer> buffer = SharedBuffer::create(arr.data(), arr.size());
    img->setData(buffer, true);
    return img.release();
}

void BitmapImage::determineMinimumSubsamplingLevel() const
{
    m_minimumSubsamplingLevel = 0;
}

#if !USE(WKC_CAIRO)
//Tiled image implementation

ImgTile::ImgTile()
    : m_texture(0)
{
}

ImgTile::ImgTile(const WKCSize& size)
    : m_texture(0)
{
    m_texture = wkcTextureNewPeer(&size);
}

ImgTile::~ImgTile()
{
    if(m_texture){
        wkcTextureDeletePeer(m_texture);
    }
}

void
ImgTile::Clip(WKCFloatRect& r)
{

}

void
ImgTile::ClearRect(WKCFloatRect& r)
{
    wkcTextureClearImagePeer(m_texture, &r);
}

void
ImgTile::Blit(const WKCPeerImage* img, WKCFloatRect& dst) const
{
    wkcTextureSetImagePeer(m_texture,(void*)img);
}

void
ImgTile::BlitToDC(void* in_context, WKCPeerImage* img, WKCFloatRect& dst, int in_op) const
{
    img->fTexture = m_texture;
    wkcDrawContextBitBltPeer(in_context, img, &dst, in_op);
//    wkcDrawContextFlushPeer(in_context);
}
void
ImgTile::BlitPatternToDC(void* in_context, WKCPeerImage* img, WKCFloatRect& dst, int in_op) const
{
    img->fTexture = m_texture;
    wkcDrawContextBlitPatternPeer(in_context, img, &dst, in_op);
//    wkcDrawContextFlushPeer(in_context);
}

/**
 * @brief     creates a new tiled image object
 * @param    size    the size in pixels of the image that will be stored
 */
ImageTilesWKC::ImageTilesWKC(const WKCSize& size, const WKCSize& maxTileSize)
    : m_size(size)
    , m_numColumns(0)
    , m_maxTileSize(maxTileSize)
{
    m_numColumns = ((m_size.fWidth - 1) / m_maxTileSize.fWidth) + 1;
    int num_tiles = (((m_size.fHeight - 1) / m_maxTileSize.fHeight + 1) * m_numColumns);
    // create each tile
    for (int i = 0; i < num_tiles; i++ ) {
            ImgTile* t = new ImgTile(m_maxTileSize);
            m_tiles.append(t);
    }
}

/**
 * @brief     destroys a tiled image object
 */
ImageTilesWKC::~ImageTilesWKC()
{

    //delete tiles ?
    for (int i = 0; i < numTiles(); i++ ) {
            ImgTile* t = m_tiles.at(i);
            delete t;
    }
}

FloatRect
ImageTilesWKC::tileRect(int xIndex, int yIndex) const
{
    ASSERT(xIndex < m_numColumns);
    ASSERT((yIndex * m_numColumns) + xIndex < m_tiles.size());

    int x = xIndex * m_maxTileSize.fWidth;
    int y = yIndex * m_maxTileSize.fHeight;

    return FloatRect(x, y,
        ((m_maxTileSize.fWidth < m_size.fWidth - x) ? m_maxTileSize.fWidth : (m_size.fWidth - x)),
        ((m_maxTileSize.fHeight < m_size.fHeight - y) ? m_maxTileSize.fHeight : (m_size.fHeight - y)));
}

IntRect
ImageTilesWKC::tilesInRect(const FloatRect& rect) const
{
    int leftIndex = static_cast<int>(rect.x()) / m_maxTileSize.fWidth;
    int topIndex = static_cast<int>(rect.y()) / m_maxTileSize.fHeight;

    if (leftIndex < 0)
        leftIndex = 0;
    if (topIndex < 0)
        topIndex = 0;

    // Round rect edges up to get the outer pixel boundaries.
    int rightIndex = (static_cast<int>(ceil(rect.x()+rect.width())) - 1) / m_maxTileSize.fWidth;
    int bottomIndex = (static_cast<int>(ceil(rect.y()+rect.height())) - 1) / m_maxTileSize.fHeight;
    int columns = (rightIndex - leftIndex) + 1;
    int rows = (bottomIndex - topIndex) + 1;

    return IntRect(leftIndex, topIndex,
        (columns <= m_numColumns) ? columns : m_numColumns,
        (rows <= (m_tiles.size() / m_numColumns)) ? rows : (m_tiles.size() / m_numColumns));
}

const ImgTile*
ImageTilesWKC::tile(int xIndex, int yIndex) const
{
    if (!(xIndex < m_numColumns))
            return 0;
    int i = (yIndex * m_numColumns) + xIndex;
    if (!(i < m_tiles.size()))
            return 0;
    return m_tiles[i];
}


static inline void WKCFloatRect_SetFloatRect(WKCFloatRect* dr, const FloatRect& sr) {
    WKCFloatRect_SetXYWH(dr, sr.x(), sr.y(), sr.width(), sr.height());
}

static inline void FloatRect_SetWKCFloatRect(FloatRect& dr, const WKCFloatRect& sr) {
    dr.setX(sr.fX);
    dr.setY(sr.fY);
    dr.setWidth(sr.fWidth);
    dr.setHeight(sr.fHeight);
}


/**
 * @brief     sets a square clipping mask for the whole image into all tiles that are intersecting it
 */
void
ImageTilesWKC::Clip(WKCFloatRect& r)
{
    //for each tile
    for (int i = 0; i < numTiles(); i++ )
    {
        // tile rect
        FloatRect ftr = tileRect(i/m_numColumns, i%m_numColumns);
        WKCFloatRect tr;
        WKCFloatRect_SetFloatRect(&tr, ftr);
        //if tile_rect intersects clip_rect
        if (WKCFloatRect_Intersects(&tr, &r)) {
            //clip tile to intersection
            WKCFloatRect intersection;
            WKCFloatRect_Intersect(&tr, &r, &intersection);
            m_tiles.at(i)->Clip(intersection);
        }
    }
}


/**
 * @brief     clears a square area of the whole image into all tiles that are intersecting it
 */
void
ImageTilesWKC::ClearRect(const WKCFloatRect& r)
{
    //for each tile
    for (int i = 0; i < numTiles(); i++ )
    {
        // tile rect
        FloatRect ftr = tileRect(i%m_numColumns, i/m_numColumns);
        WKCFloatRect tr;
        WKCFloatRect_SetFloatRect(&tr, ftr);
        //if tile_rect intersects clip_rect
        if (WKCFloatRect_Intersects(&tr, &r)) {
            //clip tile to intersection
            WKCFloatRect intersect;
            WKCFloatRect_Intersect(&tr, &r, &intersect);
            // translate intersection rect to 0,0 to clear the actual tile
            WKCFloatRect_SetXYWH(&intersect, 0, 0, intersect.fWidth, intersect.fHeight);
            m_tiles.at(i)->ClearRect(intersect);
        }
    }
}

/**
 * @brief     blits the given peer image into all tiles that are intersecting it
 * @note     currently src and dst are assumed to be equal and starting in 0,0
 *             therefore no translation is needed
 * @note    the clipping of the whole area is assumed to be already done
 */
void
ImageTilesWKC::BitBlt(WKCPeerImage* in_image, const WKCFloatRect* in_destrect)
{
    if (!in_image)
        return;

    FloatRect dst;
    FloatRect_SetWKCFloatRect(dst, *in_destrect);
    FloatRect src;
    FloatRect_SetWKCFloatRect(src, (in_image->fSrcRect));

    //FIXME: Need a better zero comparison
    if ((src!=dst) || (src.location()!=FloatPoint::zero()) || (dst.location()!=FloatPoint::zero()))
        return;

    IntRect drawnTiles = tilesInRect(src);

    for (int yIndex = drawnTiles.y(); yIndex < drawnTiles.y()+drawnTiles.height(); ++yIndex) {
        for (int xIndex = drawnTiles.x(); xIndex < drawnTiles.x()+drawnTiles.width(); ++xIndex) {
            // The srcTile rectangle is an aligned tile cropped by the src rectangle.
            FloatRect tile_rect(tileRect(xIndex, yIndex));
            FloatRect intersect = intersection(src, tile_rect);

            WKCFloatRect_SetFloatRect(&(in_image->fSrcRect), intersect);
            WKCFloatRect dstRect={0,0, tile_rect.width(), tile_rect.height()};

            const ImgTile* cur_tile = tile(xIndex, yIndex);
            if (cur_tile)
                cur_tile->Blit(in_image, dstRect);
        }
    }

    // restore src rect
    WKCFloatRect_SetFloatRect(&(in_image->fSrcRect), src);
}

void
ImageTilesWKC::BitBltToDC(void* in_context, WKCPeerImage* in_image, const WKCFloatRect& in_destrect, int in_op)
{
    //FIXME: Need a better zero comparison
    if (!in_image)
        return;

    FloatRect dst;
    FloatRect_SetWKCFloatRect(dst, in_destrect);
    FloatRect src;
    FloatRect_SetWKCFloatRect(src, in_image->fSrcRect);
    void* image_bitmap = in_image->fBitmap;

    IntRect drawnTiles = tilesInRect(src);
    AffineTransform srcToDstTransformation = makeMapBetweenRects(
            FloatRect(FloatPoint(0.0, 0.0), src.size()), dst);

    srcToDstTransformation.translate(-src.x(), -src.y());

    int bpp = 4;
    int type = in_image->fType&WKC_IMAGETYPE_TYPEMASK;
    if (type==WKC_IMAGETYPE_RGB565) {
        bpp = 2;
    }

    for (int yIndex = drawnTiles.y(); yIndex < drawnTiles.y()+drawnTiles.height(); ++yIndex) {
        for (int xIndex = drawnTiles.x(); xIndex < drawnTiles.x()+drawnTiles.width(); ++xIndex) {
            // The srcTile rectangle is an aligned tile cropped by the src rectangle.
            FloatRect tile_rect(tileRect(xIndex, yIndex));
            FloatRect intersect = intersection(src, tile_rect);

            // calculate the destination rect for the current part of the tile that will be drawn
            FloatRect d = srcToDstTransformation.mapRect(intersect);
            WKCFloatRect dstRect;
            WKCFloatRect_SetFloatRect(&dstRect, d);

            // translate the image source rect coordinates to the current tile
            WKCFloatRect_SetXYWH(&(in_image->fSrcRect),
                                    intersect.x()-tile_rect.x(), intersect.y()-tile_rect.y(),
                                    intersect.width(), intersect.height());
            // modify the bitmap pointer to point to the proper image area (the one stored by the tile)
            // we never know if the underlying layer is not actually using this
            in_image->fBitmap = (unsigned int *)((char *)image_bitmap + (int)tile_rect.x()*bpp + (int)tile_rect.y()*in_image->fRowBytes);
            //FIXME: Ugh! don't draw tile if phase > tile_size and if tiles already drawn > 1
            const ImgTile* cur_tile = tile(xIndex, yIndex);
            if (cur_tile)
                cur_tile->BlitToDC(in_context, in_image, dstRect, in_op);

        }
    }

    // restore fSrcRect from the input image
    WKCFloatRect_SetFloatRect(&(in_image->fSrcRect), src);
    // restor the bitmap pointer for the input image
    in_image->fBitmap = image_bitmap;
}
void
ImageTilesWKC::BitBltPatternToDC(void* in_context, WKCPeerImage* in_image, const WKCFloatRect& in_destrect, int in_op)
{
    if (!in_image)
                return;
        void* image_bitmap = in_image->fBitmap;
        FloatRect targetRect;
        FloatRect_SetWKCFloatRect(targetRect, in_destrect);
        FloatRect imageRect;
        FloatRect_SetWKCFloatRect(imageRect, in_image->fSrcRect);

        if(imageRect.width()<2048 && imageRect.height()<2048){
            const ImgTile* cur_tile = tile(0, 0);
            WKCFloatRect dstRect;
            WKCFloatRect_SetFloatRect(&dstRect, targetRect);
            AffineTransform srcToDstTransformation = makeMapBetweenRects(
                        FloatRect(FloatPoint(0.0, 0.0), imageRect.size()), targetRect);

                srcToDstTransformation.translate(-imageRect.x(), -imageRect.y());
            if (cur_tile) {
                cur_tile->BlitPatternToDC(in_context, in_image, dstRect, in_op);
            }
            return;
        }

        FloatRect srcRect;
        FloatRect destRect;
        FloatPoint phase = in_image->fPhase;

        phase.setX(phase.x()-targetRect.x());
        phase.setY(phase.y()-targetRect.y());

        float tWidth = targetRect.width();
        float tHeight = targetRect.height();
        float iWidth = imageRect.width();
        float iHeight = imageRect.height();

        if(phase.x()<=0){
            phase.setX(-phase.x());
        }else{
            phase.setX(phase.x());
        }
        if(phase.y()<=0){
            phase.setY(-phase.y());
        }else{
            phase.setY(phase.y());
        }
        IntRect drawnTiles = tilesInRect(imageRect);

        srcRect = FloatRect(phase.x(),phase.y(),iWidth-phase.x(),iHeight-phase.y());
        destRect = FloatRect(targetRect.x(),targetRect.y(),iWidth-phase.x(),iHeight-phase.y());
        //---------------------------------------------------------------------------------------------------------//
        //LOOP                                                                                                       //
        //---------------------------------------------------------------------------------------------------------//
        float ypos = targetRect.y();
        while(tHeight>0) {
            if(srcRect.height()>=targetRect.height())
            {
                destRect.setHeight(targetRect.height());
                srcRect.setHeight(targetRect.height());
            }


            while(tWidth>0) {
                if(srcRect.width()>=targetRect.width())
                {
                    srcRect.setWidth(targetRect.width());
                    destRect.setWidth(targetRect.width());
                }


                AffineTransform srcToDstTransformation = makeMapBetweenRects(FloatRect(FloatPoint(0.0, 0.0), srcRect.size()), destRect);
                srcToDstTransformation.translate(-srcRect.x(), -srcRect.y());

                for (int yIndex = drawnTiles.y(); yIndex < drawnTiles.y()+drawnTiles.height(); ++yIndex) {
                    for (int xIndex = drawnTiles.x(); xIndex < drawnTiles.x()+drawnTiles.width(); ++xIndex) {
                        // The srcTile rectangle is an aligned tile cropped by the src rectangle.
                        FloatRect tile_rect(tileRect(xIndex, yIndex));
                        FloatRect intersect = intersection(srcRect, tile_rect);

                        // calculate the destination rect for the current part of the tile that will be drawn

                        FloatRect d = srcToDstTransformation.mapRect(intersect);

                        WKCFloatRect dstRect;
                        WKCFloatRect_SetFloatRect(&dstRect, d);

                        // translate the image source rect coordinates to the current tile
                        WKCFloatRect_SetXYWH(&(in_image->fSrcRect),
                                intersect.x()-tile_rect.x(), intersect.y()-tile_rect.y(),
                                intersect.width(), intersect.height());
                        // modify the bitmap pointer to point to the proper image area (the one stored by the tile)
                        // we never know if the underlying layer is not actually using this
                        in_image->fBitmap = (unsigned int *)((char *)image_bitmap + (int)tile_rect.x()*4 + (int)tile_rect.y()*in_image->fRowBytes);
                        //FIXME: Currently working only for 32bit images

                        const ImgTile* cur_tile = tile(xIndex, yIndex);
                        if (cur_tile) {
                            cur_tile->BlitToDC(in_context, in_image, dstRect, in_op);
                        }

                    }
                }

                tWidth -= destRect.width();
                if(destRect.width()>tWidth){
                    destRect.setX(destRect.x()+srcRect.width());
                    srcRect = FloatRect(0,srcRect.y(),tWidth,srcRect.height());
                    destRect.setWidth(tWidth);
                }else{
                    destRect.setX(destRect.x()+srcRect.width());
                    srcRect = FloatRect(0,srcRect.y(),iWidth,srcRect.height());
                    destRect.setWidth(srcRect.width());
                }
            }
            tHeight -= destRect.height();
            if(destRect.height()<tHeight){

                //set coords of the next line
                destRect.setY(ypos+srcRect.height());
                ypos+=srcRect.height();
                //set first src rect of the new line
                srcRect = FloatRect(srcRect.x()+phase.x(),imageRect.y(),iWidth-phase.x(),iHeight);

                //Set height of the next line
                destRect.setHeight(srcRect.height());

                //Set width of next line target
                tWidth = targetRect.width();

                //First rect in new line
                destRect.setX(targetRect.x());
                destRect.setWidth(imageRect.width()-phase.x());
            }else{
                //set coords of the next line
                destRect.setY(ypos+srcRect.height());
                ypos+=tHeight;
                //set first src rect of the new line
                srcRect = FloatRect(srcRect.x()+phase.x(),imageRect.y(),iWidth-phase.x(),tHeight);

                //Set height of the next line
                destRect.setHeight(tHeight);

                //Set width of next line target
                tWidth = targetRect.width();

                //First rect in new line
                destRect.setX(targetRect.x());
                destRect.setWidth(imageRect.width()-phase.x());
            }
        }

            //---------------------------------------------------------------------------------------------------------//
            //END LOOP                                                                                                   //
            //---------------------------------------------------------------------------------------------------------//


    WKCFloatRect_SetFloatRect(&(in_image->fSrcRect), imageRect);
    // restor the bitmap pointer for the input image
    in_image->fBitmap = image_bitmap;
}
#endif // !USE(WKC_CAIRO)

}
