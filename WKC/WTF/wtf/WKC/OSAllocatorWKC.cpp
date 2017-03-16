/*
 *  Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"
#include "OSAllocator.h"

#include <wkc/wkcmpeer.h>
#include <wkc/wkcheappeer.h>

// debug print
#define NO_NXLOG
#include <utils/nxDebugPrint.h>

namespace WTF {

void*
OSAllocator::reserveUncommitted(size_t size, OSAllocator::Usage usage, bool writable, bool executable, bool /*includesGuardPages*/)
{
    void* ret = wkcHeapReserveUncommittedPeer(size, writable, executable);
    if (!ret) {
        nxLog_e("OSAllocator::reserveUncommitted() failed. Usage-ID: %d", (int)usage);
        wkcMemoryNotifyNoMemoryPeer(size);
    }
    return ret;
}

void*
OSAllocator::reserveAndCommit(size_t size, OSAllocator::Usage usage, bool writable, bool executable, bool /*includesGuardPages*/)
{
    void* ret = wkcHeapReserveAndCommitPeer(size, writable, executable);
    if (!ret) {
        nxLog_e("OSAllocator::reserveAndCommit() failed. Usage-ID: %d", (int)usage);
        wkcMemoryNotifyNoMemoryPeer(size);
    }
    return ret;
}

void
OSAllocator::releaseDecommitted(void* ptr, size_t size)
{
    wkcHeapReleaseDecommittedPeer(ptr, size);
}

void
OSAllocator::commit(void* ptr, size_t size, bool writable, bool executable)
{
    wkcHeapCommitPeer(ptr, size, writable, executable);
}

void
OSAllocator::decommit(void* ptr, size_t size)
{
    wkcHeapDecommitPeer(ptr, size);
}

} // namespace
