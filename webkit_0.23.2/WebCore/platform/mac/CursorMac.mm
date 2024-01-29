/*
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "Cursor.h"

#import "BlockExceptions.h"
#import "WebCoreSystemInterface.h"
#import <wtf/StdLibExtras.h>

@interface WebCoreCursorBundle : NSObject { }
@end

@implementation WebCoreCursorBundle
@end

namespace WebCore {

// Simple NSCursor calls shouldn't need protection,
// but creating a cursor with a bad image might throw.

#if ENABLE(MOUSE_CURSOR_SCALE)
static RetainPtr<NSCursor> createCustomCursor(Image* image, const IntPoint& hotSpot, float scale)
#else
static RetainPtr<NSCursor> createCustomCursor(Image* image, const IntPoint& hotSpot)
#endif
{
    // FIXME: The cursor won't animate.  Not sure if that's a big deal.
    NSImage* nsImage = image->getNSImage();
    if (!nsImage)
        return 0;
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

#if ENABLE(MOUSE_CURSOR_SCALE)
    NSSize size = NSMakeSize(image->width() / scale, image->height() / scale);
    NSSize expandedSize = NSMakeSize(ceil(size.width), ceil(size.height));

    // Pad the image with transparent pixels so it has an integer boundary.
    if (size.width != expandedSize.width || size.height != expandedSize.height) {
        RetainPtr<NSImage> expandedImage = adoptNS([[NSImage alloc] initWithSize:expandedSize]);
        NSRect toRect = NSMakeRect(0, expandedSize.height - size.height, size.width, size.height);
        NSRect fromRect = NSMakeRect(0, 0, image->width(), image->height());

        [expandedImage lockFocus];
        [nsImage drawInRect:toRect fromRect:fromRect operation:NSCompositeSourceOver fraction:1];
        [expandedImage unlockFocus];

        return adoptNS([[NSCursor alloc] initWithImage:expandedImage.get() hotSpot:hotSpot]);
    }

    // Scale the image and its representation to match retina resolution.
    [nsImage setSize:expandedSize];
    [[[nsImage representations] objectAtIndex:0] setSize:expandedSize];
#endif

    return adoptNS([[NSCursor alloc] initWithImage:nsImage hotSpot:hotSpot]);
    END_BLOCK_OBJC_EXCEPTIONS;
    return nullptr;
}

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor)
        return;

    switch (m_type) {
    case Cursor::Pointer:
        m_platformCursor = [NSCursor arrowCursor];
        break;

    case Cursor::Cross:
        m_platformCursor = [NSCursor crosshairCursor];
        break;

    case Cursor::Hand:
        m_platformCursor = [NSCursor pointingHandCursor];
        break;

    case Cursor::IBeam:
        m_platformCursor = [NSCursor IBeamCursor];
        break;

    case Cursor::Wait:
        m_platformCursor = wkCursor("BusyButClickable");
        break;

    case Cursor::Help:
        m_platformCursor = wkCursor("Help");
        break;

    case Cursor::Move:
    case Cursor::MiddlePanning:
        m_platformCursor = wkCursor("Move");
        break;

    case Cursor::EastResize:
    case Cursor::EastPanning:
        m_platformCursor = wkCursor("ResizeEast");
        break;

    case Cursor::NorthResize:
    case Cursor::NorthPanning:
        m_platformCursor = wkCursor("ResizeNorth");
        break;

    case Cursor::NorthEastResize:
    case Cursor::NorthEastPanning:
        m_platformCursor = wkCursor("ResizeNortheast");
        break;

    case Cursor::NorthWestResize:
    case Cursor::NorthWestPanning:
        m_platformCursor = wkCursor("ResizeNorthwest");
        break;

    case Cursor::SouthResize:
    case Cursor::SouthPanning:
        m_platformCursor = wkCursor("ResizeSouth");
        break;

    case Cursor::SouthEastResize:
    case Cursor::SouthEastPanning:
        m_platformCursor = wkCursor("ResizeSoutheast");
        break;

    case Cursor::SouthWestResize:
    case Cursor::SouthWestPanning:
        m_platformCursor = wkCursor("ResizeSouthwest");
        break;

    case Cursor::WestResize:
    case Cursor::WestPanning:
        m_platformCursor = wkCursor("ResizeWest");
        break;

    case Cursor::NorthSouthResize:
        m_platformCursor = wkCursor("ResizeNorthSouth");
        break;

    case Cursor::EastWestResize:
        m_platformCursor = wkCursor("ResizeEastWest");
        break;

    case Cursor::NorthEastSouthWestResize:
        m_platformCursor = wkCursor("ResizeNortheastSouthwest");
        break;

    case Cursor::NorthWestSouthEastResize:
        m_platformCursor = wkCursor("ResizeNorthwestSoutheast");
        break;

    case Cursor::ColumnResize:
        m_platformCursor = [NSCursor resizeLeftRightCursor];
        break;

    case Cursor::RowResize:
        m_platformCursor = [NSCursor resizeUpDownCursor];
        break;

    case Cursor::VerticalText:
        m_platformCursor = [NSCursor IBeamCursorForVerticalLayout];
        break;

    case Cursor::Cell:
        m_platformCursor = wkCursor("Cell");
        break;

    case Cursor::ContextMenu:
        m_platformCursor = [NSCursor contextualMenuCursor];
        break;

    case Cursor::Alias:
        m_platformCursor = wkCursor("MakeAlias");
        break;

    case Cursor::Progress:
        m_platformCursor = wkCursor("BusyButClickable");
        break;

    case Cursor::NoDrop:
        m_platformCursor = [NSCursor operationNotAllowedCursor];
        break;

    case Cursor::Copy:
        m_platformCursor = [NSCursor dragCopyCursor];
        break;

    case Cursor::None:
        m_platformCursor = adoptNS([[NSCursor alloc] initWithImage:adoptNS([[NSImage alloc] initWithSize:NSMakeSize(1, 1)]).get() hotSpot:NSZeroPoint]);
        break;

    case Cursor::NotAllowed:
        m_platformCursor = [NSCursor operationNotAllowedCursor];
        break;

    case Cursor::ZoomIn:
        m_platformCursor = wkCursor("ZoomIn");
        break;

    case Cursor::ZoomOut:
        m_platformCursor = wkCursor("ZoomOut");
        break;

    case Cursor::Grab:
        m_platformCursor = [NSCursor openHandCursor];
        break;

    case Cursor::Grabbing:
        m_platformCursor = [NSCursor closedHandCursor];
        break;

    case Cursor::Custom:
#if ENABLE(MOUSE_CURSOR_SCALE)
        m_platformCursor = createCustomCursor(m_image.get(), m_hotSpot, m_imageScaleFactor);
#else
        m_platformCursor = createCustomCursor(m_image.get(), m_hotSpot);
#endif
        break;
    }
}

Cursor::Cursor(const Cursor& other)
    : m_type(other.m_type)
    , m_image(other.m_image)
    , m_hotSpot(other.m_hotSpot)
    , m_imageScaleFactor(other.m_imageScaleFactor)
    , m_platformCursor(other.m_platformCursor)
{
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_type = other.m_type;
    m_image = other.m_image;
    m_hotSpot = other.m_hotSpot;
    m_imageScaleFactor = other.m_imageScaleFactor;
    m_platformCursor = other.m_platformCursor;
    return *this;
}

Cursor::~Cursor()
{
}

NSCursor *Cursor::platformCursor() const
{
    ensurePlatformCursor();
    return m_platformCursor.get();
}

} // namespace WebCore
