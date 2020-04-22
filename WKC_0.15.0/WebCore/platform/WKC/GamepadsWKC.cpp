/*
 * Copyright (c) 2013-2016 ACCESS CO., LTD. All rights reserved.
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
#include "GamepadsWKC.h"

#include "Gamepads.h"
#include "Gamepad.h"
#include "GamepadList.h"

#if ENABLE(GAMEPAD)

namespace WebCore {

#if ENABLE(GAMEPAD_DEPRECATED)

WKC_DEFINE_GLOBAL_PTR(Vector< RefPtr<Gamepad> >*, gGamepads, 0);

void initializeGamepads(int pads)
{
    // OBSOLETE
    if (gGamepads)
        delete gGamepads;
    gGamepads = new Vector< RefPtr<Gamepad> >;
    for (int i=0; i<pads; i++) {
        GamepadsWKC pad(i);
        gGamepads->append(Gamepad::create(pad));
    }
}

bool notifyGamepadEvent(int index, const WTF::String& id, long long timestamp, int naxes, const float* axes, int nbuttons, const float* buttons)
{
    // OBSOLETE
    if (index>=gGamepads->size())
        return false;
    Vector<double> vaxes, vbuttons;
    for (int i=0; i<naxes; i++)
        vaxes.append(axes[i]);
    for (int i=0; i<nbuttons; i++)
        vbuttons.append(buttons[i]);
    GamepadsWKC pad(index, id, naxes, nbuttons);

    gGamepads->at(index)->updateFromPlatformGamepad(pad);
    return true;
}

void sampleGamepads(GamepadList* list)
{
    if (!gGamepads)
        return;

    for(int i=0; i<gGamepads->size(); i++)
        list->set(i, gGamepads->at(i));
}
#endif

void
GamepadsWKC::updateValue(long long timestamp, const Vector<double>& axes, const Vector<double>& buttons)
{
    m_axes = axes;
    m_buttons = buttons;
    m_lastUpdateTime = timestamp;
}

} // namespace

#endif

