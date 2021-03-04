/*
* Copyright (c) 2016-2019 ACCESS CO., LTD. All rights reserved.
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

#ifndef GamepadsWKC_h
#define GamepadsWKC_h

#if ENABLE(GAMEPAD)

#include "PlatformGamepad.h"

namespace WebCore {

class GamepadsWKC : public PlatformGamepad
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    GamepadsWKC(int index)
        : PlatformGamepad(index)
    {
        m_connectTime = WTF::MonotonicTime::now();
    }
    GamepadsWKC(int index, const WTF::String& id, int naxes, int nbuttons)
        : PlatformGamepad(index)
    {
        m_id = id;
        m_connectTime = WTF::MonotonicTime::now();

        Vector<double> vaxes, vbuttons;
        for (int i = 0; i < naxes; i++)
            vaxes.append(0.0f);
        m_axes = vaxes;
        for (int i = 0; i < nbuttons; i++)
            vbuttons.append(0.0f);
        m_buttons = vbuttons;
    }
    ~GamepadsWKC() {}

    virtual const Vector<double>& axisValues() const { return m_axes; }
    virtual const Vector<double>& buttonValues() const { return m_buttons; }

    void updateValue(MonotonicTime timestamp, const Vector<double>& axes, const Vector<double>& buttons);

private:
    Vector<double> m_axes;
    Vector<double> m_buttons;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
#endif // GamepadsWKC_h
