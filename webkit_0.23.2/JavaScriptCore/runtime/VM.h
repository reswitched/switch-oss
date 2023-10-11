/*
 * Copyright (C) 2008, 2009, 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VM_h
#define VM_h

#include "ControlFlowProfiler.h"
#include "DateInstanceCache.h"
#include "ExecutableAllocator.h"
#include "FunctionHasExecutedCache.h"
#if ENABLE(JIT)
#include "GPRInfo.h"
#endif
#include "Heap.h"
#include "Intrinsic.h"
#include "JITThunks.h"
#include "JSCJSValue.h"
#include "JSLock.h"
#include "LLIntData.h"
#include "MacroAssemblerCodeRef.h"
#include "NumericStrings.h"
#include "PrivateName.h"
#include "PrototypeMap.h"
#include "SmallStrings.h"
#include "SourceCode.h"
#include "SourceProvider.h"
#include "SourceProviderCache.h"
#include "Strong.h"
#include "ThunkGenerators.h"
#include "TypedArrayController.h"
#include "VMEntryRecord.h"
#include "Watchdog.h"
#include "Watchpoint.h"
#include "WeakRandom.h"
#include <wtf/Bag.h>
#include <wtf/BumpPointerAllocator.h>
#include <wtf/DateMath.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/SimpleStats.h>
#include <wtf/StackBounds.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/WTFThreadData.h>
#include <wtf/text/SymbolRegistry.h>
#include <wtf/text/WTFString.h>
#if ENABLE(REGEXP_TRACING)
#include <wtf/ListHashSet.h>
#endif

namespace JSC {

class BuiltinExecutables;
class BytecodeIntrinsicRegistry;
class CodeBlock;
class CodeCache;
class CommonIdentifiers;
class ExecState;
class Exception;
class HandleStack;
class TypeProfiler;
class TypeProfilerLog;
class Identifier;
class Interpreter;
class JSGlobalObject;
class JSObject;
class Keywords;
class LLIntOffsetsExtractor;
class NativeExecutable;
class RegExpCache;
class RegisterAtOffsetList;
class ScriptExecutable;
class SourceProvider;
class SourceProviderCache;
struct StackFrame;
class Stringifier;
class Structure;
#if ENABLE(REGEXP_TRACING)
class RegExp;
#endif
class UnlinkedCodeBlock;
class UnlinkedEvalCodeBlock;
class UnlinkedFunctionExecutable;
class UnlinkedProgramCodeBlock;
class VirtualRegister;
class VMEntryScope;
class Watchpoint;
class WatchpointSet;

#if ENABLE(DFG_JIT)
namespace DFG {
class LongLivedState;
}
#endif // ENABLE(DFG_JIT)
#if ENABLE(FTL_JIT)
namespace FTL {
class Thunks;
}
#endif // ENABLE(FTL_JIT)
namespace CommonSlowPaths {
struct ArityCheckData;
}
namespace Profiler {
class Database;
}

struct HashTable;
struct Instruction;

struct LocalTimeOffsetCache {
    LocalTimeOffsetCache()
        : start(0.0)
        , end(-1.0)
        , increment(0.0)
        , timeType(WTF::UTCTime)
    {
    }

    void reset()
    {
        offset = LocalTimeOffset();
        start = 0.0;
        end = -1.0;
        increment = 0.0;
        timeType = WTF::UTCTime;
    }

    LocalTimeOffset offset;
    double start;
    double end;
    double increment;
    WTF::TimeType timeType;
};

class ConservativeRoots;

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4200) // Disable "zero-sized array in struct/union" warning
#endif
struct ScratchBuffer {
    ScratchBuffer()
    {
        u.m_activeLength = 0;
    }

    static ScratchBuffer* create(size_t size)
    {
        ScratchBuffer* result = new (fastMalloc(ScratchBuffer::allocationSize(size))) ScratchBuffer;

        return result;
    }

    static size_t allocationSize(size_t bufferSize) { return sizeof(ScratchBuffer) + bufferSize; }
    void setActiveLength(size_t activeLength) { u.m_activeLength = activeLength; }
    size_t activeLength() const { return u.m_activeLength; };
    size_t* activeLengthPtr() { return &u.m_activeLength; };
    void* dataBuffer() { return m_buffer; }

    union {
        size_t m_activeLength;
        double pad; // Make sure m_buffer is double aligned.
    } u;
#if CPU(MIPS) && (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == 2)
    void* m_buffer[0] __attribute__((aligned(8)));
#else
    void* m_buffer[0];
#endif
};
#if COMPILER(MSVC)
#pragma warning(pop)
#endif

class VM : public ThreadSafeRefCounted<VM> {
public:
    // WebCore has a one-to-one mapping of threads to VMs;
    // either create() or createLeaked() should only be called once
    // on a thread, this is the 'default' VM (it uses the
    // thread's default string uniquing table from wtfThreadData).
    // API contexts created using the new context group aware interface
    // create APIContextGroup objects which require less locking of JSC
    // than the old singleton APIShared VM created for use by
    // the original API.
    enum VMType { Default, APIContextGroup, APIShared };

    struct ClientData {
        JS_EXPORT_PRIVATE virtual ~ClientData() = 0;
    };

    bool isSharedInstance() { return vmType == APIShared; }
    bool usingAPI() { return vmType != Default; }
    JS_EXPORT_PRIVATE static bool sharedInstanceExists();
    JS_EXPORT_PRIVATE static VM& sharedInstance();

    JS_EXPORT_PRIVATE static Ref<VM> create(HeapType = SmallHeap);
    JS_EXPORT_PRIVATE static Ref<VM> createLeaked(HeapType = SmallHeap);
    static Ref<VM> createContextGroup(HeapType = SmallHeap);
    JS_EXPORT_PRIVATE ~VM();

private:
    RefPtr<JSLock> m_apiLock;

public:
#if ENABLE(ASSEMBLER)
    // executableAllocator should be destructed after the heap, as the heap can call executableAllocator
    // in its destructor.
    ExecutableAllocator executableAllocator;
#endif

    // The heap should be just after executableAllocator and before other members to ensure that it's
    // destructed after all the objects that reference it.
    Heap heap;

#if ENABLE(DFG_JIT)
    std::unique_ptr<DFG::LongLivedState> dfgState;
#endif // ENABLE(DFG_JIT)

    VMType vmType;
    ClientData* clientData;
    VMEntryFrame* topVMEntryFrame;
    ExecState* topCallFrame;
    std::unique_ptr<Watchdog> watchdog;

    Strong<Structure> structureStructure;
    Strong<Structure> structureRareDataStructure;
    Strong<Structure> terminatedExecutionErrorStructure;
    Strong<Structure> stringStructure;
    Strong<Structure> notAnObjectStructure;
    Strong<Structure> propertyNameIteratorStructure;
    Strong<Structure> propertyNameEnumeratorStructure;
    Strong<Structure> getterSetterStructure;
    Strong<Structure> customGetterSetterStructure;
    Strong<Structure> scopedArgumentsTableStructure;
    Strong<Structure> apiWrapperStructure;
    Strong<Structure> JSScopeStructure;
    Strong<Structure> executableStructure;
    Strong<Structure> nativeExecutableStructure;
    Strong<Structure> evalExecutableStructure;
    Strong<Structure> programExecutableStructure;
    Strong<Structure> functionExecutableStructure;
    Strong<Structure> regExpStructure;
    Strong<Structure> symbolStructure;
    Strong<Structure> symbolTableStructure;
    Strong<Structure> structureChainStructure;
    Strong<Structure> sparseArrayValueMapStructure;
    Strong<Structure> templateRegistryKeyStructure;
    Strong<Structure> arrayBufferNeuteringWatchpointStructure;
    Strong<Structure> unlinkedFunctionExecutableStructure;
    Strong<Structure> unlinkedProgramCodeBlockStructure;
    Strong<Structure> unlinkedEvalCodeBlockStructure;
    Strong<Structure> unlinkedFunctionCodeBlockStructure;
    Strong<Structure> propertyTableStructure;
    Strong<Structure> weakMapDataStructure;
    Strong<Structure> inferredValueStructure;
    Strong<Structure> functionRareDataStructure;
    Strong<Structure> exceptionStructure;
#if ENABLE(PROMISES)
    Strong<Structure> promiseDeferredStructure;
#endif
    Strong<JSCell> iterationTerminator;
    Strong<JSCell> emptyPropertyNameEnumerator;

    AtomicStringTable* m_atomicStringTable;
    WTF::SymbolRegistry m_symbolRegistry;
    CommonIdentifiers* propertyNames;
    const MarkedArgumentBuffer* emptyList; // Lists are supposed to be allocated on the stack to have their elements properly marked, which is not the case here - but this list has nothing to mark.
    SmallStrings smallStrings;
    NumericStrings numericStrings;
    DateInstanceCache dateInstanceCache;
    WTF::SimpleStats machineCodeBytesPerBytecodeWordForBaselineJIT;
    WeakGCMap<StringImpl*, JSString, PtrHash<StringImpl*>> stringCache;
    Strong<JSString> lastCachedString;

    AtomicStringTable* atomicStringTable() const { return m_atomicStringTable; }
    WTF::SymbolRegistry& symbolRegistry() { return m_symbolRegistry; }

    void setInDefineOwnProperty(bool inDefineOwnProperty)
    {
        m_inDefineOwnProperty = inDefineOwnProperty;
    }

    bool isInDefineOwnProperty()
    {
        return m_inDefineOwnProperty;
    }

#if ENABLE(JIT)
    bool canUseJIT() { return m_canUseJIT; }
#else
    bool canUseJIT() { return false; } // interpreter only
#endif

#if ENABLE(YARR_JIT)
    bool canUseRegExpJIT() { return m_canUseRegExpJIT; }
#else
    bool canUseRegExpJIT() { return false; } // interpreter only
#endif

    SourceProviderCache* addSourceProviderCache(SourceProvider*);
    void clearSourceProviderCaches();

    PrototypeMap prototypeMap;

    typedef HashMap<RefPtr<SourceProvider>, RefPtr<SourceProviderCache>> SourceProviderCacheMap;
    SourceProviderCacheMap sourceProviderCacheMap;
    std::unique_ptr<Keywords> keywords;
    Interpreter* interpreter;
#if ENABLE(JIT)
#if NUMBER_OF_CALLEE_SAVES_REGISTERS > 0
    intptr_t calleeSaveRegistersBuffer[NUMBER_OF_CALLEE_SAVES_REGISTERS];

    static ptrdiff_t calleeSaveRegistersBufferOffset()
    {
        return OBJECT_OFFSETOF(VM, calleeSaveRegistersBuffer);
    }
#endif // NUMBER_OF_CALLEE_SAVES_REGISTERS > 0

    std::unique_ptr<JITThunks> jitStubs;
    MacroAssemblerCodeRef getCTIStub(ThunkGenerator generator)
    {
        return jitStubs->ctiStub(this, generator);
    }
    NativeExecutable* getHostFunction(NativeFunction, Intrinsic);
    
    std::unique_ptr<RegisterAtOffsetList> allCalleeSaveRegisterOffsets;
    
    RegisterAtOffsetList* getAllCalleeSaveRegisterOffsets() { return allCalleeSaveRegisterOffsets.get(); }

#endif // ENABLE(JIT)
    std::unique_ptr<CommonSlowPaths::ArityCheckData> arityCheckData;
#if ENABLE(FTL_JIT)
    std::unique_ptr<FTL::Thunks> ftlThunks;
#endif
    NativeExecutable* getHostFunction(NativeFunction, NativeFunction constructor);

    static ptrdiff_t exceptionOffset()
    {
        return OBJECT_OFFSETOF(VM, m_exception);
    }

    static ptrdiff_t callFrameForThrowOffset()
    {
        return OBJECT_OFFSETOF(VM, callFrameForThrow);
    }

    static ptrdiff_t targetMachinePCForThrowOffset()
    {
        return OBJECT_OFFSETOF(VM, targetMachinePCForThrow);
    }

    void clearException() { m_exception = nullptr; }
    void clearLastException() { m_lastException = nullptr; }

    void setException(Exception* exception)
    {
        m_exception = exception;
        m_lastException = exception;
    }

    Exception* exception() const { return m_exception; }
    JSCell** addressOfException() { return reinterpret_cast<JSCell**>(&m_exception); }

    Exception* lastException() const { return m_lastException; }
    JSCell** addressOfLastException() { return reinterpret_cast<JSCell**>(&m_lastException); }

    JS_EXPORT_PRIVATE void throwException(ExecState*, Exception*);
    JS_EXPORT_PRIVATE JSValue throwException(ExecState*, JSValue);
    JS_EXPORT_PRIVATE JSObject* throwException(ExecState*, JSObject*);

    void setFailNextNewCodeBlock() { m_failNextNewCodeBlock = true; }
    bool getAndClearFailNextNewCodeBlock()
    {
        bool result = m_failNextNewCodeBlock;
        m_failNextNewCodeBlock = false;
        return result;
    }
    
    void* stackPointerAtVMEntry() const { return m_stackPointerAtVMEntry; }
    void setStackPointerAtVMEntry(void*);

    size_t reservedZoneSize() const { return m_reservedZoneSize; }
    size_t updateReservedZoneSize(size_t reservedZoneSize);

#if ENABLE(FTL_JIT)
    void updateFTLLargestStackSize(size_t);
    void** addressOfFTLStackLimit() { return &m_ftlStackLimit; }
#endif

#if !ENABLE(JIT)
    void* jsStackLimit() { return m_jsStackLimit; }
    void setJSStackLimit(void* limit) { m_jsStackLimit = limit; }
#endif
    void* stackLimit() { return m_stackLimit; }
    void** addressOfStackLimit() { return &m_stackLimit; }

    bool isSafeToRecurse(size_t neededStackInBytes = 0) const
    {
        ASSERT(wtfThreadData().stack().isGrowingDownward());
        int8_t* curr = reinterpret_cast<int8_t*>(&curr);
        int8_t* limit = reinterpret_cast<int8_t*>(m_stackLimit);
        return curr >= limit && static_cast<size_t>(curr - limit) >= neededStackInBytes;
    }

    void* lastStackTop() { return m_lastStackTop; }
    void setLastStackTop(void* lastStackTop) { m_lastStackTop = lastStackTop; }

    const ClassInfo* const jsArrayClassInfo;
    const ClassInfo* const jsFinalObjectClassInfo;

    JSValue hostCallReturnValue;
    unsigned varargsLength;
    ExecState* newCallFrameReturnValue;
    ExecState* callFrameForThrow;
    void* targetMachinePCForThrow;
    Instruction* targetInterpreterPCForThrow;
    uint32_t osrExitIndex;
    void* osrExitJumpDestination;
    Vector<ScratchBuffer*> scratchBuffers;
    size_t sizeOfLastScratchBuffer;

    ScratchBuffer* scratchBufferForSize(size_t size)
    {
        if (!size)
            return 0;

        if (size > sizeOfLastScratchBuffer) {
            // Protect against a N^2 memory usage pathology by ensuring
            // that at worst, we get a geometric series, meaning that the
            // total memory usage is somewhere around
            // max(scratch buffer size) * 4.
            sizeOfLastScratchBuffer = size * 2;

            ScratchBuffer* newBuffer = ScratchBuffer::create(sizeOfLastScratchBuffer);
            RELEASE_ASSERT(newBuffer);
            scratchBuffers.append(newBuffer);
        }

        ScratchBuffer* result = scratchBuffers.last();
        result->setActiveLength(0);
        return result;
    }

    void gatherConservativeRoots(ConservativeRoots&);

    VMEntryScope* entryScope;

    JSObject* stringRecursionCheckFirstObject { nullptr };
    HashSet<JSObject*> stringRecursionCheckVisitedObjects;

    LocalTimeOffsetCache localTimeOffsetCache;

    String cachedDateString;
    double cachedDateStringValue;

    std::unique_ptr<Profiler::Database> m_perBytecodeProfiler;
    RefPtr<TypedArrayController> m_typedArrayController;
    RegExpCache* m_regExpCache;
    BumpPointerAllocator m_regExpAllocator;

#if ENABLE(REGEXP_TRACING)
    typedef ListHashSet<RegExp*> RTTraceList;
    RTTraceList* m_rtTraceList;
#endif

    bool hasExclusiveThread() const { return m_apiLock->hasExclusiveThread(); }
    std::thread::id exclusiveThread() const { return m_apiLock->exclusiveThread(); }
    void setExclusiveThread(std::thread::id threadId) { m_apiLock->setExclusiveThread(threadId); }

    JS_EXPORT_PRIVATE void resetDateCache();

    JS_EXPORT_PRIVATE void startSampling();
    JS_EXPORT_PRIVATE void stopSampling();
    JS_EXPORT_PRIVATE void dumpSampleData(ExecState*);
    RegExpCache* regExpCache() { return m_regExpCache; }
#if ENABLE(REGEXP_TRACING)
    void addRegExpToTrace(RegExp*);
#endif
    JS_EXPORT_PRIVATE void dumpRegExpTrace();

    bool isCollectorBusy() { return heap.isBusy(); }
    JS_EXPORT_PRIVATE void releaseExecutableMemory();

#if ENABLE(GC_VALIDATION)
    bool isInitializingObject() const; 
    void setInitializingObjectClass(const ClassInfo*);
#endif

    unsigned m_newStringsSinceLastHashCons;

    static const unsigned s_minNumberOfNewStringsToHashCons = 100;

    bool haveEnoughNewStringsToHashCons() { return m_newStringsSinceLastHashCons > s_minNumberOfNewStringsToHashCons; }
    void resetNewStringsSinceLastHashCons() { m_newStringsSinceLastHashCons = 0; }

    bool currentThreadIsHoldingAPILock() const { return m_apiLock->currentThreadIsHoldingLock(); }

    JSLock& apiLock() { return *m_apiLock; }
    CodeCache* codeCache() { return m_codeCache.get(); }

    void prepareToDiscardCode();
        
    JS_EXPORT_PRIVATE void discardAllCode();

    void registerWatchpointForImpureProperty(const Identifier&, Watchpoint*);
    
    // FIXME: Use AtomicString once it got merged with Identifier.
    JS_EXPORT_PRIVATE void addImpureProperty(const String&);

    BuiltinExecutables* builtinExecutables() { return m_builtinExecutables.get(); }

    bool enableTypeProfiler();
    bool disableTypeProfiler();
    TypeProfilerLog* typeProfilerLog() { return m_typeProfilerLog.get(); }
    TypeProfiler* typeProfiler() { return m_typeProfiler.get(); }
    JS_EXPORT_PRIVATE void dumpTypeProfilerData();

    FunctionHasExecutedCache* functionHasExecutedCache() { return &m_functionHasExecutedCache; }

    ControlFlowProfiler* controlFlowProfiler() { return m_controlFlowProfiler.get(); }
    bool enableControlFlowProfiler();
    bool disableControlFlowProfiler();

    BytecodeIntrinsicRegistry& bytecodeIntrinsicRegistry() { return *m_bytecodeIntrinsicRegistry; }

private:
    friend class LLIntOffsetsExtractor;
    friend class ClearExceptionScope;
    friend class RecursiveAllocationScope;

    VM(VMType, HeapType);
    static VM*& sharedInstanceInternal();
    void createNativeThunk();

    void updateStackLimit();

#if ENABLE(ASSEMBLER)
    bool m_canUseAssembler;
#endif
#if ENABLE(JIT)
    bool m_canUseJIT;
#endif
#if ENABLE(YARR_JIT)
    bool m_canUseRegExpJIT;
#endif
#if ENABLE(GC_VALIDATION)
    const ClassInfo* m_initializingObjectClass;
#endif
    void* m_stackPointerAtVMEntry;
    size_t m_reservedZoneSize;
#if !ENABLE(JIT)
    struct {
        void* m_stackLimit;
        void* m_jsStackLimit;
    };
#else
    union {
        void* m_stackLimit;
        void* m_jsStackLimit;
    };
#if ENABLE(FTL_JIT)
    void* m_ftlStackLimit;
    size_t m_largestFTLStackSize;
#endif
#endif
    void* m_lastStackTop;
    Exception* m_exception { nullptr };
    Exception* m_lastException { nullptr };
    bool m_failNextNewCodeBlock { false };
    bool m_inDefineOwnProperty;
    std::unique_ptr<CodeCache> m_codeCache;
    std::unique_ptr<BuiltinExecutables> m_builtinExecutables;
    HashMap<String, RefPtr<WatchpointSet>> m_impurePropertyWatchpointSets;
    std::unique_ptr<TypeProfiler> m_typeProfiler;
    std::unique_ptr<TypeProfilerLog> m_typeProfilerLog;
    unsigned m_typeProfilerEnabledCount;
    FunctionHasExecutedCache m_functionHasExecutedCache;
    std::unique_ptr<ControlFlowProfiler> m_controlFlowProfiler;
    unsigned m_controlFlowProfilerEnabledCount;
    std::unique_ptr<BytecodeIntrinsicRegistry> m_bytecodeIntrinsicRegistry;
};

#if ENABLE(GC_VALIDATION)
inline bool VM::isInitializingObject() const
{
    return !!m_initializingObjectClass;
}

inline void VM::setInitializingObjectClass(const ClassInfo* initializingObjectClass)
{
    m_initializingObjectClass = initializingObjectClass;
}
#endif

inline Heap* WeakSet::heap() const
{
    return &m_vm->heap;
}

#if ENABLE(JIT)
extern "C" void sanitizeStackForVMImpl(VM*);
#endif

void sanitizeStackForVM(VM*);
void logSanitizeStack(VM*);

} // namespace JSC

#endif // VM_h
