/*
* Copyright (C) 2015-2018 ACCESS CO., LTD. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
*     * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
* copyright notice, this list of conditions and the following disclaimer
* in the documentation and/or other materials provided with the
* distribution.
*     * Neither the name of Google Inc. nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _WKCMUTEX_H_
#define _WKCMUTEX_H_

#include <chrono>
#include <functional>

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

#ifdef __clang__
struct once_flag {
WTF_MAKE_FAST_ALLOCATED;
public:
    once_flag();
private:
    void* m_value;
};
#endif

} // namespace

#endif // _WKCMUTEX_H_
