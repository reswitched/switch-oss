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

#pragma once

#include <mutex>
#include <atomic>

#include <wkc/wkcpeer.h>

namespace WKCStd {

struct once_flag {
public:
    once_flag() {
        wkcMemoryRegisterGlobalObjPeer(&m_state, sizeof(std::atomic_uint8_t), nullptr);
    }
    bool isCalled() const { return m_state == 0x03; }
    bool innerTestAndWait() {
        if (m_state.fetch_or(0x01) == 0)
            return true;
#if !OS(WINDOWS_WKC)
        m_state.wait(0x01);
#endif
        return false;
    }
    void setCalled() {
        m_state.store(0x03);
#if !OS(WINDOWS_WKC)
        m_state.notify_all();
#endif
    }
#if OS(WINDOWS_WKC)
    std::mutex& mutex() { return m_mutex; }
#endif
private:
    std::atomic_uint8_t m_state{ 0 };
#if OS(WINDOWS_WKC)
    std::mutex m_mutex;
#endif
};


template <class Callable, class ...Args>
void call_once(once_flag& flag, Callable func, Args&&... args)
{
    if (!flag.isCalled()) {
#if OS(WINDOWS_WKC)
        std::unique_lock lock(flag.mutex());
#endif
        if (flag.innerTestAndWait()) {
            func(std::forward(args)...);
            flag.setCalled();
        }
    }
}

} // namespace

