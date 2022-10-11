/*
* Copyright (C) 2015-2021 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKCTHREAD_H_
#define _WKCTHREAD_H_

#include <chrono>
#include <functional>
#if defined(__NX_TOOLCHAIN_VERSION__) && ((__NX_TOOLCHAIN_MAJOR__  > 1) || ((__NX_TOOLCHAIN_MAJOR__  == 1) && (__NX_TOOLCHAIN_MINOR__ > 11)) || ((__NX_TOOLCHAIN_MAJOR__  == 1) && (__NX_TOOLCHAIN_MINOR__ == 11) && (__NX_TOOLCHAIN_PATCHLEVEL__ >= 5)))
#include <thread>
#else

namespace std {

class thread {
public:
    class id {
    WTF_MAKE_FAST_ALLOCATED;
    public:
        id();
        id(void* other)
            : m_id(other)
        {}
        ~id()
        {}

        bool operator==(const std::thread::id& other)
        {
            return m_id == other.m_id;
        }
        bool operator!=(const std::thread::id& other)
        {
            return m_id != other.m_id;
        }

        void* m_id;
    };
};

class this_thread {
WTF_MAKE_FAST_ALLOCATED;
public:
    this_thread();
    ~this_thread();

    static void yield();
    static thread::id get_id();
    template <class Rep, class Period>
    static void sleep_for(const chrono::duration<Rep, Period>& rel_time);
};

} // namespace
#endif

#endif // _WKCTHREAD_H_
