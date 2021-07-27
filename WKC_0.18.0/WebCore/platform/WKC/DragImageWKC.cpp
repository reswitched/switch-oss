/*
 *  Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "DragImage.h"

#include "NotImplemented.h"
#include "CachedImage.h"
#include "Image.h"

namespace WebCore {

IntSize dragImageSize(DragImageRef image)
{
    notImplemented();
    return IntSize(0, 0);
}

void deleteDragImage(DragImageRef image)
{
    notImplemented();
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize scale)
{
    notImplemented();
    return 0;
}

DragImageRef dissolveDragImageToFraction(DragImageRef image, float)
{
    notImplemented();
    return image;
}

DragImageRef createDragImageFromImage(Image* image, ImageOrientationDescription)
{
    notImplemented();
    return 0;
}

DragImageRef createDragImageIconForCachedImage(CachedImage*)
{
    notImplemented();
    return 0;
}

DragImageRef createDragImageIconForCachedImageFilename(const String& filename)
{
    notImplemented();
    return 0;
}

}
