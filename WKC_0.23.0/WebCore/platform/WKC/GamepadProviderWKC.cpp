/*
* Copyright (c) 2016 ACCESS CO., LTD. All rights reserved.
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
#include "GamepadProviderWKC.h"

#if ENABLE(GAMEPAD)

#include "GamepadProviderClient.h"
#include "PlatformGamepad.h"
#include "GamepadsWKC.h"

namespace WebCore {

GamepadProviderWKC&
GamepadProviderWKC::singleton()
{
    WKC_DEFINE_STATIC_PTR(GamepadProviderWKC*, gamepadProvider, 0);
    if (!gamepadProvider)
        gamepadProvider = new GamepadProviderWKC();
    return *gamepadProvider;
}

GamepadProviderWKC::GamepadProviderWKC()
{
    // currently nothing to do.
}

int
GamepadProviderWKC::getUnusedIndex()
{
    int index = 0;
    while (index < m_gamepads.size() && m_gamepads[index])
        ++index;

    return index;
}

void
GamepadProviderWKC::startMonitoringGamepads(GamepadProviderClient* in_client)
{
    ASSERT(!m_clients.contains(in_client));
    m_clients.add(in_client);
}

void
GamepadProviderWKC::stopMonitoringGamepads(GamepadProviderClient* in_client)
{
    ASSERT(m_clients.contains(in_client));
    m_clients.remove(in_client);
}

int
GamepadProviderWKC::addGamepad(const WTF::String& in_id, int in_naxes, int in_nbuttons)
{
    int index = getUnusedIndex();
    GamepadsWKC* gamepad = new GamepadsWKC(index, in_id, in_naxes, in_nbuttons);
    if (!gamepad)
        return -1;

    if (m_gamepads.size() <= index)
        m_gamepads.resize(index + 1);

    m_gamepads[index] = gamepad;

    for (auto& client : m_clients)
        client->platformGamepadConnected(*m_gamepads[index]);

    return index;
}

void
GamepadProviderWKC::removeGamepad(unsigned int in_index)
{
    for (auto& client : m_clients)
        client->platformGamepadDisconnected(*m_gamepads[in_index]);

    delete m_gamepads[in_index];
    m_gamepads[in_index] = nullptr;
}

void
GamepadProviderWKC::updateGamepadValue(unsigned int in_index, long long timestamp, int in_naxes, const double* in_axes, int in_nbuttons, const double* in_buttons)
{
    if (!m_gamepads[in_index])
        return;

    GamepadsWKC* gamepad = static_cast<GamepadsWKC*>(m_gamepads[in_index]);
    Vector<double> vaxes, vbuttons;
    for (int i = 0; i < in_naxes; i++)
        vaxes.append(in_axes[i]);
    for (int i = 0; i < in_nbuttons; i++)
        vbuttons.append(in_buttons[i]);
    gamepad->updateValue(timestamp, vaxes, vbuttons);

    for (auto& client : m_clients)
        client->platformGamepadInputActivity(true);
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
