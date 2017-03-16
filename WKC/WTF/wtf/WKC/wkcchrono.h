/*
* Copyright (C) 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKCCHRONO_H_
#define _WKCCHRONO_H_

#include <chrono>
#include <limits>

namespace std {

namespace chrono {

    template <typename Clock, typename Duration = typename Clock::duration>
    class wkc_time_point {
    WTF_MAKE_FAST_ALLOCATED;
    public:
        typedef Clock clock;
        typedef Duration duration;
        typedef long long rep;
        typedef std::micro period;

        wkc_time_point()
            : m_value(0)
        {
        }
        wkc_time_point(const duration& value)
            : m_value(value)
        {
        }
        ~wkc_time_point()
        {
        }

        duration time_since_epoch() const;

        wkc_time_point& operator+=(const duration& other)
        {
            m_value += other;
            return *this;
        }
        wkc_time_point& operator-=(const duration& other)
        {
            m_value -= other;
            return *this;
        }
        wkc_time_point operator+(const wkc_time_point& other)
        {
            wkc_time_point ret = m_value + other.m_value;
            return ret;
        }
        wkc_time_point operator+(const std::chrono::seconds& other)
        {
            wkc_time_point ret = m_value + other;
            return ret;
        }
        wkc_time_point operator-(wkc_time_point& other)
        {
            wkc_time_point ret = m_value - other.m_value;
            return ret;
        }
        wkc_time_point operator-(const wkc_time_point& other)
        {
            wkc_time_point ret = m_value - other.m_value;
            return ret;
        }
        bool operator<(const wkc_time_point& other) const
        {
            return m_value < other.m_value;
        }
        bool operator<(const std::chrono::seconds& other) const
        {
            return m_value < other;
        }
        bool operator<=(const wkc_time_point& other) const
        {
            return m_value <= other.m_value;
        }
        bool operator>(const wkc_time_point& other) const
        {
            return m_value > other.m_value;
        }
        bool operator>(const std::chrono::milliseconds& other) const
        {
            return m_value > other;
        }
        bool operator>=(const wkc_time_point& other) const
        {
            return m_value >= other.m_value;
        }
        bool operator>=(const std::chrono::milliseconds& other) const
        {
            return m_value >= other;
        }
        bool operator==(const wkc_time_point& other) const
        {
            return m_value == other.m_value;
        }
        bool operator!=(const wkc_time_point& other) const
        {
            return m_value != other.m_value;
        }

        const duration& get() const { return m_value; }

        static const duration min() { return duration::min(); }
        static const duration max() { return duration::max(); }
    private:
        duration m_value;
    };

    template <class ToDuration, class Clock, class Duration>
    static wkc_time_point<Clock, ToDuration> time_point_cast(const wkc_time_point<Clock, Duration>&);

    class wkc_steady_clock {
    WTF_MAKE_FAST_ALLOCATED;
    public:
        wkc_steady_clock();
        ~wkc_steady_clock();

        typedef long long rep;
        typedef std::micro period;
        typedef std::chrono::duration<rep, period> duration;
        typedef std::chrono::wkc_time_point<std::chrono::wkc_steady_clock> wkc_time_point;

        static bool is_steady() { return true; }

        static std::chrono::wkc_time_point<wkc_steady_clock> now();
    };

    class wkc_system_clock {
    WTF_MAKE_FAST_ALLOCATED;
    public:
        wkc_system_clock();
        ~wkc_system_clock();

        typedef long long rep;
        typedef std::micro period;
        typedef std::chrono::duration<rep, period> duration;
        typedef std::chrono::wkc_time_point<std::chrono::wkc_system_clock> wkc_time_point;

        static std::chrono::wkc_time_point<wkc_system_clock> now();
        static time_t to_time_t(const std::chrono::wkc_time_point<wkc_system_clock>&);
    };

} // namespace chrono

} // namespace std

#define time_point wkc_time_point
#define steady_clock wkc_steady_clock
#define system_clock wkc_system_clock

#endif // _WKCCHRONO_H_
