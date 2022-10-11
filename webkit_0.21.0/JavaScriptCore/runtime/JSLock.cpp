/*
 * Copyright (C) 2005, 2008, 2012, 2014 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the NU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA 
 *
 */

#include "config.h"
#include "JSLock.h"

#include "Heap.h"
#include "CallFrame.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "JSCInlines.h"
#if !PLATFORM(WKC)
#include <thread>
#else
#include <wkcthread.h>
#endif

namespace JSC {

#if !PLATFORM(WKC)
std::mutex* GlobalJSLock::s_sharedInstanceMutex;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(std::mutex*, GlobalJSLock, s_sharedInstanceMutex, 0);
#endif

GlobalJSLock::GlobalJSLock()
{
    s_sharedInstanceMutex->lock();
}

GlobalJSLock::~GlobalJSLock()
{
    s_sharedInstanceMutex->unlock();
}

void GlobalJSLock::initialize()
{
#if !PLATFORM(WKC)
    s_sharedInstanceMutex = new std::mutex();
#else
    s_sharedInstanceMutex = (std::mutex *)fastMalloc(sizeof(std::mutex));
    s_sharedInstanceMutex = new (s_sharedInstanceMutex)std::mutex();
#endif
}

JSLockHolder::JSLockHolder(ExecState* exec)
    : m_vm(&exec->vm())
{
    init();
}

JSLockHolder::JSLockHolder(VM* vm)
    : m_vm(vm)
{
    init();
}

JSLockHolder::JSLockHolder(VM& vm)
    : m_vm(&vm)
{
    init();
}

void JSLockHolder::init()
{
    m_vm->apiLock().lock();
}

JSLockHolder::~JSLockHolder()
{
    RefPtr<JSLock> apiLock(&m_vm->apiLock());
    m_vm = nullptr;
    apiLock->unlock();
}

JSLock::JSLock(VM* vm)
    : m_ownerThreadID(std::thread::id())
    , m_lockCount(0)
    , m_lockDropDepth(0)
    , m_hasExclusiveThread(false)
    , m_vm(vm)
    , m_entryAtomicStringTable(nullptr)
{
}

JSLock::~JSLock()
{
}

void JSLock::willDestroyVM(VM* vm)
{
    ASSERT_UNUSED(vm, m_vm == vm);
    m_vm = nullptr;
}

void JSLock::setExclusiveThread(std::thread::id threadId)
{
    RELEASE_ASSERT(!m_lockCount && m_ownerThreadID == std::thread::id());
    m_hasExclusiveThread = (threadId != std::thread::id());
    m_ownerThreadID = threadId;
}

void JSLock::lock()
{
    lock(1);
}

void JSLock::lock(intptr_t lockCount)
{
    ASSERT(lockCount > 0);
    if (currentThreadIsHoldingLock()) {
        m_lockCount += lockCount;
        return;
    }

    if (!m_hasExclusiveThread) {
        m_lock.lock();
        m_ownerThreadID = std::this_thread::get_id();
    }
    ASSERT(!m_lockCount);
    m_lockCount = lockCount;

    didAcquireLock();
}

void JSLock::didAcquireLock()
{
    WTF::speculationFence();

    // FIXME: What should happen to the per-thread identifier table if we don't have a VM?
    if (!m_vm)
        return;

    RELEASE_ASSERT(!m_vm->stackPointerAtVMEntry());
    void* p = &p; // A proxy for the current stack pointer.
    m_vm->setStackPointerAtVMEntry(p);

    WTFThreadData& threadData = wtfThreadData();
    m_vm->setLastStackTop(threadData.savedLastStackTop());

    ASSERT(!m_entryAtomicStringTable);
    m_entryAtomicStringTable = threadData.setCurrentAtomicStringTable(m_vm->atomicStringTable());
    ASSERT(m_entryAtomicStringTable);

    m_vm->heap.machineThreads().addCurrentThread();
}

void JSLock::unlock()
{
    unlock(1);
}

void JSLock::unlock(intptr_t unlockCount)
{
    RELEASE_ASSERT(currentThreadIsHoldingLock());
    ASSERT(m_lockCount >= unlockCount);

    // Maintain m_lockCount while calling willReleaseLock() so that its callees know that
    // they still have the lock.
    if (unlockCount == m_lockCount)
        willReleaseLock();

    m_lockCount -= unlockCount;

    if (!m_lockCount) {

        if (!m_hasExclusiveThread) {
            m_ownerThreadID = std::thread::id();
            m_lock.unlock();
        }
    }
}

void JSLock::willReleaseLock()
{
    WTF::speculationFence();

    if (m_vm) {
        m_vm->heap.releaseDelayedReleasedObjects();
        m_vm->setStackPointerAtVMEntry(nullptr);
    }

    if (m_entryAtomicStringTable) {
        wtfThreadData().setCurrentAtomicStringTable(m_entryAtomicStringTable);
        m_entryAtomicStringTable = nullptr;
    }
}

void JSLock::lock(ExecState* exec)
{
    exec->vm().apiLock().lock();
}

void JSLock::unlock(ExecState* exec)
{
    exec->vm().apiLock().unlock();
}

bool JSLock::currentThreadIsHoldingLock()
{
    ASSERT(!m_hasExclusiveThread || (exclusiveThread() == std::this_thread::get_id()));
    if (m_hasExclusiveThread)
        return !!m_lockCount;
    return m_ownerThreadID == std::this_thread::get_id();
}

// This function returns the number of locks that were dropped.
unsigned JSLock::dropAllLocks(DropAllLocks* dropper)
{
    if (m_hasExclusiveThread) {
        ASSERT(exclusiveThread() == std::this_thread::get_id());
        return 0;
    }

    if (!currentThreadIsHoldingLock())
        return 0;

    ++m_lockDropDepth;

    dropper->setDropDepth(m_lockDropDepth);

    WTFThreadData& threadData = wtfThreadData();
    threadData.setSavedStackPointerAtVMEntry(m_vm->stackPointerAtVMEntry());
    threadData.setSavedLastStackTop(m_vm->lastStackTop());

    unsigned droppedLockCount = m_lockCount;
    unlock(droppedLockCount);

    return droppedLockCount;
}

void JSLock::grabAllLocks(DropAllLocks* dropper, unsigned droppedLockCount)
{
    ASSERT(!m_hasExclusiveThread || !droppedLockCount);

    // If no locks were dropped, nothing to do!
    if (!droppedLockCount)
        return;

    ASSERT(!currentThreadIsHoldingLock());
    lock(droppedLockCount);

    while (dropper->dropDepth() != m_lockDropDepth) {
        unlock(droppedLockCount);
        std::this_thread::yield();
        lock(droppedLockCount);
    }

    --m_lockDropDepth;

    WTFThreadData& threadData = wtfThreadData();
    m_vm->setStackPointerAtVMEntry(threadData.savedStackPointerAtVMEntry());
    m_vm->setLastStackTop(threadData.savedLastStackTop());
}

JSLock::DropAllLocks::DropAllLocks(VM* vm)
    : m_droppedLockCount(0)
    // If the VM is in the middle of being destroyed then we don't want to resurrect it
    // by allowing DropAllLocks to ref it. By this point the JSLock has already been 
    // released anyways, so it doesn't matter that DropAllLocks is a no-op.
    , m_vm(vm->refCount() ? vm : nullptr)
{
    if (!m_vm)
        return;
    RELEASE_ASSERT(!m_vm->apiLock().currentThreadIsHoldingLock() || !m_vm->isCollectorBusy());
    m_droppedLockCount = m_vm->apiLock().dropAllLocks(this);
}

JSLock::DropAllLocks::DropAllLocks(ExecState* exec)
    : DropAllLocks(exec ? &exec->vm() : nullptr)
{
}

JSLock::DropAllLocks::DropAllLocks(VM& vm)
    : DropAllLocks(&vm)
{
}

JSLock::DropAllLocks::~DropAllLocks()
{
    if (!m_vm)
        return;
    m_vm->apiLock().grabAllLocks(this, m_droppedLockCount);
}

} // namespace JSC
