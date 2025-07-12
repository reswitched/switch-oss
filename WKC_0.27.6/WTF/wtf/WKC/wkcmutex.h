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

#ifndef _WKCMUTEX_H_
#define _WKCMUTEX_H_

#include <chrono>
#include <functional>
#include <atomic>

#include <wkc/wkcpeer.h>

namespace std {

class mutex {
WTF_MAKE_FAST_ALLOCATED;
public:
    inline mutex()
    {
        wkcLightMutexInitializePeer(&m_mutex);
    }
    inline ~mutex()
    {
        wkcLightMutexFinalizePeer(&m_mutex);
    }
    inline void lock()
    {
        wkcLightMutexLockPeer(&m_mutex);
    }
    inline void unlock()
    {
        wkcLightMutexUnlockPeer(&m_mutex);
    }
    inline bool try_lock()
    {
        return wkcLightMutexTryLockPeer(&m_mutex);
    }

    inline WKCLightMutex* platformLock()
    {
        return &m_mutex;
    }

private:
    WKCLightMutex m_mutex;
};

struct try_to_lock_t {};

extern try_to_lock_t try_to_lock;

struct adopt_lock_t {};

extern adopt_lock_t adopt_lock;

template<class Mutex>
class unique_lock {
WTF_MAKE_FAST_ALLOCATED;
public:
    inline unique_lock()
        : m_lock(0)
        , m_have_lock(false)
        , m_locked(false)
    {
    }
    inline unique_lock(unique_lock& other)
        : m_lock(other.lock)
        , m_have_lock(true)
        , m_locked(false)
    {
    }
    inline unique_lock(Mutex& lock)
        : m_lock(&lock)
        , m_have_lock(true)
        , m_locked(false)
    {
        m_lock->lock();
        m_locked = true;
    }
    inline unique_lock(Mutex& lock, try_to_lock_t)
        : m_lock(&lock)
        , m_have_lock(true)
        , m_locked(false)
    {
        m_locked = m_lock->try_lock();
    }
    inline unique_lock(Mutex& lock, adopt_lock_t)
        : m_lock(&lock)
        , m_have_lock(true)
        , m_locked(true)
    {
    }
    inline ~unique_lock()
    {
        if (m_locked)
            m_lock->unlock();
    }

    inline void lock()
    {
        if (!m_have_lock)
            return;
        m_lock->lock();
        m_locked = true;
    }
    inline bool try_lock()
    {
        if (!m_have_lock)
            return false;
        m_locked = m_lock->try_lock();
        return m_locked;
    }
    // WORKAROUND : compile error has occurred with -fdelayed-template-parsin.
    /*inline bool try_lock_for(const std::chrono::duration<std::milli>& timeout)
    {
        if (!m_have_lock)
            return false;

        const std::chrono::microseconds timeoutmicro = std::chrono::duration_cast<std::chrono::microseconds>(timeout);
        const auto start = std::chrono::system_clock::now();
        do {
            if (m_lock.try_lock()) {
                m_locked = true;
                return true;
            }
            auto cur = std::chrono::system_clock::now();
            if (cur - start >= timeoutmicro)
                break;
        } while (1);
        m_locked = false;
        return false;
    }*/
    inline void unlock()
    {
        m_lock->unlock();
        m_locked = false;
    }
    inline bool owns_lock() const
    {
        return m_locked;
    }

    inline Mutex& platformLock()
    {
        return *m_lock;
    }

private:
    Mutex* m_lock;
    bool m_have_lock;
    bool m_locked;
};

template <class T>
class lock_guard {
WTF_MAKE_FAST_ALLOCATED;
public:
    inline lock_guard(T& v)
        : m_value(v)
    {
        m_value.lock();
    }
    inline ~lock_guard()
    {
        m_value.unlock();
    }

private:
    T& m_value;
};

template<typename...>
class scoped_lock;

template <typename First,typename... MutexTypes>
class scoped_lock <First,MutexTypes...> {
    First &m_mutex;
    scoped_lock<MutexTypes...> m_recursive;
public:
    ~scoped_lock() {
        m_mutex.unlock();
    }

    explicit scoped_lock(First& first,MutexTypes&... m)
        : m_mutex(first),
        m_recursive(m...)
    {
        first.lock();
    }
    explicit scoped_lock(First& first,MutexTypes&... m, adopt_lock_t)
        : First(m_mutex),
        m_recursive(m..., adopt_lock)
    {
    }
};

template <>
class scoped_lock <>
{
public:
    scoped_lock() {}
    scoped_lock(adopt_lock_t) {}
};

struct once_flag {
WTF_MAKE_FAST_ALLOCATED;
public:
    once_flag() {
        wkcMemoryRegisterGlobalObjPeer(&m_state, sizeof(std::atomic_uint8_t));
    }
    bool isCalled() const { return m_state == 0x03; }
    bool innerTestAndWait() {
        if (m_state.fetch_or(0x01) == 0)
            return true;
#if !OS(WINDOWS_WKC)
        m_state.wait(0x03);
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
            func(args...);
            flag.setCalled();
        }
    }
}

} // namespace

#endif // _WKCMUTEX_H_
