/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (c) 2010, 2012 ACCESS CO., LTD. All rights reserved.
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
 */

#include "config.h"
#include "EventLoop.h"

namespace WebCore {

typedef bool (*CycleProc)(void*);
WKC_DEFINE_GLOBAL_PTR(CycleProc, gCycleProc, 0);
WKC_DEFINE_GLOBAL_PTR(void*, gCycleOpaque, 0);

void EventLoop_setCycleProc(bool(*proc)(void*), void* opaque)
{
    gCycleProc = proc;
    gCycleOpaque = opaque;
}

void EventLoop::cycle()
{
    if (!gCycleProc) {
        m_ended = true;
        return;
    }

    if (!gCycleProc(gCycleOpaque))
        m_ended = true;
}

} // namespace WebCore
