/*
 * Copyright (C) 2012, 2013, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Disassembler.h"

#include "MacroAssemblerCodeRef.h"
#include <wtf/DataLog.h>
#include <wtf/Deque.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/StringPrintStream.h>
#include <wtf/Threading.h>
#include <wtf/ThreadingPrimitives.h>

namespace JSC {

void disassemble(const MacroAssemblerCodePtr& codePtr, size_t size, const char* prefix, PrintStream& out, InstructionSubsetHint subsetHint)
{
    if (tryToDisassemble(codePtr, size, prefix, out, subsetHint))
        return;
    
    out.printf("%sdisassembly not available for range %p...%p\n", prefix, codePtr.executableAddress(), static_cast<char*>(codePtr.executableAddress()) + size);
}

namespace {

// This is really a struct, except that it should be a class because that's what the WTF_* macros
// expect.
class DisassemblyTask {
    WTF_MAKE_NONCOPYABLE(DisassemblyTask);
    WTF_MAKE_FAST_ALLOCATED;
public:
    DisassemblyTask()
    {
    }
    
    ~DisassemblyTask()
    {
        if (header)
            free(header); // free() because it would have been copied by strdup.
    }
    
    char* header { nullptr };
    MacroAssemblerCodeRef codeRef;
    size_t size { 0 };
    const char* prefix { nullptr };
    InstructionSubsetHint subsetHint { MacroAssemblerSubset };
};

class AsynchronousDisassembler {
public:
    AsynchronousDisassembler()
    {
        createThread("Asynchronous Disassembler", [&] () { run(); });
    }
    
    void enqueue(std::unique_ptr<DisassemblyTask> task)
    {
        MutexLocker locker(m_lock);
        m_queue.append(WTF::move(task));
        m_condition.broadcast();
    }
    
    void waitUntilEmpty()
    {
        MutexLocker locker(m_lock);
        while (!m_queue.isEmpty() || m_working)
            m_condition.wait(m_lock);
    }
    
private:
    NO_RETURN void run()
    {
        for (;;) {
            std::unique_ptr<DisassemblyTask> task;
            {
                MutexLocker locker(m_lock);
                m_working = false;
                m_condition.broadcast();
                while (m_queue.isEmpty())
                    m_condition.wait(m_lock);
                task = m_queue.takeFirst();
                m_working = true;
            }

            dataLog(task->header);
            disassemble(
                task->codeRef.code(), task->size, task->prefix, WTF::dataFile(),
                task->subsetHint);
        }
    }
    
    Mutex m_lock;
    ThreadCondition m_condition;
    Deque<std::unique_ptr<DisassemblyTask>> m_queue;
    bool m_working { false };
};

#if !PLATFORM(WKC)
bool hadAnyAsynchronousDisassembly = false;
#else
WKC_DEFINE_GLOBAL_BOOL(hadAnyAsynchronousDisassembly, false);
#endif

AsynchronousDisassembler& asynchronousDisassembler()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AsynchronousDisassembler> disassembler;
    hadAnyAsynchronousDisassembly = true;
    return disassembler.get();
#else
    WKC_DEFINE_STATIC_PTR(AsynchronousDisassembler*, disassembler, 0);
    if (!disassembler) {
        disassembler = new AsynchronousDisassembler();
        hadAnyAsynchronousDisassembly = true;
    }
    return *disassembler;
#endif
}

} // anonymous namespace

void disassembleAsynchronously(
    const CString& header, const MacroAssemblerCodeRef& codeRef, size_t size, const char* prefix,
    InstructionSubsetHint subsetHint)
{
    std::unique_ptr<DisassemblyTask> task = std::make_unique<DisassemblyTask>();
    task->header = strdup(header.data()); // Yuck! We need this because CString does racy refcounting.
    task->codeRef = codeRef;
    task->size = size;
    task->prefix = prefix;
    task->subsetHint = subsetHint;
    
    asynchronousDisassembler().enqueue(WTF::move(task));
}

void waitForAsynchronousDisassembly()
{
    if (!hadAnyAsynchronousDisassembly)
        return;
    
    asynchronousDisassembler().waitUntilEmpty();
}

} // namespace JSC

