/*
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Christian Dywan <christian@twotoasts.de>
 * All rights reserved.
 * Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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
};
}

namespace WebCore {

Cursor::Cursor(const Cursor& other)
{
    if (this==&other)
        return;
    if (other.m_platformCursor) {
        WKC::WKCPlatformCursorPrivate* o = reinterpret_cast<WKC::WKCPlatformCursorPrivate*>(other.m_platformCursor);
        WKC::WKCPlatformCursorPrivate* c = new WKC::WKCPlatformCursorPrivate(o->fType);
        if (o->fType==WKC::ECursorTypeCustom) {
            ImageWKC* wi = reinterpret_cast<ImageWKC*>(o->fData);
            wi->ref();
            c->fBitmap = wi->bitmap(true);
            c->fRowBytes = wi->rowbytes();
            c->fBPP = wi->bpp();
            c->fSize.fWidth = o->fSize.fWidth;
            c->fSize.fHeight = o->fSize.fHeight;
            c->fHotSpot.fX = o->fHotSpot.fX;
            c->fHotSpot.fY = o->fHotSpot.fY;
            c->fData = reinterpret_cast<void*>(wi);
        }
        m_platformCursor = reinterpret_cast<PlatformCursor>(c);
    } else {
        m_platformCursor = 0;
    }
}

Cursor::Cursor(Image* image, const IntPoint& hotSpot)
{
    WKC::WKCPlatformCursorPrivate* c = new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCustom);
    ImageWKC* wi = reinterpret_cast<ImageWKC*>(image->nativeImageForCurrentFrame());
    if (!wi) {
        delete c;
        m_platformCursor = reinterpret_cast<PlatformCursor>(new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypePointer));
        return;
    }
    wi->ref();
    c->fBitmap = wi->bitmap(true);
    c->fRowBytes = wi->rowbytes();
    c->fBPP = wi->bpp();
    c->fSize.fWidth = image->size().width();
    c->fSize.fHeight = image->size().height();
    c->fHotSpot.fX = hotSpot.x();
    c->fHotSpot.fY = hotSpot.y();
    c->fData = reinterpret_cast<void*>(wi);

    m_platformCursor = reinterpret_cast<PlatformCursor>(c);
}

Cursor::Cursor(WebCore::PlatformCursor cursor)
{
    m_platformCursor = cursor;
}

Cursor::~Cursor()
{
    if (!m_platformCursor)
        return;

    WKC::WKCPlatformCursorPrivate* c = reinterpret_cast<WKC::WKCPlatformCursorPrivate*>(m_platformCursor);
    if (c->fData) {
        ImageWKC* wi = reinterpret_cast<ImageWKC*>(c->fData);
        wi->unref();
    }
    delete c;
}

Cursor& Cursor::operator=(const Cursor& other)
{
    if (this==&other)
        return *this;

    WKC::WKCPlatformCursorPrivate* cur = reinterpret_cast<WKC::WKCPlatformCursorPrivate *>(m_platformCursor);
    if (cur) {
        if (cur->fData) {
            ImageWKC* wi = reinterpret_cast<ImageWKC*>(cur->fData);
            wi->unref();
        }
        delete cur;
    }
    m_platformCursor = 0;

    if (other.m_platformCursor) {
        WKC::WKCPlatformCursorPrivate* o = reinterpret_cast<WKC::WKCPlatformCursorPrivate*>(other.m_platformCursor);
        WKC::WKCPlatformCursorPrivate* c = new WKC::WKCPlatformCursorPrivate(o->fType);
        if (o->fType==WKC::ECursorTypeCustom) {
            ImageWKC* wi = reinterpret_cast<ImageWKC*>(o->fData);
            wi->ref();
            c->fBitmap = wi->bitmap(true);
            c->fRowBytes = wi->rowbytes();
            c->fBPP = wi->bpp();
            c->fSize.fWidth = o->fSize.fWidth;
            c->fSize.fHeight = o->fSize.fHeight;
            c->fHotSpot.fX = o->fHotSpot.fX;
            c->fHotSpot.fY = o->fHotSpot.fY;
            c->fData = reinterpret_cast<void*>(wi);
        }
        m_platformCursor = reinterpret_cast<PlatformCursor>(c);
    }
    return *this;
}

const Cursor& pointerCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypePointer)));
    return cCursor;
}

const Cursor& crossCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCross)));
    return cCursor;
}

const Cursor& handCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeHand)));
    return cCursor;
}

const Cursor& moveCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeMove)));
    return cCursor;
}

const Cursor& iBeamCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeIBeam)));
    return cCursor;
}

const Cursor& waitCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeWait)));
    return cCursor;
}

const Cursor& helpCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeHelp)));
    return cCursor;
}

const Cursor& eastResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeEastResize)));
    return cCursor;
}

const Cursor& northResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthResize)));
    return cCursor;
}

const Cursor& northEastResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthEastResize)));
    return cCursor;
}

const Cursor& northWestResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthWestResize)));
    return cCursor;
}

const Cursor& southResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthResize)));
    return cCursor;
}

const Cursor& southEastResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthEastResize)));
    return cCursor;
}

const Cursor& southWestResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthWestResize)));
    return cCursor;
}

const Cursor& westResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeWestResize)));
    return cCursor;
}

const Cursor& northSouthResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthSouthResize)));
    return cCursor;
}

const Cursor& eastWestResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeEastWestResize)));
    return cCursor;
}

const Cursor& northEastSouthWestResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthEastSouthWestResize)));
    return cCursor;
}

const Cursor& northWestSouthEastResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthWestSouthEastResize)));
    return cCursor;
}

const Cursor& columnResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeColumnResize)));
    return cCursor;
}

const Cursor& rowResizeCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeRowResize)));
    return cCursor;
}
    
const Cursor& middlePanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeMiddlePanning)));
    return cCursor;
}

const Cursor& eastPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeEastPanning)));
    return cCursor;
}

const Cursor& northPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthPanning)));
    return cCursor;
}

const Cursor& northEastPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthEastPanning)));
    return cCursor;
}

const Cursor& northWestPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNorthWestPanning)));
    return cCursor;
}

const Cursor& southPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthPanning)));
    return cCursor;
}

const Cursor& southEastPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthEastPanning)));
    return cCursor;
}

const Cursor& southWestPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeSouthWestPanning)));
    return cCursor;
}

const Cursor& westPanningCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeWestPanning)));
    return cCursor;
}
    

const Cursor& verticalTextCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeVerticalText)));
    return cCursor;
}

const Cursor& cellCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCell)));
    return cCursor;
}

const Cursor& contextMenuCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeContextMenu)));
    return cCursor;
}

const Cursor& noDropCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNoDrop)));
    return cCursor;
}

const Cursor& copyCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeCopy)));
    return cCursor;
}

const Cursor& progressCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeProgress)));
    return cCursor;
}

const Cursor& aliasCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeAlias)));
    return cCursor;
}

const Cursor& noneCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNone)));
    return cCursor;
}

const Cursor& notAllowedCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeNotAllowed)));
    return cCursor;
}

const Cursor& zoomInCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeZoomIn)));
    return cCursor;
}

const Cursor& zoomOutCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeZoomOut)));
    return cCursor;
}

const Cursor& grabCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeGrab)));
    return cCursor;
}

const Cursor& grabbingCursor()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Cursor, cCursor, ((PlatformCursor)new WKC::WKCPlatformCursorPrivate(WKC::ECursorTypeGrabbing)));
    return cCursor;
}

void Cursor::ensurePlatformCursor() const
{
}

}
