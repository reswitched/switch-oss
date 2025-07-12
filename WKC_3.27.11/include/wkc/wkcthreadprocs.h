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

#ifndef _WKCTHREADPROCS_H_
#define _WKCTHREADPROCS_H_

#include <wkc/wkcbase.h>

WKC_BEGIN_C_LINKAGE

typedef bool (*wkcWillCreateThreadProc)(const void* in_thread_id, const char* in_name, int* inout_priority, int* inout_core_number, unsigned int* inout_stack_size);
typedef void (*wkcDidCreateThreadProc)(const void* in_thread_id, void* in_native_thread_handle, const char* in_name, int in_priority, int in_core_number, unsigned int in_stack_size);
typedef void (*wkcDidDestroyThreadProc)(const void* in_thread_id, void* in_native_thread_handle);
typedef void* (*wkcAllocThreadStackProc)(unsigned int in_stack_size);
typedef void (*wkcFreeThreadStackProc)(void* in_stack_address);

struct WKCThreadProcs_ {
    wkcWillCreateThreadProc fWillCreateThreadProc;
    wkcDidCreateThreadProc  fDidCreateThreadProc;
    wkcDidDestroyThreadProc fDidDestroyThreadProc;
    wkcAllocThreadStackProc fAllocThreadStackProc;
    wkcFreeThreadStackProc fFreeThreadStackProc;
};
typedef struct WKCThreadProcs_ WKCThreadProcs;

WKC_END_C_LINKAGE

#endif // _WKCTHREADPROCS_H_
