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

#ifndef _WKC_HEAP_PEER_H_
#define _WKC_HEAP_PEER_H_

#include <stdio.h>

#include <wkc/wkcbase.h>
#include <wkc/wkcmpeer.h>

WKC_BEGIN_C_LINKAGE

/**
@file
@brief heap related peers.
*/

/*@{*/

WKC_PEER_API void* wkcHeapCallocPeer(size_t p,size_t n);
WKC_PEER_API void wkcHeapFreePeer(void* p);
WKC_PEER_API void* wkcHeapMallocPeer(size_t size);
WKC_PEER_API size_t wkcHeapMallocSizePeer(const void* ptr);
WKC_PEER_API void* wkcHeapReallocPeer(void* old_ptr, size_t new_size);
WKC_PEER_API char* wkcHeapStrDupPeer(const char* src);
WKC_PEER_API void* wkcHeapZeroedMallocPeer(size_t n);
WKC_PEER_API void wkcHeapForbidPeer();
WKC_PEER_API void wkcHeapAllowPeer();
WKC_PEER_API void wkcHeapInitializePeer(void* in_memory, size_t in_physical_memory_size, size_t in_virtual_memory_size);
WKC_PEER_API void wkcHeapFinalizePeer();
WKC_PEER_API void wkcHeapForceTerminatePeer();
WKC_PEER_API void* wkcHeapMallocAlignedPeer(size_t align, size_t size, bool writable, bool executable);
WKC_PEER_API void wkcHeapFreeAlignedPeer(void* p, size_t size);
WKC_PEER_API void* wkcHeapReserveUncommittedPeer(size_t size, bool writable, bool executable);
WKC_PEER_API void* wkcHeapReserveAndCommitPeer(size_t size, bool writable, bool executable);
WKC_PEER_API void wkcHeapReleaseDecommittedPeer(void* ptr, size_t size);
WKC_PEER_API bool wkcHeapCommitPeer(void* ptr, size_t size, bool writable, bool executable);
WKC_PEER_API void wkcHeapDecommitPeer(void* ptr, size_t size);

WKC_PEER_API bool wkcHeapGetNumericPropertyPeer(const char* property, size_t* value);
WKC_PEER_API bool wkcHeapSetNumericPropertyPeer(const char* property, size_t value);
WKC_PEER_API size_t wkcHeapGetPageSizePeer(void);
WKC_PEER_API void wkcHeapCacheFlushPeer(void* in_ptr, size_t in_size);
WKC_PEER_API void wkcHeapNotifyNeverDestroyedPeer(void* in_ptr);

#ifndef WKC_HEAP_PEER_COMPILE_ASSERT
#define WKC_HEAP_PEER_COMPILE_ASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1]
#endif

/*@}*/

WKC_END_C_LINKAGE

#endif /* _WKC_HEAP_PEER_H_ */
