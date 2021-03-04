/*
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Christian Dywan <christian@twotoasts.de>
 * All rights reserved.
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

#include "Cursor.h"

#include "Image.h"
#include "IntPoint.h"
#include "ImageWKC.h"

#include "NotImplemented.h"
#include <wtf/Assertions.h>

#include "WKCEnums.h"
#include "helpers/ChromeClientIf.h"

namespace WKC {
class WKCPlatformCursorPrivate : public WKCPlatformCursor
{
WTF_MAKE_FAST_ALLOCATED;
public:
    WKCPlatformCursorPrivate(int type)
        : WKCPlatformCursor(type)
    {}
    ~WKCPlatformCursorPrivate()
    {}
    void setNativeImagePtr(WebCore::NativeImagePtr& image) { m_image = image; }
private:
    WebCore::NativeImagePtr m_image;
};
}

namespace WebCore {

Cursor::Cursor(Image* image, const IntPoint& hotSpot)
{
    WKC::WKCPlatformCursorPrivate* c = new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCustom);
    NativeImagePtr nativeImagePtr = image->nativeImageForCurrentFrame();
    if (!nativeImagePtr) {
        delete c;
        m_platformCursor = reinterpret_cast<PlatformCursor>(new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypePointer));
        return;
    }

    c->setNativeImagePtr(nativeImagePtr);

    c->fBitmap       = nativeImagePtr->bitmap(true);
    c->fRowBytes     = nativeImagePtr->rowbytes();
    c->fBPP          = nativeImagePtr->bpp();
    c->fSize.fWidth  = image->size().width();
    c->fSize.fHeight = image->size().height();
    c->fHotSpot.fX   = hotSpot.x();
    c->fHotSpot.fY   = hotSpot.y();
    c->fData         = reinterpret_cast<void*>(nativeImagePtr.get());

    m_platformCursor = reinterpret_cast<PlatformCursor>(c);
}

Cursor::Cursor(WebCore::PlatformCursor cursor)
{
    m_platformCursor = cursor;
}

const Cursor& pointerCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypePointer)));
    return cCursor;
}

const Cursor& crossCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCross)));
    return cCursor;
}

const Cursor& handCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeHand)));
    return cCursor;
}

const Cursor& moveCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeMove)));
    return cCursor;
}

const Cursor& iBeamCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeIBeam)));
    return cCursor;
}

const Cursor& waitCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeWait)));
    return cCursor;
}

const Cursor& helpCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeHelp)));
    return cCursor;
}

const Cursor& eastResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeEastResize)));
    return cCursor;
}

const Cursor& northResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthResize)));
    return cCursor;
}

const Cursor& northEastResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthEastResize)));
    return cCursor;
}

const Cursor& northWestResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthWestResize)));
    return cCursor;
}

const Cursor& southResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthResize)));
    return cCursor;
}

const Cursor& southEastResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthEastResize)));
    return cCursor;
}

const Cursor& southWestResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthWestResize)));
    return cCursor;
}

const Cursor& westResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeWestResize)));
    return cCursor;
}

const Cursor& northSouthResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthSouthResize)));
    return cCursor;
}

const Cursor& eastWestResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeEastWestResize)));
    return cCursor;
}

const Cursor& northEastSouthWestResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthEastSouthWestResize)));
    return cCursor;
}

const Cursor& northWestSouthEastResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthWestSouthEastResize)));
    return cCursor;
}

const Cursor& columnResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeColumnResize)));
    return cCursor;
}

const Cursor& rowResizeCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeRowResize)));
    return cCursor;
}
    
const Cursor& middlePanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeMiddlePanning)));
    return cCursor;
}

const Cursor& eastPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeEastPanning)));
    return cCursor;
}

const Cursor& northPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthPanning)));
    return cCursor;
}

const Cursor& northEastPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthEastPanning)));
    return cCursor;
}

const Cursor& northWestPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthWestPanning)));
    return cCursor;
}

const Cursor& southPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthPanning)));
    return cCursor;
}

const Cursor& southEastPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthEastPanning)));
    return cCursor;
}

const Cursor& southWestPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthWestPanning)));
    return cCursor;
}

const Cursor& westPanningCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeWestPanning)));
    return cCursor;
}
    

const Cursor& verticalTextCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeVerticalText)));
    return cCursor;
}

const Cursor& cellCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCell)));
    return cCursor;
}

const Cursor& contextMenuCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeContextMenu)));
    return cCursor;
}

const Cursor& noDropCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNoDrop)));
    return cCursor;
}

const Cursor& copyCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCopy)));
    return cCursor;
}

const Cursor& progressCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeProgress)));
    return cCursor;
}

const Cursor& aliasCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeAlias)));
    return cCursor;
}

const Cursor& noneCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNone)));
    return cCursor;
}

const Cursor& notAllowedCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNotAllowed)));
    return cCursor;
}

const Cursor& zoomInCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeZoomIn)));
    return cCursor;
}

const Cursor& zoomOutCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeZoomOut)));
    return cCursor;
}

const Cursor& grabCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeGrab)));
    return cCursor;
}

const Cursor& grabbingCursor()
{
    WKC_DEFINE_STATIC_TYPE(Cursor, cCursor, Cursor((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeGrabbing)));
    return cCursor;
}

void Cursor::ensurePlatformCursor() const
{
}

}
