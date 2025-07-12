/*
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

        Vector<SharedGamepadValue> vaxes, vbuttons;
        for (int i = 0; i < naxes; i++)
            vaxes.append(0.0f);
        m_axes = WTFMove(vaxes);
        for (int i = 0; i < nbuttons; i++)
            vbuttons.append(0.0f);
        m_buttons = WTFMove(vbuttons);
    }
    ~GamepadsWKC() {}

    virtual const Vector<SharedGamepadValue>& axisValues() const { return m_axes; }
    virtual const Vector<SharedGamepadValue>& buttonValues() const { return m_buttons; }

    void updateValue(MonotonicTime timestamp, const Vector<SharedGamepadValue>& axes, const Vector<SharedGamepadValue>& buttons);

private:
    Vector<SharedGamepadValue> m_axes;
    Vector<SharedGamepadValue> m_buttons;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
#endif // GamepadsWKC_h
