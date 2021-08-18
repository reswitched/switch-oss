/*
 *  wkcthreadprocs.h
 *
 *  Copyright(c) 2017-2019 ACCESS CO., LTD. All rights reserved.
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
