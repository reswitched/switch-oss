/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#import "config.h"
#import "Icon.h"

#if !PLATFORM(IOS)

#import "GraphicsContext.h"
#import "IntRect.h"
#import "LocalCurrentGraphicsContext.h"
#import <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

Icon::Icon(NSImage *image)
    : m_nsImage(image)
{
    // Need this because WebCore uses AppKit's flipped coordinate system exclusively.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [image setFlipped:YES];
#pragma clang diagnostic pop
}

Icon::~Icon()
{
}

// FIXME: Move the code to ChromeClient::iconForFiles().
PassRefPtr<Icon> Icon::createIconForFiles(const Vector<String>& filenames)
{
    if (filenames.isEmpty())
        return nullptr;

    bool useIconFromFirstFile;
    useIconFromFirstFile = filenames.size() == 1;
    if (useIconFromFirstFile) {
        // Don't pass relative filenames -- we don't want a result that depends on the current directory.
        // Need 0U here to disambiguate String::operator[] from operator(NSString*, int)[]
        if (filenames[0].isEmpty() || filenames[0][0U] != '/')
            return nullptr;

        NSImage *image = [[NSWorkspace sharedWorkspace] iconForFile:filenames[0]];
        if (!image)
            return nullptr;

        return adoptRef(new Icon(image));
    }
    NSImage *image = [NSImage imageNamed:NSImageNameMultipleDocuments];
    if (!image)
        return nullptr;

    return adoptRef(new Icon(image));
}

void Icon::paint(GraphicsContext& context, const FloatRect& rect)
{
    if (context.paintingDisabled())
        return;

    LocalCurrentGraphicsContext localCurrentGC(&context);

    [m_nsImage drawInRect:rect fromRect:NSMakeRect(0, 0, [m_nsImage size].width, [m_nsImage size].height) operation:NSCompositeSourceOver fraction:1.0f];
}

}

#endif // !PLATFORM(IOS)
