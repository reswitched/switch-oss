/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCContextMenuClient_h
#define WKCContextMenuClient_h

#include <wkc/wkcbase.h>

namespace WKC {

class ContextMenu;
class ContextMenuItem;
class KURL;
class Frame;
class String;

/*@{*/

typedef void* PlatformMenuDescription;

/** @brief Class that notifies of a context menu event */
class WKC_API ContextMenuClientIf
{
public:
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of discarding context menu
       @return (TBD) implement description 
       @endcond
    */
    virtual void contextMenuDestroyed() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::ContextMenu* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::PlatformMenuDescription getCustomMenuFromDefaultItems(WKC::ContextMenu*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::ContextMenuItem* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void contextMenuItemSelected(WKC::ContextMenuItem*, const WKC::ContextMenu*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param url (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void downloadURL(const WKC::KURL& url) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Frame* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void searchWithGoogle(const WKC::Frame*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Frame* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void lookUpInDictionary(WKC::Frame*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::String& (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void speak(const WKC::String&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void stopSpeaking() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool isSpeaking() = 0;
};

/*@}*/

} // namespace

#endif // WKCContextMenuClient_h
