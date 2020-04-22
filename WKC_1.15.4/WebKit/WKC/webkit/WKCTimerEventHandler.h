/*
 *  WKCTimerEventHandler.h
 *
 *  Copyright (c) 2010-2014 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef WKCTimerEventHandler_h
#define WKCTimerEventHandler_h

// class definition

namespace WKC {

/*@{*/

/** @brief Class that notifies of timer-related events */
class WKCTimerEventHandler
{
public:
    // requests to call WKCWebKitWakeUp from browser thread.
    /**
       @brief Requests timer firing
       @param in_timer Peer Timer instance
       @param in_proc callback function
       @param in_data Data to be used in in_proc argument
       @retval "!= false" Succeeded in setting timer
       @retval "== false" Failed to set timer
       @details Notifies of timer firing requests in order to call in_proc.
    */
    virtual bool requestWakeUp(void* in_timer, bool(*in_proc)(void*), void* in_data) = 0;

    // cancel to call WKCWebKitWakeUp from browser thread.
    /**
       @brief cancel timer
       @param in_timer Peer Timer instance
       @details Notifies of timer canceling requests in order to call in_proc.
    */
    virtual void cancelWakeUp(void* in_timer) = 0;
};

/*@}*/

} // namespace

#endif // WKCTimerEventHandler_h
