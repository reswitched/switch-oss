/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKCDragClient_h
#define WKCDragClient_h

#include <wkc/wkcbase.h>
#include "WKCHelpersEnums.h"

namespace WKC {

class DragData;
class Clipboard;
class DragImage;
class Frame;
class KURL;
class String;
typedef DragImage* DragImageRef;

/*@{*/

/** @brief Class that notifies of drag event */
class WKC_API DragClientIf {
public:
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DragDestinationAction (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void willPerformDragDestinationAction(WKC::DragDestinationAction, WKC::DragData*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DragSourceAction (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void willPerformDragSourceAction(WKC::DragSourceAction, const WKCPoint&, WKC::Clipboard*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DragData* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual WKC::DragDestinationAction actionMaskForDrag(WKC::DragData*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param windowPoint (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual WKC::DragSourceAction dragSourceActionMaskForPoint(const WKCPoint& windowPoint) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param dragImage (TBD) implement description
       @param dragImageOrigin (TBD) implement description
       @param eventPos (TBD) implement description
       @param  WKC::Clipboard* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void startDrag(WKC::DragImageRef dragImage, const WKCPoint& dragImageOrigin, const WKCPoint& eventPos, WKC::Clipboard*, WKC::Frame*, bool linkDrag = false) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::KURL& (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual WKC::DragImageRef createDragImageForLink(WKC::KURL&, const WKC::String& label, WKC::Frame*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void dragControllerDestroyed() = 0;
};

/*@}*/

} // namespace

#endif // WKCDragClient_h
