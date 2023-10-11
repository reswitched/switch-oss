/*
 *  Copyright (C) 2003-2009, 2015 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2009 Acision BV. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "MachineStackMarker.h"

#include "ConservativeRoots.h"
#include "Heap.h"
#include "JSArray.h"
#include "JSCInlines.h"
#include "VM.h"
#include <setjmp.h>
#include <stdlib.h>
#include <wtf/StdLibExtras.h>

#if OS(DARWIN)

#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/vm_map.h>

#elif OS(WINDOWS)

#include <windows.h>
#include <malloc.h>

#elif OS(UNIX)

#include <sys/mman.h>
#include <unistd.h>

#if OS(SOLARIS)
#include <thread.h>
#else
#include <pthread.h>
#endif

#if HAVE(PTHREAD_NP_H)
#include <pthread_np.h>
#endif

#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN)
#include <signal.h>
#endif

#endif

using namespace WTF;

namespace JSC {

#if OS(DARWIN)
typedef mach_port_t PlatformThread;
#elif OS(WINDOWS)
typedef DWORD PlatformThread;
#elif USE(PTHREADS)
typedef pthread_t PlatformThread;
static const int SigThreadSuspendResume = SIGUSR2;

#if defined(SA_RESTART)
static void pthreadSignalHandlerSuspendResume(int)
{
    sigset_t signalSet;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SigThreadSuspendResume);
    sigsuspend(&signalSet);
}
#endif
#elif PLATFORM(WKC)
typedef void* PlatformThread;
#endif

class ActiveMachineThreadsManager;
static ActiveMachineThreadsManager& activeMachineThreadsManager();

class ActiveMachineThreadsManager {
    WTF_MAKE_NONCOPYABLE(ActiveMachineThreadsManager);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:

    class Locker {
    public:
        Locker(ActiveMachineThreadsManager& manager)
            : m_locker(manager.m_lock)
        {
        }

    private:
        MutexLocker m_locker;
    };

    void add(MachineThreads* machineThreads)
    {
        MutexLocker managerLock(m_lock);
        m_set.add(machineThreads);
    }

    void remove(MachineThreads* machineThreads)
    {
        MutexLocker managerLock(m_lock);
        auto recordedMachineThreads = m_set.take(machineThreads);
        RELEASE_ASSERT(recordedMachineThreads = machineThreads);
    }

    bool contains(MachineThreads* machineThreads)
    {
        return m_set.contains(machineThreads);
    }

private:
    typedef HashSet<MachineThreads*> MachineThreadsSet;

    ActiveMachineThreadsManager() { }
    
    Mutex m_lock;
    MachineThreadsSet m_set;

    friend ActiveMachineThreadsManager& activeMachineThreadsManager();
};

static ActiveMachineThreadsManager& activeMachineThreadsManager()
{
#if !PLATFORM(WKC)
    static std::once_flag initializeManagerOnceFlag;
    static ActiveMachineThreadsManager* manager = nullptr;

    std::call_once(initializeManagerOnceFlag, [] {
        manager = new ActiveMachineThreadsManager();
    });
    return *manager;
#else
    WKC_DEFINE_STATIC_PTR(ActiveMachineThreadsManager*, manager, nullptr);
    if (!manager) {
        manager = new ActiveMachineThreadsManager();
    }
    return *manager;
#endif
}
    
static inline PlatformThread getCurrentPlatformThread()
{
#if OS(DARWIN)
    return pthread_mach_thread_np(pthread_self());
#elif OS(WINDOWS)
    return GetCurrentThreadId();
#elif USE(PTHREADS)
    return pthread_self();
#elif PLATFORM(WKC)
    return wkc_pthread_self();
#endif
}

class MachineThreads::Thread {
    WTF_MAKE_FAST_ALLOCATED;

    Thread(const PlatformThread& platThread, void* base)
        : platformThread(platThread)
        , stackBase(base)
    {
#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN) && defined(SA_RESTART)
        // if we have SA_RESTART, enable SIGUSR2 debugging mechanism
        struct sigaction action;
        action.sa_handler = pthreadSignalHandlerSuspendResume;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        sigaction(SigThreadSuspendResume, &action, 0);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SigThreadSuspendResume);
        pthread_sigmask(SIG_UNBLOCK, &mask, 0);
#elif OS(WINDOWS)
        ASSERT(platformThread == GetCurrentThreadId());
        bool isSuccessful =
            DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
                &platformThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
        RELEASE_ASSERT(isSuccessful);
#endif
    }

public:
    ~Thread()
    {
#if OS(WINDOWS)
        CloseHandle(platformThreadHandle);
#endif
    }

    static Thread* createForCurrentThread()
    {
        return new Thread(getCurrentPlatformThread(), wtfThreadData().stack().origin());
    }

    struct Registers {
        inline void* stackPointer() const;
        
#if OS(DARWIN)
#if CPU(X86)
        typedef i386_thread_state_t PlatformRegisters;
#elif CPU(X86_64)
        typedef x86_thread_state64_t PlatformRegisters;
#elif CPU(PPC)
        typedef ppc_thread_state_t PlatformRegisters;
#elif CPU(PPC64)
        typedef ppc_thread_state64_t PlatformRegisters;
#elif CPU(ARM)
        typedef arm_thread_state_t PlatformRegisters;
#elif CPU(ARM64)
        typedef arm_thread_state64_t PlatformRegisters;
#else
#error Unknown Architecture
#endif
        
#elif OS(WINDOWS)
        typedef CONTEXT PlatformRegisters;
#elif USE(PTHREADS)
        typedef pthread_attr_t PlatformRegisters;
#elif PLATFORM(WKC)
        typedef struct wkcRegistersStore_ {
            char fRegs[sizeof(void*)*64];
        } wkcRegistersStore;
        typedef wkcRegistersStore PlatformRegisters;
#else
#error Need a thread register struct for this platform
#endif
        
        PlatformRegisters regs;
    };
    
    inline bool operator==(const PlatformThread& other) const;
    inline bool operator!=(const PlatformThread& other) const { return !(*this == other); }

    inline bool suspend();
    inline void resume();
    size_t getRegisters(Registers&);
    void freeRegisters(Registers&);
    std::pair<void*, size_t> captureStack(void* stackTop);

    Thread* next;
    PlatformThread platformThread;
    void* stackBase;
#if OS(WINDOWS)
    HANDLE platformThreadHandle;
#endif
};

MachineThreads::MachineThreads(Heap* heap)
    : m_registeredThreads(0)
    , m_threadSpecific(0)
#if !ASSERT_DISABLED
    , m_heap(heap)
#endif
{
    UNUSED_PARAM(heap);
    threadSpecificKeyCreate(&m_threadSpecific, removeThread);
    activeMachineThreadsManager().add(this);
}

MachineThreads::~MachineThreads()
{
    activeMachineThreadsManager().remove(this);
    threadSpecificKeyDelete(m_threadSpecific);

    MutexLocker registeredThreadsLock(m_registeredThreadsMutex);
    for (Thread* t = m_registeredThreads; t;) {
        Thread* next = t->next;
        delete t;
        t = next;
    }
}

inline bool MachineThreads::Thread::operator==(const PlatformThread& other) const
{
#if OS(DARWIN) || OS(WINDOWS)
    return platformThread == other;
#elif USE(PTHREADS)
    return !!pthread_equal(platformThread, other);
#elif PLATFORM(WKC)
    return !!wkc_pthread_equal(platformThread, other);
#else
#error Need a way to compare threads on this platform
#endif
}

void MachineThreads::addCurrentThread()
{
    ASSERT(!m_heap->vm()->hasExclusiveThread() || m_heap->vm()->exclusiveThread() == std::this_thread::get_id());

    if (threadSpecificGet(m_threadSpecific)) {
        ASSERT(threadSpecificGet(m_threadSpecific) == this);
        return;
    }

    threadSpecificSet(m_threadSpecific, this);
    Thread* thread = Thread::createForCurrentThread();

    MutexLocker lock(m_registeredThreadsMutex);

    thread->next = m_registeredThreads;
    m_registeredThreads = thread;
}

void MachineThreads::removeThread(void* p)
{
    auto& manager = activeMachineThreadsManager();
    ActiveMachineThreadsManager::Locker lock(manager);
    auto machineThreads = static_cast<MachineThreads*>(p);
    if (manager.contains(machineThreads)) {
        // There's a chance that the MachineThreads registry that this thread
        // was registered with was already destructed, and another one happened
        // to be instantiated at the same address. Hence, this thread may or
        // may not be found in this MachineThreads registry. We only need to
        // do a removal if this thread is found in it.
        machineThreads->removeThreadIfFound(getCurrentPlatformThread());
    }
}

template<typename PlatformThread>
void MachineThreads::removeThreadIfFound(PlatformThread platformThread)
{
    MutexLocker lock(m_registeredThreadsMutex);
    Thread* t = m_registeredThreads;
    if (*t == platformThread) {
        m_registeredThreads = m_registeredThreads->next;
        delete t;
    } else {
        Thread* last = m_registeredThreads;
        for (t = m_registeredThreads->next; t; t = t->next) {
            if (*t == platformThread) {
                last->next = t->next;
                break;
            }
            last = t;
        }
        delete t;
    }
}

SUPPRESS_ASAN
void MachineThreads::gatherFromCurrentThread(ConservativeRoots& conservativeRoots, JITStubRoutineSet& jitStubRoutines, CodeBlockSet& codeBlocks, void* stackOrigin, void* stackTop, RegisterState& calleeSavedRegisters)
{
    void* registersBegin = &calleeSavedRegisters;
    void* registersEnd = reinterpret_cast<void*>(roundUpToMultipleOf<sizeof(void*)>(reinterpret_cast<uintptr_t>(&calleeSavedRegisters + 1)));
    conservativeRoots.add(registersBegin, registersEnd, jitStubRoutines, codeBlocks);

    conservativeRoots.add(stackTop, stackOrigin, jitStubRoutines, codeBlocks);
}

inline bool MachineThreads::Thread::suspend()
{
#if OS(DARWIN)
    kern_return_t result = thread_suspend(platformThread);
    return result == KERN_SUCCESS;
#elif OS(WINDOWS)
    bool threadIsSuspended = (SuspendThread(platformThreadHandle) != (DWORD)-1);
    ASSERT(threadIsSuspended);
    return threadIsSuspended;
#elif USE(PTHREADS)
    pthread_kill(platformThread, SigThreadSuspendResume);
    return true;
#elif PLATFORM(WKC)
    wkcThreadSuspendPeer(platformThread);
    return true;
#else
#error Need a way to suspend threads on this platform
#endif
}

inline void MachineThreads::Thread::resume()
{
#if OS(DARWIN)
    thread_resume(platformThread);
#elif OS(WINDOWS)
    ResumeThread(platformThreadHandle);
#elif USE(PTHREADS)
    pthread_kill(platformThread, SigThreadSuspendResume);
#elif PLATFORM(WKC)
    wkcThreadResumePeer(platformThread);
#else
#error Need a way to resume threads on this platform
#endif
}

size_t MachineThreads::Thread::getRegisters(MachineThreads::Thread::Registers& registers)
{
    Thread::Registers::PlatformRegisters& regs = registers.regs;
#if OS(DARWIN)
#if CPU(X86)
    unsigned user_count = sizeof(regs)/sizeof(int);
    thread_state_flavor_t flavor = i386_THREAD_STATE;
#elif CPU(X86_64)
    unsigned user_count = x86_THREAD_STATE64_COUNT;
    thread_state_flavor_t flavor = x86_THREAD_STATE64;
#elif CPU(PPC) 
    unsigned user_count = PPC_THREAD_STATE_COUNT;
    thread_state_flavor_t flavor = PPC_THREAD_STATE;
#elif CPU(PPC64)
    unsigned user_count = PPC_THREAD_STATE64_COUNT;
    thread_state_flavor_t flavor = PPC_THREAD_STATE64;
#elif CPU(ARM)
    unsigned user_count = ARM_THREAD_STATE_COUNT;
    thread_state_flavor_t flavor = ARM_THREAD_STATE;
#elif CPU(ARM64)
    unsigned user_count = ARM_THREAD_STATE64_COUNT;
    thread_state_flavor_t flavor = ARM_THREAD_STATE64;
#else
#error Unknown Architecture
#endif

    kern_return_t result = thread_get_state(platformThread, flavor, (thread_state_t)&regs, &user_count);
    if (result != KERN_SUCCESS) {
        WTFReportFatalError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, 
                            "JavaScript garbage collection failed because thread_get_state returned an error (%d). This is probably the result of running inside Rosetta, which is not supported.", result);
        CRASH();
    }
    return user_count * sizeof(uintptr_t);
// end OS(DARWIN)

#elif OS(WINDOWS)
    regs.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
    GetThreadContext(platformThreadHandle, &regs);
    return sizeof(CONTEXT);
#elif USE(PTHREADS)
    pthread_attr_init(&regs);
#if HAVE(PTHREAD_NP_H) || OS(NETBSD)
#if !OS(OPENBSD)
    // e.g. on FreeBSD 5.4, neundorf@kde.org
    pthread_attr_get_np(platformThread, &regs);
#endif
#else
    // FIXME: this function is non-portable; other POSIX systems may have different np alternatives
    pthread_getattr_np(platformThread, &regs);
#endif
    return 0;
#elif PLATFORM(WKC)
    return wkcThreadStoreThreadRegistersPeer(platformThread, (void *)&regs, sizeof(regs));
#else
#error Need a way to get thread registers on this platform
#endif
}

inline void* MachineThreads::Thread::Registers::stackPointer() const
{
#if OS(DARWIN)

#if __DARWIN_UNIX03

#if CPU(X86)
    return reinterpret_cast<void*>(regs.__esp);
#elif CPU(X86_64)
    return reinterpret_cast<void*>(regs.__rsp);
#elif CPU(PPC) || CPU(PPC64)
    return reinterpret_cast<void*>(regs.__r1);
#elif CPU(ARM)
    return reinterpret_cast<void*>(regs.__sp);
#elif CPU(ARM64)
    return reinterpret_cast<void*>(regs.__sp);
#else
#error Unknown Architecture
#endif

#else // !__DARWIN_UNIX03

#if CPU(X86)
    return reinterpret_cast<void*>(regs.esp);
#elif CPU(X86_64)
    return reinterpret_cast<void*>(regs.rsp);
#elif CPU(PPC) || CPU(PPC64)
    return reinterpret_cast<void*>(regs.r1);
#else
#error Unknown Architecture
#endif

#endif // __DARWIN_UNIX03

// end OS(DARWIN)
#elif OS(WINDOWS)

#if CPU(ARM)
    return reinterpret_cast<void*>((uintptr_t) regs.Sp);
#elif CPU(MIPS)
    return reinterpret_cast<void*>((uintptr_t) regs.IntSp);
#elif CPU(X86)
    return reinterpret_cast<void*>((uintptr_t) regs.Esp);
#elif CPU(X86_64)
    return reinterpret_cast<void*>((uintptr_t) regs.Rsp);
#else
#error Unknown Architecture
#endif

#elif USE(PTHREADS)
    void* stackBase = 0;
    size_t stackSize = 0;
#if OS(OPENBSD)
    stack_t ss;
    int rc = pthread_stackseg_np(pthread_self(), &ss);
    stackBase = (void*)((size_t) ss.ss_sp - ss.ss_size);
    stackSize = ss.ss_size;
#else
    int rc = pthread_attr_getstack(&regs, &stackBase, &stackSize);
#endif
    (void)rc; // FIXME: Deal with error code somehow? Seems fatal.
    ASSERT(stackBase);
    return static_cast<char*>(stackBase) + stackSize;
#elif PLATFORM(WKC)
    return wkcThreadGetStackPointerPeer(&regs);
#else
#error Need a way to get the stack pointer for another thread on this platform
#endif
}

void MachineThreads::Thread::freeRegisters(MachineThreads::Thread::Registers& registers)
{
    Thread::Registers::PlatformRegisters& regs = registers.regs;
#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN)
    pthread_attr_destroy(&regs);
#else
    UNUSED_PARAM(regs);
#endif
}

std::pair<void*, size_t> MachineThreads::Thread::captureStack(void* stackTop)
{
    void* begin = stackBase;
    void* end = reinterpret_cast<void*>(
        WTF::roundUpToMultipleOf<sizeof(void*)>(reinterpret_cast<uintptr_t>(stackTop)));
    if (begin > end)
        std::swap(begin, end);
    return std::make_pair(begin, static_cast<char*>(end) - static_cast<char*>(begin));
}

SUPPRESS_ASAN
static void copyMemory(void* dst, const void* src, size_t size)
{
    size_t dstAsSize = reinterpret_cast<size_t>(dst);
    size_t srcAsSize = reinterpret_cast<size_t>(src);
    RELEASE_ASSERT(dstAsSize == WTF::roundUpToMultipleOf<sizeof(intptr_t)>(dstAsSize));
    RELEASE_ASSERT(srcAsSize == WTF::roundUpToMultipleOf<sizeof(intptr_t)>(srcAsSize));
    RELEASE_ASSERT(size == WTF::roundUpToMultipleOf<sizeof(intptr_t)>(size));

    intptr_t* dstPtr = reinterpret_cast<intptr_t*>(dst);
    const intptr_t* srcPtr = reinterpret_cast<const intptr_t*>(src);
    size /= sizeof(intptr_t);
    while (size--)
        *dstPtr++ = *srcPtr++;
}
    


// This function must not call malloc(), free(), or any other function that might
// acquire a lock. Since 'thread' is suspended, trying to acquire a lock
// will deadlock if 'thread' holds that lock.
// This function, specifically the memory copying, was causing problems with Address Sanitizer in
// apps. Since we cannot blacklist the system memcpy we must use our own naive implementation,
// copyMemory, for ASan to work on either instrumented or non-instrumented builds. This is not a
// significant performance loss as tryCopyOtherThreadStack is only called as part of an O(heapsize)
// operation. As the heap is generally much larger than the stack the performance hit is minimal.
// See: https://bugs.webkit.org/show_bug.cgi?id=146297
void MachineThreads::tryCopyOtherThreadStack(Thread* thread, void* buffer, size_t capacity, size_t* size)
{
    Thread::Registers registers;
    size_t registersSize = thread->getRegisters(registers);
    std::pair<void*, size_t> stack = thread->captureStack(registers.stackPointer());

    bool canCopy = *size + registersSize + stack.second <= capacity;

    if (canCopy)
        copyMemory(static_cast<char*>(buffer) + *size, &registers, registersSize);
    *size += registersSize;

    if (canCopy)
        copyMemory(static_cast<char*>(buffer) + *size, stack.first, stack.second);
    *size += stack.second;

    thread->freeRegisters(registers);
}

bool MachineThreads::tryCopyOtherThreadStacks(MutexLocker&, void* buffer, size_t capacity, size_t* size)
{
    // Prevent two VMs from suspending each other's threads at the same time,
    // which can cause deadlock: <rdar://problem/20300842>.
#if !PLATFORM(WKC)
    static StaticSpinLock mutex;
    std::lock_guard<StaticSpinLock> lock(mutex);
#else
    WKC_DEFINE_STATIC_PTR(StaticSpinLock*, mutex, new StaticSpinLock());
    std::lock_guard<StaticSpinLock> lock(*mutex);
#endif

    *size = 0;

    PlatformThread currentPlatformThread = getCurrentPlatformThread();
    int numberOfThreads = 0; // Using 0 to denote that we haven't counted the number of threads yet.
    int index = 1;
    Thread* threadsToBeDeleted = nullptr;

    Thread* previousThread = nullptr;
    for (Thread* thread = m_registeredThreads; thread; index++) {
        if (*thread != currentPlatformThread) {
            bool success = thread->suspend();
#if OS(DARWIN)
            if (!success) {
                if (!numberOfThreads) {
                    for (Thread* countedThread = m_registeredThreads; countedThread; countedThread = countedThread->next)
                        numberOfThreads++;
                }
                
                // Re-do the suspension to get the actual failure result for logging.
                kern_return_t error = thread_suspend(thread->platformThread);
                ASSERT(error != KERN_SUCCESS);

                WTFReportError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION,
                    "JavaScript garbage collection encountered an invalid thread (err 0x%x): Thread [%d/%d: %p] platformThread %p.",
                    error, index, numberOfThreads, thread, reinterpret_cast<void*>(thread->platformThread));

                // Put the invalid thread on the threadsToBeDeleted list.
                // We can't just delete it here because we have suspended other
                // threads, and they may still be holding the C heap lock which
                // we need for deleting the invalid thread. Hence, we need to
                // defer the deletion till after we have resumed all threads.
                Thread* nextThread = thread->next;
                thread->next = threadsToBeDeleted;
                threadsToBeDeleted = thread;

                if (previousThread)
                    previousThread->next = nextThread;
                else
                    m_registeredThreads = nextThread;
                thread = nextThread;
                continue;
            }
#else
            UNUSED_PARAM(numberOfThreads);
            UNUSED_PARAM(previousThread);
            ASSERT_UNUSED(success, success);
#endif
        }
        previousThread = thread;
        thread = thread->next;
    }

    for (Thread* thread = m_registeredThreads; thread; thread = thread->next) {
        if (*thread != currentPlatformThread)
            tryCopyOtherThreadStack(thread, buffer, capacity, size);
    }

    for (Thread* thread = m_registeredThreads; thread; thread = thread->next) {
        if (*thread != currentPlatformThread)
            thread->resume();
    }

    for (Thread* thread = threadsToBeDeleted; thread; ) {
        Thread* nextThread = thread->next;
        delete thread;
        thread = nextThread;
    }
    
    return *size <= capacity;
}

static void growBuffer(size_t size, void** buffer, size_t* capacity)
{
    if (*buffer)
        fastFree(*buffer);

    *capacity = WTF::roundUpToMultipleOf(WTF::pageSize(), size * 2);
    *buffer = fastMalloc(*capacity);
}

void MachineThreads::gatherConservativeRoots(ConservativeRoots& conservativeRoots, JITStubRoutineSet& jitStubRoutines, CodeBlockSet& codeBlocks, void* stackOrigin, void* stackTop, RegisterState& calleeSavedRegisters)
{
    gatherFromCurrentThread(conservativeRoots, jitStubRoutines, codeBlocks, stackOrigin, stackTop, calleeSavedRegisters);

    size_t size;
    size_t capacity = 0;
    void* buffer = nullptr;
    MutexLocker lock(m_registeredThreadsMutex);
    while (!tryCopyOtherThreadStacks(lock, buffer, capacity, &size))
        growBuffer(size, &buffer, &capacity);

    if (!buffer)
        return;

    conservativeRoots.add(buffer, static_cast<char*>(buffer) + size, jitStubRoutines, codeBlocks);
    fastFree(buffer);
}

} // namespace JSC
