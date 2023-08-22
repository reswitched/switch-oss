/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "Cursor.h"

#include "Image.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include <wtf/Assertions.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

IntPoint determineHotSpot(Image* image, const IntPoint& specifiedHotSpot)
{
    if (image->isNull())
        return IntPoint();

    // Hot spot must be inside cursor rectangle.
    IntRect imageRect = IntRect(image->rect());
    if (imageRect.contains(specifiedHotSpot))
        return specifiedHotSpot;

    // If hot spot is not specified externally, it can be extracted from some image formats (e.g. .cur).
    if (auto intrinsicHotSpot = image->hotSpot()) {
        if (imageRect.contains(intrinsicHotSpot.value()))
            return intrinsicHotSpot.value();
    }

    return IntPoint();
}

const Cursor& Cursor::fromType(Cursor::Type type)
{
    switch (type) {
    case Cursor::Pointer:
        return pointerCursor();
    case Cursor::Cross:
        return crossCursor();
    case Cursor::Hand:
        return handCursor();
    case Cursor::IBeam:
        return iBeamCursor();
    case Cursor::Wait:
        return waitCursor();
    case Cursor::Help:
        return helpCursor();
    case Cursor::EastResize:
        return eastResizeCursor();
    case Cursor::NorthResize:
        return northResizeCursor();
    case Cursor::NorthEastResize:
        return northEastResizeCursor();
    case Cursor::NorthWestResize:
        return northWestResizeCursor();
    case Cursor::SouthResize:
        return southResizeCursor();
    case Cursor::SouthEastResize:
        return southEastResizeCursor();
    case Cursor::SouthWestResize:
        return southWestResizeCursor();
    case Cursor::WestResize:
        return westResizeCursor();
    case Cursor::NorthSouthResize:
        return northSouthResizeCursor();
    case Cursor::EastWestResize:
        return eastWestResizeCursor();
    case Cursor::NorthEastSouthWestResize:
        return northEastSouthWestResizeCursor();
    case Cursor::NorthWestSouthEastResize:
        return northWestSouthEastResizeCursor();
    case Cursor::ColumnResize:
        return columnResizeCursor();
    case Cursor::RowResize:
        return rowResizeCursor();
    case Cursor::MiddlePanning:
        return middlePanningCursor();
    case Cursor::EastPanning:
        return eastPanningCursor();
    case Cursor::NorthPanning:
        return northPanningCursor();
    case Cursor::NorthEastPanning:
        return northEastPanningCursor();
    case Cursor::NorthWestPanning:
        return northWestPanningCursor();
    case Cursor::SouthPanning:
        return southPanningCursor();
    case Cursor::SouthEastPanning:
        return southEastPanningCursor();
    case Cursor::SouthWestPanning:
        return southWestPanningCursor();
    case Cursor::WestPanning:
        return westPanningCursor();
    case Cursor::Move:
        return moveCursor();
    case Cursor::VerticalText:
        return verticalTextCursor();
    case Cursor::Cell:
        return cellCursor();
    case Cursor::ContextMenu:
        return contextMenuCursor();
    case Cursor::Alias:
        return aliasCursor();
    case Cursor::Progress:
        return progressCursor();
    case Cursor::NoDrop:
        return noDropCursor();
    case Cursor::Copy:
        return copyCursor();
    case Cursor::None:
        return noneCursor();
    case Cursor::NotAllowed:
        return notAllowedCursor();
    case Cursor::ZoomIn:
        return zoomInCursor();
    case Cursor::ZoomOut:
        return zoomOutCursor();
    case Cursor::Grab:
        return grabCursor();
    case Cursor::Grabbing:
        return grabbingCursor();
    case Cursor::Custom:
        ASSERT_NOT_REACHED();
    }
    return pointerCursor();
}

#if !PLATFORM(WKC)
Cursor::Cursor(Image* image, const IntPoint& hotSpot)
    : m_type(Custom)
    , m_image(image)
    , m_hotSpot(determineHotSpot(image, hotSpot))
#if ENABLE(MOUSE_CURSOR_SCALE)
    , m_imageScaleFactor(1)
#endif
    , m_platformCursor(0)
{
}
#endif

#if ENABLE(MOUSE_CURSOR_SCALE)
Cursor::Cursor(Image* image, const IntPoint& hotSpot, float scale)
    : m_type(Custom)
    , m_image(image)
    , m_hotSpot(determineHotSpot(image, hotSpot))
    , m_imageScaleFactor(scale)
    , m_platformCursor(0)
{
}
#endif

Cursor::Cursor(Type type)
    : m_type(type)
#if ENABLE(MOUSE_CURSOR_SCALE)
    , m_imageScaleFactor(1)
#endif
    , m_platformCursor(0)
{
}

#if !HAVE(NSCURSOR)

PlatformCursor Cursor::platformCursor() const
{
    ensurePlatformCursor();
    return m_platformCursor;
}

#endif

#if !PLATFORM(WKC)
const Cursor& pointerCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Pointer);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Pointer);
#endif
    return c;
}

const Cursor& crossCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Cross);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Cross);
#endif
    return c;
}

const Cursor& handCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Hand);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Hand);
#endif
    return c;
}

const Cursor& moveCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Move);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Move);
#endif
    return c;
}

const Cursor& verticalTextCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::VerticalText);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::VerticalText);
#endif
    return c;
}

const Cursor& cellCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Cell);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Cell);
#endif
    return c;
}

const Cursor& contextMenuCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::ContextMenu);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::ContextMenu);
#endif
    return c;
}

const Cursor& aliasCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Alias);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Alias);
#endif
    return c;
}

const Cursor& zoomInCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::ZoomIn);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::ZoomIn);
#endif
    return c;
}

const Cursor& zoomOutCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::ZoomOut);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::ZoomOut);
#endif
    return c;
}

const Cursor& copyCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Copy);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Copy);
#endif
    return c;
}

const Cursor& noneCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::None);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::None);
#endif
    return c;
}

const Cursor& progressCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Progress);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Progress);
#endif
    return c;
}

const Cursor& noDropCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NoDrop);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NoDrop);
#endif
    return c;
}

const Cursor& notAllowedCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NotAllowed);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NotAllowed);
#endif
    return c;
}

const Cursor& iBeamCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::IBeam);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::IBeam);
#endif
    return c;
}

const Cursor& waitCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Wait);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Wait);
#endif
    return c;
}

const Cursor& helpCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Help);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Help);
#endif
    return c;
}

const Cursor& eastResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::EastResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::EastResize);
#endif
    return c;
}

const Cursor& northResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthResize);
#endif
    return c;
}

const Cursor& northEastResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthEastResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthEastResize);
#endif
    return c;
}

const Cursor& northWestResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthWestResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthWestResize);
#endif
    return c;
}

const Cursor& southResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::SouthResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::SouthResize);
#endif
    return c;
}

const Cursor& southEastResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::SouthEastResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::SouthEastResize);
#endif
    return c;
}

const Cursor& southWestResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::SouthWestResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::SouthWestResize);
#endif
    return c;
}

const Cursor& westResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::WestResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::WestResize);
#endif
    return c;
}

const Cursor& northSouthResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthSouthResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthSouthResize);
#endif
    return c;
}

const Cursor& eastWestResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::EastWestResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::EastWestResize);
#endif
    return c;
}

const Cursor& northEastSouthWestResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthEastSouthWestResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthEastSouthWestResize);
#endif
    return c;
}

const Cursor& northWestSouthEastResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthWestSouthEastResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthWestSouthEastResize);
#endif
    return c;
}

const Cursor& columnResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::ColumnResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::ColumnResize);
#endif
    return c;
}

const Cursor& rowResizeCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::RowResize);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::RowResize);
#endif
    return c;
}

const Cursor& middlePanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::MiddlePanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::MiddlePanning);
#endif
    return c;
}
    
const Cursor& eastPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::EastPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::EastPanning);
#endif
    return c;
}
    
const Cursor& northPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthPanning);
#endif
    return c;
}
    
const Cursor& northEastPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthEastPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthEastPanning);
#endif
    return c;
}
    
const Cursor& northWestPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::NorthWestPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::NorthWestPanning);
#endif
    return c;
}
    
const Cursor& southPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::SouthPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::SouthPanning);
#endif
    return c;
}
    
const Cursor& southEastPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::SouthEastPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::SouthEastPanning);
#endif
    return c;
}
    
const Cursor& southWestPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::SouthWestPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::SouthWestPanning);
#endif
    return c;
}
    
const Cursor& westPanningCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::WestPanning);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::WestPanning);
#endif
    return c;
}

const Cursor& grabCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Grab);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Grab);
#endif
    return c;
}

const Cursor& grabbingCursor()
{
    static NeverDestroyed<Cursor> c(Cursor::Grabbing);
#if PLATFORM(WKC)
    if (c.isNull())
        c.construct(Cursor::Grabbing);
#endif
    return c;
}

#if !HAVE(NSCURSOR) && !PLATFORM(GTK) && !PLATFORM(WIN)
void Cursor::ensurePlatformCursor() const
{
    notImplemented();
}
#endif

#if !HAVE(NSCURSOR)
void Cursor::setAsPlatformCursor() const
{
    notImplemented();
}
#endif

#endif
} // namespace WebCore
