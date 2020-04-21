/*
 *  Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCSPEECHRECOGNITIONCLIENTIF_H
#define WKCSPEECHRECOGNITIONCLIENTIF_H

#include <wkc/wkcbase.h>

namespace WKC {

class SpeechRecognition;
class SpeechGrammarList;
class String;

/*@{*/

/** @brief Class that notifies of speech recognition event */
class WKC_API SpeechRecognitionClientIf
{
public:
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Starts speech recognition
       @param TBD
       @endcond
    */
    virtual void start(WKC::SpeechRecognition*, const WKC::SpeechGrammarList*, const WKC::String& lang, bool continuous, bool interimResults, unsigned long maxAlternatives) = 0;
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Stops speech recognition
       @param TBD.
       @endcond
    */
    virtual void stop(WKC::SpeechRecognition*) = 0;
    /**
       @cond WKC_PRIVARE_DOCUMENT
       @brief Aborts speech recognition
       @param TBD.
       @endcond
    */
    virtual void abort(WKC::SpeechRecognition*) = 0;
};

/*@}*/

} // namespace

#endif // WKCSPEECHRECOGNITIONCLIENTIF_H
