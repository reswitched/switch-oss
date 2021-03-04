/*
 * Copyright (C) 2007 Apple Computer, Kevin Ollivier.  All rights reserved.
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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

ImageWKC::ImageWKC(int type, void* bitmap, int rowbytes, const IntSize& size, bool ownbitmap)
    : m_type(type)
    , m_bitmap(bitmap)
    , m_rowbytes(rowbytes)
    , m_size(size)
    , m_ownbitmap(ownbitmap)
    , m_hasAlpha(false)
    , m_scalex(1)
    , m_scaley(1)
    , m_allowReduceColor(gReduceTo565IfPossible)
    , m_offscreen(0)
    , m_userdata(0)
    , m_userdataDestroyFunc(0)
{
    switch (type) {
    case EColorARGB8888:
        m_bpp = 4;
        m_hasAlpha = true;
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

ImageWKC::~ImageWKC()
{
    if (m_userdataDestroyFunc) {
        (*m_userdataDestroyFunc)(m_userdata);
        m_userdata = 0;
        m_userdataDestroyFunc = 0;
    }
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

    if (wkcMemoryCheckMemoryAllocatablePeer(len * m_bpp, WKC_MEMORYALLOC_TYPE_IMAGE)) {
        WTF::TryMallocReturnValue rv = WTF::tryFastMalloc(len * m_bpp);
        allocSucceeded = rv.getValue(newbitmap);
    }
    if (!allocSucceeded) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(len * m_bpp, WKC_MEMORYALLOC_TYPE_IMAGE);
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

void
ImageWKC::setUserdata(void* userdata, ImageWKCUserdataDestroyFunc destroyfunc)
{
    if (!userdata || !destroyfunc)
        return;
    m_userdata = userdata;
    m_userdataDestroyFunc = destroyfunc;
}

float Image::platformResourceScale()
{
    return wkcStockImageGetImageScalePeer();
}

Ref<Image> Image::loadPlatformResource(const char* name)
{
    const unsigned char* bitmap = 0;
    unsigned int width=0, height=0;

    bitmap = wkcStockImageGetPlatformResourceImagePeer(name, &width, &height);
    if (bitmap) {
        Ref<ImageWKC> wimg = ImageWKC::create();
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
            return BitmapImage::create(WTFMove(wimg));
        }
    }

    Ref<Image> img = BitmapImage::create();
    Vector<char> arr;
    Ref<SharedBuffer> buffer = SharedBuffer::create(arr.data(), arr.size());
    img->setData(WTFMove(buffer), true);
    return img;
}

void
BitmapImage::invalidatePlatformData()
{
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
