/*
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

#ifndef ImageWKC_h
#define ImageWKC_h

#include "IntSize.h"
#include "GraphicsTypes.h"

#include <wkc/wkcgpeer.h>

#include "wtf/Vector.h"
#include "FloatRect.h"

namespace WebCore {

class ImageTilesWKC;

class ImageWKC {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static void setInternalColorFormatARGB8888(bool reduceifpossible=false);
    static void setInternalColorFormatRGB565();

public:
    static ImageWKC* create(int type=EColorARGB8888, void* bitmap=0, int rowbytes=0, const IntSize& size=IntSize(), bool ownbitmap=true);
    ~ImageWKC();

    void ref();
    void unref();

    static int platformOperator(CompositeOperator op);

    void notifyStatus(int status, bool hasduration=false);
    void zeroFill();
    void zeroFillRect(const IntRect&);
    bool copyImage(const ImageWKC*, bool hasduration);

    bool resize(const IntSize&);
    void clear();

    inline void* decodebuffer() const { return m_bitmap; }

    void* bitmap(bool retain=false) const;
    inline bool isBitmapAvail() const { return m_bitmap ? true : false; }
    inline int rowbytes() const { return m_rowbytes; }
    inline const IntSize& size() const { return m_size; }
    enum {
        EColorNone,
        EColorARGB8888,
        EColorRGB565,
        EColors
    };
    inline int type() const { return m_type; }
    inline int bpp() const { return m_bpp; };
    inline void setHasAlpha(bool flag) { m_hasAlpha = flag; }
    inline bool hasAlpha() const { return m_hasAlpha; }
    inline double scalex() const { return m_scalex; }
    inline double scaley() const { return m_scaley; }

    void setAllowReduceColor(bool flag);
    inline bool allowReduceColor() const { return m_allowReduceColor; }

    inline bool onstack() const { return m_onstack; }

    inline ImageTilesWKC* offscreen() const {return m_offscreen; }

protected:
    ImageWKC(int type, void* bitmap, int rowbytes, const IntSize& size, bool ownbitmap=true, bool onstack=false);

private:
    int m_refcount;

protected:
    int m_type;
    int m_bpp;

    void* m_bitmap;
    int m_rowbytes;

    IntSize m_size;
    bool m_ownbitmap;
    bool m_onstack;

    bool m_hasAlpha;
    double m_scalex;
    double m_scaley;

    bool m_allowReduceColor;

    ImageTilesWKC* m_offscreen;
};

class ImageContainerWKC : public ImageWKC {
public:
    ImageContainerWKC(int type, void* bitmap, int rowbytes, const IntSize& size)
        : ImageWKC(type, bitmap, rowbytes, size, false, true) {}
};

class ImgTile {
public:
    ImgTile(const WKCSize& size);
    ImgTile();
    ~ImgTile();
    void Clip(WKCFloatRect& r);
    void ClearRect(WKCFloatRect& r);
    void Blit(const WKCPeerImage* img, WKCFloatRect& dst) const;
    void BlitToDC(void* in_context, WKCPeerImage* img, WKCFloatRect& dst, int in_op) const;
    void BlitPatternToDC(void* in_context, WKCPeerImage* img, WKCFloatRect& dst, int in_op) const;
protected:
    //we cannot copy an offscreen and its context
    ImgTile& operator=(const ImgTile&);
    ImgTile(const ImgTile&);
private:
    void* m_texture;
};


class ImageTilesWKC {
public:
    ImageTilesWKC(const WKCSize& size, const WKCSize& maxTileSize);
    //ImageTilesWKC(const ImageTilesWKC&);
    ~ImageTilesWKC();

    //ImageTilesWKC& operator=(const ImageTilesWKC&);

    const WKCSize& size() const {return m_size;}
    const WKCSize& maxTileSize() const { return m_maxTileSize; }

    void Clip(WKCFloatRect& r);
    void ClearRect(const WKCFloatRect& r);
    void BitBlt(WKCPeerImage* in_image, const WKCFloatRect* in_destrect);
    void BitBltToDC(void* in_context, WKCPeerImage* in_image, const WKCFloatRect& in_destrect, int in_op);
    void BitBltPatternToDC(void* in_context, WKCPeerImage* in_image, const WKCFloatRect& in_destrect, int in_op);

    inline int numTiles() const {return m_tiles.size();}
    inline int numColumns() const {return m_numColumns;}
    inline int numRows() const {return m_tiles.size() / m_numColumns;}

private:
    WKCSize m_size;
    int m_numColumns;
    Vector<ImgTile*> m_tiles;
    //FIXME: replace this with something dynamically read from the Graphical Engine
    WKCSize m_maxTileSize;

    FloatRect tileRect(int xIndex, int yIndex) const;
    IntRect         tilesInRect(const FloatRect& rect) const;
    const ImgTile*     tile(int xIndex, int yIndex) const;
};

} // namespace

#endif // ImageWKC_h
