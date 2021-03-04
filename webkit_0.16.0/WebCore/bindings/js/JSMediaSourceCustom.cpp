/*
 *  Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(MEDIA_SOURCE)

#if PLATFORM(WKC)

#include "JSMediaSource.h"

#include <runtime/JSFunction.h>
#include <wtf/text/WTFString.h>

using namespace JSC;

namespace WebCore {

void JSMediaSource::visitAdditionalChildren(SlotVisitor& visitor)
{
    MediaSource* mediaSource = static_cast<MediaSource*>(&impl());
    visitor.addOpaqueRoot(mediaSource);

    // -- end boiler plate code --

    // Mark SourceBufferList
    SourceBufferList* sourceBufferList = mediaSource->sourceBuffers();
    if (sourceBufferList) {
        visitor.addOpaqueRoot(sourceBufferList);

        // Mark SourceBuffer
        for (unsigned long i = 0, len = sourceBufferList->length(); i < len; i++) {
            SourceBuffer* sourceBuffer = sourceBufferList->item(i);
            if (sourceBuffer)
                visitor.addOpaqueRoot(sourceBuffer);
        }
    }
}

} // namespace WebCore

#endif

#endif
