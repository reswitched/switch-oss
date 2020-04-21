/*
 * Copyright (C) 2007 Kevin Ollivier
 * All rights reserved.
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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
#include "LocalizedStrings.h"

#include <wkc/wkcmediapeer.h>

#include "NotImplemented.h"
#include "WTFString.h"

#include <float.h>

extern "C" WKC_PEER_API const unsigned short* wkcSystemGetLanguagePeer(void);
extern "C" WKC_PEER_API const unsigned short* wkcSystemGetButtonLabelSubmitPeer(void);
extern "C" WKC_PEER_API const unsigned short* wkcSystemGetButtonLabelResetPeer(void);
extern "C" WKC_PEER_API const unsigned short* wkcSystemGetButtonLabelFilePeer(void);

extern "C" WKC_PEER_API const unsigned char* wkcSystemGetSystemStringPeer(const unsigned char* in_key);

namespace WTF {

Vector<String> platformUserPreferredLanguages() 
{
    Vector<String> items;
    items.append(String(wkcSystemGetLanguagePeer()));
    return items;
}

}

namespace WebCore {

String localizedString(const char* cstr)
{
    if (!cstr)
        return String();

    String str(cstr);
    if (str=="Submit")
        return String(wkcSystemGetButtonLabelSubmitPeer()); 
    else if (str=="Reset")
        return String(wkcSystemGetButtonLabelResetPeer()); 
    else if (str=="Choose File")
        return String(wkcSystemGetButtonLabelFilePeer());
    // This case is for the multiple file INPUT.
    // We use the same label for multiple as for single.
    // If you want to use a different label for each type, change this code.
    else if (str == "Choose Files")
        return String(wkcSystemGetButtonLabelFilePeer());
#if ENABLE(VIDEO)
    else if (str== "Live Broadcast")
        return wkcMediaPlayerGetUIStringPeer(WKC_MEDIA_UISTRING_BROADCAST);
    else if (str=="Loading...")
        return wkcMediaPlayerGetUIStringPeer(WKC_MEDIA_UISTRING_LOADING);
#endif // ENABLE(VIDEO)
    else
        return WTF::String::fromUTF8((const char *)wkcSystemGetSystemStringPeer((const unsigned char *)cstr));
}

#if ENABLE(VIDEO)
String localizedMediaTimeDescription(float in_time)
{
    return wkcMediaPlayerGetUIStringTimePeer(in_time);
}
#endif // ENABLE(VIDEO)

String weekFormatInLDML()
{
    notImplemented();
    return String();
}


} // namespace WebCore
