/*
 * Copyright (C) 2003, 2007, 2010 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#ifndef WindowFeatures_h
#define WindowFeatures_h

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Optional.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class FloatRect;

struct WindowFeatures {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    WindowFeatures()
        : menuBarVisible(true)
        , statusBarVisible(true)
        , toolBarVisible(true)
        , locationBarVisible(true)
        , scrollbarsVisible(true)
        , resizable(true)
        , fullscreen(false)
        , dialog(false)
    {
    }
    explicit WindowFeatures(const String& windowFeaturesString);
    WindowFeatures(const String& dialogFeaturesString, const FloatRect& screenAvailableRect);

    Optional<float> x;
    Optional<float> y;
    Optional<float> width;
    Optional<float> height;

    bool menuBarVisible;
    bool statusBarVisible;
    bool toolBarVisible;
    bool locationBarVisible;
    bool scrollbarsVisible;
    bool resizable;

    bool fullscreen;
    bool dialog;

    Vector<String> additionalFeatures;

private:
    static HashMap<String, String> parseDialogFeatures(const String&);
    static bool boolFeature(const HashMap<String, String>&, const char* key, bool defaultValue = false);
    static float floatFeature(const HashMap<String, String>&, const char* key, float min, float max, float defaultValue);
    void setWindowFeature(const String& keyString, const String& valueString);
};

} // namespace WebCore

#endif // WindowFeatures_h
