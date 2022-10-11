/*--------------------------------------------------------------------------------*
  Copyright (C)Nintendo. All rights reserved.

  These coded instructions, statements, and computer programs contain
  information of Nintendo and/or its licensed developers and are protected
  by national and international copyright laws.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *--------------------------------------------------------------------------------*/

extern "C" {
    struct AtExitEntry
    {
        void (*func) (void*);
        void* pObject;
        void* pDsoHandle;
    };

    extern AtExitEntry __atexit_start[];
    extern AtExitEntry __atexit_end[];
}

namespace {
    unsigned long g_AtExitEntryCount = 0;

    AtExitEntry* AllocEntry()
    {
        const unsigned long numAtExitEntry = __atexit_end - __atexit_start;
        if (numAtExitEntry <= g_AtExitEntryCount)
        {
            return nullptr;
        }
        unsigned long index = __sync_fetch_and_add(&g_AtExitEntryCount, 1);
        if (numAtExitEntry <= index)
        {
            return nullptr;
        }
        else
        {
            return &__atexit_start[index];
        }
    }

    void CallFinalize(
        void *pDsoHandle, AtExitEntry* pEntryArray, unsigned long* pIndex, unsigned long begin, unsigned long end)
    {
        const unsigned long numAtExitEntry = __atexit_end - __atexit_start;
        if (numAtExitEntry < end)
        {
            end = numAtExitEntry;
        }

        for (unsigned long index = end; index > begin; index--)
        {
            unsigned long savedIndex = *pIndex;
            AtExitEntry* entry = &pEntryArray[index - 1];

            if (pDsoHandle == entry->pDsoHandle)
            {
                entry->func(entry->pObject);
            }

            CallFinalize(pDsoHandle, pEntryArray, pIndex, savedIndex, *pIndex);
        }
    }

    inline int CxaAtExitImpl(
        void (*pDestroyer)(void*), void* pObject, void* pDsoHandle, AtExitEntry* entry)
    {
        if (entry)
        {
            entry->func = pDestroyer;
            entry->pObject = pObject;
            entry->pDsoHandle = pDsoHandle;
            return 0;
        }
        else
        {
            return -1;
        }
    }

    inline void CxaFinalizeImpl(void* pDsoHandle, AtExitEntry* pEntryArray, unsigned long* pArrayIndex)
    {
        CallFinalize(pDsoHandle, pEntryArray, pArrayIndex, 0, *pArrayIndex);
    }
}

extern "C"
{
    extern void (*__init_array_start []) ();
    extern void (*__init_array_end []) ();
    extern void (*__fini_array_start []) ();
    extern void (*__fini_array_end []) ();

    // for TLS
    extern unsigned char            __EX_start[] __attribute__((weak));
    extern unsigned char            __EX_end[] __attribute__((weak));
    extern unsigned char            __tdata_start[] __attribute__((weak));
    extern unsigned char            __tdata_end[] __attribute__((weak));
    extern unsigned char            __tdata_align_abs[] __attribute__((weak));
    extern unsigned char            __tdata_align_rel[] __attribute__((weak));
    extern unsigned char            __tbss_start[] __attribute__((weak));
    extern unsigned char            __tbss_end[] __attribute__((weak));
    extern unsigned char            __tbss_align_abs[] __attribute__((weak));
    extern unsigned char            __tbss_align_rel[] __attribute__((weak));
    extern unsigned char            __rela_dyn_start[] __attribute__((weak));
    extern unsigned char            __rela_dyn_end[] __attribute__((weak));
    extern unsigned char            __rel_dyn_start[] __attribute__((weak));
    extern unsigned char            __rel_dyn_end[] __attribute__((weak));
    extern unsigned char            __rela_plt_start[] __attribute__((weak));
    extern unsigned char            __rela_plt_end[] __attribute__((weak));
    extern unsigned char            __rel_plt_start[] __attribute__((weak));
    extern unsigned char            __rel_plt_end[] __attribute__((weak));
    extern unsigned char            __got_start[] __attribute__((weak));
    extern unsigned char            __got_end[] __attribute__((weak));
    extern unsigned char            _DYNAMIC[] __attribute__((weak));
    int __nnmusl_init_dso(unsigned char *EX_start, unsigned char *EX_end,
                            unsigned char *tdata_start, unsigned char *tdata_end,
                            unsigned char *tdata_align_abs, unsigned char *tdata_align_rel,
                            unsigned char *tbss_start, unsigned char *tbss_end,
                            unsigned char *tbss_align_abs, unsigned char *tbss_align_rel,
                            unsigned char *got_start, unsigned char *got_end,
                            unsigned char *rela_dyn_start, unsigned char *rela_dyn_end,
                            unsigned char *rel_dyn_start, unsigned char *rel_dyn_end,
                            unsigned char *rela_plt_start, unsigned char *rela_plt_end,
                            unsigned char *rel_plt_start, unsigned char *rel_plt_end,
                            unsigned char *DYNAMIC);
    void __nnmusl_fini_dso(unsigned char *EX_start, unsigned char *EX_end,
                            unsigned char *tdata_start, unsigned char *tdata_end,
                            unsigned char *tbss_start, unsigned char *tbss_end);

    void* __dso_handle __attribute__ ((visibility ("hidden"))) = &__dso_handle;
    int __aeabi_atexit(void* object, void (*destroyer)(void*), void* dso_handle) __attribute__ ((visibility ("hidden")));
    int __cxa_atexit(void (*destroyer)(void*), void* pObject, void* dso_handle) __attribute__ ((visibility ("hidden")));
    int __cxa_finalize(void* pDsoHandle) __attribute__ ((visibility ("hidden")));
    void _init() __attribute__ ((visibility ("protected")));
    void _fini() __attribute__ ((visibility ("protected")));
    static volatile int nnmuslTlsInitializationPhase = 0;

    void _init()
    {
        if (nnmuslTlsInitializationPhase == 0)
        {
            nnmuslTlsInitializationPhase = __nnmusl_init_dso( __EX_start, __EX_end,
                                            __tdata_start, __tdata_end,
                                            __tdata_align_abs, __tdata_align_rel,
                                            __tbss_start, __tbss_end,
                                            __tbss_align_abs, __tbss_align_rel,
                                            __got_start, __got_end,
                                            __rela_dyn_start, __rela_dyn_end,
                                            __rel_dyn_start, __rel_dyn_end,
                                            __rela_plt_start, __rela_plt_end,
                                            __rel_plt_start, __rel_plt_end,
                                            (unsigned char *)_DYNAMIC );
            if (nnmuslTlsInitializationPhase == 1)
            {
                return;
            }
        }

        for (void (**f)() = __init_array_start; f < __init_array_end; ++f)
        {
            (*f)();
        }
    }

    void _fini()
    {
        __cxa_finalize(__dso_handle);

        for (void (**f)() = __fini_array_end; f > __fini_array_start; --f)
        {
            (*(f - 1))();
        }

        __nnmusl_fini_dso( __EX_start, __EX_end,
                            __tdata_start, __tdata_end,
                            __tbss_start, __tbss_end );
    }

    int __aeabi_atexit(void* object, void (*destroyer)(void*), void* dso_handle)
    {
        return __cxa_atexit(destroyer, object, dso_handle);
    }

    int __cxa_atexit(void (*pDestroyer)(void*), void* pObject, void* pDsoHandle)
    {
        return CxaAtExitImpl(pDestroyer, pObject, pDsoHandle, pDsoHandle ? AllocEntry() : nullptr);
    }

    int __cxa_finalize(void* pDsoHandle)
    {
        CxaFinalizeImpl(pDsoHandle, __atexit_start, &g_AtExitEntryCount);
        return 0;
    }

}

