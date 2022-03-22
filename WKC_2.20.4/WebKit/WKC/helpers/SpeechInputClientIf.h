/*
 *  Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCSPEECHINPUTCLIENTIF_H
#define WKCSPEECHINPUTCLIENTIF_H

#include <wkc/wkcbase.h>

namespace WKC {

class AtomString;
class SpeechInputListener;
class String;
class SecurityOrigin;

/*@{*/

/** @brief Class that notifies of speech input event */
class WKC_API SpeechInputClientIf
{
public:
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Destructor
       @endcond
    */
    virtual ~SpeechInputClientIf() {}

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void speechInputDestroyed() = 0;
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Sets speech input listener
       @param in_listener Listener
       @endcond
    */
    virtual void setListener(SpeechInputListener*) = 0;
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Starts voice recognization
       @param requestId Request id
       @param elementRect Rectangle of the element
       @param language Language
       @param grammar Grammar
       @param origin Security origin
       @endcond
    */
    virtual bool startRecognition(int requestId, const WKCRect& elementRect, const AtomString& language, const String& grammar, SecurityOrigin*) = 0;
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Stops voice recognization
       @param requestId Request id
       @endcond
    */
    virtual void stopRecording(int requestId) = 0;
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Cancels voice recognization
       @param requestId Request id
       @endcond
    */
    virtual void cancelRecognition(int requestId) = 0;
};

/*@}*/

} // namespace

#endif // WKCSPEECHINPUTCLIENTIF_H
