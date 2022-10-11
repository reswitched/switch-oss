/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
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
#include "FTLLowerDFGToLLVM.h"

#if ENABLE(FTL_JIT)

#include "CodeBlockWithJITType.h"
#include "DFGAbstractInterpreterInlines.h"
#include "DFGInPlaceAbstractState.h"
#include "DFGOSRAvailabilityAnalysisPhase.h"
#include "DFGOSRExitFuzz.h"
#include "DirectArguments.h"
#include "FTLAbstractHeapRepository.h"
#include "FTLAvailableRecovery.h"
#include "FTLForOSREntryJITCode.h"
#include "FTLFormattedValue.h"
#include "FTLInlineCacheSize.h"
#include "FTLLoweredNodeValue.h"
#include "FTLOperations.h"
#include "FTLOutput.h"
#include "FTLThunks.h"
#include "FTLWeightedTarget.h"
#include "JSArrowFunction.h"
#include "JSCInlines.h"
#include "JSLexicalEnvironment.h"
#include "OperandsInlines.h"
#include "ScopedArguments.h"
#include "ScopedArgumentsTable.h"
#include "VirtualRegister.h"
#include <atomic>
#include <dlfcn.h>
#include <llvm/InitializeLLVM.h>
#include <unordered_set>
#include <wtf/ProcessID.h>

#if ENABLE(FTL_NATIVE_CALL_INLINING)
#include "BundlePath.h"
#endif

namespace JSC { namespace FTL {

using namespace DFG;

namespace {

std::atomic<int> compileCounter;

#if ASSERT_DISABLED
NO_RETURN_DUE_TO_CRASH static void ftlUnreachable()
{
    CRASH();
}
#else
NO_RETURN_DUE_TO_CRASH static void ftlUnreachable(
    CodeBlock* codeBlock, BlockIndex blockIndex, unsigned nodeIndex)
{
    dataLog("Crashing in thought-to-be-unreachable FTL-generated code for ", pointerDump(codeBlock), " at basic block #", blockIndex);
    if (nodeIndex != UINT_MAX)
        dataLog(", node @", nodeIndex);
    dataLog(".\n");
    CRASH();
}
#endif

// Using this instead of typeCheck() helps to reduce the load on LLVM, by creating
// significantly less dead code.
#define FTL_TYPE_CHECK(lowValue, highValue, typesPassedThrough, failCondition) do { \
        FormattedValue _ftc_lowValue = (lowValue);                      \
        Edge _ftc_highValue = (highValue);                              \
        SpeculatedType _ftc_typesPassedThrough = (typesPassedThrough);  \
        if (!m_interpreter.needsTypeCheck(_ftc_highValue, _ftc_typesPassedThrough)) \
            break;                                                      \
        typeCheck(_ftc_lowValue, _ftc_highValue, _ftc_typesPassedThrough, (failCondition)); \
    } while (false)

class LowerDFGToLLVM {
public:
    LowerDFGToLLVM(State& state)
        : m_graph(state.graph)
        , m_ftlState(state)
        , m_heaps(state.context)
        , m_out(state.context)
        , m_state(state.graph)
        , m_interpreter(state.graph, m_state)
        , m_stackmapIDs(0)
        , m_tbaaKind(mdKindID(state.context, "tbaa"))
        , m_tbaaStructKind(mdKindID(state.context, "tbaa.struct"))
    {
    }
    
    void lower()
    {
        CString name;
        if (verboseCompilationEnabled()) {
            name = toCString(
                "jsBody_", ++compileCounter, "_", codeBlock()->inferredName(),
                "_", codeBlock()->hash());
        } else
            name = "jsBody";
        
        m_graph.m_dominators.computeIfNecessary(m_graph);
        
        m_ftlState.module =
            moduleCreateWithNameInContext(name.data(), m_ftlState.context);
        
        m_ftlState.function = addFunction(
            m_ftlState.module, name.data(), functionType(m_out.int64));
        setFunctionCallingConv(m_ftlState.function, LLVMCCallConv);
        if (isX86() && Options::llvmDisallowAVX()) {
            // AVX makes V8/raytrace 80% slower. It makes Kraken/audio-oscillator 4.5x
            // slower. It should be disabled.
            addTargetDependentFunctionAttr(m_ftlState.function, "target-features", "-avx");
        }
        
        if (verboseCompilationEnabled())
            dataLog("Function ready, beginning lowering.\n");
        
        m_out.initialize(m_ftlState.module, m_ftlState.function, m_heaps);
        
        m_prologue = FTL_NEW_BLOCK(m_out, ("Prologue"));
        LBasicBlock stackOverflow = FTL_NEW_BLOCK(m_out, ("Stack overflow"));
        m_handleExceptions = FTL_NEW_BLOCK(m_out, ("Handle Exceptions"));
        
        LBasicBlock checkArguments = FTL_NEW_BLOCK(m_out, ("Check arguments"));

        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            m_highBlock = m_graph.block(blockIndex);
            if (!m_highBlock)
                continue;
            m_blocks.add(m_highBlock, FTL_NEW_BLOCK(m_out, ("Block ", *m_highBlock)));
        }
        
        m_out.appendTo(m_prologue, stackOverflow);
        createPhiVariables();

        auto preOrder = m_graph.blocksInPreOrder();

        int maxNumberOfArguments = -1;
        for (BasicBlock* block : preOrder) {
            for (unsigned nodeIndex = block->size(); nodeIndex--; ) {
                Node* node = block->at(nodeIndex);
                switch (node->op()) {
                case NativeCall:
                case NativeConstruct: {
                    int numArgs = node->numChildren();
                    if (numArgs > maxNumberOfArguments)
                        maxNumberOfArguments = numArgs;
                    break;
                }
                default:
                    break;
                }
            }
        }
        
        if (maxNumberOfArguments >= 0) {
            m_execState = m_out.alloca(arrayType(m_out.int64, JSStack::CallFrameHeaderSize + maxNumberOfArguments));
            m_execStorage = m_out.ptrToInt(m_execState, m_out.intPtr);        
        }

        LValue capturedAlloca = m_out.alloca(arrayType(m_out.int64, m_graph.m_nextMachineLocal));
        
        m_captured = m_out.add(
            m_out.ptrToInt(capturedAlloca, m_out.intPtr),
            m_out.constIntPtr(m_graph.m_nextMachineLocal * sizeof(Register)));
        
        m_ftlState.capturedStackmapID = m_stackmapIDs++;
        m_out.call(
            m_out.stackmapIntrinsic(), m_out.constInt64(m_ftlState.capturedStackmapID),
            m_out.int32Zero, capturedAlloca);
        
        // If we have any CallVarargs then we nee to have a spill slot for it.
        bool hasVarargs = false;
        for (BasicBlock* block : preOrder) {
            for (Node* node : *block) {
                switch (node->op()) {
                case CallVarargs:
                case CallForwardVarargs:
                case ConstructVarargs:
                case ConstructForwardVarargs:
                    hasVarargs = true;
                    break;
                default:
                    break;
                }
            }
        }
        if (hasVarargs) {
            LValue varargsSpillSlots = m_out.alloca(
                arrayType(m_out.int64, JSCallVarargs::numSpillSlotsNeeded()));
            m_ftlState.varargsSpillSlotsStackmapID = m_stackmapIDs++;
            m_out.call(
                m_out.stackmapIntrinsic(), m_out.constInt64(m_ftlState.varargsSpillSlotsStackmapID),
                m_out.int32Zero, varargsSpillSlots);
        }
        
        // We should not create any alloca's after this point, since they will cease to
        // be mem2reg candidates.
        
        m_callFrame = m_out.ptrToInt(
            m_out.call(m_out.frameAddressIntrinsic(), m_out.int32Zero), m_out.intPtr);
        m_tagTypeNumber = m_out.constInt64(TagTypeNumber);
        m_tagMask = m_out.constInt64(TagMask);
        
        m_out.storePtr(m_out.constIntPtr(codeBlock()), addressFor(JSStack::CodeBlock));
        
        m_out.branch(
            didOverflowStack(), rarely(stackOverflow), usually(checkArguments));
        
        m_out.appendTo(stackOverflow, m_handleExceptions);
        m_out.call(m_out.operation(operationThrowStackOverflowError), m_callFrame, m_out.constIntPtr(codeBlock()));
        m_ftlState.handleStackOverflowExceptionStackmapID = m_stackmapIDs++;
        m_out.call(
            m_out.stackmapIntrinsic(), m_out.constInt64(m_ftlState.handleStackOverflowExceptionStackmapID),
            m_out.constInt32(MacroAssembler::maxJumpReplacementSize()));
        m_out.unreachable();
        
        m_out.appendTo(m_handleExceptions, checkArguments);
        m_ftlState.handleExceptionStackmapID = m_stackmapIDs++;
        m_out.call(
            m_out.stackmapIntrinsic(), m_out.constInt64(m_ftlState.handleExceptionStackmapID),
            m_out.constInt32(MacroAssembler::maxJumpReplacementSize()));
        m_out.unreachable();
        
        m_out.appendTo(checkArguments, lowBlock(m_graph.block(0)));
        availabilityMap().clear();
        availabilityMap().m_locals = Operands<Availability>(codeBlock()->numParameters(), 0);
        for (unsigned i = codeBlock()->numParameters(); i--;) {
            availabilityMap().m_locals.argument(i) =
                Availability(FlushedAt(FlushedJSValue, virtualRegisterForArgument(i)));
        }
        m_codeOriginForExitTarget = CodeOrigin(0);
        m_codeOriginForExitProfile = CodeOrigin(0);
        m_node = nullptr;
        for (unsigned i = codeBlock()->numParameters(); i--;) {
            Node* node = m_graph.m_arguments[i];
            VirtualRegister operand = virtualRegisterForArgument(i);
            
            LValue jsValue = m_out.load64(addressFor(operand));
            
            if (node) {
                DFG_ASSERT(m_graph, node, operand == node->stackAccessData()->machineLocal);
                
                // This is a hack, but it's an effective one. It allows us to do CSE on the
                // primordial load of arguments. This assumes that the GetLocal that got put in
                // place of the original SetArgument doesn't have any effects before it. This
                // should hold true.
                m_loadedArgumentValues.add(node, jsValue);
            }
            
            switch (m_graph.m_argumentFormats[i]) {
            case FlushedInt32:
                speculate(BadType, jsValueValue(jsValue), node, isNotInt32(jsValue));
                break;
            case FlushedBoolean:
                speculate(BadType, jsValueValue(jsValue), node, isNotBoolean(jsValue));
                break;
            case FlushedCell:
                speculate(BadType, jsValueValue(jsValue), node, isNotCell(jsValue));
                break;
            case FlushedJSValue:
                break;
            default:
                DFG_CRASH(m_graph, node, "Bad flush format for argument");
                break;
            }
        }
        m_out.jump(lowBlock(m_graph.block(0)));
        
        for (BasicBlock* block : preOrder)
            compileBlock(block);
        
        if (Options::dumpLLVMIR())
            dumpModule(m_ftlState.module);
        
        if (verboseCompilationEnabled())
            m_ftlState.dumpState("after lowering");
        if (validationEnabled())
            verifyModule(m_ftlState.module);
    }

private:
    
    void createPhiVariables()
    {
        for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            for (unsigned nodeIndex = block->size(); nodeIndex--;) {
                Node* node = block->at(nodeIndex);
                if (node->op() != Phi)
                    continue;
                LType type;
                switch (node->flags() & NodeResultMask) {
                case NodeResultDouble:
                    type = m_out.doubleType;
                    break;
                case NodeResultInt32:
                    type = m_out.int32;
                    break;
                case NodeResultInt52:
                    type = m_out.int64;
                    break;
                case NodeResultBoolean:
                    type = m_out.boolean;
                    break;
                case NodeResultJS:
                    type = m_out.int64;
                    break;
                default:
                    DFG_CRASH(m_graph, node, "Bad Phi node result type");
                    break;
                }
                m_phis.add(node, buildAlloca(m_out.m_builder, type));
            }
        }
    }
    
    void compileBlock(BasicBlock* block)
    {
        if (!block)
            return;
        
        if (verboseCompilationEnabled())
            dataLog("Compiling block ", *block, "\n");
        
        m_highBlock = block;
        
        LBasicBlock lowBlock = m_blocks.get(m_highBlock);
        
        m_nextHighBlock = 0;
        for (BlockIndex nextBlockIndex = m_highBlock->index + 1; nextBlockIndex < m_graph.numBlocks(); ++nextBlockIndex) {
            m_nextHighBlock = m_graph.block(nextBlockIndex);
            if (m_nextHighBlock)
                break;
        }
        m_nextLowBlock = m_nextHighBlock ? m_blocks.get(m_nextHighBlock) : 0;
        
        // All of this effort to find the next block gives us the ability to keep the
        // generated IR in roughly program order. This ought not affect the performance
        // of the generated code (since we expect LLVM to reorder things) but it will
        // make IR dumps easier to read.
        m_out.appendTo(lowBlock, m_nextLowBlock);
        
        if (Options::ftlCrashes())
            m_out.trap();
        
        if (!m_highBlock->cfaHasVisited) {
            if (verboseCompilationEnabled())
                dataLog("Bailing because CFA didn't reach.\n");
            crash(m_highBlock->index, UINT_MAX);
            return;
        }
        
        m_availabilityCalculator.beginBlock(m_highBlock);
        
        m_state.reset();
        m_state.beginBasicBlock(m_highBlock);
        
        for (m_nodeIndex = 0; m_nodeIndex < m_highBlock->size(); ++m_nodeIndex) {
            if (!compileNode(m_nodeIndex))
                break;
        }
    }

    void safelyInvalidateAfterTermination()
    {
        if (verboseCompilationEnabled())
            dataLog("Bailing.\n");
        crash();

        // Invalidate dominated blocks. Under normal circumstances we would expect
        // them to be invalidated already. But you can have the CFA become more
        // precise over time because the structures of objects change on the main
        // thread. Failing to do this would result in weird crashes due to a value
        // being used but not defined. Race conditions FTW!
        for (BlockIndex blockIndex = m_graph.numBlocks(); blockIndex--;) {
            BasicBlock* target = m_graph.block(blockIndex);
            if (!target)
                continue;
            if (m_graph.m_dominators.dominates(m_highBlock, target)) {
                if (verboseCompilationEnabled())
                    dataLog("Block ", *target, " will bail also.\n");
                target->cfaHasVisited = false;
            }
        }
    }

    bool compileNode(unsigned nodeIndex)
    {
        if (!m_state.isValid()) {
            safelyInvalidateAfterTermination();
            return false;
        }
        
        m_node = m_highBlock->at(nodeIndex);
        m_codeOriginForExitProfile = m_node->origin.semantic;
        m_codeOriginForExitTarget = m_node->origin.forExit;
        
        if (verboseCompilationEnabled())
            dataLog("Lowering ", m_node, "\n");
        
        m_availableRecoveries.resize(0);
        
        m_interpreter.startExecuting();
        
        switch (m_node->op()) {
        case Upsilon:
            compileUpsilon();
            break;
        case Phi:
            compilePhi();
            break;
        case JSConstant:
            break;
        case DoubleConstant:
            compileDoubleConstant();
            break;
        case Int52Constant:
            compileInt52Constant();
            break;
        case DoubleRep:
            compileDoubleRep();
            break;
        case DoubleAsInt32:
            compileDoubleAsInt32();
            break;
        case ValueRep:
            compileValueRep();
            break;
        case Int52Rep:
            compileInt52Rep();
            break;
        case ValueToInt32:
            compileValueToInt32();
            break;
        case BooleanToNumber:
            compileBooleanToNumber();
            break;
        case ExtractOSREntryLocal:
            compileExtractOSREntryLocal();
            break;
        case GetStack:
            compileGetStack();
            break;
        case PutStack:
            compilePutStack();
            break;
        case Check:
            compileNoOp();
            break;
        case ToThis:
            compileToThis();
            break;
        case ValueAdd:
            compileValueAdd();
            break;
        case ArithAdd:
        case ArithSub:
            compileArithAddOrSub();
            break;
        case ArithClz32:
            compileArithClz32();
            break;
        case ArithMul:
            compileArithMul();
            break;
        case ArithDiv:
            compileArithDiv();
            break;
        case ArithMod:
            compileArithMod();
            break;
        case ArithMin:
        case ArithMax:
            compileArithMinOrMax();
            break;
        case ArithAbs:
            compileArithAbs();
            break;
        case ArithSin:
            compileArithSin();
            break;
        case ArithCos:
            compileArithCos();
            break;
        case ArithPow:
            compileArithPow();
            break;
        case ArithRound:
            compileArithRound();
            break;
        case ArithSqrt:
            compileArithSqrt();
            break;
        case ArithLog:
            compileArithLog();
            break;
        case ArithFRound:
            compileArithFRound();
            break;
        case ArithNegate:
            compileArithNegate();
            break;
        case BitAnd:
            compileBitAnd();
            break;
        case BitOr:
            compileBitOr();
            break;
        case BitXor:
            compileBitXor();
            break;
        case BitRShift:
            compileBitRShift();
            break;
        case BitLShift:
            compileBitLShift();
            break;
        case BitURShift:
            compileBitURShift();
            break;
        case UInt32ToNumber:
            compileUInt32ToNumber();
            break;
        case CheckStructure:
            compileCheckStructure();
            break;
        case CheckCell:
            compileCheckCell();
            break;
        case CheckNotEmpty:
            compileCheckNotEmpty();
            break;
        case CheckBadCell:
            compileCheckBadCell();
            break;
        case GetExecutable:
            compileGetExecutable();
            break;
        case ArrayifyToStructure:
            compileArrayifyToStructure();
            break;
        case PutStructure:
            compilePutStructure();
            break;
        case GetById:
            compileGetById();
            break;
        case In:
            compileIn();
            break;
        case PutById:
        case PutByIdDirect:
            compilePutById();
            break;
        case GetButterfly:
            compileGetButterfly();
            break;
        case ConstantStoragePointer:
            compileConstantStoragePointer();
            break;
        case GetIndexedPropertyStorage:
            compileGetIndexedPropertyStorage();
            break;
        case CheckArray:
            compileCheckArray();
            break;
        case GetArrayLength:
            compileGetArrayLength();
            break;
        case CheckInBounds:
            compileCheckInBounds();
            break;
        case GetByVal:
            compileGetByVal();
            break;
        case GetMyArgumentByVal:
            compileGetMyArgumentByVal();
            break;
        case PutByVal:
        case PutByValAlias:
        case PutByValDirect:
            compilePutByVal();
            break;
        case ArrayPush:
            compileArrayPush();
            break;
        case ArrayPop:
            compileArrayPop();
            break;
        case CreateActivation:
            compileCreateActivation();
            break;
        case NewFunction:
        case NewArrowFunction:
            compileNewFunction();
            break;
        case CreateDirectArguments:
            compileCreateDirectArguments();
            break;
        case CreateScopedArguments:
            compileCreateScopedArguments();
            break;
        case CreateClonedArguments:
            compileCreateClonedArguments();
            break;
        case NewObject:
            compileNewObject();
            break;
        case NewArray:
            compileNewArray();
            break;
        case NewArrayBuffer:
            compileNewArrayBuffer();
            break;
        case NewArrayWithSize:
            compileNewArrayWithSize();
            break;
        case GetTypedArrayByteOffset:
            compileGetTypedArrayByteOffset();
            break;
        case AllocatePropertyStorage:
            compileAllocatePropertyStorage();
            break;
        case ReallocatePropertyStorage:
            compileReallocatePropertyStorage();
            break;
        case ToString:
        case CallStringConstructor:
            compileToStringOrCallStringConstructor();
            break;
        case ToPrimitive:
            compileToPrimitive();
            break;
        case MakeRope:
            compileMakeRope();
            break;
        case StringCharAt:
            compileStringCharAt();
            break;
        case StringCharCodeAt:
            compileStringCharCodeAt();
            break;
        case GetByOffset:
        case GetGetterSetterByOffset:
            compileGetByOffset();
            break;
        case GetGetter:
            compileGetGetter();
            break;
        case GetSetter:
            compileGetSetter();
            break;
        case MultiGetByOffset:
            compileMultiGetByOffset();
            break;
        case PutByOffset:
            compilePutByOffset();
            break;
        case MultiPutByOffset:
            compileMultiPutByOffset();
            break;
        case GetGlobalVar:
            compileGetGlobalVar();
            break;
        case PutGlobalVar:
            compilePutGlobalVar();
            break;
        case NotifyWrite:
            compileNotifyWrite();
            break;
        case GetCallee:
            compileGetCallee();
            break;
        case GetArgumentCount:
            compileGetArgumentCount();
            break;
        case GetScope:
            compileGetScope();
            break;
        case LoadArrowFunctionThis:
            compileLoadArrowFunctionThis();
            break;
        case SkipScope:
            compileSkipScope();
            break;
        case GetClosureVar:
            compileGetClosureVar();
            break;
        case PutClosureVar:
            compilePutClosureVar();
            break;
        case GetFromArguments:
            compileGetFromArguments();
            break;
        case PutToArguments:
            compilePutToArguments();
            break;
        case CompareEq:
            compileCompareEq();
            break;
        case CompareEqConstant:
            compileCompareEqConstant();
            break;
        case CompareStrictEq:
            compileCompareStrictEq();
            break;
        case CompareLess:
            compileCompareLess();
            break;
        case CompareLessEq:
            compileCompareLessEq();
            break;
        case CompareGreater:
            compileCompareGreater();
            break;
        case CompareGreaterEq:
            compileCompareGreaterEq();
            break;
        case LogicalNot:
            compileLogicalNot();
            break;
        case Call:
        case Construct:
            compileCallOrConstruct();
            break;
        case CallVarargs:
        case CallForwardVarargs:
        case ConstructVarargs:
        case ConstructForwardVarargs:
            compileCallOrConstructVarargs();
            break;
        case LoadVarargs:
            compileLoadVarargs();
            break;
        case ForwardVarargs:
            compileForwardVarargs();
            break;
#if ENABLE(FTL_NATIVE_CALL_INLINING)
        case NativeCall:
        case NativeConstruct:
            compileNativeCallOrConstruct();
            break;
#endif
        case Jump:
            compileJump();
            break;
        case Branch:
            compileBranch();
            break;
        case Switch:
            compileSwitch();
            break;
        case Return:
            compileReturn();
            break;
        case ForceOSRExit:
            compileForceOSRExit();
            break;
        case Throw:
        case ThrowReferenceError:
            compileThrow();
            break;
        case InvalidationPoint:
            compileInvalidationPoint();
            break;
        case IsUndefined:
            compileIsUndefined();
            break;
        case IsBoolean:
            compileIsBoolean();
            break;
        case IsNumber:
            compileIsNumber();
            break;
        case IsString:
            compileIsString();
            break;
        case IsObject:
            compileIsObject();
            break;
        case IsObjectOrNull:
            compileIsObjectOrNull();
            break;
        case IsFunction:
            compileIsFunction();
            break;
        case TypeOf:
            compileTypeOf();
            break;
        case CheckHasInstance:
            compileCheckHasInstance();
            break;
        case InstanceOf:
            compileInstanceOf();
            break;
        case CountExecution:
            compileCountExecution();
            break;
        case StoreBarrier:
            compileStoreBarrier();
            break;
        case HasIndexedProperty:
            compileHasIndexedProperty();
            break;
        case HasGenericProperty:
            compileHasGenericProperty();
            break;
        case HasStructureProperty:
            compileHasStructureProperty();
            break;
        case GetDirectPname:
            compileGetDirectPname();
            break;
        case GetEnumerableLength:
            compileGetEnumerableLength();
            break;
        case GetPropertyEnumerator:
            compileGetPropertyEnumerator();
            break;
        case GetEnumeratorStructurePname:
            compileGetEnumeratorStructurePname();
            break;
        case GetEnumeratorGenericPname:
            compileGetEnumeratorGenericPname();
            break;
        case ToIndexString:
            compileToIndexString();
            break;
        case CheckStructureImmediate:
            compileCheckStructureImmediate();
            break;
        case MaterializeNewObject:
            compileMaterializeNewObject();
            break;
        case MaterializeCreateActivation:
            compileMaterializeCreateActivation();
            break;

        case PhantomLocal:
        case LoopHint:
        case MovHint:
        case ZombieHint:
        case PhantomNewObject:
        case PhantomNewFunction:
        case PhantomCreateActivation:
        case PhantomDirectArguments:
        case PhantomClonedArguments:
        case PutHint:
        case BottomValue:
        case KillStack:
            break;
        default:
            DFG_CRASH(m_graph, m_node, "Unrecognized node in FTL backend");
            break;
        }
        
        if (m_node->isTerminal())
            return false;
        
        if (!m_state.isValid()) {
            safelyInvalidateAfterTermination();
            return false;
        }

        m_availabilityCalculator.executeNode(m_node);
        m_interpreter.executeEffects(nodeIndex);
        
        return true;
    }

    void compileUpsilon()
    {
        LValue destination = m_phis.get(m_node->phi());
        
        switch (m_node->child1().useKind()) {
        case DoubleRepUse:
            m_out.set(lowDouble(m_node->child1()), destination);
            break;
        case Int32Use:
            m_out.set(lowInt32(m_node->child1()), destination);
            break;
        case Int52RepUse:
            m_out.set(lowInt52(m_node->child1()), destination);
            break;
        case BooleanUse:
            m_out.set(lowBoolean(m_node->child1()), destination);
            break;
        case CellUse:
            m_out.set(lowCell(m_node->child1()), destination);
            break;
        case UntypedUse:
            m_out.set(lowJSValue(m_node->child1()), destination);
            break;
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compilePhi()
    {
        LValue source = m_phis.get(m_node);
        
        switch (m_node->flags() & NodeResultMask) {
        case NodeResultDouble:
            setDouble(m_out.get(source));
            break;
        case NodeResultInt32:
            setInt32(m_out.get(source));
            break;
        case NodeResultInt52:
            setInt52(m_out.get(source));
            break;
        case NodeResultBoolean:
            setBoolean(m_out.get(source));
            break;
        case NodeResultJS:
            setJSValue(m_out.get(source));
            break;
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compileDoubleConstant()
    {
        setDouble(m_out.constDouble(m_node->asNumber()));
    }
    
    void compileInt52Constant()
    {
        int64_t value = m_node->asMachineInt();
        
        setInt52(m_out.constInt64(value << JSValue::int52ShiftAmount));
        setStrictInt52(m_out.constInt64(value));
    }

    void compileDoubleRep()
    {
        switch (m_node->child1().useKind()) {
        case RealNumberUse: {
            LValue value = lowJSValue(m_node->child1(), ManualOperandSpeculation);
            
            LValue doubleValue = unboxDouble(value);
            
            LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("DoubleRep RealNumberUse int case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("DoubleRep continuation"));
            
            ValueFromBlock fastResult = m_out.anchor(doubleValue);
            m_out.branch(
                m_out.doubleEqual(doubleValue, doubleValue),
                usually(continuation), rarely(intCase));
            
            LBasicBlock lastNext = m_out.appendTo(intCase, continuation);
            
            FTL_TYPE_CHECK(
                jsValueValue(value), m_node->child1(), SpecBytecodeRealNumber,
                isNotInt32(value, provenType(m_node->child1()) & ~SpecFullDouble));
            ValueFromBlock slowResult = m_out.anchor(m_out.intToDouble(unboxInt32(value)));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            
            setDouble(m_out.phi(m_out.doubleType, fastResult, slowResult));
            return;
        }
            
        case NotCellUse:
        case NumberUse: {
            bool shouldConvertNonNumber = m_node->child1().useKind() == NotCellUse;
            
            LValue value = lowJSValue(m_node->child1(), ManualOperandSpeculation);

            LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble unboxing int case"));
            LBasicBlock doubleTesting = FTL_NEW_BLOCK(m_out, ("jsValueToDouble testing double case"));
            LBasicBlock doubleCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble unboxing double case"));
            LBasicBlock nonDoubleCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble testing undefined case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("jsValueToDouble unboxing continuation"));
            
            m_out.branch(
                isNotInt32(value, provenType(m_node->child1())),
                unsure(doubleTesting), unsure(intCase));
            
            LBasicBlock lastNext = m_out.appendTo(intCase, doubleTesting);
            
            ValueFromBlock intToDouble = m_out.anchor(
                m_out.intToDouble(unboxInt32(value)));
            m_out.jump(continuation);
            
            m_out.appendTo(doubleTesting, doubleCase);
            LValue valueIsNumber = isNumber(value, provenType(m_node->child1()));
            m_out.branch(valueIsNumber, usually(doubleCase), rarely(nonDoubleCase));

            m_out.appendTo(doubleCase, nonDoubleCase);
            ValueFromBlock unboxedDouble = m_out.anchor(unboxDouble(value));
            m_out.jump(continuation);

            if (shouldConvertNonNumber) {
                LBasicBlock undefinedCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble converting undefined case"));
                LBasicBlock testNullCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble testing null case"));
                LBasicBlock nullCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble converting null case"));
                LBasicBlock testBooleanTrueCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble testing boolean true case"));
                LBasicBlock convertBooleanTrueCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble convert boolean true case"));
                LBasicBlock convertBooleanFalseCase = FTL_NEW_BLOCK(m_out, ("jsValueToDouble convert boolean false case"));

                m_out.appendTo(nonDoubleCase, undefinedCase);
                LValue valueIsUndefined = m_out.equal(value, m_out.constInt64(ValueUndefined));
                m_out.branch(valueIsUndefined, unsure(undefinedCase), unsure(testNullCase));

                m_out.appendTo(undefinedCase, testNullCase);
                ValueFromBlock convertedUndefined = m_out.anchor(m_out.constDouble(PNaN));
                m_out.jump(continuation);

                m_out.appendTo(testNullCase, nullCase);
                LValue valueIsNull = m_out.equal(value, m_out.constInt64(ValueNull));
                m_out.branch(valueIsNull, unsure(nullCase), unsure(testBooleanTrueCase));

                m_out.appendTo(nullCase, testBooleanTrueCase);
                ValueFromBlock convertedNull = m_out.anchor(m_out.constDouble(0));
                m_out.jump(continuation);

                m_out.appendTo(testBooleanTrueCase, convertBooleanTrueCase);
                LValue valueIsBooleanTrue = m_out.equal(value, m_out.constInt64(ValueTrue));
                m_out.branch(valueIsBooleanTrue, unsure(convertBooleanTrueCase), unsure(convertBooleanFalseCase));

                m_out.appendTo(convertBooleanTrueCase, convertBooleanFalseCase);
                ValueFromBlock convertedTrue = m_out.anchor(m_out.constDouble(1));
                m_out.jump(continuation);

                m_out.appendTo(convertBooleanFalseCase, continuation);

                LValue valueIsNotBooleanFalse = m_out.notEqual(value, m_out.constInt64(ValueFalse));
                FTL_TYPE_CHECK(jsValueValue(value), m_node->child1(), ~SpecCell, valueIsNotBooleanFalse);
                ValueFromBlock convertedFalse = m_out.anchor(m_out.constDouble(0));
                m_out.jump(continuation);

                m_out.appendTo(continuation, lastNext);
                setDouble(m_out.phi(m_out.doubleType, intToDouble, unboxedDouble, convertedUndefined, convertedNull, convertedTrue, convertedFalse));
                return;
            }
            m_out.appendTo(nonDoubleCase, continuation);
            FTL_TYPE_CHECK(jsValueValue(value), m_node->child1(), SpecBytecodeNumber, m_out.booleanTrue);
            m_out.unreachable();

            m_out.appendTo(continuation, lastNext);

            setDouble(m_out.phi(m_out.doubleType, intToDouble, unboxedDouble));
            return;
        }
            
        case Int52RepUse: {
            setDouble(strictInt52ToDouble(lowStrictInt52(m_node->child1())));
            return;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
        }
    }

    void compileDoubleAsInt32()
    {
        LValue integerValue = convertDoubleToInt32(lowDouble(m_node->child1()), shouldCheckNegativeZero(m_node->arithMode()));
        setInt32(integerValue);
    }

    void compileValueRep()
    {
        switch (m_node->child1().useKind()) {
        case DoubleRepUse: {
            LValue value = lowDouble(m_node->child1());
            
            if (m_interpreter.needsTypeCheck(m_node->child1(), ~SpecDoubleImpureNaN)) {
                value = m_out.select(
                    m_out.doubleEqual(value, value), value, m_out.constDouble(PNaN));
            }
            
            setJSValue(boxDouble(value));
            return;
        }
            
        case Int52RepUse: {
            setJSValue(strictInt52ToJSValue(lowStrictInt52(m_node->child1())));
            return;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
        }
    }
    
    void compileInt52Rep()
    {
        switch (m_node->child1().useKind()) {
        case Int32Use:
            setStrictInt52(m_out.signExt(lowInt32(m_node->child1()), m_out.int64));
            return;
            
        case MachineIntUse:
            setStrictInt52(
                jsValueToStrictInt52(
                    m_node->child1(), lowJSValue(m_node->child1(), ManualOperandSpeculation)));
            return;
            
        case DoubleRepMachineIntUse:
            setStrictInt52(
                doubleToStrictInt52(
                    m_node->child1(), lowDouble(m_node->child1())));
            return;
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    void compileValueToInt32()
    {
        switch (m_node->child1().useKind()) {
        case Int52RepUse:
            setInt32(m_out.castToInt32(lowStrictInt52(m_node->child1())));
            break;
            
        case DoubleRepUse:
            setInt32(doubleToInt32(lowDouble(m_node->child1())));
            break;
            
        case NumberUse:
        case NotCellUse: {
            LoweredNodeValue value = m_int32Values.get(m_node->child1().node());
            if (isValid(value)) {
                setInt32(value.value());
                break;
            }
            
            value = m_jsValueValues.get(m_node->child1().node());
            if (isValid(value)) {
                setInt32(numberOrNotCellToInt32(m_node->child1(), value.value()));
                break;
            }
            
            // We'll basically just get here for constants. But it's good to have this
            // catch-all since we often add new representations into the mix.
            setInt32(
                numberOrNotCellToInt32(
                    m_node->child1(),
                    lowJSValue(m_node->child1(), ManualOperandSpeculation)));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compileBooleanToNumber()
    {
        switch (m_node->child1().useKind()) {
        case BooleanUse: {
            setInt32(m_out.zeroExt(lowBoolean(m_node->child1()), m_out.int32));
            return;
        }
            
        case UntypedUse: {
            LValue value = lowJSValue(m_node->child1());
            
            if (!m_interpreter.needsTypeCheck(m_node->child1(), SpecBoolInt32 | SpecBoolean)) {
                setInt32(m_out.bitAnd(m_out.castToInt32(value), m_out.int32One));
                return;
            }
            
            LBasicBlock booleanCase = FTL_NEW_BLOCK(m_out, ("BooleanToNumber boolean case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("BooleanToNumber continuation"));
            
            ValueFromBlock notBooleanResult = m_out.anchor(value);
            m_out.branch(
                isBoolean(value, provenType(m_node->child1())),
                unsure(booleanCase), unsure(continuation));
            
            LBasicBlock lastNext = m_out.appendTo(booleanCase, continuation);
            ValueFromBlock booleanResult = m_out.anchor(m_out.bitOr(
                m_out.zeroExt(unboxBoolean(value), m_out.int64), m_tagTypeNumber));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, booleanResult, notBooleanResult));
            return;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return;
        }
    }

    void compileExtractOSREntryLocal()
    {
        EncodedJSValue* buffer = static_cast<EncodedJSValue*>(
            m_ftlState.jitCode->ftlForOSREntry()->entryBuffer()->dataBuffer());
        setJSValue(m_out.load64(m_out.absolute(buffer + m_node->unlinkedLocal().toLocal())));
    }
    
    void compileGetStack()
    {
        // GetLocals arise only for captured variables and arguments. For arguments, we might have
        // already loaded it.
        if (LValue value = m_loadedArgumentValues.get(m_node)) {
            setJSValue(value);
            return;
        }
        
        StackAccessData* data = m_node->stackAccessData();
        AbstractValue& value = m_state.variables().operand(data->local);
        
        DFG_ASSERT(m_graph, m_node, isConcrete(data->format));
        DFG_ASSERT(m_graph, m_node, data->format != FlushedDouble); // This just happens to not arise for GetStacks, right now. It would be trivial to support.
        
        if (isInt32Speculation(value.m_type))
            setInt32(m_out.load32(payloadFor(data->machineLocal)));
        else
            setJSValue(m_out.load64(addressFor(data->machineLocal)));
    }
    
    void compilePutStack()
    {
        StackAccessData* data = m_node->stackAccessData();
        switch (data->format) {
        case FlushedJSValue: {
            LValue value = lowJSValue(m_node->child1());
            m_out.store64(value, addressFor(data->machineLocal));
            break;
        }
            
        case FlushedDouble: {
            LValue value = lowDouble(m_node->child1());
            m_out.storeDouble(value, addressFor(data->machineLocal));
            break;
        }
            
        case FlushedInt32: {
            LValue value = lowInt32(m_node->child1());
            m_out.store32(value, payloadFor(data->machineLocal));
            break;
        }
            
        case FlushedInt52: {
            LValue value = lowInt52(m_node->child1());
            m_out.store64(value, addressFor(data->machineLocal));
            break;
        }
            
        case FlushedCell: {
            LValue value = lowCell(m_node->child1());
            m_out.store64(value, addressFor(data->machineLocal));
            break;
        }
            
        case FlushedBoolean: {
            speculateBoolean(m_node->child1());
            m_out.store64(
                lowJSValue(m_node->child1(), ManualOperandSpeculation),
                addressFor(data->machineLocal));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad flush format");
            break;
        }
    }
    
    void compileNoOp()
    {
        DFG_NODE_DO_TO_CHILDREN(m_graph, m_node, speculate);
    }
    
    void compileToThis()
    {
        LValue value = lowJSValue(m_node->child1());
        
        LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("ToThis is cell case"));
        LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("ToThis slow case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ToThis continuation"));
        
        m_out.branch(
            isCell(value, provenType(m_node->child1())), usually(isCellCase), rarely(slowCase));
        
        LBasicBlock lastNext = m_out.appendTo(isCellCase, slowCase);
        ValueFromBlock fastResult = m_out.anchor(value);
        m_out.branch(isType(value, FinalObjectType), usually(continuation), rarely(slowCase));
        
        m_out.appendTo(slowCase, continuation);
        J_JITOperation_EJ function;
        if (m_graph.isStrictModeFor(m_node->origin.semantic))
            function = operationToThisStrict;
        else
            function = operationToThis;
        ValueFromBlock slowResult = m_out.anchor(
            vmCall(m_out.operation(function), m_callFrame, value));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, fastResult, slowResult));
    }
    
    void compileValueAdd()
    {
        J_JITOperation_EJJ operation;
        if (!(provenType(m_node->child1()) & SpecFullNumber)
            && !(provenType(m_node->child2()) & SpecFullNumber))
            operation = operationValueAddNotNumber;
        else
            operation = operationValueAdd;
        setJSValue(vmCall(
            m_out.operation(operation), m_callFrame,
            lowJSValue(m_node->child1()), lowJSValue(m_node->child2())));
    }
    
    void compileArithAddOrSub()
    {
        bool isSub =  m_node->op() == ArithSub;
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());

            if (!shouldCheckOverflow(m_node->arithMode())) {
                setInt32(isSub ? m_out.sub(left, right) : m_out.add(left, right));
                break;
            }

            LValue result;
            if (!isSub) {
                result = m_out.addWithOverflow32(left, right);
                
                if (doesKill(m_node->child2())) {
                    addAvailableRecovery(
                        m_node->child2(), SubRecovery,
                        m_out.extractValue(result, 0), left, ValueFormatInt32);
                } else if (doesKill(m_node->child1())) {
                    addAvailableRecovery(
                        m_node->child1(), SubRecovery,
                        m_out.extractValue(result, 0), right, ValueFormatInt32);
                }
            } else {
                result = m_out.subWithOverflow32(left, right);
                
                if (doesKill(m_node->child2())) {
                    // result = left - right
                    // result - left = -right
                    // right = left - result
                    addAvailableRecovery(
                        m_node->child2(), SubRecovery,
                        left, m_out.extractValue(result, 0), ValueFormatInt32);
                } else if (doesKill(m_node->child1())) {
                    // result = left - right
                    // result + right = left
                    addAvailableRecovery(
                        m_node->child1(), AddRecovery,
                        m_out.extractValue(result, 0), right, ValueFormatInt32);
                }
            }

            speculate(Overflow, noValue(), 0, m_out.extractValue(result, 1));
            setInt32(m_out.extractValue(result, 0));
            break;
        }
            
        case Int52RepUse: {
            if (!abstractValue(m_node->child1()).couldBeType(SpecInt52)
                && !abstractValue(m_node->child2()).couldBeType(SpecInt52)) {
                Int52Kind kind;
                LValue left = lowWhicheverInt52(m_node->child1(), kind);
                LValue right = lowInt52(m_node->child2(), kind);
                setInt52(isSub ? m_out.sub(left, right) : m_out.add(left, right), kind);
                break;
            }
            
            LValue left = lowInt52(m_node->child1());
            LValue right = lowInt52(m_node->child2());

            LValue result;
            if (!isSub) {
                result = m_out.addWithOverflow64(left, right);
                
                if (doesKill(m_node->child2())) {
                    addAvailableRecovery(
                        m_node->child2(), SubRecovery,
                        m_out.extractValue(result, 0), left, ValueFormatInt52);
                } else if (doesKill(m_node->child1())) {
                    addAvailableRecovery(
                        m_node->child1(), SubRecovery,
                        m_out.extractValue(result, 0), right, ValueFormatInt52);
                }
            } else {
                result = m_out.subWithOverflow64(left, right);
                
                if (doesKill(m_node->child2())) {
                    // result = left - right
                    // result - left = -right
                    // right = left - result
                    addAvailableRecovery(
                        m_node->child2(), SubRecovery,
                        left, m_out.extractValue(result, 0), ValueFormatInt52);
                } else if (doesKill(m_node->child1())) {
                    // result = left - right
                    // result + right = left
                    addAvailableRecovery(
                        m_node->child1(), AddRecovery,
                        m_out.extractValue(result, 0), right, ValueFormatInt52);
                }
            }

            speculate(Int52Overflow, noValue(), 0, m_out.extractValue(result, 1));
            setInt52(m_out.extractValue(result, 0));
            break;
        }
            
        case DoubleRepUse: {
            LValue C1 = lowDouble(m_node->child1());
            LValue C2 = lowDouble(m_node->child2());

            setDouble(isSub ? m_out.doubleSub(C1, C2) : m_out.doubleAdd(C1, C2));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }

    void compileArithClz32()
    {
        LValue operand = lowInt32(m_node->child1());
        LValue isZeroUndef = m_out.booleanFalse;
        setInt32(m_out.ctlz32(operand, isZeroUndef));
    }
    
    void compileArithMul()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            
            LValue result;

            if (!shouldCheckOverflow(m_node->arithMode()))
                result = m_out.mul(left, right);
            else {
                LValue overflowResult = m_out.mulWithOverflow32(left, right);
                speculate(Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
                result = m_out.extractValue(overflowResult, 0);
            }
            
            if (shouldCheckNegativeZero(m_node->arithMode())) {
                LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("ArithMul slow case"));
                LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMul continuation"));
                
                m_out.branch(
                    m_out.notZero32(result), usually(continuation), rarely(slowCase));
                
                LBasicBlock lastNext = m_out.appendTo(slowCase, continuation);
                LValue cond = m_out.bitOr(m_out.lessThan(left, m_out.int32Zero), m_out.lessThan(right, m_out.int32Zero));
                speculate(NegativeZero, noValue(), 0, cond);
                m_out.jump(continuation);
                m_out.appendTo(continuation, lastNext);
            }
            
            setInt32(result);
            break;
        }
            
        case Int52RepUse: {
            Int52Kind kind;
            LValue left = lowWhicheverInt52(m_node->child1(), kind);
            LValue right = lowInt52(m_node->child2(), opposite(kind));

            LValue overflowResult = m_out.mulWithOverflow64(left, right);
            speculate(Int52Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
            LValue result = m_out.extractValue(overflowResult, 0);

            if (shouldCheckNegativeZero(m_node->arithMode())) {
                LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("ArithMul slow case"));
                LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMul continuation"));
                
                m_out.branch(
                    m_out.notZero64(result), usually(continuation), rarely(slowCase));
                
                LBasicBlock lastNext = m_out.appendTo(slowCase, continuation);
                LValue cond = m_out.bitOr(m_out.lessThan(left, m_out.int64Zero), m_out.lessThan(right, m_out.int64Zero));
                speculate(NegativeZero, noValue(), 0, cond);
                m_out.jump(continuation);
                m_out.appendTo(continuation, lastNext);
            }
            
            setInt52(result);
            break;
        }
            
        case DoubleRepUse: {
            setDouble(
                m_out.doubleMul(lowDouble(m_node->child1()), lowDouble(m_node->child2())));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }

    void compileArithDiv()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue numerator = lowInt32(m_node->child1());
            LValue denominator = lowInt32(m_node->child2());
            
            LBasicBlock unsafeDenominator = FTL_NEW_BLOCK(m_out, ("ArithDiv unsafe denominator"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithDiv continuation"));
            LBasicBlock done = FTL_NEW_BLOCK(m_out, ("ArithDiv done"));
            
            Vector<ValueFromBlock, 3> results;
            
            LValue adjustedDenominator = m_out.add(denominator, m_out.int32One);
            
            m_out.branch(
                m_out.above(adjustedDenominator, m_out.int32One),
                usually(continuation), rarely(unsafeDenominator));
            
            LBasicBlock lastNext = m_out.appendTo(unsafeDenominator, continuation);
            
            LValue neg2ToThe31 = m_out.constInt32(-2147483647-1);
            
            if (shouldCheckOverflow(m_node->arithMode())) {
                LValue cond = m_out.bitOr(m_out.isZero32(denominator), m_out.equal(numerator, neg2ToThe31));
                speculate(Overflow, noValue(), 0, cond);
                m_out.jump(continuation);
            } else {
                // This is the case where we convert the result to an int after we're done. So,
                // if the denominator is zero, then the result should be zero.
                // If the denominator is not zero (i.e. it's -1 because we're guarded by the
                // check above) and the numerator is -2^31 then the result should be -2^31.
                
                LBasicBlock divByZero = FTL_NEW_BLOCK(m_out, ("ArithDiv divide by zero"));
                LBasicBlock notDivByZero = FTL_NEW_BLOCK(m_out, ("ArithDiv not divide by zero"));
                LBasicBlock neg2ToThe31ByNeg1 = FTL_NEW_BLOCK(m_out, ("ArithDiv -2^31/-1"));
                
                m_out.branch(
                    m_out.isZero32(denominator), rarely(divByZero), usually(notDivByZero));
                
                m_out.appendTo(divByZero, notDivByZero);
                results.append(m_out.anchor(m_out.int32Zero));
                m_out.jump(done);
                
                m_out.appendTo(notDivByZero, neg2ToThe31ByNeg1);
                m_out.branch(
                    m_out.equal(numerator, neg2ToThe31),
                    rarely(neg2ToThe31ByNeg1), usually(continuation));
                
                m_out.appendTo(neg2ToThe31ByNeg1, continuation);
                results.append(m_out.anchor(neg2ToThe31));
                m_out.jump(done);
            }
            
            m_out.appendTo(continuation, done);
            
            if (shouldCheckNegativeZero(m_node->arithMode())) {
                LBasicBlock zeroNumerator = FTL_NEW_BLOCK(m_out, ("ArithDiv zero numerator"));
                LBasicBlock numeratorContinuation = FTL_NEW_BLOCK(m_out, ("ArithDiv numerator continuation"));
                
                m_out.branch(
                    m_out.isZero32(numerator),
                    rarely(zeroNumerator), usually(numeratorContinuation));
                
                LBasicBlock innerLastNext = m_out.appendTo(zeroNumerator, numeratorContinuation);
                
                speculate(
                    NegativeZero, noValue(), 0, m_out.lessThan(denominator, m_out.int32Zero));
                
                m_out.jump(numeratorContinuation);
                
                m_out.appendTo(numeratorContinuation, innerLastNext);
            }
            
            LValue result = m_out.div(numerator, denominator);
            
            if (shouldCheckOverflow(m_node->arithMode())) {
                speculate(
                    Overflow, noValue(), 0,
                    m_out.notEqual(m_out.mul(result, denominator), numerator));
            }
            
            results.append(m_out.anchor(result));
            m_out.jump(done);
            
            m_out.appendTo(done, lastNext);
            
            setInt32(m_out.phi(m_out.int32, results));
            break;
        }
            
        case DoubleRepUse: {
            setDouble(m_out.doubleDiv(
                lowDouble(m_node->child1()), lowDouble(m_node->child2())));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compileArithMod()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue numerator = lowInt32(m_node->child1());
            LValue denominator = lowInt32(m_node->child2());
            
            LBasicBlock unsafeDenominator = FTL_NEW_BLOCK(m_out, ("ArithMod unsafe denominator"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMod continuation"));
            LBasicBlock done = FTL_NEW_BLOCK(m_out, ("ArithMod done"));
            
            Vector<ValueFromBlock, 3> results;
            
            LValue adjustedDenominator = m_out.add(denominator, m_out.int32One);
            
            m_out.branch(
                m_out.above(adjustedDenominator, m_out.int32One),
                usually(continuation), rarely(unsafeDenominator));
            
            LBasicBlock lastNext = m_out.appendTo(unsafeDenominator, continuation);
            
            LValue neg2ToThe31 = m_out.constInt32(-2147483647-1);
            
            // FIXME: -2^31 / -1 will actually yield negative zero, so we could have a
            // separate case for that. But it probably doesn't matter so much.
            if (shouldCheckOverflow(m_node->arithMode())) {
                LValue cond = m_out.bitOr(m_out.isZero32(denominator), m_out.equal(numerator, neg2ToThe31));
                speculate(Overflow, noValue(), 0, cond);
                m_out.jump(continuation);
            } else {
                // This is the case where we convert the result to an int after we're done. So,
                // if the denominator is zero, then the result should be result should be zero.
                // If the denominator is not zero (i.e. it's -1 because we're guarded by the
                // check above) and the numerator is -2^31 then the result should be -2^31.
                
                LBasicBlock modByZero = FTL_NEW_BLOCK(m_out, ("ArithMod modulo by zero"));
                LBasicBlock notModByZero = FTL_NEW_BLOCK(m_out, ("ArithMod not modulo by zero"));
                LBasicBlock neg2ToThe31ByNeg1 = FTL_NEW_BLOCK(m_out, ("ArithMod -2^31/-1"));
                
                m_out.branch(
                    m_out.isZero32(denominator), rarely(modByZero), usually(notModByZero));
                
                m_out.appendTo(modByZero, notModByZero);
                results.append(m_out.anchor(m_out.int32Zero));
                m_out.jump(done);
                
                m_out.appendTo(notModByZero, neg2ToThe31ByNeg1);
                m_out.branch(
                    m_out.equal(numerator, neg2ToThe31),
                    rarely(neg2ToThe31ByNeg1), usually(continuation));
                
                m_out.appendTo(neg2ToThe31ByNeg1, continuation);
                results.append(m_out.anchor(m_out.int32Zero));
                m_out.jump(done);
            }
            
            m_out.appendTo(continuation, done);
            
            LValue remainder = m_out.rem(numerator, denominator);
            
            if (shouldCheckNegativeZero(m_node->arithMode())) {
                LBasicBlock negativeNumerator = FTL_NEW_BLOCK(m_out, ("ArithMod negative numerator"));
                LBasicBlock numeratorContinuation = FTL_NEW_BLOCK(m_out, ("ArithMod numerator continuation"));
                
                m_out.branch(
                    m_out.lessThan(numerator, m_out.int32Zero),
                    unsure(negativeNumerator), unsure(numeratorContinuation));
                
                LBasicBlock innerLastNext = m_out.appendTo(negativeNumerator, numeratorContinuation);
                
                speculate(NegativeZero, noValue(), 0, m_out.isZero32(remainder));
                
                m_out.jump(numeratorContinuation);
                
                m_out.appendTo(numeratorContinuation, innerLastNext);
            }
            
            results.append(m_out.anchor(remainder));
            m_out.jump(done);
            
            m_out.appendTo(done, lastNext);
            
            setInt32(m_out.phi(m_out.int32, results));
            break;
        }
            
        case DoubleRepUse: {
            setDouble(
                m_out.doubleRem(lowDouble(m_node->child1()), lowDouble(m_node->child2())));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }

    void compileArithMinOrMax()
    {
        switch (m_node->binaryUseKind()) {
        case Int32Use: {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            
            setInt32(
                m_out.select(
                    m_node->op() == ArithMin
                        ? m_out.lessThan(left, right)
                        : m_out.lessThan(right, left),
                    left, right));
            break;
        }
            
        case DoubleRepUse: {
            LValue left = lowDouble(m_node->child1());
            LValue right = lowDouble(m_node->child2());
            
            LBasicBlock notLessThan = FTL_NEW_BLOCK(m_out, ("ArithMin/ArithMax not less than"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithMin/ArithMax continuation"));
            
            Vector<ValueFromBlock, 2> results;
            
            results.append(m_out.anchor(left));
            m_out.branch(
                m_node->op() == ArithMin
                    ? m_out.doubleLessThan(left, right)
                    : m_out.doubleGreaterThan(left, right),
                unsure(continuation), unsure(notLessThan));
            
            LBasicBlock lastNext = m_out.appendTo(notLessThan, continuation);
            results.append(m_out.anchor(m_out.select(
                m_node->op() == ArithMin
                    ? m_out.doubleGreaterThanOrEqual(left, right)
                    : m_out.doubleLessThanOrEqual(left, right),
                right, m_out.constDouble(PNaN))));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setDouble(m_out.phi(m_out.doubleType, results));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compileArithAbs()
    {
        switch (m_node->child1().useKind()) {
        case Int32Use: {
            LValue value = lowInt32(m_node->child1());
            
            LValue mask = m_out.aShr(value, m_out.constInt32(31));
            LValue result = m_out.bitXor(mask, m_out.add(mask, value));
            
            speculate(Overflow, noValue(), 0, m_out.equal(result, m_out.constInt32(1 << 31)));
            
            setInt32(result);
            break;
        }
            
        case DoubleRepUse: {
            setDouble(m_out.doubleAbs(lowDouble(m_node->child1())));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }

    void compileArithSin() { setDouble(m_out.doubleSin(lowDouble(m_node->child1()))); }

    void compileArithCos() { setDouble(m_out.doubleCos(lowDouble(m_node->child1()))); }

    void compileArithPow()
    {
        // FIXME: investigate llvm.powi to better understand its performance characteristics.
        // It might be better to have the inline loop in DFG too.
        if (m_node->child2().useKind() == Int32Use)
            setDouble(m_out.doublePowi(lowDouble(m_node->child1()), lowInt32(m_node->child2())));
        else {
            LValue base = lowDouble(m_node->child1());
            LValue exponent = lowDouble(m_node->child2());

            LBasicBlock integerExponentIsSmallBlock = FTL_NEW_BLOCK(m_out, ("ArithPow test integer exponent is small."));
            LBasicBlock integerExponentPowBlock = FTL_NEW_BLOCK(m_out, ("ArithPow pow(double, (int)double)."));
            LBasicBlock doubleExponentPowBlockEntry = FTL_NEW_BLOCK(m_out, ("ArithPow pow(double, double)."));
            LBasicBlock nanExceptionExponentIsInfinity = FTL_NEW_BLOCK(m_out, ("ArithPow NaN Exception, check exponent is infinity."));
            LBasicBlock nanExceptionBaseIsOne = FTL_NEW_BLOCK(m_out, ("ArithPow NaN Exception, check base is one."));
            LBasicBlock powBlock = FTL_NEW_BLOCK(m_out, ("ArithPow regular pow"));
            LBasicBlock nanExceptionResultIsNaN = FTL_NEW_BLOCK(m_out, ("ArithPow NaN Exception, result is NaN."));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithPow continuation"));

            LValue integerExponent = m_out.fpToInt32(exponent);
            LValue integerExponentConvertedToDouble = m_out.intToDouble(integerExponent);
            LValue exponentIsInteger = m_out.doubleEqual(exponent, integerExponentConvertedToDouble);
            m_out.branch(exponentIsInteger, unsure(integerExponentIsSmallBlock), unsure(doubleExponentPowBlockEntry));

            LBasicBlock lastNext = m_out.appendTo(integerExponentIsSmallBlock, integerExponentPowBlock);
            LValue integerExponentBelow1000 = m_out.below(integerExponent, m_out.constInt32(1000));
            m_out.branch(integerExponentBelow1000, usually(integerExponentPowBlock), rarely(doubleExponentPowBlockEntry));

            m_out.appendTo(integerExponentPowBlock, doubleExponentPowBlockEntry);
            ValueFromBlock powDoubleIntResult = m_out.anchor(m_out.doublePowi(base, integerExponent));
            m_out.jump(continuation);

            // If y is NaN, the result is NaN.
            m_out.appendTo(doubleExponentPowBlockEntry, nanExceptionExponentIsInfinity);
            LValue exponentIsNaN;
            if (provenType(m_node->child2()) & SpecDoubleNaN)
                exponentIsNaN = m_out.doubleNotEqualOrUnordered(exponent, exponent);
            else
                exponentIsNaN = m_out.booleanFalse;
            m_out.branch(exponentIsNaN, rarely(nanExceptionResultIsNaN), usually(nanExceptionExponentIsInfinity));

            // If abs(x) is 1 and y is +infinity, the result is NaN.
            // If abs(x) is 1 and y is -infinity, the result is NaN.
            m_out.appendTo(nanExceptionExponentIsInfinity, nanExceptionBaseIsOne);
            LValue absoluteExponent = m_out.doubleAbs(exponent);
            LValue absoluteExponentIsInfinity = m_out.doubleEqual(absoluteExponent, m_out.constDouble(std::numeric_limits<double>::infinity()));
            m_out.branch(absoluteExponentIsInfinity, rarely(nanExceptionBaseIsOne), usually(powBlock));

            m_out.appendTo(nanExceptionBaseIsOne, powBlock);
            LValue absoluteBase = m_out.doubleAbs(base);
            LValue absoluteBaseIsOne = m_out.doubleEqual(absoluteBase, m_out.constDouble(1));
            m_out.branch(absoluteBaseIsOne, unsure(nanExceptionResultIsNaN), unsure(powBlock));

            m_out.appendTo(powBlock, nanExceptionResultIsNaN);
            ValueFromBlock powResult = m_out.anchor(m_out.doublePow(base, exponent));
            m_out.jump(continuation);

            m_out.appendTo(nanExceptionResultIsNaN, continuation);
            ValueFromBlock pureNan = m_out.anchor(m_out.constDouble(PNaN));
            m_out.jump(continuation);

            m_out.appendTo(continuation, lastNext);
            setDouble(m_out.phi(m_out.doubleType, powDoubleIntResult, powResult, pureNan));
        }
    }

    void compileArithRound()
    {
        LBasicBlock realPartIsMoreThanHalf = FTL_NEW_BLOCK(m_out, ("ArithRound should round down"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArithRound continuation"));

        LValue value = lowDouble(m_node->child1());
        LValue integerValue = m_out.ceil64(value);
        ValueFromBlock integerValueResult = m_out.anchor(integerValue);

        LValue realPart = m_out.doubleSub(integerValue, value);

        m_out.branch(m_out.doubleGreaterThanOrUnordered(realPart, m_out.constDouble(0.5)), unsure(realPartIsMoreThanHalf), unsure(continuation));

        LBasicBlock lastNext = m_out.appendTo(realPartIsMoreThanHalf, continuation);
        LValue integerValueRoundedDown = m_out.doubleSub(integerValue, m_out.constDouble(1));
        ValueFromBlock integerValueRoundedDownResult = m_out.anchor(integerValueRoundedDown);
        m_out.jump(continuation);
        m_out.appendTo(continuation, lastNext);

        LValue result = m_out.phi(m_out.doubleType, integerValueResult, integerValueRoundedDownResult);

        if (producesInteger(m_node->arithRoundingMode())) {
            LValue integerValue = convertDoubleToInt32(result, shouldCheckNegativeZero(m_node->arithRoundingMode()));
            setInt32(integerValue);
        } else
            setDouble(result);
    }

    void compileArithSqrt() { setDouble(m_out.doubleSqrt(lowDouble(m_node->child1()))); }

    void compileArithLog() { setDouble(m_out.doubleLog(lowDouble(m_node->child1()))); }
    
    void compileArithFRound()
    {
        LValue floatValue = m_out.fpCast(lowDouble(m_node->child1()), m_out.floatType);
        setDouble(m_out.fpCast(floatValue, m_out.doubleType));
    }
    
    void compileArithNegate()
    {
        switch (m_node->child1().useKind()) {
        case Int32Use: {
            LValue value = lowInt32(m_node->child1());
            
            LValue result;
            if (!shouldCheckOverflow(m_node->arithMode()))
                result = m_out.neg(value);
            else if (!shouldCheckNegativeZero(m_node->arithMode())) {
                // We don't have a negate-with-overflow intrinsic. Hopefully this
                // does the trick, though.
                LValue overflowResult = m_out.subWithOverflow32(m_out.int32Zero, value);
                speculate(Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
                result = m_out.extractValue(overflowResult, 0);
            } else {
                speculate(Overflow, noValue(), 0, m_out.testIsZero32(value, m_out.constInt32(0x7fffffff)));
                result = m_out.neg(value);
            }

            setInt32(result);
            break;
        }
            
        case Int52RepUse: {
            if (!abstractValue(m_node->child1()).couldBeType(SpecInt52)) {
                Int52Kind kind;
                LValue value = lowWhicheverInt52(m_node->child1(), kind);
                LValue result = m_out.neg(value);
                if (shouldCheckNegativeZero(m_node->arithMode()))
                    speculate(NegativeZero, noValue(), 0, m_out.isZero64(result));
                setInt52(result, kind);
                break;
            }
            
            LValue value = lowInt52(m_node->child1());
            LValue overflowResult = m_out.subWithOverflow64(m_out.int64Zero, value);
            speculate(Int52Overflow, noValue(), 0, m_out.extractValue(overflowResult, 1));
            LValue result = m_out.extractValue(overflowResult, 0);
            speculate(NegativeZero, noValue(), 0, m_out.isZero64(result));
            setInt52(result);
            break;
        }
            
        case DoubleRepUse: {
            setDouble(m_out.doubleNeg(lowDouble(m_node->child1())));
            break;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compileBitAnd()
    {
        setInt32(m_out.bitAnd(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileBitOr()
    {
        setInt32(m_out.bitOr(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileBitXor()
    {
        setInt32(m_out.bitXor(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileBitRShift()
    {
        setInt32(m_out.aShr(
            lowInt32(m_node->child1()),
            m_out.bitAnd(lowInt32(m_node->child2()), m_out.constInt32(31))));
    }
    
    void compileBitLShift()
    {
        setInt32(m_out.shl(
            lowInt32(m_node->child1()),
            m_out.bitAnd(lowInt32(m_node->child2()), m_out.constInt32(31))));
    }
    
    void compileBitURShift()
    {
        setInt32(m_out.lShr(
            lowInt32(m_node->child1()),
            m_out.bitAnd(lowInt32(m_node->child2()), m_out.constInt32(31))));
    }
    
    void compileUInt32ToNumber()
    {
        LValue value = lowInt32(m_node->child1());

        if (doesOverflow(m_node->arithMode())) {
            setDouble(m_out.unsignedToDouble(value));
            return;
        }
        
        speculate(Overflow, noValue(), 0, m_out.lessThan(value, m_out.int32Zero));
        setInt32(value);
    }
    
    void compileCheckStructure()
    {
        LValue cell = lowCell(m_node->child1());
        
        ExitKind exitKind;
        if (m_node->child1()->hasConstant())
            exitKind = BadConstantCache;
        else
            exitKind = BadCache;
        
        LValue structureID = m_out.load32(cell, m_heaps.JSCell_structureID);
        
        checkStructure(
            structureID, jsValueValue(cell), exitKind, m_node->structureSet(),
            [this] (Structure* structure) {
                return weakStructureID(structure);
            });
    }
    
    void compileCheckCell()
    {
        LValue cell = lowCell(m_node->child1());
        
        speculate(
            BadCell, jsValueValue(cell), m_node->child1().node(),
            m_out.notEqual(cell, weakPointer(m_node->cellOperand()->cell())));
    }
    
    void compileCheckBadCell()
    {
        terminate(BadCell);
    }

    void compileCheckNotEmpty()
    {
        speculate(TDZFailure, noValue(), nullptr, m_out.isZero64(lowJSValue(m_node->child1())));
    }

    void compileGetExecutable()
    {
        LValue cell = lowCell(m_node->child1());
        speculateFunction(m_node->child1(), cell);
        setJSValue(m_out.loadPtr(cell, m_heaps.JSFunction_executable));
    }
    
    void compileArrayifyToStructure()
    {
        LValue cell = lowCell(m_node->child1());
        LValue property = !!m_node->child2() ? lowInt32(m_node->child2()) : 0;
        
        LBasicBlock unexpectedStructure = FTL_NEW_BLOCK(m_out, ("ArrayifyToStructure unexpected structure"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArrayifyToStructure continuation"));
        
        LValue structureID = m_out.load32(cell, m_heaps.JSCell_structureID);
        
        m_out.branch(
            m_out.notEqual(structureID, weakStructureID(m_node->structure())),
            rarely(unexpectedStructure), usually(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(unexpectedStructure, continuation);
        
        if (property) {
            switch (m_node->arrayMode().type()) {
            case Array::Int32:
            case Array::Double:
            case Array::Contiguous:
                speculate(
                    Uncountable, noValue(), 0,
                    m_out.aboveOrEqual(property, m_out.constInt32(MIN_SPARSE_ARRAY_INDEX)));
                break;
            default:
                break;
            }
        }
        
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
            vmCall(m_out.operation(operationEnsureInt32), m_callFrame, cell);
            break;
        case Array::Double:
            vmCall(m_out.operation(operationEnsureDouble), m_callFrame, cell);
            break;
        case Array::Contiguous:
            vmCall(m_out.operation(operationEnsureContiguous), m_callFrame, cell);
            break;
        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage:
            vmCall(m_out.operation(operationEnsureArrayStorage), m_callFrame, cell);
            break;
        default:
            DFG_CRASH(m_graph, m_node, "Bad array type");
            break;
        }
        
        structureID = m_out.load32(cell, m_heaps.JSCell_structureID);
        speculate(
            BadIndexingType, jsValueValue(cell), 0,
            m_out.notEqual(structureID, weakStructureID(m_node->structure())));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void compilePutStructure()
    {
        m_ftlState.jitCode->common.notifyCompilingStructureTransition(m_graph.m_plan, codeBlock(), m_node);

        Structure* oldStructure = m_node->transition()->previous;
        Structure* newStructure = m_node->transition()->next;
        ASSERT_UNUSED(oldStructure, oldStructure->indexingType() == newStructure->indexingType());
        ASSERT(oldStructure->typeInfo().inlineTypeFlags() == newStructure->typeInfo().inlineTypeFlags());
        ASSERT(oldStructure->typeInfo().type() == newStructure->typeInfo().type());

        LValue cell = lowCell(m_node->child1()); 
        m_out.store32(
            weakStructureID(newStructure),
            cell, m_heaps.JSCell_structureID);
    }
    
    void compileGetById()
    {
        // Pretty much the only reason why we don't also support GetByIdFlush is because:
        // https://bugs.webkit.org/show_bug.cgi?id=125711
        
        switch (m_node->child1().useKind()) {
        case CellUse: {
            setJSValue(getById(lowCell(m_node->child1())));
            return;
        }
            
        case UntypedUse: {
            // This is pretty weird, since we duplicate the slow path both here and in the
            // code generated by the IC. We should investigate making this less bad.
            // https://bugs.webkit.org/show_bug.cgi?id=127830
            LValue value = lowJSValue(m_node->child1());
            
            LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("GetById untyped cell case"));
            LBasicBlock notCellCase = FTL_NEW_BLOCK(m_out, ("GetById untyped not cell case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetById untyped continuation"));
            
            m_out.branch(
                isCell(value, provenType(m_node->child1())), unsure(cellCase), unsure(notCellCase));
            
            LBasicBlock lastNext = m_out.appendTo(cellCase, notCellCase);
            ValueFromBlock cellResult = m_out.anchor(getById(value));
            m_out.jump(continuation);
            
            m_out.appendTo(notCellCase, continuation);
            ValueFromBlock notCellResult = m_out.anchor(vmCall(
                m_out.operation(operationGetByIdGeneric),
                m_callFrame, value,
                m_out.constIntPtr(m_graph.identifiers()[m_node->identifierNumber()])));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, cellResult, notCellResult));
            return;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            return;
        }
    }
    
    void compilePutById()
    {
        // See above; CellUse is easier so we do only that for now.
        ASSERT(m_node->child1().useKind() == CellUse);
        
        LValue base = lowCell(m_node->child1());
        LValue value = lowJSValue(m_node->child2());
        auto uid = m_graph.identifiers()[m_node->identifierNumber()];

        // Arguments: id, bytes, target, numArgs, args...
        unsigned stackmapID = m_stackmapIDs++;

        if (verboseCompilationEnabled())
            dataLog("    Emitting PutById patchpoint with stackmap #", stackmapID, "\n");
        
        LValue call = m_out.call(
            m_out.patchpointVoidIntrinsic(),
            m_out.constInt64(stackmapID), m_out.constInt32(sizeOfPutById()),
            constNull(m_out.ref8), m_out.constInt32(2), base, value);
        setInstructionCallingConvention(call, LLVMAnyRegCallConv);
        
        m_ftlState.putByIds.append(PutByIdDescriptor(
            stackmapID, m_node->origin.semantic, uid,
            m_graph.executableFor(m_node->origin.semantic)->ecmaMode(),
            m_node->op() == PutByIdDirect ? Direct : NotDirect));
    }
    
    void compileGetButterfly()
    {
        setStorage(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.JSObject_butterfly));
    }
    
    void compileConstantStoragePointer()
    {
        setStorage(m_out.constIntPtr(m_node->storagePointer()));
    }
    
    void compileGetIndexedPropertyStorage()
    {
        LValue cell = lowCell(m_node->child1());
        
        if (m_node->arrayMode().type() == Array::String) {
            LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("GetIndexedPropertyStorage String slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetIndexedPropertyStorage String continuation"));
            
            ValueFromBlock fastResult = m_out.anchor(
                m_out.loadPtr(cell, m_heaps.JSString_value));
            
            m_out.branch(
                m_out.notNull(fastResult.value()), usually(continuation), rarely(slowPath));
            
            LBasicBlock lastNext = m_out.appendTo(slowPath, continuation);
            
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(m_out.operation(operationResolveRope), m_callFrame, cell));
            
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            
            setStorage(m_out.loadPtr(m_out.phi(m_out.intPtr, fastResult, slowResult), m_heaps.StringImpl_data));
            return;
        }
        
        setStorage(m_out.loadPtr(cell, m_heaps.JSArrayBufferView_vector));
    }
    
    void compileCheckArray()
    {
        Edge edge = m_node->child1();
        LValue cell = lowCell(edge);
        
        if (m_node->arrayMode().alreadyChecked(m_graph, m_node, abstractValue(edge)))
            return;
        
        speculate(
            BadIndexingType, jsValueValue(cell), 0,
            m_out.bitNot(isArrayType(cell, m_node->arrayMode())));
    }

    void compileGetTypedArrayByteOffset()
    {
        LValue basePtr = lowCell(m_node->child1());    

        LBasicBlock simpleCase = FTL_NEW_BLOCK(m_out, ("wasteless typed array"));
        LBasicBlock wastefulCase = FTL_NEW_BLOCK(m_out, ("wasteful typed array"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("continuation branch"));
        
        LValue mode = m_out.load32(basePtr, m_heaps.JSArrayBufferView_mode);
        m_out.branch(
            m_out.notEqual(mode, m_out.constInt32(WastefulTypedArray)),
            unsure(simpleCase), unsure(wastefulCase));

        // begin simple case        
        LBasicBlock lastNext = m_out.appendTo(simpleCase, wastefulCase);

        ValueFromBlock simpleOut = m_out.anchor(m_out.constIntPtr(0));

        m_out.jump(continuation);

        // begin wasteful case
        m_out.appendTo(wastefulCase, continuation);

        LValue vectorPtr = m_out.loadPtr(basePtr, m_heaps.JSArrayBufferView_vector);
        LValue butterflyPtr = m_out.loadPtr(basePtr, m_heaps.JSObject_butterfly);
        LValue arrayBufferPtr = m_out.loadPtr(butterflyPtr, m_heaps.Butterfly_arrayBuffer);
        LValue dataPtr = m_out.loadPtr(arrayBufferPtr, m_heaps.ArrayBuffer_data);

        ValueFromBlock wastefulOut = m_out.anchor(m_out.sub(vectorPtr, dataPtr));

        m_out.jump(continuation);
        m_out.appendTo(continuation, lastNext);

        // output
        setInt32(m_out.castToInt32(m_out.phi(m_out.intPtr, simpleOut, wastefulOut)));
    }
    
    void compileGetArrayLength()
    {
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            setInt32(m_out.load32NonNegative(lowStorage(m_node->child2()), m_heaps.Butterfly_publicLength));
            return;
        }
            
        case Array::String: {
            LValue string = lowCell(m_node->child1());
            setInt32(m_out.load32NonNegative(string, m_heaps.JSString_length));
            return;
        }
            
        case Array::DirectArguments: {
            LValue arguments = lowCell(m_node->child1());
            speculate(
                ExoticObjectMode, noValue(), nullptr,
                m_out.notNull(m_out.loadPtr(arguments, m_heaps.DirectArguments_overrides)));
            setInt32(m_out.load32NonNegative(arguments, m_heaps.DirectArguments_length));
            return;
        }
            
        case Array::ScopedArguments: {
            LValue arguments = lowCell(m_node->child1());
            speculate(
                ExoticObjectMode, noValue(), nullptr,
                m_out.notZero8(m_out.load8(arguments, m_heaps.ScopedArguments_overrodeThings)));
            setInt32(m_out.load32NonNegative(arguments, m_heaps.ScopedArguments_totalLength));
            return;
        }
            
        default:
            if (isTypedView(m_node->arrayMode().typedArrayType())) {
                setInt32(
                    m_out.load32NonNegative(lowCell(m_node->child1()), m_heaps.JSArrayBufferView_length));
                return;
            }
            
            DFG_CRASH(m_graph, m_node, "Bad array type");
            return;
        }
    }
    
    void compileCheckInBounds()
    {
        speculate(
            OutOfBounds, noValue(), 0,
            m_out.aboveOrEqual(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
    }
    
    void compileGetByVal()
    {
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            IndexedAbstractHeap& heap = m_node->arrayMode().type() == Array::Int32 ?
                m_heaps.indexedInt32Properties : m_heaps.indexedContiguousProperties;
            
            if (m_node->arrayMode().isInBounds()) {
                LValue result = m_out.load64(baseIndex(heap, storage, index, m_node->child2()));
                LValue isHole = m_out.isZero64(result);
                if (m_node->arrayMode().isSaneChain()) {
                    DFG_ASSERT(
                        m_graph, m_node, m_node->arrayMode().type() == Array::Contiguous);
                    result = m_out.select(
                        isHole, m_out.constInt64(JSValue::encode(jsUndefined())), result);
                } else
                    speculate(LoadFromHole, noValue(), 0, isHole);
                setJSValue(result);
                return;
            }
            
            LValue base = lowCell(m_node->child1());
            
            LBasicBlock fastCase = FTL_NEW_BLOCK(m_out, ("GetByVal int/contiguous fast case"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("GetByVal int/contiguous slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal int/contiguous continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(
                    index, m_out.load32NonNegative(storage, m_heaps.Butterfly_publicLength)),
                rarely(slowCase), usually(fastCase));
            
            LBasicBlock lastNext = m_out.appendTo(fastCase, slowCase);
            
            ValueFromBlock fastResult = m_out.anchor(
                m_out.load64(baseIndex(heap, storage, index, m_node->child2())));
            m_out.branch(
                m_out.isZero64(fastResult.value()), rarely(slowCase), usually(continuation));
            
            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(m_out.operation(operationGetByValArrayInt), m_callFrame, base, index));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, fastResult, slowResult));
            return;
        }
            
        case Array::Double: {
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            IndexedAbstractHeap& heap = m_heaps.indexedDoubleProperties;
            
            if (m_node->arrayMode().isInBounds()) {
                LValue result = m_out.loadDouble(
                    baseIndex(heap, storage, index, m_node->child2()));
                
                if (!m_node->arrayMode().isSaneChain()) {
                    speculate(
                        LoadFromHole, noValue(), 0,
                        m_out.doubleNotEqualOrUnordered(result, result));
                }
                setDouble(result);
                break;
            }
            
            LValue base = lowCell(m_node->child1());
            
            LBasicBlock inBounds = FTL_NEW_BLOCK(m_out, ("GetByVal double in bounds"));
            LBasicBlock boxPath = FTL_NEW_BLOCK(m_out, ("GetByVal double boxing"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("GetByVal double slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal double continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(
                    index, m_out.load32NonNegative(storage, m_heaps.Butterfly_publicLength)),
                rarely(slowCase), usually(inBounds));
            
            LBasicBlock lastNext = m_out.appendTo(inBounds, boxPath);
            LValue doubleValue = m_out.loadDouble(
                baseIndex(heap, storage, index, m_node->child2()));
            m_out.branch(
                m_out.doubleNotEqualOrUnordered(doubleValue, doubleValue),
                rarely(slowCase), usually(boxPath));
            
            m_out.appendTo(boxPath, slowCase);
            ValueFromBlock fastResult = m_out.anchor(boxDouble(doubleValue));
            m_out.jump(continuation);
            
            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(m_out.operation(operationGetByValArrayInt), m_callFrame, base, index));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, fastResult, slowResult));
            return;
        }
            
        case Array::DirectArguments: {
            LValue base = lowCell(m_node->child1());
            LValue index = lowInt32(m_node->child2());
            
            speculate(
                ExoticObjectMode, noValue(), nullptr,
                m_out.notNull(m_out.loadPtr(base, m_heaps.DirectArguments_overrides)));
            speculate(
                ExoticObjectMode, noValue(), nullptr,
                m_out.aboveOrEqual(
                    index,
                    m_out.load32NonNegative(base, m_heaps.DirectArguments_length)));

            TypedPointer address = m_out.baseIndex(
                m_heaps.DirectArguments_storage, base, m_out.zeroExtPtr(index));
            setJSValue(m_out.load64(address));
            return;
        }
            
        case Array::ScopedArguments: {
            LValue base = lowCell(m_node->child1());
            LValue index = lowInt32(m_node->child2());
            
            speculate(
                ExoticObjectMode, noValue(), nullptr,
                m_out.aboveOrEqual(
                    index,
                    m_out.load32NonNegative(base, m_heaps.ScopedArguments_totalLength)));
            
            LValue table = m_out.loadPtr(base, m_heaps.ScopedArguments_table);
            LValue namedLength = m_out.load32(table, m_heaps.ScopedArgumentsTable_length);
            
            LBasicBlock namedCase = FTL_NEW_BLOCK(m_out, ("GetByVal ScopedArguments named case"));
            LBasicBlock overflowCase = FTL_NEW_BLOCK(m_out, ("GetByVal ScopedArguments overflow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal ScopedArguments continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(index, namedLength), unsure(overflowCase), unsure(namedCase));
            
            LBasicBlock lastNext = m_out.appendTo(namedCase, overflowCase);
            
            LValue scope = m_out.loadPtr(base, m_heaps.ScopedArguments_scope);
            LValue arguments = m_out.loadPtr(table, m_heaps.ScopedArgumentsTable_arguments);
            
            TypedPointer address = m_out.baseIndex(
                m_heaps.scopedArgumentsTableArguments, arguments, m_out.zeroExtPtr(index));
            LValue scopeOffset = m_out.load32(address);
            
            speculate(
                ExoticObjectMode, noValue(), nullptr,
                m_out.equal(scopeOffset, m_out.constInt32(ScopeOffset::invalidOffset)));
            
            address = m_out.baseIndex(
                m_heaps.JSEnvironmentRecord_variables, scope, m_out.zeroExtPtr(scopeOffset));
            ValueFromBlock namedResult = m_out.anchor(m_out.load64(address));
            m_out.jump(continuation);
            
            m_out.appendTo(overflowCase, continuation);
            
            address = m_out.baseIndex(
                m_heaps.ScopedArguments_overflowStorage, base,
                m_out.zeroExtPtr(m_out.sub(index, namedLength)));
            LValue overflowValue = m_out.load64(address);
            speculate(ExoticObjectMode, noValue(), nullptr, m_out.isZero64(overflowValue));
            ValueFromBlock overflowResult = m_out.anchor(overflowValue);
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, namedResult, overflowResult));
            return;
        }
            
        case Array::Generic: {
            setJSValue(vmCall(
                m_out.operation(operationGetByVal), m_callFrame,
                lowJSValue(m_node->child1()), lowJSValue(m_node->child2())));
            return;
        }
            
        case Array::String: {
            compileStringCharAt();
            return;
        }
            
        default: {
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            TypedArrayType type = m_node->arrayMode().typedArrayType();
            
            if (isTypedView(type)) {
                TypedPointer pointer = TypedPointer(
                    m_heaps.typedArrayProperties,
                    m_out.add(
                        storage,
                        m_out.shl(
                            m_out.zeroExtPtr(index),
                            m_out.constIntPtr(logElementSize(type)))));
                
                if (isInt(type)) {
                    LValue result;
                    switch (elementSize(type)) {
                    case 1:
                        result = m_out.load8(pointer);
                        break;
                    case 2:
                        result = m_out.load16(pointer);
                        break;
                    case 4:
                        result = m_out.load32(pointer);
                        break;
                    default:
                        DFG_CRASH(m_graph, m_node, "Bad element size");
                    }
                    
                    if (elementSize(type) < 4) {
                        if (isSigned(type))
                            result = m_out.signExt(result, m_out.int32);
                        else
                            result = m_out.zeroExt(result, m_out.int32);
                        setInt32(result);
                        return;
                    }
                    
                    if (isSigned(type)) {
                        setInt32(result);
                        return;
                    }
                    
                    if (m_node->shouldSpeculateInt32()) {
                        speculate(
                            Overflow, noValue(), 0, m_out.lessThan(result, m_out.int32Zero));
                        setInt32(result);
                        return;
                    }
                    
                    if (m_node->shouldSpeculateMachineInt()) {
                        setStrictInt52(m_out.zeroExt(result, m_out.int64));
                        return;
                    }
                    
                    setDouble(m_out.unsignedToFP(result, m_out.doubleType));
                    return;
                }
            
                ASSERT(isFloat(type));
                
                LValue result;
                switch (type) {
                case TypeFloat32:
                    result = m_out.fpCast(m_out.loadFloat(pointer), m_out.doubleType);
                    break;
                case TypeFloat64:
                    result = m_out.loadDouble(pointer);
                    break;
                default:
                    DFG_CRASH(m_graph, m_node, "Bad typed array type");
                }
                
                setDouble(result);
                return;
            }
            
            DFG_CRASH(m_graph, m_node, "Bad array type");
            return;
        } }
    }
    
    void compileGetMyArgumentByVal()
    {
        InlineCallFrame* inlineCallFrame = m_node->child1()->origin.semantic.inlineCallFrame;
        
        LValue index = lowInt32(m_node->child2());
        
        LValue limit;
        if (inlineCallFrame && !inlineCallFrame->isVarargs())
            limit = m_out.constInt32(inlineCallFrame->arguments.size() - 1);
        else {
            VirtualRegister argumentCountRegister;
            if (!inlineCallFrame)
                argumentCountRegister = VirtualRegister(JSStack::ArgumentCount);
            else
                argumentCountRegister = inlineCallFrame->argumentCountRegister;
            limit = m_out.sub(m_out.load32(payloadFor(argumentCountRegister)), m_out.int32One);
        }
        
        speculate(ExoticObjectMode, noValue(), 0, m_out.aboveOrEqual(index, limit));
        
        TypedPointer base;
        if (inlineCallFrame) {
            if (inlineCallFrame->arguments.size() <= 1) {
                // We should have already exited due to the bounds check, above. Just tell the
                // compiler that anything dominated by this instruction is not reachable, so
                // that we don't waste time generating such code. This will also plant some
                // kind of crashing instruction so that if by some fluke the bounds check didn't
                // work, we'll crash in an easy-to-see way.
                didAlreadyTerminate();
                return;
            }
            base = addressFor(inlineCallFrame->arguments[1].virtualRegister());
        } else
            base = addressFor(virtualRegisterForArgument(1));
        
        LValue pointer = m_out.baseIndex(
            base.value(), m_out.zeroExt(index, m_out.intPtr), ScaleEight);
        setJSValue(m_out.load64(TypedPointer(m_heaps.variables.atAnyIndex(), pointer)));
    }
    
    void compilePutByVal()
    {
        Edge child1 = m_graph.varArgChild(m_node, 0);
        Edge child2 = m_graph.varArgChild(m_node, 1);
        Edge child3 = m_graph.varArgChild(m_node, 2);
        Edge child4 = m_graph.varArgChild(m_node, 3);
        Edge child5 = m_graph.varArgChild(m_node, 4);
        
        switch (m_node->arrayMode().type()) {
        case Array::Generic: {
            V_JITOperation_EJJJ operation;
            if (m_node->op() == PutByValDirect) {
                if (m_graph.isStrictModeFor(m_node->origin.semantic))
                    operation = operationPutByValDirectStrict;
                else
                    operation = operationPutByValDirectNonStrict;
            } else {
                if (m_graph.isStrictModeFor(m_node->origin.semantic))
                    operation = operationPutByValStrict;
                else
                    operation = operationPutByValNonStrict;
            }
                
            vmCall(
                m_out.operation(operation), m_callFrame,
                lowJSValue(child1), lowJSValue(child2), lowJSValue(child3));
            return;
        }
            
        default:
            break;
        }

        LValue base = lowCell(child1);
        LValue index = lowInt32(child2);
        LValue storage = lowStorage(child4);
        
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal continuation"));
            LBasicBlock outerLastNext = m_out.appendTo(m_out.m_block, continuation);
            
            switch (m_node->arrayMode().type()) {
            case Array::Int32:
            case Array::Contiguous: {
                LValue value = lowJSValue(child3, ManualOperandSpeculation);
                
                if (m_node->arrayMode().type() == Array::Int32)
                    FTL_TYPE_CHECK(jsValueValue(value), child3, SpecInt32, isNotInt32(value));
                
                TypedPointer elementPointer = m_out.baseIndex(
                    m_node->arrayMode().type() == Array::Int32 ?
                    m_heaps.indexedInt32Properties : m_heaps.indexedContiguousProperties,
                    storage, m_out.zeroExtPtr(index), provenValue(child2));
                
                if (m_node->op() == PutByValAlias) {
                    m_out.store64(value, elementPointer);
                    break;
                }
                
                contiguousPutByValOutOfBounds(
                    codeBlock()->isStrictMode()
                    ? operationPutByValBeyondArrayBoundsStrict
                    : operationPutByValBeyondArrayBoundsNonStrict,
                    base, storage, index, value, continuation);
                
                m_out.store64(value, elementPointer);
                break;
            }
                
            case Array::Double: {
                LValue value = lowDouble(child3);
                
                FTL_TYPE_CHECK(
                    doubleValue(value), child3, SpecDoubleReal,
                    m_out.doubleNotEqualOrUnordered(value, value));
                
                TypedPointer elementPointer = m_out.baseIndex(
                    m_heaps.indexedDoubleProperties, storage, m_out.zeroExtPtr(index),
                    provenValue(child2));
                
                if (m_node->op() == PutByValAlias) {
                    m_out.storeDouble(value, elementPointer);
                    break;
                }
                
                contiguousPutByValOutOfBounds(
                    codeBlock()->isStrictMode()
                    ? operationPutDoubleByValBeyondArrayBoundsStrict
                    : operationPutDoubleByValBeyondArrayBoundsNonStrict,
                    base, storage, index, value, continuation);
                
                m_out.storeDouble(value, elementPointer);
                break;
            }
                
            default:
                DFG_CRASH(m_graph, m_node, "Bad array type");
            }

            m_out.jump(continuation);
            m_out.appendTo(continuation, outerLastNext);
            return;
        }
            
        default:
            TypedArrayType type = m_node->arrayMode().typedArrayType();
            
            if (isTypedView(type)) {
                TypedPointer pointer = TypedPointer(
                    m_heaps.typedArrayProperties,
                    m_out.add(
                        storage,
                        m_out.shl(
                            m_out.zeroExt(index, m_out.intPtr),
                            m_out.constIntPtr(logElementSize(type)))));
                
                LType refType;
                LValue valueToStore;
                
                if (isInt(type)) {
                    LValue intValue;
                    switch (child3.useKind()) {
                    case Int52RepUse:
                    case Int32Use: {
                        if (child3.useKind() == Int32Use)
                            intValue = lowInt32(child3);
                        else
                            intValue = m_out.castToInt32(lowStrictInt52(child3));

                        if (isClamped(type)) {
                            ASSERT(elementSize(type) == 1);
                            
                            LBasicBlock atLeastZero = FTL_NEW_BLOCK(m_out, ("PutByVal int clamp atLeastZero"));
                            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal int clamp continuation"));
                            
                            Vector<ValueFromBlock, 2> intValues;
                            intValues.append(m_out.anchor(m_out.int32Zero));
                            m_out.branch(
                                m_out.lessThan(intValue, m_out.int32Zero),
                                unsure(continuation), unsure(atLeastZero));
                            
                            LBasicBlock lastNext = m_out.appendTo(atLeastZero, continuation);
                            
                            intValues.append(m_out.anchor(m_out.select(
                                m_out.greaterThan(intValue, m_out.constInt32(255)),
                                m_out.constInt32(255),
                                intValue)));
                            m_out.jump(continuation);
                            
                            m_out.appendTo(continuation, lastNext);
                            intValue = m_out.phi(m_out.int32, intValues);
                        }
                        break;
                    }
                        
                    case DoubleRepUse: {
                        LValue doubleValue = lowDouble(child3);
                        
                        if (isClamped(type)) {
                            ASSERT(elementSize(type) == 1);
                            
                            LBasicBlock atLeastZero = FTL_NEW_BLOCK(m_out, ("PutByVal double clamp atLeastZero"));
                            LBasicBlock withinRange = FTL_NEW_BLOCK(m_out, ("PutByVal double clamp withinRange"));
                            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal double clamp continuation"));
                            
                            Vector<ValueFromBlock, 3> intValues;
                            intValues.append(m_out.anchor(m_out.int32Zero));
                            m_out.branch(
                                m_out.doubleLessThanOrUnordered(doubleValue, m_out.doubleZero),
                                unsure(continuation), unsure(atLeastZero));
                            
                            LBasicBlock lastNext = m_out.appendTo(atLeastZero, withinRange);
                            intValues.append(m_out.anchor(m_out.constInt32(255)));
                            m_out.branch(
                                m_out.doubleGreaterThan(doubleValue, m_out.constDouble(255)),
                                unsure(continuation), unsure(withinRange));
                            
                            m_out.appendTo(withinRange, continuation);
                            intValues.append(m_out.anchor(m_out.fpToInt32(doubleValue)));
                            m_out.jump(continuation);
                            
                            m_out.appendTo(continuation, lastNext);
                            intValue = m_out.phi(m_out.int32, intValues);
                        } else
                            intValue = doubleToInt32(doubleValue);
                        break;
                    }
                        
                    default:
                        DFG_CRASH(m_graph, m_node, "Bad use kind");
                    }
                    
                    switch (elementSize(type)) {
                    case 1:
                        valueToStore = m_out.intCast(intValue, m_out.int8);
                        refType = m_out.ref8;
                        break;
                    case 2:
                        valueToStore = m_out.intCast(intValue, m_out.int16);
                        refType = m_out.ref16;
                        break;
                    case 4:
                        valueToStore = intValue;
                        refType = m_out.ref32;
                        break;
                    default:
                        DFG_CRASH(m_graph, m_node, "Bad element size");
                    }
                } else /* !isInt(type) */ {
                    LValue value = lowDouble(child3);
                    switch (type) {
                    case TypeFloat32:
                        valueToStore = m_out.fpCast(value, m_out.floatType);
                        refType = m_out.refFloat;
                        break;
                    case TypeFloat64:
                        valueToStore = value;
                        refType = m_out.refDouble;
                        break;
                    default:
                        DFG_CRASH(m_graph, m_node, "Bad typed array type");
                    }
                }
                
                if (m_node->arrayMode().isInBounds() || m_node->op() == PutByValAlias)
                    m_out.store(valueToStore, pointer, refType);
                else {
                    LBasicBlock isInBounds = FTL_NEW_BLOCK(m_out, ("PutByVal typed array in bounds case"));
                    LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("PutByVal typed array continuation"));
                    
                    m_out.branch(
                        m_out.aboveOrEqual(index, lowInt32(child5)),
                        unsure(continuation), unsure(isInBounds));
                    
                    LBasicBlock lastNext = m_out.appendTo(isInBounds, continuation);
                    m_out.store(valueToStore, pointer, refType);
                    m_out.jump(continuation);
                    
                    m_out.appendTo(continuation, lastNext);
                }
                
                return;
            }
            
            DFG_CRASH(m_graph, m_node, "Bad array type");
            break;
        }
    }
    
    void compileArrayPush()
    {
        LValue base = lowCell(m_node->child1());
        LValue storage = lowStorage(m_node->child3());
        
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous:
        case Array::Double: {
            LValue value;
            LType refType;
            
            if (m_node->arrayMode().type() != Array::Double) {
                value = lowJSValue(m_node->child2(), ManualOperandSpeculation);
                if (m_node->arrayMode().type() == Array::Int32) {
                    FTL_TYPE_CHECK(
                        jsValueValue(value), m_node->child2(), SpecInt32, isNotInt32(value));
                }
                refType = m_out.ref64;
            } else {
                value = lowDouble(m_node->child2());
                FTL_TYPE_CHECK(
                    doubleValue(value), m_node->child2(), SpecDoubleReal,
                    m_out.doubleNotEqualOrUnordered(value, value));
                refType = m_out.refDouble;
            }
            
            IndexedAbstractHeap& heap = m_heaps.forArrayType(m_node->arrayMode().type());

            LValue prevLength = m_out.load32(storage, m_heaps.Butterfly_publicLength);
            
            LBasicBlock fastPath = FTL_NEW_BLOCK(m_out, ("ArrayPush fast path"));
            LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("ArrayPush slow path"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArrayPush continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(
                    prevLength, m_out.load32(storage, m_heaps.Butterfly_vectorLength)),
                rarely(slowPath), usually(fastPath));
            
            LBasicBlock lastNext = m_out.appendTo(fastPath, slowPath);
            m_out.store(
                value, m_out.baseIndex(heap, storage, m_out.zeroExtPtr(prevLength)), refType);
            LValue newLength = m_out.add(prevLength, m_out.int32One);
            m_out.store32(newLength, storage, m_heaps.Butterfly_publicLength);
            
            ValueFromBlock fastResult = m_out.anchor(boxInt32(newLength));
            m_out.jump(continuation);
            
            m_out.appendTo(slowPath, continuation);
            LValue operation;
            if (m_node->arrayMode().type() != Array::Double)
                operation = m_out.operation(operationArrayPush);
            else
                operation = m_out.operation(operationArrayPushDouble);
            ValueFromBlock slowResult = m_out.anchor(
                vmCall(operation, m_callFrame, value, base));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, fastResult, slowResult));
            return;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad array type");
            return;
        }
    }
    
    void compileArrayPop()
    {
        LValue base = lowCell(m_node->child1());
        LValue storage = lowStorage(m_node->child2());
        
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            IndexedAbstractHeap& heap = m_heaps.forArrayType(m_node->arrayMode().type());
            
            LBasicBlock fastCase = FTL_NEW_BLOCK(m_out, ("ArrayPop fast case"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("ArrayPop slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ArrayPop continuation"));
            
            LValue prevLength = m_out.load32(storage, m_heaps.Butterfly_publicLength);
            
            Vector<ValueFromBlock, 3> results;
            results.append(m_out.anchor(m_out.constInt64(JSValue::encode(jsUndefined()))));
            m_out.branch(
                m_out.isZero32(prevLength), rarely(continuation), usually(fastCase));
            
            LBasicBlock lastNext = m_out.appendTo(fastCase, slowCase);
            LValue newLength = m_out.sub(prevLength, m_out.int32One);
            m_out.store32(newLength, storage, m_heaps.Butterfly_publicLength);
            TypedPointer pointer = m_out.baseIndex(heap, storage, m_out.zeroExtPtr(newLength));
            if (m_node->arrayMode().type() != Array::Double) {
                LValue result = m_out.load64(pointer);
                m_out.store64(m_out.int64Zero, pointer);
                results.append(m_out.anchor(result));
                m_out.branch(
                    m_out.notZero64(result), usually(continuation), rarely(slowCase));
            } else {
                LValue result = m_out.loadDouble(pointer);
                m_out.store64(m_out.constInt64(bitwise_cast<int64_t>(PNaN)), pointer);
                results.append(m_out.anchor(boxDouble(result)));
                m_out.branch(
                    m_out.doubleEqual(result, result),
                    usually(continuation), rarely(slowCase));
            }
            
            m_out.appendTo(slowCase, continuation);
            results.append(m_out.anchor(vmCall(
                m_out.operation(operationArrayPopAndRecoverLength), m_callFrame, base)));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, results));
            return;
        }

        default:
            DFG_CRASH(m_graph, m_node, "Bad array type");
            return;
        }
    }
    
    void compileCreateActivation()
    {
        LValue scope = lowCell(m_node->child1());
        SymbolTable* table = m_node->castOperand<SymbolTable*>();
        Structure* structure = m_graph.globalObjectFor(m_node->origin.semantic)->activationStructure();
        
        if (table->singletonScope()->isStillValid()) {
            LValue callResult = vmCall(
                m_out.operation(operationCreateActivationDirect), m_callFrame, weakPointer(structure),
                scope, weakPointer(table));
            setJSValue(callResult);
            return;
        }
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("CreateActivation slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CreateActivation continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        LValue fastObject = allocateObject<JSLexicalEnvironment>(
            JSLexicalEnvironment::allocationSize(table), structure, m_out.intPtrZero, slowPath);
        
        // We don't need memory barriers since we just fast-created the activation, so the
        // activation must be young.
        m_out.storePtr(scope, fastObject, m_heaps.JSScope_next);
        m_out.storePtr(weakPointer(table), fastObject, m_heaps.JSSymbolTableObject_symbolTable);
        
        for (unsigned i = 0; i < table->scopeSize(); ++i) {
            m_out.store64(
                m_out.constInt64(JSValue::encode(jsUndefined())),
                fastObject, m_heaps.JSEnvironmentRecord_variables[i]);
        }
        
        ValueFromBlock fastResult = m_out.anchor(fastObject);
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        LValue callResult = vmCall(
            m_out.operation(operationCreateActivationDirect), m_callFrame, weakPointer(structure),
            scope, weakPointer(table));
        ValueFromBlock slowResult = m_out.anchor(callResult);
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.intPtr, fastResult, slowResult));
    }
    
    void compileNewFunction()
    {
        ASSERT(m_node->op() == NewFunction || m_node->op() == NewArrowFunction);
        
        bool isArrowFunction = m_node->op() == NewArrowFunction;
        
        LValue scope = lowCell(m_node->child1());
        LValue thisValue = isArrowFunction ? lowCell(m_node->child2()) : nullptr;
        
        FunctionExecutable* executable = m_node->castOperand<FunctionExecutable*>();
        if (executable->singletonFunction()->isStillValid()) {
            LValue callResult = isArrowFunction
                ? vmCall(m_out.operation(operationNewArrowFunction), m_callFrame, scope, weakPointer(executable), thisValue)
                : vmCall(m_out.operation(operationNewFunction), m_callFrame, scope, weakPointer(executable));
            setJSValue(callResult);
            return;
        }
        
        Structure* structure = isArrowFunction
            ? m_graph.globalObjectFor(m_node->origin.semantic)->arrowFunctionStructure()
            : m_graph.globalObjectFor(m_node->origin.semantic)->functionStructure();
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("NewFunction slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("NewFunction continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        LValue fastObject = isArrowFunction
            ? allocateObject<JSArrowFunction>(structure, m_out.intPtrZero, slowPath)
            : allocateObject<JSFunction>(structure, m_out.intPtrZero, slowPath);
        
        
        // We don't need memory barriers since we just fast-created the function, so it
        // must be young.
        m_out.storePtr(scope, fastObject, isArrowFunction ? m_heaps.JSArrowFunction_scope : m_heaps.JSFunction_scope);
        m_out.storePtr(weakPointer(executable), fastObject, isArrowFunction ?  m_heaps.JSArrowFunction_executable : m_heaps.JSFunction_executable);
        
        if (isArrowFunction)
            m_out.storePtr(thisValue, fastObject, m_heaps.JSArrowFunction_this);
        
        m_out.storePtr(m_out.intPtrZero, fastObject, isArrowFunction ?  m_heaps.JSArrowFunction_rareData : m_heaps.JSFunction_rareData);
        
        ValueFromBlock fastResult = m_out.anchor(fastObject);
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        LValue callResult = isArrowFunction
            ? vmCall(m_out.operation(operationNewArrowFunctionWithInvalidatedReallocationWatchpoint), m_callFrame, scope, weakPointer(executable), thisValue)
            : vmCall(m_out.operation(operationNewFunctionWithInvalidatedReallocationWatchpoint), m_callFrame, scope, weakPointer(executable));
        
        ValueFromBlock slowResult = m_out.anchor(callResult);
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.intPtr, fastResult, slowResult));
    }
    
    void compileCreateDirectArguments()
    {
        // FIXME: A more effective way of dealing with the argument count and callee is to have
        // them be explicit arguments to this node.
        // https://bugs.webkit.org/show_bug.cgi?id=142207
        
        Structure* structure =
            m_graph.globalObjectFor(m_node->origin.semantic)->directArgumentsStructure();
        
        unsigned minCapacity = m_graph.baselineCodeBlockFor(m_node->origin.semantic)->numParameters() - 1;
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("CreateDirectArguments slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CreateDirectArguments continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        ArgumentsLength length = getArgumentsLength();
        
        LValue fastObject;
        if (length.isKnown) {
            fastObject = allocateObject<DirectArguments>(
                DirectArguments::allocationSize(std::max(length.known, minCapacity)), structure,
                m_out.intPtrZero, slowPath);
        } else {
            LValue size = m_out.add(
                m_out.shl(length.value, m_out.constInt32(3)),
                m_out.constInt32(DirectArguments::storageOffset()));
            
            size = m_out.select(
                m_out.aboveOrEqual(length.value, m_out.constInt32(minCapacity)),
                size, m_out.constInt32(DirectArguments::allocationSize(minCapacity)));
            
            fastObject = allocateVariableSizedObject<DirectArguments>(
                size, structure, m_out.intPtrZero, slowPath);
        }
        
        m_out.store32(length.value, fastObject, m_heaps.DirectArguments_length);
        m_out.store32(m_out.constInt32(minCapacity), fastObject, m_heaps.DirectArguments_minCapacity);
        m_out.storePtr(m_out.intPtrZero, fastObject, m_heaps.DirectArguments_overrides);
        
        ValueFromBlock fastResult = m_out.anchor(fastObject);
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        LValue callResult = vmCall(
            m_out.operation(operationCreateDirectArguments), m_callFrame, weakPointer(structure),
            length.value, m_out.constInt32(minCapacity));
        ValueFromBlock slowResult = m_out.anchor(callResult);
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        LValue result = m_out.phi(m_out.intPtr, fastResult, slowResult);

        m_out.storePtr(getCurrentCallee(), result, m_heaps.DirectArguments_callee);
        
        if (length.isKnown) {
            VirtualRegister start = AssemblyHelpers::argumentsStart(m_node->origin.semantic);
            for (unsigned i = 0; i < std::max(length.known, minCapacity); ++i) {
                m_out.store64(
                    m_out.load64(addressFor(start + i)),
                    result, m_heaps.DirectArguments_storage[i]);
            }
        } else {
            LValue stackBase = getArgumentsStart();
            
            LBasicBlock loop = FTL_NEW_BLOCK(m_out, ("CreateDirectArguments loop body"));
            LBasicBlock end = FTL_NEW_BLOCK(m_out, ("CreateDirectArguments loop end"));
            
            ValueFromBlock originalLength;
            if (minCapacity) {
                LValue capacity = m_out.select(
                    m_out.aboveOrEqual(length.value, m_out.constInt32(minCapacity)),
                    length.value,
                    m_out.constInt32(minCapacity));
                originalLength = m_out.anchor(m_out.zeroExtPtr(capacity));
                m_out.jump(loop);
            } else {
                originalLength = m_out.anchor(m_out.zeroExtPtr(length.value));
                m_out.branch(m_out.isNull(originalLength.value()), unsure(end), unsure(loop));
            }
            
            lastNext = m_out.appendTo(loop, end);
            LValue previousIndex = m_out.phi(m_out.intPtr, originalLength);
            LValue index = m_out.sub(previousIndex, m_out.intPtrOne);
            m_out.store64(
                m_out.load64(m_out.baseIndex(m_heaps.variables, stackBase, index)),
                m_out.baseIndex(m_heaps.DirectArguments_storage, result, index));
            ValueFromBlock nextIndex = m_out.anchor(index);
            addIncoming(previousIndex, nextIndex);
            m_out.branch(m_out.isNull(index), unsure(end), unsure(loop));
            
            m_out.appendTo(end, lastNext);
        }
        
        setJSValue(result);
    }
    
    void compileCreateScopedArguments()
    {
        LValue scope = lowCell(m_node->child1());
        
        LValue result = vmCall(
            m_out.operation(operationCreateScopedArguments), m_callFrame,
            weakPointer(
                m_graph.globalObjectFor(m_node->origin.semantic)->scopedArgumentsStructure()),
            getArgumentsStart(), getArgumentsLength().value, getCurrentCallee(), scope);
        
        setJSValue(result);
    }
    
    void compileCreateClonedArguments()
    {
        LValue result = vmCall(
            m_out.operation(operationCreateClonedArguments), m_callFrame,
            weakPointer(
                m_graph.globalObjectFor(m_node->origin.semantic)->outOfBandArgumentsStructure()),
            getArgumentsStart(), getArgumentsLength().value, getCurrentCallee());
        
        setJSValue(result);
    }
    
    void compileNewObject()
    {
        setJSValue(allocateObject(m_node->structure()));
    }
    
    void compileNewArray()
    {
        // First speculate appropriately on all of the children. Do this unconditionally up here
        // because some of the slow paths may otherwise forget to do it. It's sort of arguable
        // that doing the speculations up here might be unprofitable for RA - so we can consider
        // sinking this to below the allocation fast path if we find that this has a lot of
        // register pressure.
        for (unsigned operandIndex = 0; operandIndex < m_node->numChildren(); ++operandIndex)
            speculate(m_graph.varArgChild(m_node, operandIndex));
        
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
        Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(
            m_node->indexingType());

        if (!globalObject->isHavingABadTime() && !hasAnyArrayStorage(m_node->indexingType())) {
            unsigned numElements = m_node->numChildren();
            
            ArrayValues arrayValues = allocateJSArray(structure, numElements);
            
            for (unsigned operandIndex = 0; operandIndex < m_node->numChildren(); ++operandIndex) {
                Edge edge = m_graph.varArgChild(m_node, operandIndex);
                
                switch (m_node->indexingType()) {
                case ALL_BLANK_INDEXING_TYPES:
                case ALL_UNDECIDED_INDEXING_TYPES:
                    DFG_CRASH(m_graph, m_node, "Bad indexing type");
                    break;
                    
                case ALL_DOUBLE_INDEXING_TYPES:
                    m_out.storeDouble(
                        lowDouble(edge),
                        arrayValues.butterfly, m_heaps.indexedDoubleProperties[operandIndex]);
                    break;
                    
                case ALL_INT32_INDEXING_TYPES:
                case ALL_CONTIGUOUS_INDEXING_TYPES:
                    m_out.store64(
                        lowJSValue(edge, ManualOperandSpeculation),
                        arrayValues.butterfly,
                        m_heaps.forIndexingType(m_node->indexingType())->at(operandIndex));
                    break;
                    
                default:
                    DFG_CRASH(m_graph, m_node, "Corrupt indexing type");
                    break;
                }
            }
            
            setJSValue(arrayValues.array);
            return;
        }
        
        if (!m_node->numChildren()) {
            setJSValue(vmCall(
                m_out.operation(operationNewEmptyArray), m_callFrame,
                m_out.constIntPtr(structure)));
            return;
        }
        
        size_t scratchSize = sizeof(EncodedJSValue) * m_node->numChildren();
        ASSERT(scratchSize);
        ScratchBuffer* scratchBuffer = vm().scratchBufferForSize(scratchSize);
        EncodedJSValue* buffer = static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer());
        
        for (unsigned operandIndex = 0; operandIndex < m_node->numChildren(); ++operandIndex) {
            Edge edge = m_graph.varArgChild(m_node, operandIndex);
            m_out.store64(
                lowJSValue(edge, ManualOperandSpeculation),
                m_out.absolute(buffer + operandIndex));
        }
        
        m_out.storePtr(
            m_out.constIntPtr(scratchSize), m_out.absolute(scratchBuffer->activeLengthPtr()));
        
        LValue result = vmCall(
            m_out.operation(operationNewArray), m_callFrame,
            m_out.constIntPtr(structure), m_out.constIntPtr(buffer),
            m_out.constIntPtr(m_node->numChildren()));
        
        m_out.storePtr(m_out.intPtrZero, m_out.absolute(scratchBuffer->activeLengthPtr()));
        
        setJSValue(result);
    }
    
    void compileNewArrayBuffer()
    {
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
        Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(
            m_node->indexingType());
        
        if (!globalObject->isHavingABadTime() && !hasAnyArrayStorage(m_node->indexingType())) {
            unsigned numElements = m_node->numConstants();
            
            ArrayValues arrayValues = allocateJSArray(structure, numElements);
            
            JSValue* data = codeBlock()->constantBuffer(m_node->startConstant());
            for (unsigned index = 0; index < m_node->numConstants(); ++index) {
                int64_t value;
                if (hasDouble(m_node->indexingType()))
                    value = bitwise_cast<int64_t>(data[index].asNumber());
                else
                    value = JSValue::encode(data[index]);
                
                m_out.store64(
                    m_out.constInt64(value),
                    arrayValues.butterfly,
                    m_heaps.forIndexingType(m_node->indexingType())->at(index));
            }
            
            setJSValue(arrayValues.array);
            return;
        }
        
        setJSValue(vmCall(
            m_out.operation(operationNewArrayBuffer), m_callFrame,
            m_out.constIntPtr(structure), m_out.constIntPtr(m_node->startConstant()),
            m_out.constIntPtr(m_node->numConstants())));
    }
    
    void compileNewArrayWithSize()
    {
        LValue publicLength = lowInt32(m_node->child1());
        
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
        Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(
            m_node->indexingType());
        
        if (!globalObject->isHavingABadTime() && !hasAnyArrayStorage(m_node->indexingType())) {
            ASSERT(
                hasUndecided(structure->indexingType())
                || hasInt32(structure->indexingType())
                || hasDouble(structure->indexingType())
                || hasContiguous(structure->indexingType()));

            LBasicBlock fastCase = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize fast case"));
            LBasicBlock largeCase = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize large case"));
            LBasicBlock failCase = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize fail case"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize continuation"));
            
            m_out.branch(
                m_out.aboveOrEqual(publicLength, m_out.constInt32(MIN_ARRAY_STORAGE_CONSTRUCTION_LENGTH)),
                rarely(largeCase), usually(fastCase));

            LBasicBlock lastNext = m_out.appendTo(fastCase, largeCase);
            
            // We don't round up to BASE_VECTOR_LEN for new Array(blah).
            LValue vectorLength = publicLength;
            
            LValue payloadSize =
                m_out.shl(m_out.zeroExt(vectorLength, m_out.intPtr), m_out.constIntPtr(3));
            
            LValue butterflySize = m_out.add(
                payloadSize, m_out.constIntPtr(sizeof(IndexingHeader)));
            
            LValue endOfStorage = allocateBasicStorageAndGetEnd(butterflySize, failCase);
            
            LValue butterfly = m_out.sub(endOfStorage, payloadSize);
            
            LValue object = allocateObject<JSArray>(
                structure, butterfly, failCase);
            
            m_out.store32(publicLength, butterfly, m_heaps.Butterfly_publicLength);
            m_out.store32(vectorLength, butterfly, m_heaps.Butterfly_vectorLength);
            
            if (hasDouble(m_node->indexingType())) {
                LBasicBlock initLoop = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize double init loop"));
                LBasicBlock initDone = FTL_NEW_BLOCK(m_out, ("NewArrayWithSize double init done"));
                
                ValueFromBlock originalIndex = m_out.anchor(vectorLength);
                ValueFromBlock originalPointer = m_out.anchor(butterfly);
                m_out.branch(
                    m_out.notZero32(vectorLength), unsure(initLoop), unsure(initDone));
                
                LBasicBlock initLastNext = m_out.appendTo(initLoop, initDone);
                LValue index = m_out.phi(m_out.int32, originalIndex);
                LValue pointer = m_out.phi(m_out.intPtr, originalPointer);
                
                m_out.store64(
                    m_out.constInt64(bitwise_cast<int64_t>(PNaN)),
                    TypedPointer(m_heaps.indexedDoubleProperties.atAnyIndex(), pointer));
                
                LValue nextIndex = m_out.sub(index, m_out.int32One);
                addIncoming(index, m_out.anchor(nextIndex));
                addIncoming(pointer, m_out.anchor(m_out.add(pointer, m_out.intPtrEight)));
                m_out.branch(
                    m_out.notZero32(nextIndex), unsure(initLoop), unsure(initDone));
                
                m_out.appendTo(initDone, initLastNext);
            }
            
            ValueFromBlock fastResult = m_out.anchor(object);
            m_out.jump(continuation);
            
            m_out.appendTo(largeCase, failCase);
            ValueFromBlock largeStructure = m_out.anchor(m_out.constIntPtr(
                globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage)));
            m_out.jump(slowCase);
            
            m_out.appendTo(failCase, slowCase);
            ValueFromBlock failStructure = m_out.anchor(m_out.constIntPtr(structure));
            m_out.jump(slowCase);
            
            m_out.appendTo(slowCase, continuation);
            LValue structureValue = m_out.phi(
                m_out.intPtr, largeStructure, failStructure);
            ValueFromBlock slowResult = m_out.anchor(vmCall(
                m_out.operation(operationNewArrayWithSize),
                m_callFrame, structureValue, publicLength));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.intPtr, fastResult, slowResult));
            return;
        }
        
        LValue structureValue = m_out.select(
            m_out.aboveOrEqual(publicLength, m_out.constInt32(MIN_ARRAY_STORAGE_CONSTRUCTION_LENGTH)),
            m_out.constIntPtr(
                globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage)),
            m_out.constIntPtr(structure));
        setJSValue(vmCall(m_out.operation(operationNewArrayWithSize), m_callFrame, structureValue, publicLength));
    }
    
    void compileAllocatePropertyStorage()
    {
        LValue object = lowCell(m_node->child1());
        setStorage(allocatePropertyStorage(object, m_node->transition()->previous));
    }

    void compileReallocatePropertyStorage()
    {
        Transition* transition = m_node->transition();
        LValue object = lowCell(m_node->child1());
        LValue oldStorage = lowStorage(m_node->child2());
        
        setStorage(
            reallocatePropertyStorage(
                object, oldStorage, transition->previous, transition->next));
    }
    
    void compileToStringOrCallStringConstructor()
    {
        switch (m_node->child1().useKind()) {
        case StringObjectUse: {
            LValue cell = lowCell(m_node->child1());
            speculateStringObjectForCell(m_node->child1(), cell);
            m_interpreter.filter(m_node->child1(), SpecStringObject);
            
            setJSValue(m_out.loadPtr(cell, m_heaps.JSWrapperObject_internalValue));
            return;
        }
            
        case StringOrStringObjectUse: {
            LValue cell = lowCell(m_node->child1());
            LValue structureID = m_out.load32(cell, m_heaps.JSCell_structureID);
            
            LBasicBlock notString = FTL_NEW_BLOCK(m_out, ("ToString StringOrStringObject not string case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ToString StringOrStringObject continuation"));
            
            ValueFromBlock simpleResult = m_out.anchor(cell);
            m_out.branch(
                m_out.equal(structureID, m_out.constInt32(vm().stringStructure->id())),
                unsure(continuation), unsure(notString));
            
            LBasicBlock lastNext = m_out.appendTo(notString, continuation);
            speculateStringObjectForStructureID(m_node->child1(), structureID);
            ValueFromBlock unboxedResult = m_out.anchor(
                m_out.loadPtr(cell, m_heaps.JSWrapperObject_internalValue));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, simpleResult, unboxedResult));
            
            m_interpreter.filter(m_node->child1(), SpecString | SpecStringObject);
            return;
        }
            
        case CellUse:
        case UntypedUse: {
            LValue value;
            if (m_node->child1().useKind() == CellUse)
                value = lowCell(m_node->child1());
            else
                value = lowJSValue(m_node->child1());
            
            LBasicBlock isCell = FTL_NEW_BLOCK(m_out, ("ToString CellUse/UntypedUse is cell"));
            LBasicBlock notString = FTL_NEW_BLOCK(m_out, ("ToString CellUse/UntypedUse not string"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ToString CellUse/UntypedUse continuation"));
            
            LValue isCellPredicate;
            if (m_node->child1().useKind() == CellUse)
                isCellPredicate = m_out.booleanTrue;
            else
                isCellPredicate = this->isCell(value, provenType(m_node->child1()));
            m_out.branch(isCellPredicate, unsure(isCell), unsure(notString));
            
            LBasicBlock lastNext = m_out.appendTo(isCell, notString);
            ValueFromBlock simpleResult = m_out.anchor(value);
            LValue isStringPredicate;
            if (m_node->child1()->prediction() & SpecString) {
                isStringPredicate = isString(value, provenType(m_node->child1()));
            } else
                isStringPredicate = m_out.booleanFalse;
            m_out.branch(isStringPredicate, unsure(continuation), unsure(notString));
            
            m_out.appendTo(notString, continuation);
            LValue operation;
            if (m_node->child1().useKind() == CellUse)
                operation = m_out.operation(m_node->op() == ToString ? operationToStringOnCell : operationCallStringConstructorOnCell);
            else
                operation = m_out.operation(m_node->op() == ToString ? operationToString : operationCallStringConstructor);
            ValueFromBlock convertedResult = m_out.anchor(vmCall(operation, m_callFrame, value));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setJSValue(m_out.phi(m_out.int64, simpleResult, convertedResult));
            return;
        }
            
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            break;
        }
    }
    
    void compileToPrimitive()
    {
        LValue value = lowJSValue(m_node->child1());
        
        LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("ToPrimitive cell case"));
        LBasicBlock isObjectCase = FTL_NEW_BLOCK(m_out, ("ToPrimitive object case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ToPrimitive continuation"));
        
        Vector<ValueFromBlock, 3> results;
        
        results.append(m_out.anchor(value));
        m_out.branch(
            isCell(value, provenType(m_node->child1())), unsure(isCellCase), unsure(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(isCellCase, isObjectCase);
        results.append(m_out.anchor(value));
        m_out.branch(
            isObject(value, provenType(m_node->child1())),
            unsure(isObjectCase), unsure(continuation));
        
        m_out.appendTo(isObjectCase, continuation);
        results.append(m_out.anchor(vmCall(
            m_out.operation(operationToPrimitive), m_callFrame, value)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, results));
    }
    
    void compileMakeRope()
    {
        LValue kids[3];
        unsigned numKids;
        kids[0] = lowCell(m_node->child1());
        kids[1] = lowCell(m_node->child2());
        if (m_node->child3()) {
            kids[2] = lowCell(m_node->child3());
            numKids = 3;
        } else {
            kids[2] = 0;
            numKids = 2;
        }
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("MakeRope slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("MakeRope continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        MarkedAllocator& allocator =
            vm().heap.allocatorForObjectWithDestructor(sizeof(JSRopeString));
        
        LValue result = allocateCell(
            m_out.constIntPtr(&allocator),
            vm().stringStructure.get(),
            slowPath);
        
        m_out.storePtr(m_out.intPtrZero, result, m_heaps.JSString_value);
        for (unsigned i = 0; i < numKids; ++i)
            m_out.storePtr(kids[i], result, m_heaps.JSRopeString_fibers[i]);
        for (unsigned i = numKids; i < JSRopeString::s_maxInternalRopeLength; ++i)
            m_out.storePtr(m_out.intPtrZero, result, m_heaps.JSRopeString_fibers[i]);
        LValue flags = m_out.load32(kids[0], m_heaps.JSString_flags);
        LValue length = m_out.load32(kids[0], m_heaps.JSString_length);
        for (unsigned i = 1; i < numKids; ++i) {
            flags = m_out.bitAnd(flags, m_out.load32(kids[i], m_heaps.JSString_flags));
            LValue lengthAndOverflow = m_out.addWithOverflow32(
                length, m_out.load32(kids[i], m_heaps.JSString_length));
            speculate(Uncountable, noValue(), 0, m_out.extractValue(lengthAndOverflow, 1));
            length = m_out.extractValue(lengthAndOverflow, 0);
        }
        m_out.store32(
            m_out.bitAnd(m_out.constInt32(JSString::Is8Bit), flags),
            result, m_heaps.JSString_flags);
        m_out.store32(length, result, m_heaps.JSString_length);
        
        ValueFromBlock fastResult = m_out.anchor(result);
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        ValueFromBlock slowResult;
        switch (numKids) {
        case 2:
            slowResult = m_out.anchor(vmCall(
                m_out.operation(operationMakeRope2), m_callFrame, kids[0], kids[1]));
            break;
        case 3:
            slowResult = m_out.anchor(vmCall(
                m_out.operation(operationMakeRope3), m_callFrame, kids[0], kids[1], kids[2]));
            break;
        default:
            DFG_CRASH(m_graph, m_node, "Bad number of children");
            break;
        }
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, fastResult, slowResult));
    }
    
    void compileStringCharAt()
    {
        LValue base = lowCell(m_node->child1());
        LValue index = lowInt32(m_node->child2());
        LValue storage = lowStorage(m_node->child3());
            
        LBasicBlock fastPath = FTL_NEW_BLOCK(m_out, ("GetByVal String fast path"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("GetByVal String slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetByVal String continuation"));
            
        m_out.branch(
            m_out.aboveOrEqual(
                index, m_out.load32NonNegative(base, m_heaps.JSString_length)),
            rarely(slowPath), usually(fastPath));
            
        LBasicBlock lastNext = m_out.appendTo(fastPath, slowPath);
            
        LValue stringImpl = m_out.loadPtr(base, m_heaps.JSString_value);
            
        LBasicBlock is8Bit = FTL_NEW_BLOCK(m_out, ("GetByVal String 8-bit case"));
        LBasicBlock is16Bit = FTL_NEW_BLOCK(m_out, ("GetByVal String 16-bit case"));
        LBasicBlock bitsContinuation = FTL_NEW_BLOCK(m_out, ("GetByVal String bitness continuation"));
        LBasicBlock bigCharacter = FTL_NEW_BLOCK(m_out, ("GetByVal String big character"));
            
        m_out.branch(
            m_out.testIsZero32(
                m_out.load32(stringImpl, m_heaps.StringImpl_hashAndFlags),
                m_out.constInt32(StringImpl::flagIs8Bit())),
            unsure(is16Bit), unsure(is8Bit));
            
        m_out.appendTo(is8Bit, is16Bit);
            
        ValueFromBlock char8Bit = m_out.anchor(m_out.zeroExt(
            m_out.load8(m_out.baseIndex(
                m_heaps.characters8, storage, m_out.zeroExtPtr(index),
                provenValue(m_node->child2()))),
            m_out.int32));
        m_out.jump(bitsContinuation);
            
        m_out.appendTo(is16Bit, bigCharacter);
            
        ValueFromBlock char16Bit = m_out.anchor(m_out.zeroExt(
            m_out.load16(m_out.baseIndex(
                m_heaps.characters16, storage, m_out.zeroExtPtr(index),
                provenValue(m_node->child2()))),
            m_out.int32));
        m_out.branch(
            m_out.aboveOrEqual(char16Bit.value(), m_out.constInt32(0x100)),
            rarely(bigCharacter), usually(bitsContinuation));
            
        m_out.appendTo(bigCharacter, bitsContinuation);
            
        Vector<ValueFromBlock, 4> results;
        results.append(m_out.anchor(vmCall(
            m_out.operation(operationSingleCharacterString),
            m_callFrame, char16Bit.value())));
        m_out.jump(continuation);
            
        m_out.appendTo(bitsContinuation, slowPath);
            
        LValue character = m_out.phi(m_out.int32, char8Bit, char16Bit);
            
        LValue smallStrings = m_out.constIntPtr(vm().smallStrings.singleCharacterStrings());
            
        results.append(m_out.anchor(m_out.loadPtr(m_out.baseIndex(
            m_heaps.singleCharacterStrings, smallStrings, m_out.zeroExtPtr(character)))));
        m_out.jump(continuation);
            
        m_out.appendTo(slowPath, continuation);
            
        if (m_node->arrayMode().isInBounds()) {
            speculate(OutOfBounds, noValue(), 0, m_out.booleanTrue);
            results.append(m_out.anchor(m_out.intPtrZero));
        } else {
            JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
                
            if (globalObject->stringPrototypeChainIsSane()) {
                // FIXME: This could be captured using a Speculation mode that means
                // "out-of-bounds loads return a trivial value", something like
                // SaneChainOutOfBounds.
                // https://bugs.webkit.org/show_bug.cgi?id=144668
                
                m_graph.watchpoints().addLazily(globalObject->stringPrototype()->structure()->transitionWatchpointSet());
                m_graph.watchpoints().addLazily(globalObject->objectPrototype()->structure()->transitionWatchpointSet());
                
                LBasicBlock negativeIndex = FTL_NEW_BLOCK(m_out, ("GetByVal String negative index"));
                    
                results.append(m_out.anchor(m_out.constInt64(JSValue::encode(jsUndefined()))));
                m_out.branch(
                    m_out.lessThan(index, m_out.int32Zero),
                    rarely(negativeIndex), usually(continuation));
                    
                m_out.appendTo(negativeIndex, continuation);
            }
                
            results.append(m_out.anchor(vmCall(
                m_out.operation(operationGetByValStringInt), m_callFrame, base, index)));
        }
            
        m_out.jump(continuation);
            
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, results));
    }
    
    void compileStringCharCodeAt()
    {
        LBasicBlock is8Bit = FTL_NEW_BLOCK(m_out, ("StringCharCodeAt 8-bit case"));
        LBasicBlock is16Bit = FTL_NEW_BLOCK(m_out, ("StringCharCodeAt 16-bit case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("StringCharCodeAt continuation"));

        LValue base = lowCell(m_node->child1());
        LValue index = lowInt32(m_node->child2());
        LValue storage = lowStorage(m_node->child3());
        
        speculate(
            Uncountable, noValue(), 0,
            m_out.aboveOrEqual(
                index, m_out.load32NonNegative(base, m_heaps.JSString_length)));
        
        LValue stringImpl = m_out.loadPtr(base, m_heaps.JSString_value);
        
        m_out.branch(
            m_out.testIsZero32(
                m_out.load32(stringImpl, m_heaps.StringImpl_hashAndFlags),
                m_out.constInt32(StringImpl::flagIs8Bit())),
            unsure(is16Bit), unsure(is8Bit));
            
        LBasicBlock lastNext = m_out.appendTo(is8Bit, is16Bit);
            
        ValueFromBlock char8Bit = m_out.anchor(m_out.zeroExt(
            m_out.load8(m_out.baseIndex(
                m_heaps.characters8, storage, m_out.zeroExtPtr(index),
                provenValue(m_node->child2()))),
            m_out.int32));
        m_out.jump(continuation);
            
        m_out.appendTo(is16Bit, continuation);
            
        ValueFromBlock char16Bit = m_out.anchor(m_out.zeroExt(
            m_out.load16(m_out.baseIndex(
                m_heaps.characters16, storage, m_out.zeroExtPtr(index),
                provenValue(m_node->child2()))),
            m_out.int32));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        setInt32(m_out.phi(m_out.int32, char8Bit, char16Bit));
    }
    
    void compileGetByOffset()
    {
        StorageAccessData& data = m_node->storageAccessData();
        
        setJSValue(loadProperty(
            lowStorage(m_node->child1()), data.identifierNumber, data.offset));
    }
    
    void compileGetGetter()
    {
        setJSValue(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.GetterSetter_getter));
    }
    
    void compileGetSetter()
    {
        setJSValue(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.GetterSetter_setter));
    }
    
    void compileMultiGetByOffset()
    {
        LValue base = lowCell(m_node->child1());
        
        MultiGetByOffsetData& data = m_node->multiGetByOffsetData();

        if (data.cases.isEmpty()) {
            // Protect against creating a Phi function with zero inputs. LLVM doesn't like that.
            terminate(BadCache);
            return;
        }
        
        Vector<LBasicBlock, 2> blocks(data.cases.size());
        for (unsigned i = data.cases.size(); i--;)
            blocks[i] = FTL_NEW_BLOCK(m_out, ("MultiGetByOffset case ", i));
        LBasicBlock exit = FTL_NEW_BLOCK(m_out, ("MultiGetByOffset fail"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("MultiGetByOffset continuation"));
        
        Vector<SwitchCase, 2> cases;
        StructureSet baseSet;
        for (unsigned i = data.cases.size(); i--;) {
            MultiGetByOffsetCase getCase = data.cases[i];
            for (unsigned j = getCase.set().size(); j--;) {
                Structure* structure = getCase.set()[j];
                baseSet.add(structure);
                cases.append(SwitchCase(weakStructureID(structure), blocks[i], Weight(1)));
            }
        }
        m_out.switchInstruction(
            m_out.load32(base, m_heaps.JSCell_structureID), cases, exit, Weight(0));
        
        LBasicBlock lastNext = m_out.m_nextBlock;
        
        Vector<ValueFromBlock, 2> results;
        for (unsigned i = data.cases.size(); i--;) {
            MultiGetByOffsetCase getCase = data.cases[i];
            GetByOffsetMethod method = getCase.method();
            
            m_out.appendTo(blocks[i], i + 1 < data.cases.size() ? blocks[i + 1] : exit);
            
            LValue result;
            
            switch (method.kind()) {
            case GetByOffsetMethod::Invalid:
                RELEASE_ASSERT_NOT_REACHED();
                break;
                
            case GetByOffsetMethod::Constant:
                result = m_out.constInt64(JSValue::encode(method.constant()->value()));
                break;
                
            case GetByOffsetMethod::Load:
            case GetByOffsetMethod::LoadFromPrototype: {
                LValue propertyBase;
                if (method.kind() == GetByOffsetMethod::Load)
                    propertyBase = base;
                else
                    propertyBase = weakPointer(method.prototype()->value().asCell());
                if (!isInlineOffset(method.offset()))
                    propertyBase = m_out.loadPtr(propertyBase, m_heaps.JSObject_butterfly);
                result = loadProperty(
                    propertyBase, data.identifierNumber, method.offset());
                break;
            } }
            
            results.append(m_out.anchor(result));
            m_out.jump(continuation);
        }
        
        m_out.appendTo(exit, continuation);
        if (!m_interpreter.forNode(m_node->child1()).m_structure.isSubsetOf(baseSet))
            speculate(BadCache, noValue(), nullptr, m_out.booleanTrue);
        m_out.unreachable();
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, results));
    }
    
    void compilePutByOffset()
    {
        StorageAccessData& data = m_node->storageAccessData();
        
        storeProperty(
            lowJSValue(m_node->child3()),
            lowStorage(m_node->child1()), data.identifierNumber, data.offset);
    }
    
    void compileMultiPutByOffset()
    {
        LValue base = lowCell(m_node->child1());
        LValue value = lowJSValue(m_node->child2());
        
        MultiPutByOffsetData& data = m_node->multiPutByOffsetData();
        
        Vector<LBasicBlock, 2> blocks(data.variants.size());
        for (unsigned i = data.variants.size(); i--;)
            blocks[i] = FTL_NEW_BLOCK(m_out, ("MultiPutByOffset case ", i));
        LBasicBlock exit = FTL_NEW_BLOCK(m_out, ("MultiPutByOffset fail"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("MultiPutByOffset continuation"));
        
        Vector<SwitchCase, 2> cases;
        StructureSet baseSet;
        for (unsigned i = data.variants.size(); i--;) {
            PutByIdVariant variant = data.variants[i];
            for (unsigned j = variant.oldStructure().size(); j--;) {
                Structure* structure = variant.oldStructure()[j];
                baseSet.add(structure);
                cases.append(SwitchCase(weakStructureID(structure), blocks[i], Weight(1)));
            }
        }
        m_out.switchInstruction(
            m_out.load32(base, m_heaps.JSCell_structureID), cases, exit, Weight(0));
        
        LBasicBlock lastNext = m_out.m_nextBlock;
        
        for (unsigned i = data.variants.size(); i--;) {
            m_out.appendTo(blocks[i], i + 1 < data.variants.size() ? blocks[i + 1] : exit);
            
            PutByIdVariant variant = data.variants[i];
            
            LValue storage;
            if (variant.kind() == PutByIdVariant::Replace) {
                if (isInlineOffset(variant.offset()))
                    storage = base;
                else
                    storage = m_out.loadPtr(base, m_heaps.JSObject_butterfly);
            } else {
                m_graph.m_plan.transitions.addLazily(
                    codeBlock(), m_node->origin.semantic.codeOriginOwner(),
                    variant.oldStructureForTransition(), variant.newStructure());
                
                storage = storageForTransition(
                    base, variant.offset(),
                    variant.oldStructureForTransition(), variant.newStructure());

                ASSERT(variant.oldStructureForTransition()->indexingType() == variant.newStructure()->indexingType());
                ASSERT(variant.oldStructureForTransition()->typeInfo().inlineTypeFlags() == variant.newStructure()->typeInfo().inlineTypeFlags());
                ASSERT(variant.oldStructureForTransition()->typeInfo().type() == variant.newStructure()->typeInfo().type());
                m_out.store32(
                    weakStructureID(variant.newStructure()), base, m_heaps.JSCell_structureID);
            }
            
            storeProperty(value, storage, data.identifierNumber, variant.offset());
            m_out.jump(continuation);
        }
        
        m_out.appendTo(exit, continuation);
        if (!m_interpreter.forNode(m_node->child1()).m_structure.isSubsetOf(baseSet))
            speculate(BadCache, noValue(), nullptr, m_out.booleanTrue);
        m_out.unreachable();
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void compileGetGlobalVar()
    {
        setJSValue(m_out.load64(m_out.absolute(m_node->variablePointer())));
    }
    
    void compilePutGlobalVar()
    {
        m_out.store64(
            lowJSValue(m_node->child2()), m_out.absolute(m_node->variablePointer()));
    }
    
    void compileNotifyWrite()
    {
        WatchpointSet* set = m_node->watchpointSet();
        
        LBasicBlock isNotInvalidated = FTL_NEW_BLOCK(m_out, ("NotifyWrite not invalidated case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("NotifyWrite continuation"));
        
        LValue state = m_out.load8(m_out.absolute(set->addressOfState()));
        m_out.branch(
            m_out.equal(state, m_out.constInt8(IsInvalidated)),
            usually(continuation), rarely(isNotInvalidated));
        
        LBasicBlock lastNext = m_out.appendTo(isNotInvalidated, continuation);

        vmCall(m_out.operation(operationNotifyWrite), m_callFrame, m_out.constIntPtr(set));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void compileGetCallee()
    {
        setJSValue(m_out.loadPtr(addressFor(JSStack::Callee)));
    }
    
    void compileGetArgumentCount()
    {
        setInt32(m_out.load32(payloadFor(JSStack::ArgumentCount)));
    }
    
    void compileGetScope()
    {
        setJSValue(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.JSFunction_scope));
    }
    
    void compileLoadArrowFunctionThis()
    {
        setJSValue(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.JSArrowFunction_this));
    }
    
    void compileSkipScope()
    {
        setJSValue(m_out.loadPtr(lowCell(m_node->child1()), m_heaps.JSScope_next));
    }
    
    void compileGetClosureVar()
    {
        setJSValue(
            m_out.load64(
                lowCell(m_node->child1()),
                m_heaps.JSEnvironmentRecord_variables[m_node->scopeOffset().offset()]));
    }
    
    void compilePutClosureVar()
    {
        m_out.store64(
            lowJSValue(m_node->child2()),
            lowCell(m_node->child1()),
            m_heaps.JSEnvironmentRecord_variables[m_node->scopeOffset().offset()]);
    }
    
    void compileGetFromArguments()
    {
        setJSValue(
            m_out.load64(
                lowCell(m_node->child1()),
                m_heaps.DirectArguments_storage[m_node->capturedArgumentsOffset().offset()]));
    }
    
    void compilePutToArguments()
    {
        m_out.store64(
            lowJSValue(m_node->child2()),
            lowCell(m_node->child1()),
            m_heaps.DirectArguments_storage[m_node->capturedArgumentsOffset().offset()]);
    }
    
    void compileCompareEq()
    {
        if (m_node->isBinaryUseKind(Int32Use)
            || m_node->isBinaryUseKind(Int52RepUse)
            || m_node->isBinaryUseKind(DoubleRepUse)
            || m_node->isBinaryUseKind(ObjectUse)
            || m_node->isBinaryUseKind(BooleanUse)
            || m_node->isBinaryUseKind(StringIdentUse)) {
            compileCompareStrictEq();
            return;
        }
        
        if (m_node->isBinaryUseKind(ObjectUse, ObjectOrOtherUse)) {
            compareEqObjectOrOtherToObject(m_node->child2(), m_node->child1());
            return;
        }
        
        if (m_node->isBinaryUseKind(ObjectOrOtherUse, ObjectUse)) {
            compareEqObjectOrOtherToObject(m_node->child1(), m_node->child2());
            return;
        }
        
        if (m_node->isBinaryUseKind(UntypedUse)) {
            nonSpeculativeCompare(LLVMIntEQ, operationCompareEq);
            return;
        }
        
        DFG_CRASH(m_graph, m_node, "Bad use kinds");
    }
    
    void compileCompareEqConstant()
    {
        ASSERT(m_node->child2()->asJSValue().isNull());
        setBoolean(
            equalNullOrUndefined(
                m_node->child1(), AllCellsAreFalse, EqualNullOrUndefined));
    }
    
    void compileCompareStrictEq()
    {
        if (m_node->isBinaryUseKind(Int32Use)) {
            setBoolean(
                m_out.equal(lowInt32(m_node->child1()), lowInt32(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(Int52RepUse)) {
            Int52Kind kind;
            LValue left = lowWhicheverInt52(m_node->child1(), kind);
            LValue right = lowInt52(m_node->child2(), kind);
            setBoolean(m_out.equal(left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(DoubleRepUse)) {
            setBoolean(
                m_out.doubleEqual(lowDouble(m_node->child1()), lowDouble(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(StringIdentUse)) {
            setBoolean(
                m_out.equal(lowStringIdent(m_node->child1()), lowStringIdent(m_node->child2())));
            return;
        }

        if (m_node->isBinaryUseKind(ObjectUse, UntypedUse)) {
            setBoolean(
                m_out.equal(
                    lowNonNullObject(m_node->child1()),
                    lowJSValue(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(UntypedUse, ObjectUse)) {
            setBoolean(
                m_out.equal(
                    lowNonNullObject(m_node->child2()),
                    lowJSValue(m_node->child1())));
            return;
        }

        if (m_node->isBinaryUseKind(ObjectUse)) {
            setBoolean(
                m_out.equal(
                    lowNonNullObject(m_node->child1()),
                    lowNonNullObject(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(BooleanUse)) {
            setBoolean(
                m_out.equal(lowBoolean(m_node->child1()), lowBoolean(m_node->child2())));
            return;
        }
        
        if (m_node->isBinaryUseKind(MiscUse, UntypedUse)
            || m_node->isBinaryUseKind(UntypedUse, MiscUse)) {
            speculate(m_node->child1());
            speculate(m_node->child2());
            LValue left = lowJSValue(m_node->child1(), ManualOperandSpeculation);
            LValue right = lowJSValue(m_node->child2(), ManualOperandSpeculation);
            setBoolean(m_out.equal(left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(StringIdentUse, NotStringVarUse)
            || m_node->isBinaryUseKind(NotStringVarUse, StringIdentUse)) {
            Edge leftEdge = m_node->childFor(StringIdentUse);
            Edge rightEdge = m_node->childFor(NotStringVarUse);
            
            LValue left = lowStringIdent(leftEdge);
            LValue rightValue = lowJSValue(rightEdge, ManualOperandSpeculation);
            
            LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("CompareStrictEq StringIdent to NotStringVar is cell case"));
            LBasicBlock isStringCase = FTL_NEW_BLOCK(m_out, ("CompareStrictEq StringIdent to NotStringVar is string case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CompareStrictEq StringIdent to NotStringVar continuation"));
            
            ValueFromBlock notCellResult = m_out.anchor(m_out.booleanFalse);
            m_out.branch(
                isCell(rightValue, provenType(rightEdge)),
                unsure(isCellCase), unsure(continuation));
            
            LBasicBlock lastNext = m_out.appendTo(isCellCase, isStringCase);
            ValueFromBlock notStringResult = m_out.anchor(m_out.booleanFalse);
            m_out.branch(
                isString(rightValue, provenType(rightEdge)),
                unsure(isStringCase), unsure(continuation));
            
            m_out.appendTo(isStringCase, continuation);
            LValue right = m_out.loadPtr(rightValue, m_heaps.JSString_value);
            speculateStringIdent(rightEdge, rightValue, right);
            ValueFromBlock isStringResult = m_out.anchor(m_out.equal(left, right));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setBoolean(m_out.phi(m_out.boolean, notCellResult, notStringResult, isStringResult));
            return;
        }
        
        DFG_CRASH(m_graph, m_node, "Bad use kinds");
    }
    
    void compileCompareStrictEqConstant()
    {
        JSValue constant = m_node->child2()->asJSValue();

        setBoolean(
            m_out.equal(
                lowJSValue(m_node->child1()),
                m_out.constInt64(JSValue::encode(constant))));
    }
    
    void compileCompareLess()
    {
        compare(LLVMIntSLT, LLVMRealOLT, operationCompareLess);
    }
    
    void compileCompareLessEq()
    {
        compare(LLVMIntSLE, LLVMRealOLE, operationCompareLessEq);
    }
    
    void compileCompareGreater()
    {
        compare(LLVMIntSGT, LLVMRealOGT, operationCompareGreater);
    }
    
    void compileCompareGreaterEq()
    {
        compare(LLVMIntSGE, LLVMRealOGE, operationCompareGreaterEq);
    }
    
    void compileLogicalNot()
    {
        setBoolean(m_out.bitNot(boolify(m_node->child1())));
    }
#if ENABLE(FTL_NATIVE_CALL_INLINING)
    void compileNativeCallOrConstruct() 
    {
        int numPassedArgs = m_node->numChildren() - 1;
        int numArgs = numPassedArgs;

        JSFunction* knownFunction = m_node->castOperand<JSFunction*>();
        NativeFunction function = knownFunction->nativeFunction();

        Dl_info info;
        if (!dladdr((void*)function, &info))
            ASSERT(false); // if we couldn't find the native function this doesn't bode well.

        LValue callee = getFunctionBySymbol(info.dli_sname);

        bool notInlinable;
        if ((notInlinable = !callee))
            callee = m_out.operation(function);

        m_out.storePtr(m_callFrame, m_execStorage, m_heaps.CallFrame_callerFrame);
        m_out.storePtr(constNull(m_out.intPtr), addressFor(m_execStorage, JSStack::CodeBlock));
        m_out.storePtr(weakPointer(knownFunction), addressFor(m_execStorage, JSStack::Callee));

        m_out.store64(m_out.constInt64(numArgs), addressFor(m_execStorage, JSStack::ArgumentCount));

        for (int i = 0; i < numPassedArgs; ++i) {
            m_out.storePtr(lowJSValue(m_graph.varArgChild(m_node, 1 + i)),
                addressFor(m_execStorage, JSStack::ThisArgument, i * sizeof(Register)));
        }

        LValue calleeCallFrame = m_out.address(m_execState, m_heaps.CallFrame_callerFrame).value();
        m_out.storePtr(m_out.ptrToInt(calleeCallFrame, m_out.intPtr), m_out.absolute(&vm().topCallFrame));

        LType typeCalleeArg;
        getParamTypes(getElementType(typeOf(callee)), &typeCalleeArg);

        LValue argument = notInlinable 
            ? m_out.ptrToInt(calleeCallFrame, typeCalleeArg) 
            : m_out.bitCast(calleeCallFrame, typeCalleeArg);
        LValue call = vmCall(callee, argument);

        if (verboseCompilationEnabled())
            dataLog("Native calling: ", info.dli_sname, "\n");

        setJSValue(call);
    }
#endif

    void compileCallOrConstruct()
    {
        int numArgs = m_node->numChildren() - 1;

        LValue jsCallee = lowJSValue(m_graph.varArgChild(m_node, 0));

        unsigned stackmapID = m_stackmapIDs++;

        unsigned frameSize = JSStack::CallFrameHeaderSize + numArgs;
        unsigned alignedFrameSize = WTF::roundUpToMultipleOf(stackAlignmentRegisters(), frameSize);
        unsigned padding = alignedFrameSize - frameSize;

        Vector<LValue> arguments;
        arguments.append(m_out.constInt64(stackmapID));
        arguments.append(m_out.constInt32(sizeOfCall()));
        arguments.append(constNull(m_out.ref8));
        arguments.append(m_out.constInt32(1 + alignedFrameSize - JSStack::CallerFrameAndPCSize));
        arguments.append(jsCallee); // callee -> %rax
        arguments.append(getUndef(m_out.int64)); // code block
        arguments.append(jsCallee); // callee -> stack
        arguments.append(m_out.constInt64(numArgs)); // argument count and zeros for the tag
        for (int i = 0; i < numArgs; ++i)
            arguments.append(lowJSValue(m_graph.varArgChild(m_node, 1 + i)));
        for (unsigned i = 0; i < padding; ++i)
            arguments.append(getUndef(m_out.int64));
        
        callPreflight();
        
        LValue call = m_out.call(m_out.patchpointInt64Intrinsic(), arguments);
        setInstructionCallingConvention(call, LLVMWebKitJSCallConv);
        
        m_ftlState.jsCalls.append(JSCall(stackmapID, m_node));
        
        setJSValue(call);
    }
    
    void compileCallOrConstructVarargs()
    {
        LValue jsCallee = lowJSValue(m_node->child1());
        LValue thisArg = lowJSValue(m_node->child3());
        
        LValue jsArguments = nullptr;
        
        switch (m_node->op()) {
        case CallVarargs:
        case ConstructVarargs:
            jsArguments = lowJSValue(m_node->child2());
            break;
        case CallForwardVarargs:
        case ConstructForwardVarargs:
            break;
        default:
            DFG_CRASH(m_graph, m_node, "bad node type");
            break;
        }
        
        unsigned stackmapID = m_stackmapIDs++;
        
        Vector<LValue> arguments;
        arguments.append(m_out.constInt64(stackmapID));
        arguments.append(m_out.constInt32(sizeOfICFor(m_node)));
        arguments.append(constNull(m_out.ref8));
        arguments.append(m_out.constInt32(2 + !!jsArguments));
        arguments.append(jsCallee);
        if (jsArguments)
            arguments.append(jsArguments);
        ASSERT(thisArg);
        arguments.append(thisArg);
        
        callPreflight();
        
        LValue call = m_out.call(m_out.patchpointInt64Intrinsic(), arguments);
        setInstructionCallingConvention(call, LLVMCCallConv);
        
        m_ftlState.jsCallVarargses.append(JSCallVarargs(stackmapID, m_node));
        
        setJSValue(call);
    }
    
    void compileLoadVarargs()
    {
        LoadVarargsData* data = m_node->loadVarargsData();
        LValue jsArguments = lowJSValue(m_node->child1());
        
        LValue length = vmCall(
            m_out.operation(operationSizeOfVarargs), m_callFrame, jsArguments,
            m_out.constInt32(data->offset));
        
        // FIXME: There is a chance that we will call an effectful length property twice. This is safe
        // from the standpoint of the VM's integrity, but it's subtly wrong from a spec compliance
        // standpoint. The best solution would be one where we can exit *into* the op_call_varargs right
        // past the sizing.
        // https://bugs.webkit.org/show_bug.cgi?id=141448
        
        LValue lengthIncludingThis = m_out.add(length, m_out.int32One);
        speculate(
            VarargsOverflow, noValue(), nullptr,
            m_out.above(lengthIncludingThis, m_out.constInt32(data->limit)));
        
        m_out.store32(lengthIncludingThis, payloadFor(data->machineCount));
        
        // FIXME: This computation is rather silly. If operationLaodVarargs just took a pointer instead
        // of a VirtualRegister, we wouldn't have to do this.
        // https://bugs.webkit.org/show_bug.cgi?id=141660
        LValue machineStart = m_out.lShr(
            m_out.sub(addressFor(data->machineStart.offset()).value(), m_callFrame),
            m_out.constIntPtr(3));
        
        vmCall(
            m_out.operation(operationLoadVarargs), m_callFrame,
            m_out.castToInt32(machineStart), jsArguments, m_out.constInt32(data->offset),
            length, m_out.constInt32(data->mandatoryMinimum));
    }
    
    void compileForwardVarargs()
    {
        LoadVarargsData* data = m_node->loadVarargsData();
        InlineCallFrame* inlineCallFrame = m_node->child1()->origin.semantic.inlineCallFrame;
        
        LValue length = getArgumentsLength(inlineCallFrame).value;
        LValue lengthIncludingThis = m_out.add(length, m_out.constInt32(1 - data->offset));
        
        speculate(
            VarargsOverflow, noValue(), nullptr,
            m_out.above(lengthIncludingThis, m_out.constInt32(data->limit)));
        
        m_out.store32(lengthIncludingThis, payloadFor(data->machineCount));
        
        LValue sourceStart = getArgumentsStart(inlineCallFrame);
        LValue targetStart = addressFor(data->machineStart).value();

        LBasicBlock undefinedLoop = FTL_NEW_BLOCK(m_out, ("ForwardVarargs undefined loop body"));
        LBasicBlock mainLoopEntry = FTL_NEW_BLOCK(m_out, ("ForwardVarargs main loop entry"));
        LBasicBlock mainLoop = FTL_NEW_BLOCK(m_out, ("ForwardVarargs main loop body"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ForwardVarargs continuation"));
        
        LValue lengthAsPtr = m_out.zeroExtPtr(length);
        ValueFromBlock loopBound = m_out.anchor(m_out.constIntPtr(data->mandatoryMinimum));
        m_out.branch(
            m_out.above(loopBound.value(), lengthAsPtr), unsure(undefinedLoop), unsure(mainLoopEntry));
        
        LBasicBlock lastNext = m_out.appendTo(undefinedLoop, mainLoopEntry);
        LValue previousIndex = m_out.phi(m_out.intPtr, loopBound);
        LValue currentIndex = m_out.sub(previousIndex, m_out.intPtrOne);
        m_out.store64(
            m_out.constInt64(JSValue::encode(jsUndefined())),
            m_out.baseIndex(m_heaps.variables, targetStart, currentIndex));
        ValueFromBlock nextIndex = m_out.anchor(currentIndex);
        addIncoming(previousIndex, nextIndex);
        m_out.branch(
            m_out.above(currentIndex, lengthAsPtr), unsure(undefinedLoop), unsure(mainLoopEntry));
        
        m_out.appendTo(mainLoopEntry, mainLoop);
        loopBound = m_out.anchor(lengthAsPtr);
        m_out.branch(m_out.notNull(loopBound.value()), unsure(mainLoop), unsure(continuation));
        
        m_out.appendTo(mainLoop, continuation);
        previousIndex = m_out.phi(m_out.intPtr, loopBound);
        currentIndex = m_out.sub(previousIndex, m_out.intPtrOne);
        LValue value = m_out.load64(
            m_out.baseIndex(
                m_heaps.variables, sourceStart,
                m_out.add(currentIndex, m_out.constIntPtr(data->offset))));
        m_out.store64(value, m_out.baseIndex(m_heaps.variables, targetStart, currentIndex));
        nextIndex = m_out.anchor(currentIndex);
        addIncoming(previousIndex, nextIndex);
        m_out.branch(m_out.isNull(currentIndex), unsure(continuation), unsure(mainLoop));
        
        m_out.appendTo(continuation, lastNext);
    }

    void compileJump()
    {
        m_out.jump(lowBlock(m_node->targetBlock()));
    }
    
    void compileBranch()
    {
        m_out.branch(
            boolify(m_node->child1()),
            WeightedTarget(
                lowBlock(m_node->branchData()->taken.block),
                m_node->branchData()->taken.count),
            WeightedTarget(
                lowBlock(m_node->branchData()->notTaken.block),
                m_node->branchData()->notTaken.count));
    }
    
    void compileSwitch()
    {
        SwitchData* data = m_node->switchData();
        switch (data->kind) {
        case SwitchImm: {
            Vector<ValueFromBlock, 2> intValues;
            LBasicBlock switchOnInts = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm int case"));
            
            LBasicBlock lastNext = m_out.appendTo(m_out.m_block, switchOnInts);
            
            switch (m_node->child1().useKind()) {
            case Int32Use: {
                intValues.append(m_out.anchor(lowInt32(m_node->child1())));
                m_out.jump(switchOnInts);
                break;
            }
                
            case UntypedUse: {
                LBasicBlock isInt = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm is int"));
                LBasicBlock isNotInt = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm is not int"));
                LBasicBlock isDouble = FTL_NEW_BLOCK(m_out, ("Switch/SwitchImm is double"));
                
                LValue boxedValue = lowJSValue(m_node->child1());
                m_out.branch(isNotInt32(boxedValue), unsure(isNotInt), unsure(isInt));
                
                LBasicBlock innerLastNext = m_out.appendTo(isInt, isNotInt);
                
                intValues.append(m_out.anchor(unboxInt32(boxedValue)));
                m_out.jump(switchOnInts);
                
                m_out.appendTo(isNotInt, isDouble);
                m_out.branch(
                    isCellOrMisc(boxedValue, provenType(m_node->child1())),
                    usually(lowBlock(data->fallThrough.block)), rarely(isDouble));
                
                m_out.appendTo(isDouble, innerLastNext);
                LValue doubleValue = unboxDouble(boxedValue);
                LValue intInDouble = m_out.fpToInt32(doubleValue);
                intValues.append(m_out.anchor(intInDouble));
                m_out.branch(
                    m_out.doubleEqual(m_out.intToDouble(intInDouble), doubleValue),
                    unsure(switchOnInts), unsure(lowBlock(data->fallThrough.block)));
                break;
            }
                
            default:
                DFG_CRASH(m_graph, m_node, "Bad use kind");
                break;
            }
            
            m_out.appendTo(switchOnInts, lastNext);
            buildSwitch(data, m_out.int32, m_out.phi(m_out.int32, intValues));
            return;
        }
        
        case SwitchChar: {
            LValue stringValue;
            
            // FIXME: We should use something other than unsure() for the branch weight
            // of the fallThrough block. The main challenge is just that we have multiple
            // branches to fallThrough but a single count, so we would need to divvy it up
            // among the different lowered branches.
            // https://bugs.webkit.org/show_bug.cgi?id=129082
            
            switch (m_node->child1().useKind()) {
            case StringUse: {
                stringValue = lowString(m_node->child1());
                break;
            }
                
            case UntypedUse: {
                LValue unboxedValue = lowJSValue(m_node->child1());
                
                LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar is cell"));
                LBasicBlock isStringCase = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar is string"));
                
                m_out.branch(
                    isNotCell(unboxedValue, provenType(m_node->child1())),
                    unsure(lowBlock(data->fallThrough.block)), unsure(isCellCase));
                
                LBasicBlock lastNext = m_out.appendTo(isCellCase, isStringCase);
                LValue cellValue = unboxedValue;
                m_out.branch(
                    isNotString(cellValue, provenType(m_node->child1())),
                    unsure(lowBlock(data->fallThrough.block)), unsure(isStringCase));
                
                m_out.appendTo(isStringCase, lastNext);
                stringValue = cellValue;
                break;
            }
                
            default:
                DFG_CRASH(m_graph, m_node, "Bad use kind");
                break;
            }
            
            LBasicBlock lengthIs1 = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar length is 1"));
            LBasicBlock needResolution = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar resolution"));
            LBasicBlock resolved = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar resolved"));
            LBasicBlock is8Bit = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar 8bit"));
            LBasicBlock is16Bit = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar 16bit"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Switch/SwitchChar continuation"));
            
            m_out.branch(
                m_out.notEqual(
                    m_out.load32NonNegative(stringValue, m_heaps.JSString_length),
                    m_out.int32One),
                unsure(lowBlock(data->fallThrough.block)), unsure(lengthIs1));
            
            LBasicBlock lastNext = m_out.appendTo(lengthIs1, needResolution);
            Vector<ValueFromBlock, 2> values;
            LValue fastValue = m_out.loadPtr(stringValue, m_heaps.JSString_value);
            values.append(m_out.anchor(fastValue));
            m_out.branch(m_out.isNull(fastValue), rarely(needResolution), usually(resolved));
            
            m_out.appendTo(needResolution, resolved);
            values.append(m_out.anchor(
                vmCall(m_out.operation(operationResolveRope), m_callFrame, stringValue)));
            m_out.jump(resolved);
            
            m_out.appendTo(resolved, is8Bit);
            LValue value = m_out.phi(m_out.intPtr, values);
            LValue characterData = m_out.loadPtr(value, m_heaps.StringImpl_data);
            m_out.branch(
                m_out.testNonZero32(
                    m_out.load32(value, m_heaps.StringImpl_hashAndFlags),
                    m_out.constInt32(StringImpl::flagIs8Bit())),
                unsure(is8Bit), unsure(is16Bit));
            
            Vector<ValueFromBlock, 2> characters;
            m_out.appendTo(is8Bit, is16Bit);
            characters.append(m_out.anchor(
                m_out.zeroExt(m_out.load8(characterData, m_heaps.characters8[0]), m_out.int16)));
            m_out.jump(continuation);
            
            m_out.appendTo(is16Bit, continuation);
            characters.append(m_out.anchor(m_out.load16(characterData, m_heaps.characters16[0])));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            buildSwitch(data, m_out.int16, m_out.phi(m_out.int16, characters));
            return;
        }
        
        case SwitchString: {
            switch (m_node->child1().useKind()) {
            case StringIdentUse: {
                LValue stringImpl = lowStringIdent(m_node->child1());
                
                Vector<SwitchCase> cases;
                for (unsigned i = 0; i < data->cases.size(); ++i) {
                    LValue value = m_out.constIntPtr(data->cases[i].value.stringImpl());
                    LBasicBlock block = lowBlock(data->cases[i].target.block);
                    Weight weight = Weight(data->cases[i].target.count);
                    cases.append(SwitchCase(value, block, weight));
                }
                
                m_out.switchInstruction(
                    stringImpl, cases, lowBlock(data->fallThrough.block),
                    Weight(data->fallThrough.count));
                return;
            }
                
            case StringUse: {
                switchString(data, lowString(m_node->child1()));
                return;
            }
                
            case UntypedUse: {
                LValue value = lowJSValue(m_node->child1());
                
                LBasicBlock isCellBlock = FTL_NEW_BLOCK(m_out, ("Switch/SwitchString Untyped cell case"));
                LBasicBlock isStringBlock = FTL_NEW_BLOCK(m_out, ("Switch/SwitchString Untyped string case"));
                
                m_out.branch(
                    isCell(value, provenType(m_node->child1())),
                    unsure(isCellBlock), unsure(lowBlock(data->fallThrough.block)));
                
                LBasicBlock lastNext = m_out.appendTo(isCellBlock, isStringBlock);
                
                m_out.branch(
                    isString(value, provenType(m_node->child1())),
                    unsure(isStringBlock), unsure(lowBlock(data->fallThrough.block)));
                
                m_out.appendTo(isStringBlock, lastNext);
                
                switchString(data, value);
                return;
            }
                
            default:
                DFG_CRASH(m_graph, m_node, "Bad use kind");
                return;
            }
            return;
        }
            
        case SwitchCell: {
            LValue cell;
            switch (m_node->child1().useKind()) {
            case CellUse: {
                cell = lowCell(m_node->child1());
                break;
            }
                
            case UntypedUse: {
                LValue value = lowJSValue(m_node->child1());
                LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("Switch/SwitchCell cell case"));
                m_out.branch(
                    isCell(value, provenType(m_node->child1())),
                    unsure(cellCase), unsure(lowBlock(data->fallThrough.block)));
                m_out.appendTo(cellCase);
                cell = value;
                break;
            }
                
            default:
                DFG_CRASH(m_graph, m_node, "Bad use kind");
                return;
            }
            
            buildSwitch(m_node->switchData(), m_out.intPtr, cell);
            return;
        } }
        
        DFG_CRASH(m_graph, m_node, "Bad switch kind");
    }
    
    void compileReturn()
    {
        m_out.ret(lowJSValue(m_node->child1()));
    }
    
    void compileForceOSRExit()
    {
        terminate(InadequateCoverage);
    }
    
    void compileThrow()
    {
        terminate(Uncountable);
    }
    
    void compileInvalidationPoint()
    {
        if (verboseCompilationEnabled())
            dataLog("    Invalidation point with availability: ", availabilityMap(), "\n");
        
        m_ftlState.jitCode->osrExit.append(OSRExit(
            UncountableInvalidation, InvalidValueFormat, MethodOfGettingAValueProfile(),
            m_codeOriginForExitTarget, m_codeOriginForExitProfile,
            availabilityMap().m_locals.numberOfArguments(),
            availabilityMap().m_locals.numberOfLocals()));
        m_ftlState.finalizer->osrExit.append(OSRExitCompilationInfo());
        
        OSRExit& exit = m_ftlState.jitCode->osrExit.last();
        OSRExitCompilationInfo& info = m_ftlState.finalizer->osrExit.last();
        
        ExitArgumentList arguments;
        
        buildExitArguments(exit, arguments, FormattedValue(), exit.m_codeOrigin);
        callStackmap(exit, arguments);
        
        info.m_isInvalidationPoint = true;
    }
    
    void compileIsUndefined()
    {
        setBoolean(equalNullOrUndefined(m_node->child1(), AllCellsAreFalse, EqualUndefined));
    }
    
    void compileIsBoolean()
    {
        setBoolean(isBoolean(lowJSValue(m_node->child1()), provenType(m_node->child1())));
    }
    
    void compileIsNumber()
    {
        setBoolean(isNumber(lowJSValue(m_node->child1()), provenType(m_node->child1())));
    }
    
    void compileIsString()
    {
        LValue value = lowJSValue(m_node->child1());
        
        LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("IsString cell case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("IsString continuation"));
        
        ValueFromBlock notCellResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(
            isCell(value, provenType(m_node->child1())), unsure(isCellCase), unsure(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(isCellCase, continuation);
        ValueFromBlock cellResult = m_out.anchor(isString(value, provenType(m_node->child1())));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setBoolean(m_out.phi(m_out.boolean, notCellResult, cellResult));
    }

    void compileIsObject()
    {
        LValue value = lowJSValue(m_node->child1());

        LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("IsObject cell case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("IsObject continuation"));

        ValueFromBlock notCellResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(
            isCell(value, provenType(m_node->child1())), unsure(isCellCase), unsure(continuation));

        LBasicBlock lastNext = m_out.appendTo(isCellCase, continuation);
        ValueFromBlock cellResult = m_out.anchor(isObject(value, provenType(m_node->child1())));
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
        setBoolean(m_out.phi(m_out.boolean, notCellResult, cellResult));
    }

    void compileIsObjectOrNull()
    {
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
        
        Edge child = m_node->child1();
        LValue value = lowJSValue(child);
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("IsObjectOrNull cell case"));
        LBasicBlock notFunctionCase = FTL_NEW_BLOCK(m_out, ("IsObjectOrNull not function case"));
        LBasicBlock objectCase = FTL_NEW_BLOCK(m_out, ("IsObjectOrNull object case"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("IsObjectOrNull slow path"));
        LBasicBlock notCellCase = FTL_NEW_BLOCK(m_out, ("IsObjectOrNull not cell case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("IsObjectOrNull continuation"));
        
        m_out.branch(isCell(value, provenType(child)), unsure(cellCase), unsure(notCellCase));
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, notFunctionCase);
        ValueFromBlock isFunctionResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(
            isFunction(value, provenType(child)),
            unsure(continuation), unsure(notFunctionCase));
        
        m_out.appendTo(notFunctionCase, objectCase);
        ValueFromBlock notObjectResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(
            isObject(value, provenType(child)),
            unsure(objectCase), unsure(continuation));
        
        m_out.appendTo(objectCase, slowPath);
        ValueFromBlock objectResult = m_out.anchor(m_out.booleanTrue);
        m_out.branch(
            isExoticForTypeof(value, provenType(child)),
            rarely(slowPath), usually(continuation));
        
        m_out.appendTo(slowPath, notCellCase);
        LValue slowResultValue = vmCall(
            m_out.operation(operationObjectIsObject), m_callFrame, weakPointer(globalObject),
            value);
        ValueFromBlock slowResult = m_out.anchor(m_out.notNull(slowResultValue));
        m_out.jump(continuation);
        
        m_out.appendTo(notCellCase, continuation);
        LValue notCellResultValue = m_out.equal(value, m_out.constInt64(JSValue::encode(jsNull())));
        ValueFromBlock notCellResult = m_out.anchor(notCellResultValue);
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        LValue result = m_out.phi(
            m_out.boolean,
            isFunctionResult, notObjectResult, objectResult, slowResult, notCellResult);
        setBoolean(result);
    }
    
    void compileIsFunction()
    {
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
        
        Edge child = m_node->child1();
        LValue value = lowJSValue(child);
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("IsFunction cell case"));
        LBasicBlock notFunctionCase = FTL_NEW_BLOCK(m_out, ("IsFunction not function case"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("IsFunction slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("IsFunction continuation"));
        
        ValueFromBlock notCellResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(
            isCell(value, provenType(child)), unsure(cellCase), unsure(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, notFunctionCase);
        ValueFromBlock functionResult = m_out.anchor(m_out.booleanTrue);
        m_out.branch(
            isFunction(value, provenType(child)),
            unsure(continuation), unsure(notFunctionCase));
        
        m_out.appendTo(notFunctionCase, slowPath);
        ValueFromBlock objectResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(
            isExoticForTypeof(value, provenType(child)),
            rarely(slowPath), usually(continuation));
        
        m_out.appendTo(slowPath, continuation);
        LValue slowResultValue = vmCall(
            m_out.operation(operationObjectIsFunction), m_callFrame, weakPointer(globalObject),
            value);
        ValueFromBlock slowResult = m_out.anchor(m_out.notNull(slowResultValue));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        LValue result = m_out.phi(
            m_out.boolean, notCellResult, functionResult, objectResult, slowResult);
        setBoolean(result);
    }
    
    void compileTypeOf()
    {
        Edge child = m_node->child1();
        LValue value = lowJSValue(child);
        
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("TypeOf continuation"));
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(continuation);
        
        Vector<ValueFromBlock> results;
        
        buildTypeOf(
            child, value,
            [&] (TypeofType type) {
                results.append(m_out.anchor(weakPointer(vm().smallStrings.typeString(type))));
                m_out.jump(continuation);
            });
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, results));
    }
    
    void compileIn()
    {
        Edge base = m_node->child2();
        LValue cell = lowCell(base);
        speculateObject(base, cell);
        if (JSString* string = m_node->child1()->dynamicCastConstant<JSString*>()) {
            if (string->tryGetValueImpl() && string->tryGetValueImpl()->isAtomic()) {

                const auto str = static_cast<const AtomicStringImpl*>(string->tryGetValueImpl());
                unsigned stackmapID = m_stackmapIDs++;
            
                LValue call = m_out.call(
                    m_out.patchpointInt64Intrinsic(),
                    m_out.constInt64(stackmapID), m_out.constInt32(sizeOfIn()),
                    constNull(m_out.ref8), m_out.constInt32(1), cell);

                setInstructionCallingConvention(call, LLVMAnyRegCallConv);

                m_ftlState.checkIns.append(CheckInDescriptor(stackmapID, m_node->origin.semantic, str));
                setJSValue(call);
                return;
            }
        } 

        setJSValue(vmCall(m_out.operation(operationGenericIn), m_callFrame, cell, lowJSValue(m_node->child1())));
    }

    void compileCheckHasInstance()
    {
        speculate(
            Uncountable, noValue(), 0,
            m_out.testIsZero8(
                m_out.load8(lowCell(m_node->child1()), m_heaps.JSCell_typeInfoFlags),
                m_out.constInt8(ImplementsDefaultHasInstance)));
    }
    
    void compileInstanceOf()
    {
        LValue cell;
        
        if (m_node->child1().useKind() == UntypedUse)
            cell = lowJSValue(m_node->child1());
        else
            cell = lowCell(m_node->child1());
        
        LValue prototype = lowCell(m_node->child2());
        
        LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("InstanceOf cell case"));
        LBasicBlock loop = FTL_NEW_BLOCK(m_out, ("InstanceOf loop"));
        LBasicBlock notYetInstance = FTL_NEW_BLOCK(m_out, ("InstanceOf not yet instance"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("InstanceOf continuation"));
        
        LValue condition;
        if (m_node->child1().useKind() == UntypedUse)
            condition = isCell(cell, provenType(m_node->child1()));
        else
            condition = m_out.booleanTrue;
        
        ValueFromBlock notCellResult = m_out.anchor(m_out.booleanFalse);
        m_out.branch(condition, unsure(isCellCase), unsure(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(isCellCase, loop);
        
        speculate(BadType, noValue(), 0, isNotObject(prototype, provenType(m_node->child2())));
        
        ValueFromBlock originalValue = m_out.anchor(cell);
        m_out.jump(loop);
        
        m_out.appendTo(loop, notYetInstance);
        LValue value = m_out.phi(m_out.int64, originalValue);
        LValue structure = loadStructure(value);
        LValue currentPrototype = m_out.load64(structure, m_heaps.Structure_prototype);
        ValueFromBlock isInstanceResult = m_out.anchor(m_out.booleanTrue);
        m_out.branch(
            m_out.equal(currentPrototype, prototype),
            unsure(continuation), unsure(notYetInstance));
        
        m_out.appendTo(notYetInstance, continuation);
        ValueFromBlock notInstanceResult = m_out.anchor(m_out.booleanFalse);
        addIncoming(value, m_out.anchor(currentPrototype));
        m_out.branch(isCell(currentPrototype), unsure(loop), unsure(continuation));
        
        m_out.appendTo(continuation, lastNext);
        setBoolean(
            m_out.phi(m_out.boolean, notCellResult, isInstanceResult, notInstanceResult));
    }
    
    void compileCountExecution()
    {
        TypedPointer counter = m_out.absolute(m_node->executionCounter()->address());
        m_out.store64(m_out.add(m_out.load64(counter), m_out.constInt64(1)), counter);
    }
    
    void compileStoreBarrier()
    {
        emitStoreBarrier(lowCell(m_node->child1()));
    }

    void compileHasIndexedProperty()
    {
        switch (m_node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            LValue base = lowCell(m_node->child1());
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());

            IndexedAbstractHeap& heap = m_node->arrayMode().type() == Array::Int32 ?
                m_heaps.indexedInt32Properties : m_heaps.indexedContiguousProperties;

            LBasicBlock checkHole = FTL_NEW_BLOCK(m_out, ("HasIndexedProperty int/contiguous check hole"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("HasIndexedProperty int/contiguous slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("HasIndexedProperty int/contiguous continuation"));

            if (!m_node->arrayMode().isInBounds()) {
                m_out.branch(
                    m_out.aboveOrEqual(
                        index, m_out.load32NonNegative(storage, m_heaps.Butterfly_publicLength)),
                    rarely(slowCase), usually(checkHole));
            } else
                m_out.jump(checkHole);

            LBasicBlock lastNext = m_out.appendTo(checkHole, slowCase); 
            ValueFromBlock checkHoleResult = m_out.anchor(
                m_out.notZero64(m_out.load64(baseIndex(heap, storage, index, m_node->child2()))));
            m_out.branch(checkHoleResult.value(), usually(continuation), rarely(slowCase));

            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(m_out.equal(
                m_out.constInt64(JSValue::encode(jsBoolean(true))), 
                vmCall(m_out.operation(operationHasIndexedProperty), m_callFrame, base, index)));
            m_out.jump(continuation);

            m_out.appendTo(continuation, lastNext);
            setBoolean(m_out.phi(m_out.boolean, checkHoleResult, slowResult));
            return;
        }
        case Array::Double: {
            LValue base = lowCell(m_node->child1());
            LValue index = lowInt32(m_node->child2());
            LValue storage = lowStorage(m_node->child3());
            
            IndexedAbstractHeap& heap = m_heaps.indexedDoubleProperties;
            
            LBasicBlock checkHole = FTL_NEW_BLOCK(m_out, ("HasIndexedProperty double check hole"));
            LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("HasIndexedProperty double slow case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("HasIndexedProperty double continuation"));
            
            if (!m_node->arrayMode().isInBounds()) {
                m_out.branch(
                    m_out.aboveOrEqual(
                        index, m_out.load32NonNegative(storage, m_heaps.Butterfly_publicLength)),
                    rarely(slowCase), usually(checkHole));
            } else
                m_out.jump(checkHole);

            LBasicBlock lastNext = m_out.appendTo(checkHole, slowCase);
            LValue doubleValue = m_out.loadDouble(baseIndex(heap, storage, index, m_node->child2()));
            ValueFromBlock checkHoleResult = m_out.anchor(m_out.doubleEqual(doubleValue, doubleValue));
            m_out.branch(checkHoleResult.value(), usually(continuation), rarely(slowCase));
            
            m_out.appendTo(slowCase, continuation);
            ValueFromBlock slowResult = m_out.anchor(m_out.equal(
                m_out.constInt64(JSValue::encode(jsBoolean(true))), 
                vmCall(m_out.operation(operationHasIndexedProperty), m_callFrame, base, index)));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            setBoolean(m_out.phi(m_out.boolean, checkHoleResult, slowResult));
            return;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return;
        }
    }

    void compileHasGenericProperty()
    {
        LValue base = lowJSValue(m_node->child1());
        LValue property = lowCell(m_node->child2());
        setJSValue(vmCall(m_out.operation(operationHasGenericProperty), m_callFrame, base, property));
    }

    void compileHasStructureProperty()
    {
        LValue base = lowJSValue(m_node->child1());
        LValue property = lowString(m_node->child2());
        LValue enumerator = lowCell(m_node->child3());

        LBasicBlock correctStructure = FTL_NEW_BLOCK(m_out, ("HasStructureProperty correct structure"));
        LBasicBlock wrongStructure = FTL_NEW_BLOCK(m_out, ("HasStructureProperty wrong structure"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("HasStructureProperty continuation"));

        m_out.branch(m_out.notEqual(
            m_out.load32(base, m_heaps.JSCell_structureID),
            m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_cachedStructureID)),
            rarely(wrongStructure), usually(correctStructure));

        LBasicBlock lastNext = m_out.appendTo(correctStructure, wrongStructure);
        ValueFromBlock correctStructureResult = m_out.anchor(m_out.booleanTrue);
        m_out.jump(continuation);

        m_out.appendTo(wrongStructure, continuation);
        ValueFromBlock wrongStructureResult = m_out.anchor(
            m_out.equal(
                m_out.constInt64(JSValue::encode(jsBoolean(true))), 
                vmCall(m_out.operation(operationHasGenericProperty), m_callFrame, base, property)));
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
        setBoolean(m_out.phi(m_out.boolean, correctStructureResult, wrongStructureResult));
    }

    void compileGetDirectPname()
    {
        LValue base = lowCell(m_graph.varArgChild(m_node, 0));
        LValue property = lowCell(m_graph.varArgChild(m_node, 1));
        LValue index = lowInt32(m_graph.varArgChild(m_node, 2));
        LValue enumerator = lowCell(m_graph.varArgChild(m_node, 3));

        LBasicBlock checkOffset = FTL_NEW_BLOCK(m_out, ("GetDirectPname check offset"));
        LBasicBlock inlineLoad = FTL_NEW_BLOCK(m_out, ("GetDirectPname inline load"));
        LBasicBlock outOfLineLoad = FTL_NEW_BLOCK(m_out, ("GetDirectPname out-of-line load"));
        LBasicBlock slowCase = FTL_NEW_BLOCK(m_out, ("GetDirectPname slow case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetDirectPname continuation"));

        m_out.branch(m_out.notEqual(
            m_out.load32(base, m_heaps.JSCell_structureID),
            m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_cachedStructureID)),
            rarely(slowCase), usually(checkOffset));

        LBasicBlock lastNext = m_out.appendTo(checkOffset, inlineLoad);
        m_out.branch(m_out.aboveOrEqual(index, m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_cachedInlineCapacity)),
            unsure(outOfLineLoad), unsure(inlineLoad));

        m_out.appendTo(inlineLoad, outOfLineLoad);
        ValueFromBlock inlineResult = m_out.anchor(
            m_out.load64(m_out.baseIndex(m_heaps.properties.atAnyNumber(), 
                base, m_out.zeroExt(index, m_out.int64), ScaleEight, JSObject::offsetOfInlineStorage())));
        m_out.jump(continuation);

        m_out.appendTo(outOfLineLoad, slowCase);
        LValue storage = m_out.loadPtr(base, m_heaps.JSObject_butterfly);
        LValue realIndex = m_out.signExt(
            m_out.neg(m_out.sub(index, m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_cachedInlineCapacity))), 
            m_out.int64);
        int32_t offsetOfFirstProperty = static_cast<int32_t>(offsetInButterfly(firstOutOfLineOffset)) * sizeof(EncodedJSValue);
        ValueFromBlock outOfLineResult = m_out.anchor(
            m_out.load64(m_out.baseIndex(m_heaps.properties.atAnyNumber(), storage, realIndex, ScaleEight, offsetOfFirstProperty)));
        m_out.jump(continuation);

        m_out.appendTo(slowCase, continuation);
        ValueFromBlock slowCaseResult = m_out.anchor(
            vmCall(m_out.operation(operationGetByVal), m_callFrame, base, property));
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, inlineResult, outOfLineResult, slowCaseResult));
    }

    void compileGetEnumerableLength()
    {
        LValue enumerator = lowCell(m_node->child1());
        setInt32(m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_indexLength));
    }

    void compileGetPropertyEnumerator()
    {
        LValue base = lowCell(m_node->child1());
        setJSValue(vmCall(m_out.operation(operationGetPropertyEnumerator), m_callFrame, base));
    }

    void compileGetEnumeratorStructurePname()
    {
        LValue enumerator = lowCell(m_node->child1());
        LValue index = lowInt32(m_node->child2());

        LBasicBlock inBounds = FTL_NEW_BLOCK(m_out, ("GetEnumeratorStructurePname in bounds"));
        LBasicBlock outOfBounds = FTL_NEW_BLOCK(m_out, ("GetEnumeratorStructurePname out of bounds"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetEnumeratorStructurePname continuation"));

        m_out.branch(m_out.below(index, m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_endStructurePropertyIndex)),
            usually(inBounds), rarely(outOfBounds));

        LBasicBlock lastNext = m_out.appendTo(inBounds, outOfBounds);
        LValue storage = m_out.loadPtr(enumerator, m_heaps.JSPropertyNameEnumerator_cachedPropertyNamesVector);
        ValueFromBlock inBoundsResult = m_out.anchor(
            m_out.loadPtr(m_out.baseIndex(m_heaps.JSPropertyNameEnumerator_cachedPropertyNamesVectorContents, storage, m_out.zeroExtPtr(index))));
        m_out.jump(continuation);

        m_out.appendTo(outOfBounds, continuation);
        ValueFromBlock outOfBoundsResult = m_out.anchor(m_out.constInt64(ValueNull));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, inBoundsResult, outOfBoundsResult));
    }

    void compileGetEnumeratorGenericPname()
    {
        LValue enumerator = lowCell(m_node->child1());
        LValue index = lowInt32(m_node->child2());

        LBasicBlock inBounds = FTL_NEW_BLOCK(m_out, ("GetEnumeratorGenericPname in bounds"));
        LBasicBlock outOfBounds = FTL_NEW_BLOCK(m_out, ("GetEnumeratorGenericPname out of bounds"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("GetEnumeratorGenericPname continuation"));

        m_out.branch(m_out.below(index, m_out.load32(enumerator, m_heaps.JSPropertyNameEnumerator_endGenericPropertyIndex)),
            usually(inBounds), rarely(outOfBounds));

        LBasicBlock lastNext = m_out.appendTo(inBounds, outOfBounds);
        LValue storage = m_out.loadPtr(enumerator, m_heaps.JSPropertyNameEnumerator_cachedPropertyNamesVector);
        ValueFromBlock inBoundsResult = m_out.anchor(
            m_out.loadPtr(m_out.baseIndex(m_heaps.JSPropertyNameEnumerator_cachedPropertyNamesVectorContents, storage, m_out.zeroExtPtr(index))));
        m_out.jump(continuation);

        m_out.appendTo(outOfBounds, continuation);
        ValueFromBlock outOfBoundsResult = m_out.anchor(m_out.constInt64(ValueNull));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setJSValue(m_out.phi(m_out.int64, inBoundsResult, outOfBoundsResult));
    }
    
    void compileToIndexString()
    {
        LValue index = lowInt32(m_node->child1());
        setJSValue(vmCall(m_out.operation(operationToIndexString), m_callFrame, index));
    }
    
    void compileCheckStructureImmediate()
    {
        LValue structure = lowCell(m_node->child1());
        checkStructure(
            structure, noValue(), BadCache, m_node->structureSet(),
            [this] (Structure* structure) {
                return weakStructure(structure);
            });
    }
    
    void compileMaterializeNewObject()
    {
        ObjectMaterializationData& data = m_node->objectMaterializationData();
        
        // Lower the values first, to avoid creating values inside a control flow diamond.
        
        Vector<LValue, 8> values;
        for (unsigned i = 0; i < data.m_properties.size(); ++i)
            values.append(lowJSValue(m_graph.varArgChild(m_node, 1 + i)));
        
        StructureSet set;
        m_interpreter.phiChildren()->forAllTransitiveIncomingValues(
            m_graph.varArgChild(m_node, 0).node(),
            [&] (Node* incoming) {
                set.add(incoming->castConstant<Structure*>());
            });
        
        Vector<LBasicBlock, 1> blocks(set.size());
        for (unsigned i = set.size(); i--;)
            blocks[i] = FTL_NEW_BLOCK(m_out, ("MaterializeNewObject case ", i));
        LBasicBlock dummyDefault = FTL_NEW_BLOCK(m_out, ("MaterializeNewObject default case"));
        LBasicBlock outerContinuation = FTL_NEW_BLOCK(m_out, ("MaterializeNewObject continuation"));
        
        Vector<SwitchCase, 1> cases(set.size());
        for (unsigned i = set.size(); i--;)
            cases[i] = SwitchCase(weakStructure(set[i]), blocks[i], Weight(1));
        m_out.switchInstruction(
            lowCell(m_graph.varArgChild(m_node, 0)), cases, dummyDefault, Weight(0));
        
        LBasicBlock outerLastNext = m_out.m_nextBlock;
        
        Vector<ValueFromBlock, 1> results;
        
        for (unsigned i = set.size(); i--;) {
            m_out.appendTo(blocks[i], i + 1 < set.size() ? blocks[i + 1] : dummyDefault);
            
            Structure* structure = set[i];
            
            LValue object;
            LValue butterfly;
            
            if (structure->outOfLineCapacity()) {
                size_t allocationSize = JSFinalObject::allocationSize(structure->inlineCapacity());
                MarkedAllocator* allocator = &vm().heap.allocatorForObjectWithoutDestructor(allocationSize);
                
                LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("MaterializeNewObject complex object allocation slow path"));
                LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("MaterializeNewObject complex object allocation continuation"));
                
                LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
                
                LValue endOfStorage = allocateBasicStorageAndGetEnd(
                    m_out.constIntPtr(structure->outOfLineCapacity() * sizeof(JSValue)),
                    slowPath);
                
                LValue fastButterflyValue = m_out.add(
                    m_out.constIntPtr(sizeof(IndexingHeader)), endOfStorage);
                
                LValue fastObjectValue = allocateObject(
                    m_out.constIntPtr(allocator), structure, fastButterflyValue, slowPath);

                ValueFromBlock fastObject = m_out.anchor(fastObjectValue);
                ValueFromBlock fastButterfly = m_out.anchor(fastButterflyValue);
                m_out.jump(continuation);
                
                m_out.appendTo(slowPath, continuation);
                
                ValueFromBlock slowObject = m_out.anchor(vmCall(
                    m_out.operation(operationNewObjectWithButterfly),
                    m_callFrame, m_out.constIntPtr(structure)));
                ValueFromBlock slowButterfly = m_out.anchor(
                    m_out.loadPtr(slowObject.value(), m_heaps.JSObject_butterfly));
                
                m_out.jump(continuation);
                
                m_out.appendTo(continuation, lastNext);
                
                object = m_out.phi(m_out.intPtr, fastObject, slowObject);
                butterfly = m_out.phi(m_out.intPtr, fastButterfly, slowButterfly);
            } else {
                // In the easy case where we can do a one-shot allocation, we simply allocate the
                // object to directly have the desired structure.
                object = allocateObject(structure);
                butterfly = nullptr; // Don't have one, don't need one.
            }
            
            for (PropertyMapEntry entry : structure->getPropertiesConcurrently()) {
                for (unsigned i = data.m_properties.size(); i--;) {
                    PhantomPropertyValue value = data.m_properties[i];
                    if (m_graph.identifiers()[value.m_identifierNumber] != entry.key)
                        continue;
                    
                    LValue base = isInlineOffset(entry.offset) ? object : butterfly;
                    storeProperty(values[i], base, value.m_identifierNumber, entry.offset);
                    break;
                }
            }
            
            results.append(m_out.anchor(object));
            m_out.jump(outerContinuation);
        }
        
        m_out.appendTo(dummyDefault, outerContinuation);
        m_out.unreachable();
        
        m_out.appendTo(outerContinuation, outerLastNext);
        setJSValue(m_out.phi(m_out.intPtr, results));
    }

    void compileMaterializeCreateActivation()
    {
        ObjectMaterializationData& data = m_node->objectMaterializationData();

        Vector<LValue, 8> values;
        for (unsigned i = 0; i < data.m_properties.size(); ++i)
            values.append(lowJSValue(m_graph.varArgChild(m_node, 1 + i)));

        LValue scope = lowCell(m_graph.varArgChild(m_node, 0));
        SymbolTable* table = m_node->castOperand<SymbolTable*>();
        Structure* structure = m_graph.globalObjectFor(m_node->origin.semantic)->activationStructure();

        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("MaterializeCreateActivation slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("MaterializeCreateActivation continuation"));

        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);

        LValue fastObject = allocateObject<JSLexicalEnvironment>(
            JSLexicalEnvironment::allocationSize(table), structure, m_out.intPtrZero, slowPath);

        m_out.storePtr(scope, fastObject, m_heaps.JSScope_next);
        m_out.storePtr(weakPointer(table), fastObject, m_heaps.JSSymbolTableObject_symbolTable);


        ValueFromBlock fastResult = m_out.anchor(fastObject);
        m_out.jump(continuation);

        m_out.appendTo(slowPath, continuation);
        LValue callResult = vmCall(
            m_out.operation(operationCreateActivationDirect), m_callFrame, weakPointer(structure),
            scope, weakPointer(table));
        ValueFromBlock slowResult =  m_out.anchor(callResult);
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
        LValue activation = m_out.phi(m_out.intPtr, fastResult, slowResult);
        RELEASE_ASSERT(data.m_properties.size() == table->scopeSize());
        for (unsigned i = 0; i < data.m_properties.size(); ++i) {
            m_out.store64(values[i],
                activation,
                m_heaps.JSEnvironmentRecord_variables[data.m_properties[i].m_identifierNumber]);
        }

        if (validationEnabled()) {
            // Validate to make sure every slot in the scope has one value.
            ConcurrentJITLocker locker(table->m_lock);
            for (auto iter = table->begin(locker), end = table->end(locker); iter != end; ++iter) {
                bool found = false;
                for (unsigned i = 0; i < data.m_properties.size(); ++i) {
                    if (iter->value.scopeOffset().offset() == data.m_properties[i].m_identifierNumber) {
                        found = true;
                        break;
                    }
                }
                ASSERT_UNUSED(found, found);
            }
        }

        setJSValue(activation);
    }

#if ENABLE(FTL_NATIVE_CALL_INLINING)
    LValue getFunctionBySymbol(const CString symbol)
    {
        if (!m_ftlState.symbolTable.contains(symbol)) 
            return nullptr;
        if (!getModuleByPathForSymbol(m_ftlState.symbolTable.get(symbol), symbol))
            return nullptr;
        return getNamedFunction(m_ftlState.module, symbol.data());
    }

    bool getModuleByPathForSymbol(const CString path, const CString symbol)
    {
        if (m_ftlState.nativeLoadedLibraries.contains(path)) {
            LValue function = getNamedFunction(m_ftlState.module, symbol.data());
            if (!isInlinableSize(function)) {
                // We had no choice but to compile this function, but don't try to inline it ever again.
                m_ftlState.symbolTable.remove(symbol);
                return false;
            }
            return true;
        }

        LMemoryBuffer memBuf;
        
        ASSERT(isX86() || isARM64());

#if PLATFORM(EFL)
        const CString actualPath = toCString(bundlePath().data(), "/runtime/", path.data());
#else
        const CString actualPath = toCString(bundlePath().data(), 
            isX86() ? "/Resources/Runtime/x86_64/" : "/Resources/Runtime/arm64/",
            path.data());
#endif

        char* outMsg;
        
        if (createMemoryBufferWithContentsOfFile(actualPath.data(), &memBuf, &outMsg)) {
            if (Options::verboseFTLFailure())
                dataLog("Failed to load module at ", actualPath, "\n for symbol ", symbol, "\nERROR: ", outMsg, "\n");
            disposeMessage(outMsg);
            return false;
        }

        LModule module;

        if (parseBitcodeInContext(m_ftlState.context, memBuf, &module, &outMsg)) {
            if (Options::verboseFTLFailure())
                dataLog("Failed to parse module at ", actualPath, "\n for symbol ", symbol, "\nERROR: ", outMsg, "\n");
            disposeMemoryBuffer(memBuf);
            disposeMessage(outMsg);
            return false;
        }

        disposeMemoryBuffer(memBuf);

        if (LValue function = getNamedFunction(m_ftlState.module, symbol.data())) {
            if (!isInlinableSize(function)) {
                m_ftlState.symbolTable.remove(symbol);
                disposeModule(module);
                return false;
            }
        }

        Vector<CString> namedFunctions;
        for (LValue function = getFirstFunction(module); function; function = getNextFunction(function)) {
            CString functionName(getValueName(function));
            namedFunctions.append(functionName);
            
            for (LBasicBlock basicBlock = getFirstBasicBlock(function); basicBlock; basicBlock = getNextBasicBlock(basicBlock)) {
                for (LValue instruction = getFirstInstruction(basicBlock); instruction; instruction = getNextInstruction(instruction)) {
                    setMetadata(instruction, m_tbaaKind, nullptr);
                    setMetadata(instruction, m_tbaaStructKind, nullptr);
                }
            }
        }

        Vector<CString> namedGlobals;
        for (LValue global = getFirstGlobal(module); global; global = getNextGlobal(global)) {
            CString globalName(getValueName(global));
            namedGlobals.append(globalName);
        }

        if (linkModules(m_ftlState.module, module, LLVMLinkerDestroySource, &outMsg)) {
            if (Options::verboseFTLFailure())
                dataLog("Failed to link module at ", actualPath, "\n for symbol ", symbol, "\nERROR: ", outMsg, "\n");
            disposeMessage(outMsg);
            return false;
        }
        
        for (CString* symbol = namedFunctions.begin(); symbol != namedFunctions.end(); ++symbol) {
            LValue function = getNamedFunction(m_ftlState.module, symbol->data());
            LLVMLinkage linkage = getLinkage(function);
            if (linkage != LLVMInternalLinkage && linkage != LLVMPrivateLinkage)
                setVisibility(function, LLVMHiddenVisibility);
            if (!isDeclaration(function)) {
                setLinkage(function, LLVMPrivateLinkage);
                setLinkage(function, LLVMAvailableExternallyLinkage);

                if (ASSERT_DISABLED)
                    removeFunctionAttr(function, LLVMStackProtectAttribute);
            }
        }

        for (CString* symbol = namedGlobals.begin(); symbol != namedGlobals.end(); ++symbol) {
            LValue global = getNamedGlobal(m_ftlState.module, symbol->data());
            LLVMLinkage linkage = getLinkage(global);
            if (linkage != LLVMInternalLinkage && linkage != LLVMPrivateLinkage)
                setVisibility(global, LLVMHiddenVisibility);
            if (!isDeclaration(global))
                setLinkage(global, LLVMPrivateLinkage);
        }

        m_ftlState.nativeLoadedLibraries.add(path);
        return true;
    }
#endif

    bool isInlinableSize(LValue function)
    {
        size_t instructionCount = 0;
        size_t maxSize = Options::maximumLLVMInstructionCountForNativeInlining();
        for (LBasicBlock basicBlock = getFirstBasicBlock(function); basicBlock; basicBlock = getNextBasicBlock(basicBlock)) {
            for (LValue instruction = getFirstInstruction(basicBlock); instruction; instruction = getNextInstruction(instruction)) {
                if (++instructionCount >= maxSize)
                    return false;
            }
        }
        return true;
    }

    LValue didOverflowStack()
    {
        // This does a very simple leaf function analysis. The invariant of FTL call
        // frames is that the caller had already done enough of a stack check to
        // prove that this call frame has enough stack to run, and also enough stack
        // to make runtime calls. So, we only need to stack check when making calls
        // to other JS functions. If we don't find such calls then we don't need to
        // do any stack checks.
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.numBlocks(); ++blockIndex) {
            BasicBlock* block = m_graph.block(blockIndex);
            if (!block)
                continue;
            
            for (unsigned nodeIndex = block->size(); nodeIndex--;) {
                Node* node = block->at(nodeIndex);
                
                switch (node->op()) {
                case GetById:
                case PutById:
                case Call:
                case Construct:
                case NativeCall:
                case NativeConstruct:
                    return m_out.below(
                        m_callFrame,
                        m_out.loadPtr(
                            m_out.absolute(vm().addressOfFTLStackLimit())));
                    
                default:
                    break;
                }
            }
        }
        
        return m_out.booleanFalse;
    }
    
    struct ArgumentsLength {
        ArgumentsLength()
            : isKnown(false)
            , known(UINT_MAX)
            , value(nullptr)
        {
        }
        
        bool isKnown;
        unsigned known;
        LValue value;
    };
    ArgumentsLength getArgumentsLength(InlineCallFrame* inlineCallFrame)
    {
        ArgumentsLength length;

        if (inlineCallFrame && !inlineCallFrame->isVarargs()) {
            length.known = inlineCallFrame->arguments.size() - 1;
            length.isKnown = true;
            length.value = m_out.constInt32(length.known);
        } else {
            length.known = UINT_MAX;
            length.isKnown = false;
            
            VirtualRegister argumentCountRegister;
            if (!inlineCallFrame)
                argumentCountRegister = VirtualRegister(JSStack::ArgumentCount);
            else
                argumentCountRegister = inlineCallFrame->argumentCountRegister;
            length.value = m_out.sub(m_out.load32(payloadFor(argumentCountRegister)), m_out.int32One);
        }
        
        return length;
    }
    
    ArgumentsLength getArgumentsLength()
    {
        return getArgumentsLength(m_node->origin.semantic.inlineCallFrame);
    }
    
    LValue getCurrentCallee()
    {
        if (InlineCallFrame* frame = m_node->origin.semantic.inlineCallFrame) {
            if (frame->isClosureCall)
                return m_out.loadPtr(addressFor(frame->calleeRecovery.virtualRegister()));
            return weakPointer(frame->calleeRecovery.constant().asCell());
        }
        return m_out.loadPtr(addressFor(JSStack::Callee));
    }
    
    LValue getArgumentsStart(InlineCallFrame* inlineCallFrame)
    {
        VirtualRegister start = AssemblyHelpers::argumentsStart(inlineCallFrame);
        return addressFor(start).value();
    }
    
    LValue getArgumentsStart()
    {
        return getArgumentsStart(m_node->origin.semantic.inlineCallFrame);
    }
    
    template<typename Functor>
    void checkStructure(
        LValue structureDiscriminant, const FormattedValue& formattedValue, ExitKind exitKind,
        const StructureSet& set, const Functor& weakStructureDiscriminant)
    {
        if (set.size() == 1) {
            speculate(
                exitKind, formattedValue, 0,
                m_out.notEqual(structureDiscriminant, weakStructureDiscriminant(set[0])));
            return;
        }
        
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("checkStructure continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(continuation);
        for (unsigned i = 0; i < set.size() - 1; ++i) {
            LBasicBlock nextStructure = FTL_NEW_BLOCK(m_out, ("checkStructure nextStructure"));
            m_out.branch(
                m_out.equal(structureDiscriminant, weakStructureDiscriminant(set[i])),
                unsure(continuation), unsure(nextStructure));
            m_out.appendTo(nextStructure);
        }
        
        speculate(
            exitKind, formattedValue, 0,
            m_out.notEqual(structureDiscriminant, weakStructureDiscriminant(set.last())));
        
        m_out.jump(continuation);
        m_out.appendTo(continuation, lastNext);
    }
    
    LValue numberOrNotCellToInt32(Edge edge, LValue value)
    {
        LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 int case"));
        LBasicBlock notIntCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 not int case"));
        LBasicBlock doubleCase = 0;
        LBasicBlock notNumberCase = 0;
        if (edge.useKind() == NotCellUse) {
            doubleCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 double case"));
            notNumberCase = FTL_NEW_BLOCK(m_out, ("ValueToInt32 not number case"));
        }
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ValueToInt32 continuation"));
        
        Vector<ValueFromBlock> results;
        
        m_out.branch(isNotInt32(value), unsure(notIntCase), unsure(intCase));
        
        LBasicBlock lastNext = m_out.appendTo(intCase, notIntCase);
        results.append(m_out.anchor(unboxInt32(value)));
        m_out.jump(continuation);
        
        if (edge.useKind() == NumberUse) {
            m_out.appendTo(notIntCase, continuation);
            FTL_TYPE_CHECK(jsValueValue(value), edge, SpecBytecodeNumber, isCellOrMisc(value));
            results.append(m_out.anchor(doubleToInt32(unboxDouble(value))));
            m_out.jump(continuation);
        } else {
            m_out.appendTo(notIntCase, doubleCase);
            m_out.branch(
                isCellOrMisc(value, provenType(edge)), unsure(notNumberCase), unsure(doubleCase));
            
            m_out.appendTo(doubleCase, notNumberCase);
            results.append(m_out.anchor(doubleToInt32(unboxDouble(value))));
            m_out.jump(continuation);
            
            m_out.appendTo(notNumberCase, continuation);
            
            FTL_TYPE_CHECK(jsValueValue(value), edge, ~SpecCell, isCell(value));
            
            LValue specialResult = m_out.select(
                m_out.equal(value, m_out.constInt64(JSValue::encode(jsBoolean(true)))),
                m_out.int32One, m_out.int32Zero);
            results.append(m_out.anchor(specialResult));
            m_out.jump(continuation);
        }
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int32, results);
    }
    
    LValue loadProperty(LValue storage, unsigned identifierNumber, PropertyOffset offset)
    {
        return m_out.load64(addressOfProperty(storage, identifierNumber, offset));
    }
    
    void storeProperty(
        LValue value, LValue storage, unsigned identifierNumber, PropertyOffset offset)
    {
        m_out.store64(value, addressOfProperty(storage, identifierNumber, offset));
    }
    
    TypedPointer addressOfProperty(
        LValue storage, unsigned identifierNumber, PropertyOffset offset)
    {
        return m_out.address(
            m_heaps.properties[identifierNumber], storage, offsetRelativeToBase(offset));
    }
    
    LValue storageForTransition(
        LValue object, PropertyOffset offset,
        Structure* previousStructure, Structure* nextStructure)
    {
        if (isInlineOffset(offset))
            return object;
        
        if (previousStructure->outOfLineCapacity() == nextStructure->outOfLineCapacity())
            return m_out.loadPtr(object, m_heaps.JSObject_butterfly);
        
        LValue result;
        if (!previousStructure->outOfLineCapacity())
            result = allocatePropertyStorage(object, previousStructure);
        else {
            result = reallocatePropertyStorage(
                object, m_out.loadPtr(object, m_heaps.JSObject_butterfly),
                previousStructure, nextStructure);
        }
        
        emitStoreBarrier(object);
        
        return result;
    }
    
    LValue allocatePropertyStorage(LValue object, Structure* previousStructure)
    {
        if (previousStructure->couldHaveIndexingHeader()) {
            return vmCall(
                m_out.operation(
                    operationReallocateButterflyToHavePropertyStorageWithInitialCapacity),
                m_callFrame, object);
        }
        
        LValue result = allocatePropertyStorageWithSizeImpl(initialOutOfLineCapacity);
        m_out.storePtr(result, object, m_heaps.JSObject_butterfly);
        return result;
    }
    
    LValue reallocatePropertyStorage(
        LValue object, LValue oldStorage, Structure* previous, Structure* next)
    {
        size_t oldSize = previous->outOfLineCapacity();
        size_t newSize = oldSize * outOfLineGrowthFactor; 

        ASSERT_UNUSED(next, newSize == next->outOfLineCapacity());
        
        if (previous->couldHaveIndexingHeader()) {
            LValue newAllocSize = m_out.constIntPtr(newSize);                    
            return vmCall(m_out.operation(operationReallocateButterflyToGrowPropertyStorage), m_callFrame, object, newAllocSize);
        }
        
        LValue result = allocatePropertyStorageWithSizeImpl(newSize);

        ptrdiff_t headerSize = -sizeof(IndexingHeader) - sizeof(void*);
        ptrdiff_t endStorage = headerSize - static_cast<ptrdiff_t>(oldSize * sizeof(JSValue));

        for (ptrdiff_t offset = headerSize; offset > endStorage; offset -= sizeof(void*)) {
            LValue loaded = 
                m_out.loadPtr(m_out.address(m_heaps.properties.atAnyNumber(), oldStorage, offset));
            m_out.storePtr(loaded, m_out.address(m_heaps.properties.atAnyNumber(), result, offset));
        }
        
        m_out.storePtr(result, m_out.address(object, m_heaps.JSObject_butterfly));
        
        return result;
    }
    
    LValue allocatePropertyStorageWithSizeImpl(size_t sizeInValues)
    {
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("allocatePropertyStorageWithSizeImpl slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("allocatePropertyStorageWithSizeImpl continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        LValue endOfStorage = allocateBasicStorageAndGetEnd(
            m_out.constIntPtr(sizeInValues * sizeof(JSValue)), slowPath);
        
        ValueFromBlock fastButterfly = m_out.anchor(
            m_out.add(m_out.constIntPtr(sizeof(IndexingHeader)), endOfStorage));
        
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        LValue slowButterflyValue;
        if (sizeInValues == initialOutOfLineCapacity) {
            slowButterflyValue = vmCall(
                m_out.operation(operationAllocatePropertyStorageWithInitialCapacity),
                m_callFrame);
        } else {
            slowButterflyValue = vmCall(
                m_out.operation(operationAllocatePropertyStorage),
                m_callFrame, m_out.constIntPtr(sizeInValues));
        }
        ValueFromBlock slowButterfly = m_out.anchor(slowButterflyValue);
        
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        return m_out.phi(m_out.intPtr, fastButterfly, slowButterfly);
    }
    
    LValue getById(LValue base)
    {
        auto uid = m_graph.identifiers()[m_node->identifierNumber()];

        // Arguments: id, bytes, target, numArgs, args...
        unsigned stackmapID = m_stackmapIDs++;
        
        if (Options::verboseCompilation())
            dataLog("    Emitting GetById patchpoint with stackmap #", stackmapID, "\n");
        
        LValue call = m_out.call(
            m_out.patchpointInt64Intrinsic(),
            m_out.constInt64(stackmapID), m_out.constInt32(sizeOfGetById()),
            constNull(m_out.ref8), m_out.constInt32(1), base);
        setInstructionCallingConvention(call, LLVMAnyRegCallConv);
        
        m_ftlState.getByIds.append(GetByIdDescriptor(stackmapID, m_node->origin.semantic, uid));
        
        return call;
    }
    
    TypedPointer baseIndex(IndexedAbstractHeap& heap, LValue storage, LValue index, Edge edge, ptrdiff_t offset = 0)
    {
        return m_out.baseIndex(
            heap, storage, m_out.zeroExtPtr(index), provenValue(edge), offset);
    }
    
    void compare(
        LIntPredicate intCondition, LRealPredicate realCondition,
        S_JITOperation_EJJ helperFunction)
    {
        if (m_node->isBinaryUseKind(Int32Use)) {
            LValue left = lowInt32(m_node->child1());
            LValue right = lowInt32(m_node->child2());
            setBoolean(m_out.icmp(intCondition, left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(Int52RepUse)) {
            Int52Kind kind;
            LValue left = lowWhicheverInt52(m_node->child1(), kind);
            LValue right = lowInt52(m_node->child2(), kind);
            setBoolean(m_out.icmp(intCondition, left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(DoubleRepUse)) {
            LValue left = lowDouble(m_node->child1());
            LValue right = lowDouble(m_node->child2());
            setBoolean(m_out.fcmp(realCondition, left, right));
            return;
        }
        
        if (m_node->isBinaryUseKind(UntypedUse)) {
            nonSpeculativeCompare(intCondition, helperFunction);
            return;
        }
        
        DFG_CRASH(m_graph, m_node, "Bad use kinds");
    }
    
    void compareEqObjectOrOtherToObject(Edge leftChild, Edge rightChild)
    {
        LValue rightCell = lowCell(rightChild);
        LValue leftValue = lowJSValue(leftChild, ManualOperandSpeculation);
        
        speculateTruthyObject(rightChild, rightCell, SpecObject);
        
        LBasicBlock leftCellCase = FTL_NEW_BLOCK(m_out, ("CompareEqObjectOrOtherToObject left cell case"));
        LBasicBlock leftNotCellCase = FTL_NEW_BLOCK(m_out, ("CompareEqObjectOrOtherToObject left not cell case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CompareEqObjectOrOtherToObject continuation"));
        
        m_out.branch(
            isCell(leftValue, provenType(leftChild)),
            unsure(leftCellCase), unsure(leftNotCellCase));
        
        LBasicBlock lastNext = m_out.appendTo(leftCellCase, leftNotCellCase);
        speculateTruthyObject(leftChild, leftValue, SpecObject | (~SpecCell));
        ValueFromBlock cellResult = m_out.anchor(m_out.equal(rightCell, leftValue));
        m_out.jump(continuation);
        
        m_out.appendTo(leftNotCellCase, continuation);
        FTL_TYPE_CHECK(
            jsValueValue(leftValue), leftChild, SpecOther | SpecCell, isNotOther(leftValue));
        ValueFromBlock notCellResult = m_out.anchor(m_out.booleanFalse);
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setBoolean(m_out.phi(m_out.boolean, cellResult, notCellResult));
    }
    
    void speculateTruthyObject(Edge edge, LValue cell, SpeculatedType filter)
    {
        if (masqueradesAsUndefinedWatchpointIsStillValid()) {
            FTL_TYPE_CHECK(jsValueValue(cell), edge, filter, isNotObject(cell));
            return;
        }
        
        FTL_TYPE_CHECK(jsValueValue(cell), edge, filter, isNotObject(cell));
        speculate(
            BadType, jsValueValue(cell), edge.node(),
            m_out.testNonZero8(
                m_out.load8(cell, m_heaps.JSCell_typeInfoFlags),
                m_out.constInt8(MasqueradesAsUndefined)));
    }
    
    void nonSpeculativeCompare(LIntPredicate intCondition, S_JITOperation_EJJ helperFunction)
    {
        LValue left = lowJSValue(m_node->child1());
        LValue right = lowJSValue(m_node->child2());
        
        LBasicBlock leftIsInt = FTL_NEW_BLOCK(m_out, ("CompareEq untyped left is int"));
        LBasicBlock fastPath = FTL_NEW_BLOCK(m_out, ("CompareEq untyped fast path"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("CompareEq untyped slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("CompareEq untyped continuation"));
        
        m_out.branch(isNotInt32(left), rarely(slowPath), usually(leftIsInt));
        
        LBasicBlock lastNext = m_out.appendTo(leftIsInt, fastPath);
        m_out.branch(isNotInt32(right), rarely(slowPath), usually(fastPath));
        
        m_out.appendTo(fastPath, slowPath);
        ValueFromBlock fastResult = m_out.anchor(
            m_out.icmp(intCondition, unboxInt32(left), unboxInt32(right)));
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        ValueFromBlock slowResult = m_out.anchor(m_out.notNull(vmCall(
            m_out.operation(helperFunction), m_callFrame, left, right)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        setBoolean(m_out.phi(m_out.boolean, fastResult, slowResult));
    }

    LValue allocateCell(LValue allocator, LBasicBlock slowPath)
    {
        LBasicBlock success = FTL_NEW_BLOCK(m_out, ("object allocation success"));
        
        LValue result = m_out.loadPtr(
            allocator, m_heaps.MarkedAllocator_freeListHead);
        
        m_out.branch(m_out.notNull(result), usually(success), rarely(slowPath));
        
        m_out.appendTo(success);
        
        m_out.storePtr(
            m_out.loadPtr(result, m_heaps.JSCell_freeListNext),
            allocator, m_heaps.MarkedAllocator_freeListHead);
        
        return result;
    }
    
    void storeStructure(LValue object, Structure* structure)
    {
        m_out.store32(m_out.constInt32(structure->id()), object, m_heaps.JSCell_structureID);
        m_out.store32(
            m_out.constInt32(structure->objectInitializationBlob()),
            object, m_heaps.JSCell_usefulBytes);
    }

    LValue allocateCell(LValue allocator, Structure* structure, LBasicBlock slowPath)
    {
        LValue result = allocateCell(allocator, slowPath);
        storeStructure(result, structure);
        return result;
    }

    LValue allocateObject(
        LValue allocator, Structure* structure, LValue butterfly, LBasicBlock slowPath)
    {
        LValue result = allocateCell(allocator, structure, slowPath);
        m_out.storePtr(butterfly, result, m_heaps.JSObject_butterfly);
        return result;
    }
    
    template<typename ClassType>
    LValue allocateObject(
        size_t size, Structure* structure, LValue butterfly, LBasicBlock slowPath)
    {
        MarkedAllocator* allocator = &vm().heap.allocatorForObjectOfType<ClassType>(size);
        return allocateObject(m_out.constIntPtr(allocator), structure, butterfly, slowPath);
    }
    
    template<typename ClassType>
    LValue allocateObject(Structure* structure, LValue butterfly, LBasicBlock slowPath)
    {
        return allocateObject<ClassType>(
            ClassType::allocationSize(0), structure, butterfly, slowPath);
    }
    
    template<typename ClassType>
    LValue allocateVariableSizedObject(
        LValue size, Structure* structure, LValue butterfly, LBasicBlock slowPath)
    {
        static_assert(!(MarkedSpace::preciseStep & (MarkedSpace::preciseStep - 1)), "MarkedSpace::preciseStep must be a power of two.");
        static_assert(!(MarkedSpace::impreciseStep & (MarkedSpace::impreciseStep - 1)), "MarkedSpace::impreciseStep must be a power of two.");

        LValue subspace = m_out.constIntPtr(&vm().heap.subspaceForObjectOfType<ClassType>());
        
        LBasicBlock smallCaseBlock = FTL_NEW_BLOCK(m_out, ("allocateVariableSizedObject small case"));
        LBasicBlock largeOrOversizeCaseBlock = FTL_NEW_BLOCK(m_out, ("allocateVariableSizedObject large or oversize case"));
        LBasicBlock largeCaseBlock = FTL_NEW_BLOCK(m_out, ("allocateVariableSizedObject large case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("allocateVariableSizedObject continuation"));
        
        LValue uproundedSize = m_out.add(size, m_out.constInt32(MarkedSpace::preciseStep - 1));
        LValue isSmall = m_out.below(uproundedSize, m_out.constInt32(MarkedSpace::preciseCutoff));
        m_out.branch(isSmall, unsure(smallCaseBlock), unsure(largeOrOversizeCaseBlock));
        
        LBasicBlock lastNext = m_out.appendTo(smallCaseBlock, largeOrOversizeCaseBlock);
        TypedPointer address = m_out.baseIndex(
            m_heaps.MarkedSpace_Subspace_preciseAllocators, subspace,
            m_out.zeroExtPtr(m_out.lShr(uproundedSize, m_out.constInt32(getLSBSet(MarkedSpace::preciseStep)))));
        ValueFromBlock smallAllocator = m_out.anchor(address.value());
        m_out.jump(continuation);
        
        m_out.appendTo(largeOrOversizeCaseBlock, largeCaseBlock);
        m_out.branch(
            m_out.below(uproundedSize, m_out.constInt32(MarkedSpace::impreciseCutoff)),
            usually(largeCaseBlock), rarely(slowPath));
        
        m_out.appendTo(largeCaseBlock, continuation);
        address = m_out.baseIndex(
            m_heaps.MarkedSpace_Subspace_impreciseAllocators, subspace,
            m_out.zeroExtPtr(m_out.lShr(uproundedSize, m_out.constInt32(getLSBSet(MarkedSpace::impreciseStep)))));
        ValueFromBlock largeAllocator = m_out.anchor(address.value());
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        LValue allocator = m_out.phi(m_out.intPtr, smallAllocator, largeAllocator);
        
        return allocateObject(allocator, structure, butterfly, slowPath);
    }
    
    // Returns a pointer to the end of the allocation.
    LValue allocateBasicStorageAndGetEnd(LValue size, LBasicBlock slowPath)
    {
        CopiedAllocator& allocator = vm().heap.storageAllocator();
        
        LBasicBlock success = FTL_NEW_BLOCK(m_out, ("storage allocation success"));
        
        LValue remaining = m_out.loadPtr(m_out.absolute(&allocator.m_currentRemaining));
        LValue newRemaining = m_out.sub(remaining, size);
        
        m_out.branch(
            m_out.lessThan(newRemaining, m_out.intPtrZero),
            rarely(slowPath), usually(success));
        
        m_out.appendTo(success);
        
        m_out.storePtr(newRemaining, m_out.absolute(&allocator.m_currentRemaining));
        return m_out.sub(
            m_out.loadPtr(m_out.absolute(&allocator.m_currentPayloadEnd)), newRemaining);
    }
    
    LValue allocateObject(Structure* structure)
    {
        size_t allocationSize = JSFinalObject::allocationSize(structure->inlineCapacity());
        MarkedAllocator* allocator = &vm().heap.allocatorForObjectWithoutDestructor(allocationSize);
        
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("allocateObject slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("allocateObject continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        ValueFromBlock fastResult = m_out.anchor(allocateObject(
            m_out.constIntPtr(allocator), structure, m_out.intPtrZero, slowPath));
        
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        ValueFromBlock slowResult = m_out.anchor(vmCall(
            m_out.operation(operationNewObject), m_callFrame, m_out.constIntPtr(structure)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.intPtr, fastResult, slowResult);
    }
    
    struct ArrayValues {
        ArrayValues()
            : array(0)
            , butterfly(0)
        {
        }
        
        ArrayValues(LValue array, LValue butterfly)
            : array(array)
            , butterfly(butterfly)
        {
        }
        
        LValue array;
        LValue butterfly;
    };
    ArrayValues allocateJSArray(
        Structure* structure, unsigned numElements, LBasicBlock slowPath)
    {
        ASSERT(
            hasUndecided(structure->indexingType())
            || hasInt32(structure->indexingType())
            || hasDouble(structure->indexingType())
            || hasContiguous(structure->indexingType()));
        
        unsigned vectorLength = std::max(BASE_VECTOR_LEN, numElements);
        
        LValue endOfStorage = allocateBasicStorageAndGetEnd(
            m_out.constIntPtr(sizeof(JSValue) * vectorLength + sizeof(IndexingHeader)),
            slowPath);
        
        LValue butterfly = m_out.sub(
            endOfStorage, m_out.constIntPtr(sizeof(JSValue) * vectorLength));
        
        LValue object = allocateObject<JSArray>(
            structure, butterfly, slowPath);
        
        m_out.store32(m_out.constInt32(numElements), butterfly, m_heaps.Butterfly_publicLength);
        m_out.store32(m_out.constInt32(vectorLength), butterfly, m_heaps.Butterfly_vectorLength);
        
        if (hasDouble(structure->indexingType())) {
            for (unsigned i = numElements; i < vectorLength; ++i) {
                m_out.store64(
                    m_out.constInt64(bitwise_cast<int64_t>(PNaN)),
                    butterfly, m_heaps.indexedDoubleProperties[i]);
            }
        }
        
        return ArrayValues(object, butterfly);
    }
    
    ArrayValues allocateJSArray(Structure* structure, unsigned numElements)
    {
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("JSArray allocation slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("JSArray allocation continuation"));
        
        LBasicBlock lastNext = m_out.insertNewBlocksBefore(slowPath);
        
        ArrayValues fastValues = allocateJSArray(structure, numElements, slowPath);
        ValueFromBlock fastArray = m_out.anchor(fastValues.array);
        ValueFromBlock fastButterfly = m_out.anchor(fastValues.butterfly);
        
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        
        ValueFromBlock slowArray = m_out.anchor(vmCall(
            m_out.operation(operationNewArrayWithSize), m_callFrame,
            m_out.constIntPtr(structure), m_out.constInt32(numElements)));
        ValueFromBlock slowButterfly = m_out.anchor(
            m_out.loadPtr(slowArray.value(), m_heaps.JSObject_butterfly));

        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        return ArrayValues(
            m_out.phi(m_out.intPtr, fastArray, slowArray),
            m_out.phi(m_out.intPtr, fastButterfly, slowButterfly));
    }
    
    LValue typedArrayLength(Edge baseEdge, ArrayMode arrayMode, LValue base)
    {
        JSArrayBufferView* view = m_graph.tryGetFoldableView(provenValue(baseEdge), arrayMode);
        if (view)
            return m_out.constInt32(view->length());
        return m_out.load32NonNegative(base, m_heaps.JSArrayBufferView_length);
    }
    
    LValue typedArrayLength(Edge baseEdge, ArrayMode arrayMode)
    {
        return typedArrayLength(baseEdge, arrayMode, lowCell(baseEdge));
    }
    
    LValue boolify(Edge edge)
    {
        switch (edge.useKind()) {
        case BooleanUse:
            return lowBoolean(edge);
        case Int32Use:
            return m_out.notZero32(lowInt32(edge));
        case DoubleRepUse:
            return m_out.doubleNotEqual(lowDouble(edge), m_out.doubleZero);
        case ObjectOrOtherUse:
            return m_out.bitNot(
                equalNullOrUndefined(
                    edge, CellCaseSpeculatesObject, SpeculateNullOrUndefined,
                    ManualOperandSpeculation));
        case StringUse: {
            LValue stringValue = lowString(edge);
            LValue length = m_out.load32NonNegative(stringValue, m_heaps.JSString_length);
            return m_out.notEqual(length, m_out.int32Zero);
        }
        case UntypedUse: {
            LValue value = lowJSValue(edge);
            
            // Implements the following control flow structure:
            // if (value is cell) {
            //     if (value is string)
            //         result = !!value->length
            //     else {
            //         do evil things for masquerades-as-undefined
            //         result = true
            //     }
            // } else if (value is int32) {
            //     result = !!unboxInt32(value)
            // } else if (value is number) {
            //     result = !!unboxDouble(value)
            // } else {
            //     result = value == jsTrue
            // }
            
            LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped cell case"));
            LBasicBlock stringCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped string case"));
            LBasicBlock notStringCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped not string case"));
            LBasicBlock notCellCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped not cell case"));
            LBasicBlock int32Case = FTL_NEW_BLOCK(m_out, ("Boolify untyped int32 case"));
            LBasicBlock notInt32Case = FTL_NEW_BLOCK(m_out, ("Boolify untyped not int32 case"));
            LBasicBlock doubleCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped double case"));
            LBasicBlock notDoubleCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped not double case"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Boolify untyped continuation"));
            
            Vector<ValueFromBlock> results;
            
            m_out.branch(isCell(value, provenType(edge)), unsure(cellCase), unsure(notCellCase));
            
            LBasicBlock lastNext = m_out.appendTo(cellCase, stringCase);
            m_out.branch(
                isString(value, provenType(edge) & SpecCell),
                unsure(stringCase), unsure(notStringCase));
            
            m_out.appendTo(stringCase, notStringCase);
            LValue nonEmptyString = m_out.notZero32(
                m_out.load32NonNegative(value, m_heaps.JSString_length));
            results.append(m_out.anchor(nonEmptyString));
            m_out.jump(continuation);
            
            m_out.appendTo(notStringCase, notCellCase);
            LValue isTruthyObject;
            if (masqueradesAsUndefinedWatchpointIsStillValid())
                isTruthyObject = m_out.booleanTrue;
            else {
                LBasicBlock masqueradesCase = FTL_NEW_BLOCK(m_out, ("Boolify untyped masquerades case"));
                
                results.append(m_out.anchor(m_out.booleanTrue));
                
                m_out.branch(
                    m_out.testIsZero8(
                        m_out.load8(value, m_heaps.JSCell_typeInfoFlags),
                        m_out.constInt8(MasqueradesAsUndefined)),
                    usually(continuation), rarely(masqueradesCase));
                
                m_out.appendTo(masqueradesCase);
                
                isTruthyObject = m_out.notEqual(
                    m_out.constIntPtr(m_graph.globalObjectFor(m_node->origin.semantic)),
                    m_out.loadPtr(loadStructure(value), m_heaps.Structure_globalObject));
            }
            results.append(m_out.anchor(isTruthyObject));
            m_out.jump(continuation);
            
            m_out.appendTo(notCellCase, int32Case);
            m_out.branch(
                isInt32(value, provenType(edge) & ~SpecCell),
                unsure(int32Case), unsure(notInt32Case));
            
            m_out.appendTo(int32Case, notInt32Case);
            results.append(m_out.anchor(m_out.notZero32(unboxInt32(value))));
            m_out.jump(continuation);
            
            m_out.appendTo(notInt32Case, doubleCase);
            m_out.branch(
                isNumber(value, provenType(edge) & ~SpecCell),
                unsure(doubleCase), unsure(notDoubleCase));
            
            m_out.appendTo(doubleCase, notDoubleCase);
            // Note that doubleNotEqual() really means not-equal-and-ordered. It will return false
            // if value is NaN.
            LValue doubleIsTruthy = m_out.doubleNotEqual(
                unboxDouble(value), m_out.constDouble(0));
            results.append(m_out.anchor(doubleIsTruthy));
            m_out.jump(continuation);
            
            m_out.appendTo(notDoubleCase, continuation);
            LValue miscIsTruthy = m_out.equal(
                value, m_out.constInt64(JSValue::encode(jsBoolean(true))));
            results.append(m_out.anchor(miscIsTruthy));
            m_out.jump(continuation);
            
            m_out.appendTo(continuation, lastNext);
            return m_out.phi(m_out.boolean, results);
        }
        default:
            DFG_CRASH(m_graph, m_node, "Bad use kind");
            return 0;
        }
    }
    
    enum StringOrObjectMode {
        AllCellsAreFalse,
        CellCaseSpeculatesObject
    };
    enum EqualNullOrUndefinedMode {
        EqualNull,
        EqualUndefined,
        EqualNullOrUndefined,
        SpeculateNullOrUndefined
    };
    LValue equalNullOrUndefined(
        Edge edge, StringOrObjectMode cellMode, EqualNullOrUndefinedMode primitiveMode,
        OperandSpeculationMode operandMode = AutomaticOperandSpeculation)
    {
        bool validWatchpoint = masqueradesAsUndefinedWatchpointIsStillValid();
        
        LValue value = lowJSValue(edge, operandMode);
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined cell case"));
        LBasicBlock primitiveCase = FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined primitive case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined continuation"));
        
        m_out.branch(isNotCell(value, provenType(edge)), unsure(primitiveCase), unsure(cellCase));
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, primitiveCase);
        
        Vector<ValueFromBlock, 3> results;
        
        switch (cellMode) {
        case AllCellsAreFalse:
            break;
        case CellCaseSpeculatesObject:
            FTL_TYPE_CHECK(
                jsValueValue(value), edge, (~SpecCell) | SpecObject, isNotObject(value));
            break;
        }
        
        if (validWatchpoint) {
            results.append(m_out.anchor(m_out.booleanFalse));
            m_out.jump(continuation);
        } else {
            LBasicBlock masqueradesCase =
                FTL_NEW_BLOCK(m_out, ("EqualNullOrUndefined masquerades case"));
                
            results.append(m_out.anchor(m_out.booleanFalse));
            
            m_out.branch(
                m_out.testNonZero8(
                    m_out.load8(value, m_heaps.JSCell_typeInfoFlags),
                    m_out.constInt8(MasqueradesAsUndefined)),
                rarely(masqueradesCase), usually(continuation));
            
            m_out.appendTo(masqueradesCase, primitiveCase);
            
            LValue structure = loadStructure(value);
            
            results.append(m_out.anchor(
                m_out.equal(
                    m_out.constIntPtr(m_graph.globalObjectFor(m_node->origin.semantic)),
                    m_out.loadPtr(structure, m_heaps.Structure_globalObject))));
            m_out.jump(continuation);
        }
        
        m_out.appendTo(primitiveCase, continuation);
        
        LValue primitiveResult;
        switch (primitiveMode) {
        case EqualNull:
            primitiveResult = m_out.equal(value, m_out.constInt64(ValueNull));
            break;
        case EqualUndefined:
            primitiveResult = m_out.equal(value, m_out.constInt64(ValueUndefined));
            break;
        case EqualNullOrUndefined:
            primitiveResult = isOther(value, provenType(edge));
            break;
        case SpeculateNullOrUndefined:
            FTL_TYPE_CHECK(
                jsValueValue(value), edge, SpecCell | SpecOther, isNotOther(value));
            primitiveResult = m_out.booleanTrue;
            break;
        }
        results.append(m_out.anchor(primitiveResult));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        return m_out.phi(m_out.boolean, results);
    }
    
    template<typename FunctionType>
    void contiguousPutByValOutOfBounds(
        FunctionType slowPathFunction, LValue base, LValue storage, LValue index, LValue value,
        LBasicBlock continuation)
    {
        LValue isNotInBounds = m_out.aboveOrEqual(
            index, m_out.load32NonNegative(storage, m_heaps.Butterfly_publicLength));
        if (!m_node->arrayMode().isInBounds()) {
            LBasicBlock notInBoundsCase =
                FTL_NEW_BLOCK(m_out, ("PutByVal not in bounds"));
            LBasicBlock performStore =
                FTL_NEW_BLOCK(m_out, ("PutByVal perform store"));
                
            m_out.branch(isNotInBounds, unsure(notInBoundsCase), unsure(performStore));
                
            LBasicBlock lastNext = m_out.appendTo(notInBoundsCase, performStore);
                
            LValue isOutOfBounds = m_out.aboveOrEqual(
                index, m_out.load32NonNegative(storage, m_heaps.Butterfly_vectorLength));
                
            if (!m_node->arrayMode().isOutOfBounds())
                speculate(OutOfBounds, noValue(), 0, isOutOfBounds);
            else {
                LBasicBlock outOfBoundsCase =
                    FTL_NEW_BLOCK(m_out, ("PutByVal out of bounds"));
                LBasicBlock holeCase =
                    FTL_NEW_BLOCK(m_out, ("PutByVal hole case"));
                    
                m_out.branch(isOutOfBounds, unsure(outOfBoundsCase), unsure(holeCase));
                    
                LBasicBlock innerLastNext = m_out.appendTo(outOfBoundsCase, holeCase);
                    
                vmCall(
                    m_out.operation(slowPathFunction),
                    m_callFrame, base, index, value);
                    
                m_out.jump(continuation);
                    
                m_out.appendTo(holeCase, innerLastNext);
            }
            
            m_out.store32(
                m_out.add(index, m_out.int32One),
                storage, m_heaps.Butterfly_publicLength);
                
            m_out.jump(performStore);
            m_out.appendTo(performStore, lastNext);
        }
    }
    
    void buildSwitch(SwitchData* data, LType type, LValue switchValue)
    {
        Vector<SwitchCase> cases;
        for (unsigned i = 0; i < data->cases.size(); ++i) {
            cases.append(SwitchCase(
                constInt(type, data->cases[i].value.switchLookupValue(data->kind)),
                lowBlock(data->cases[i].target.block), Weight(data->cases[i].target.count)));
        }
        
        m_out.switchInstruction(
            switchValue, cases,
            lowBlock(data->fallThrough.block), Weight(data->fallThrough.count));
    }
    
    void switchString(SwitchData* data, LValue string)
    {
        bool canDoBinarySwitch = true;
        unsigned totalLength = 0;
        
        for (DFG::SwitchCase myCase : data->cases) {
            StringImpl* string = myCase.value.stringImpl();
            if (!string->is8Bit()) {
                canDoBinarySwitch = false;
                break;
            }
            if (string->length() > Options::maximumBinaryStringSwitchCaseLength()) {
                canDoBinarySwitch = false;
                break;
            }
            totalLength += string->length();
        }
        
        if (!canDoBinarySwitch || totalLength > Options::maximumBinaryStringSwitchTotalLength()) {
            switchStringSlow(data, string);
            return;
        }
        
        LValue stringImpl = m_out.loadPtr(string, m_heaps.JSString_value);
        LValue length = m_out.load32(string, m_heaps.JSString_length);
        
        LBasicBlock hasImplBlock = FTL_NEW_BLOCK(m_out, ("Switch/SwitchString has impl case"));
        LBasicBlock is8BitBlock = FTL_NEW_BLOCK(m_out, ("Switch/SwitchString is 8 bit case"));
        LBasicBlock slowBlock = FTL_NEW_BLOCK(m_out, ("Switch/SwitchString slow case"));
        
        m_out.branch(m_out.isNull(stringImpl), unsure(slowBlock), unsure(hasImplBlock));
        
        LBasicBlock lastNext = m_out.appendTo(hasImplBlock, is8BitBlock);
        
        m_out.branch(
            m_out.testIsZero32(
                m_out.load32(stringImpl, m_heaps.StringImpl_hashAndFlags),
                m_out.constInt32(StringImpl::flagIs8Bit())),
            unsure(slowBlock), unsure(is8BitBlock));
        
        m_out.appendTo(is8BitBlock, slowBlock);
        
        LValue buffer = m_out.loadPtr(stringImpl, m_heaps.StringImpl_data);
        
        // FIXME: We should propagate branch weight data to the cases of this switch.
        // https://bugs.webkit.org/show_bug.cgi?id=144368
        
        Vector<StringSwitchCase> cases;
        for (DFG::SwitchCase myCase : data->cases)
            cases.append(StringSwitchCase(myCase.value.stringImpl(), lowBlock(myCase.target.block)));
        std::sort(cases.begin(), cases.end());
        switchStringRecurse(data, buffer, length, cases, 0, 0, cases.size(), 0, false);

        m_out.appendTo(slowBlock, lastNext);
        switchStringSlow(data, string);
    }
    
    // The code for string switching is based closely on the same code in the DFG backend. While it
    // would be nice to reduce the amount of similar-looking code, it seems like this is one of
    // those algorithms where factoring out the common bits would result in more code than just
    // duplicating.
    
    struct StringSwitchCase {
        StringSwitchCase() { }
        
        StringSwitchCase(StringImpl* string, LBasicBlock target)
            : string(string)
            , target(target)
        {
        }

        bool operator<(const StringSwitchCase& other) const
        {
            return stringLessThan(*string, *other.string);
        }
        
        StringImpl* string;
        LBasicBlock target;
    };
    
    struct CharacterCase {
        CharacterCase()
            : character(0)
            , begin(0)
            , end(0)
        {
        }
        
        CharacterCase(LChar character, unsigned begin, unsigned end)
            : character(character)
            , begin(begin)
            , end(end)
        {
        }
        
        bool operator<(const CharacterCase& other) const
        {
            return character < other.character;
        }
        
        LChar character;
        unsigned begin;
        unsigned end;
    };
    
    void switchStringRecurse(
        SwitchData* data, LValue buffer, LValue length, const Vector<StringSwitchCase>& cases,
        unsigned numChecked, unsigned begin, unsigned end, unsigned alreadyCheckedLength,
        unsigned checkedExactLength)
    {
        LBasicBlock fallThrough = lowBlock(data->fallThrough.block);
        
        if (begin == end) {
            m_out.jump(fallThrough);
            return;
        }
        
        unsigned minLength = cases[begin].string->length();
        unsigned commonChars = minLength;
        bool allLengthsEqual = true;
        for (unsigned i = begin + 1; i < end; ++i) {
            unsigned myCommonChars = numChecked;
            unsigned limit = std::min(cases[begin].string->length(), cases[i].string->length());
            for (unsigned j = numChecked; j < limit; ++j) {
                if (cases[begin].string->at(j) != cases[i].string->at(j))
                    break;
                myCommonChars++;
            }
            commonChars = std::min(commonChars, myCommonChars);
            if (minLength != cases[i].string->length())
                allLengthsEqual = false;
            minLength = std::min(minLength, cases[i].string->length());
        }
        
        if (checkedExactLength) {
            DFG_ASSERT(m_graph, m_node, alreadyCheckedLength == minLength);
            DFG_ASSERT(m_graph, m_node, allLengthsEqual);
        }
        
        DFG_ASSERT(m_graph, m_node, minLength >= commonChars);
        
        if (!allLengthsEqual && alreadyCheckedLength < minLength)
            m_out.check(m_out.below(length, m_out.constInt32(minLength)), unsure(fallThrough));
        if (allLengthsEqual && (alreadyCheckedLength < minLength || !checkedExactLength))
            m_out.check(m_out.notEqual(length, m_out.constInt32(minLength)), unsure(fallThrough));
        
        for (unsigned i = numChecked; i < commonChars; ++i) {
            m_out.check(
                m_out.notEqual(
                    m_out.load8(buffer, m_heaps.characters8[i]),
                    m_out.constInt8(cases[begin].string->at(i))),
                unsure(fallThrough));
        }
        
        if (minLength == commonChars) {
            // This is the case where one of the cases is a prefix of all of the other cases.
            // We've already checked that the input string is a prefix of all of the cases,
            // so we just check length to jump to that case.
            
            DFG_ASSERT(m_graph, m_node, cases[begin].string->length() == commonChars);
            for (unsigned i = begin + 1; i < end; ++i)
                DFG_ASSERT(m_graph, m_node, cases[i].string->length() > commonChars);
            
            if (allLengthsEqual) {
                DFG_ASSERT(m_graph, m_node, end == begin + 1);
                m_out.jump(cases[begin].target);
                return;
            }
            
            m_out.check(
                m_out.equal(length, m_out.constInt32(commonChars)),
                unsure(cases[begin].target));
            
            // We've checked if the length is >= minLength, and then we checked if the length is
            // == commonChars. We get to this point if it is >= minLength but not == commonChars.
            // Hence we know that it now must be > minLength, i.e. that it's >= minLength + 1.
            switchStringRecurse(
                data, buffer, length, cases, commonChars, begin + 1, end, minLength + 1, false);
            return;
        }
        
        // At this point we know that the string is longer than commonChars, and we've only verified
        // commonChars. Use a binary switch on the next unchecked character, i.e.
        // string[commonChars].
        
        DFG_ASSERT(m_graph, m_node, end >= begin + 2);
        
        LValue uncheckedChar = m_out.load8(buffer, m_heaps.characters8[commonChars]);
        
        Vector<CharacterCase> characterCases;
        CharacterCase currentCase(cases[begin].string->at(commonChars), begin, begin + 1);
        for (unsigned i = begin + 1; i < end; ++i) {
            LChar currentChar = cases[i].string->at(commonChars);
            if (currentChar != currentCase.character) {
                currentCase.end = i;
                characterCases.append(currentCase);
                currentCase = CharacterCase(currentChar, i, i + 1);
            } else
                currentCase.end = i + 1;
        }
        characterCases.append(currentCase);
        
        Vector<LBasicBlock> characterBlocks;
        for (CharacterCase& myCase : characterCases)
            characterBlocks.append(FTL_NEW_BLOCK(m_out, ("Switch/SwitchString case for ", myCase.character, " at index ", commonChars)));
        
        Vector<SwitchCase> switchCases;
        for (unsigned i = 0; i < characterCases.size(); ++i) {
            if (i)
                DFG_ASSERT(m_graph, m_node, characterCases[i - 1].character < characterCases[i].character);
            switchCases.append(SwitchCase(
                m_out.constInt8(characterCases[i].character), characterBlocks[i], Weight()));
        }
        m_out.switchInstruction(uncheckedChar, switchCases, fallThrough, Weight());
        
        LBasicBlock lastNext = m_out.m_nextBlock;
        characterBlocks.append(lastNext); // Makes it convenient to set nextBlock.
        for (unsigned i = 0; i < characterCases.size(); ++i) {
            m_out.appendTo(characterBlocks[i], characterBlocks[i + 1]);
            switchStringRecurse(
                data, buffer, length, cases, commonChars + 1,
                characterCases[i].begin, characterCases[i].end, minLength, allLengthsEqual);
        }
        
        DFG_ASSERT(m_graph, m_node, m_out.m_nextBlock == lastNext);
    }
    
    void switchStringSlow(SwitchData* data, LValue string)
    {
        // FIXME: We ought to be able to use computed gotos here. We would save the labels of the
        // blocks we want to jump to, and then request their addresses after compilation completes.
        // https://bugs.webkit.org/show_bug.cgi?id=144369
        
        LValue branchOffset = vmCall(
            m_out.operation(operationSwitchStringAndGetBranchOffset),
            m_callFrame, m_out.constIntPtr(data->switchTableIndex), string);
        
        StringJumpTable& table = codeBlock()->stringSwitchJumpTable(data->switchTableIndex);
        
        Vector<SwitchCase> cases;
        std::unordered_set<int32_t> alreadyHandled; // These may be negative, or zero, or probably other stuff, too. We don't want to mess with HashSet's corner cases and we don't really care about throughput here.
        for (unsigned i = 0; i < data->cases.size(); ++i) {
            // FIXME: The fact that we're using the bytecode's switch table means that the
            // following DFG IR transformation would be invalid.
            //
            // Original code:
            //     switch (v) {
            //     case "foo":
            //     case "bar":
            //         things();
            //         break;
            //     default:
            //         break;
            //     }
            //
            // New code:
            //     switch (v) {
            //     case "foo":
            //         instrumentFoo();
            //         goto _things;
            //     case "bar":
            //         instrumentBar();
            //     _things:
            //         things();
            //         break;
            //     default:
            //         break;
            //     }
            //
            // Luckily, we don't currently do any such transformation. But it's kind of silly that
            // this is an issue.
            // https://bugs.webkit.org/show_bug.cgi?id=144635
            
            DFG::SwitchCase myCase = data->cases[i];
            StringJumpTable::StringOffsetTable::iterator iter =
                table.offsetTable.find(myCase.value.stringImpl());
            DFG_ASSERT(m_graph, m_node, iter != table.offsetTable.end());
            
            if (!alreadyHandled.insert(iter->value.branchOffset).second)
                continue;

            cases.append(SwitchCase(
                m_out.constInt32(iter->value.branchOffset),
                lowBlock(myCase.target.block), Weight(myCase.target.count)));
        }
        
        m_out.switchInstruction(
            branchOffset, cases, lowBlock(data->fallThrough.block),
            Weight(data->fallThrough.count));
    }
    
    // Calls the functor at the point of code generation where we know what the result type is.
    // You can emit whatever code you like at that point. Expects you to terminate the basic block.
    // When buildTypeOf() returns, it will have terminated all basic blocks that it created. So, if
    // you aren't using this as the terminator of a high-level block, you should create your own
    // contination and set it as the nextBlock (m_out.insertNewBlocksBefore(continuation)) before
    // calling this. For example:
    //
    // LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("My continuation"));
    // LBasicBlock lastNext = m_out.insertNewBlocksBefore(continuation);
    // buildTypeOf(
    //     child, value,
    //     [&] (TypeofType type) {
    //          do things;
    //          m_out.jump(continuation);
    //     });
    // m_out.appendTo(continuation, lastNext);
    template<typename Functor>
    void buildTypeOf(Edge child, LValue value, const Functor& functor)
    {
        JSGlobalObject* globalObject = m_graph.globalObjectFor(m_node->origin.semantic);
        
        // Implements the following branching structure:
        //
        // if (is cell) {
        //     if (is object) {
        //         if (is function) {
        //             return function;
        //         } else if (doesn't have call trap and doesn't masquerade as undefined) {
        //             return object
        //         } else {
        //             return slowPath();
        //         }
        //     } else if (is string) {
        //         return string
        //     } else {
        //         return symbol
        //     }
        // } else if (is number) {
        //     return number
        // } else if (is null) {
        //     return object
        // } else if (is boolean) {
        //     return boolean
        // } else {
        //     return undefined
        // }
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf cell case"));
        LBasicBlock objectCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf object case"));
        LBasicBlock functionCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf function case"));
        LBasicBlock notFunctionCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf not function case"));
        LBasicBlock reallyObjectCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf really object case"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("buildTypeOf slow path"));
        LBasicBlock unreachable = FTL_NEW_BLOCK(m_out, ("buildTypeOf unreachable"));
        LBasicBlock notObjectCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf not object case"));
        LBasicBlock stringCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf string case"));
        LBasicBlock symbolCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf symbol case"));
        LBasicBlock notCellCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf not cell case"));
        LBasicBlock numberCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf number case"));
        LBasicBlock notNumberCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf not number case"));
        LBasicBlock notNullCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf not null case"));
        LBasicBlock booleanCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf boolean case"));
        LBasicBlock undefinedCase = FTL_NEW_BLOCK(m_out, ("buildTypeOf undefined case"));
        
        m_out.branch(isCell(value, provenType(child)), unsure(cellCase), unsure(notCellCase));
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, objectCase);
        m_out.branch(isObject(value, provenType(child)), unsure(objectCase), unsure(notObjectCase));
        
        m_out.appendTo(objectCase, functionCase);
        m_out.branch(
            isFunction(value, provenType(child) & SpecObject),
            unsure(functionCase), unsure(notFunctionCase));
        
        m_out.appendTo(functionCase, notFunctionCase);
        functor(TypeofType::Function);
        
        m_out.appendTo(notFunctionCase, reallyObjectCase);
        m_out.branch(
            isExoticForTypeof(value, provenType(child) & (SpecObject - SpecFunction)),
            rarely(slowPath), usually(reallyObjectCase));
        
        m_out.appendTo(reallyObjectCase, slowPath);
        functor(TypeofType::Object);
        
        m_out.appendTo(slowPath, unreachable);
        LValue result = vmCall(
            m_out.operation(operationTypeOfObjectAsTypeofType), m_callFrame,
            weakPointer(globalObject), value);
        Vector<SwitchCase, 3> cases;
        cases.append(SwitchCase(m_out.constInt32(static_cast<int32_t>(TypeofType::Undefined)), undefinedCase));
        cases.append(SwitchCase(m_out.constInt32(static_cast<int32_t>(TypeofType::Object)), reallyObjectCase));
        cases.append(SwitchCase(m_out.constInt32(static_cast<int32_t>(TypeofType::Function)), functionCase));
        m_out.switchInstruction(result, cases, unreachable, Weight());
        
        m_out.appendTo(unreachable, notObjectCase);
        m_out.unreachable();
        
        m_out.appendTo(notObjectCase, stringCase);
        m_out.branch(
            isString(value, provenType(child) & (SpecCell - SpecObject)),
            unsure(stringCase), unsure(symbolCase));
        
        m_out.appendTo(stringCase, symbolCase);
        functor(TypeofType::String);
        
        m_out.appendTo(symbolCase, notCellCase);
        functor(TypeofType::Symbol);
        
        m_out.appendTo(notCellCase, numberCase);
        m_out.branch(
            isNumber(value, provenType(child) & ~SpecCell),
            unsure(numberCase), unsure(notNumberCase));
        
        m_out.appendTo(numberCase, notNumberCase);
        functor(TypeofType::Number);
        
        m_out.appendTo(notNumberCase, notNullCase);
        LValue isNull;
        if (provenType(child) & SpecOther)
            isNull = m_out.equal(value, m_out.constInt64(ValueNull));
        else
            isNull = m_out.booleanFalse;
        m_out.branch(isNull, unsure(reallyObjectCase), unsure(notNullCase));
        
        m_out.appendTo(notNullCase, booleanCase);
        m_out.branch(
            isBoolean(value, provenType(child) & ~(SpecCell | SpecFullNumber)),
            unsure(booleanCase), unsure(undefinedCase));
        
        m_out.appendTo(booleanCase, undefinedCase);
        functor(TypeofType::Boolean);
        
        m_out.appendTo(undefinedCase, lastNext);
        functor(TypeofType::Undefined);
    }
    
    LValue doubleToInt32(LValue doubleValue, double low, double high, bool isSigned = true)
    {
        LBasicBlock greatEnough = FTL_NEW_BLOCK(m_out, ("doubleToInt32 greatEnough"));
        LBasicBlock withinRange = FTL_NEW_BLOCK(m_out, ("doubleToInt32 withinRange"));
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("doubleToInt32 slowPath"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("doubleToInt32 continuation"));
        
        Vector<ValueFromBlock, 2> results;
        
        m_out.branch(
            m_out.doubleGreaterThanOrEqual(doubleValue, m_out.constDouble(low)),
            unsure(greatEnough), unsure(slowPath));
        
        LBasicBlock lastNext = m_out.appendTo(greatEnough, withinRange);
        m_out.branch(
            m_out.doubleLessThanOrEqual(doubleValue, m_out.constDouble(high)),
            unsure(withinRange), unsure(slowPath));
        
        m_out.appendTo(withinRange, slowPath);
        LValue fastResult;
        if (isSigned)
            fastResult = m_out.fpToInt32(doubleValue);
        else
            fastResult = m_out.fpToUInt32(doubleValue);
        results.append(m_out.anchor(fastResult));
        m_out.jump(continuation);
        
        m_out.appendTo(slowPath, continuation);
        results.append(m_out.anchor(m_out.call(m_out.operation(toInt32), doubleValue)));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int32, results);
    }
    
    LValue doubleToInt32(LValue doubleValue)
    {
        if (Output::hasSensibleDoubleToInt())
            return sensibleDoubleToInt32(doubleValue);
        
        double limit = pow(2, 31) - 1;
        return doubleToInt32(doubleValue, -limit, limit);
    }
    
    LValue sensibleDoubleToInt32(LValue doubleValue)
    {
        LBasicBlock slowPath = FTL_NEW_BLOCK(m_out, ("sensible doubleToInt32 slow path"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("sensible doubleToInt32 continuation"));
        
        ValueFromBlock fastResult = m_out.anchor(
            m_out.sensibleDoubleToInt(doubleValue));
        m_out.branch(
            m_out.equal(fastResult.value(), m_out.constInt32(0x80000000)),
            rarely(slowPath), usually(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(slowPath, continuation);
        ValueFromBlock slowResult = m_out.anchor(
            m_out.call(m_out.operation(toInt32), doubleValue));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int32, fastResult, slowResult);
    }
    
    void speculate(
        ExitKind kind, FormattedValue lowValue, Node* highValue, LValue failCondition)
    {
        appendOSRExit(kind, lowValue, highValue, failCondition);
    }
    
    void terminate(ExitKind kind)
    {
        speculate(kind, noValue(), nullptr, m_out.booleanTrue);
        didAlreadyTerminate();
    }
    
    void didAlreadyTerminate()
    {
        m_state.setIsValid(false);
    }
    
    void typeCheck(
        FormattedValue lowValue, Edge highValue, SpeculatedType typesPassedThrough,
        LValue failCondition)
    {
        appendTypeCheck(lowValue, highValue, typesPassedThrough, failCondition);
    }
    
    void appendTypeCheck(
        FormattedValue lowValue, Edge highValue, SpeculatedType typesPassedThrough,
        LValue failCondition)
    {
        if (!m_interpreter.needsTypeCheck(highValue, typesPassedThrough))
            return;
        ASSERT(mayHaveTypeCheck(highValue.useKind()));
        appendOSRExit(BadType, lowValue, highValue.node(), failCondition);
        m_interpreter.filter(highValue, typesPassedThrough);
    }
    
    LValue lowInt32(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || (edge.useKind() == Int32Use || edge.useKind() == KnownInt32Use));
        
        if (edge->hasConstant()) {
            JSValue value = edge->asJSValue();
            if (!value.isInt32()) {
                terminate(Uncountable);
                return m_out.int32Zero;
            }
            return m_out.constInt32(value.asInt32());
        }
        
        LoweredNodeValue value = m_int32Values.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_strictInt52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToInt32(edge, value.value());
        
        value = m_int52Values.get(edge.node());
        if (isValid(value))
            return strictInt52ToInt32(edge, int52ToStrictInt52(value.value()));
        
        value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue boxedResult = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(boxedResult), edge, SpecInt32, isNotInt32(boxedResult));
            LValue result = unboxInt32(boxedResult);
            setInt32(edge.node(), result);
            return result;
        }

        DFG_ASSERT(m_graph, m_node, !(provenType(edge) & SpecInt32));
        terminate(Uncountable);
        return m_out.int32Zero;
    }
    
    enum Int52Kind { StrictInt52, Int52 };
    LValue lowInt52(Edge edge, Int52Kind kind)
    {
        DFG_ASSERT(m_graph, m_node, edge.useKind() == Int52RepUse);
        
        LoweredNodeValue value;
        
        switch (kind) {
        case Int52:
            value = m_int52Values.get(edge.node());
            if (isValid(value))
                return value.value();
            
            value = m_strictInt52Values.get(edge.node());
            if (isValid(value))
                return strictInt52ToInt52(value.value());
            break;
            
        case StrictInt52:
            value = m_strictInt52Values.get(edge.node());
            if (isValid(value))
                return value.value();
            
            value = m_int52Values.get(edge.node());
            if (isValid(value))
                return int52ToStrictInt52(value.value());
            break;
        }

        DFG_ASSERT(m_graph, m_node, !provenType(edge));
        terminate(Uncountable);
        return m_out.int64Zero;
    }
    
    LValue lowInt52(Edge edge)
    {
        return lowInt52(edge, Int52);
    }
    
    LValue lowStrictInt52(Edge edge)
    {
        return lowInt52(edge, StrictInt52);
    }
    
    bool betterUseStrictInt52(Node* node)
    {
        return !isValid(m_int52Values.get(node));
    }
    bool betterUseStrictInt52(Edge edge)
    {
        return betterUseStrictInt52(edge.node());
    }
    template<typename T>
    Int52Kind bestInt52Kind(T node)
    {
        return betterUseStrictInt52(node) ? StrictInt52 : Int52;
    }
    Int52Kind opposite(Int52Kind kind)
    {
        switch (kind) {
        case Int52:
            return StrictInt52;
        case StrictInt52:
            return Int52;
        }
        DFG_CRASH(m_graph, m_node, "Bad use kind");
        return Int52;
    }
    
    LValue lowWhicheverInt52(Edge edge, Int52Kind& kind)
    {
        kind = bestInt52Kind(edge);
        return lowInt52(edge, kind);
    }
    
    LValue lowCell(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        DFG_ASSERT(m_graph, m_node, mode == ManualOperandSpeculation || DFG::isCell(edge.useKind()));
        
        if (edge->op() == JSConstant) {
            JSValue value = edge->asJSValue();
            if (!value.isCell()) {
                terminate(Uncountable);
                return m_out.intPtrZero;
            }
            return m_out.constIntPtr(value.asCell());
        }
        
        LoweredNodeValue value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue uncheckedValue = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(uncheckedValue), edge, SpecCell, isNotCell(uncheckedValue));
            return uncheckedValue;
        }
        
        DFG_ASSERT(m_graph, m_node, !(provenType(edge) & SpecCell));
        terminate(Uncountable);
        return m_out.intPtrZero;
    }
    
    LValue lowObject(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == ObjectUse);
        
        LValue result = lowCell(edge, mode);
        speculateObject(edge, result);
        return result;
    }
    
    LValue lowString(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == StringUse || edge.useKind() == KnownStringUse || edge.useKind() == StringIdentUse);
        
        LValue result = lowCell(edge, mode);
        speculateString(edge, result);
        return result;
    }
    
    LValue lowStringIdent(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == StringIdentUse);
        
        LValue string = lowString(edge, mode);
        LValue stringImpl = m_out.loadPtr(string, m_heaps.JSString_value);
        speculateStringIdent(edge, string, stringImpl);
        return stringImpl;
    }
    
    LValue lowNonNullObject(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == ObjectUse);
        
        LValue result = lowCell(edge, mode);
        speculateNonNullObject(edge, result);
        return result;
    }
    
    LValue lowBoolean(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == BooleanUse);
        
        if (edge->hasConstant()) {
            JSValue value = edge->asJSValue();
            if (!value.isBoolean()) {
                terminate(Uncountable);
                return m_out.booleanFalse;
            }
            return m_out.constBool(value.asBoolean());
        }
        
        LoweredNodeValue value = m_booleanValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_jsValueValues.get(edge.node());
        if (isValid(value)) {
            LValue unboxedResult = value.value();
            FTL_TYPE_CHECK(
                jsValueValue(unboxedResult), edge, SpecBoolean, isNotBoolean(unboxedResult));
            LValue result = unboxBoolean(unboxedResult);
            setBoolean(edge.node(), result);
            return result;
        }
        
        DFG_ASSERT(m_graph, m_node, !(provenType(edge) & SpecBoolean));
        terminate(Uncountable);
        return m_out.booleanFalse;
    }
    
    LValue lowDouble(Edge edge)
    {
        DFG_ASSERT(m_graph, m_node, isDouble(edge.useKind()));
        
        LoweredNodeValue value = m_doubleValues.get(edge.node());
        if (isValid(value))
            return value.value();
        DFG_ASSERT(m_graph, m_node, !provenType(edge));
        terminate(Uncountable);
        return m_out.doubleZero;
    }
    
    LValue lowJSValue(Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
    {
        DFG_ASSERT(m_graph, m_node, mode == ManualOperandSpeculation || edge.useKind() == UntypedUse);
        DFG_ASSERT(m_graph, m_node, !isDouble(edge.useKind()));
        DFG_ASSERT(m_graph, m_node, edge.useKind() != Int52RepUse);
        
        if (edge->hasConstant())
            return m_out.constInt64(JSValue::encode(edge->asJSValue()));

        LoweredNodeValue value = m_jsValueValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        value = m_int32Values.get(edge.node());
        if (isValid(value)) {
            LValue result = boxInt32(value.value());
            setJSValue(edge.node(), result);
            return result;
        }
        
        value = m_booleanValues.get(edge.node());
        if (isValid(value)) {
            LValue result = boxBoolean(value.value());
            setJSValue(edge.node(), result);
            return result;
        }
        
        DFG_CRASH(m_graph, m_node, "Value not defined");
        return 0;
    }
    
    LValue lowStorage(Edge edge)
    {
        LoweredNodeValue value = m_storageValues.get(edge.node());
        if (isValid(value))
            return value.value();
        
        LValue result = lowCell(edge);
        setStorage(edge.node(), result);
        return result;
    }
    
    LValue strictInt52ToInt32(Edge edge, LValue value)
    {
        LValue result = m_out.castToInt32(value);
        FTL_TYPE_CHECK(
            noValue(), edge, SpecInt32,
            m_out.notEqual(m_out.signExt(result, m_out.int64), value));
        setInt32(edge.node(), result);
        return result;
    }
    
    LValue strictInt52ToDouble(LValue value)
    {
        return m_out.intToDouble(value);
    }
    
    LValue strictInt52ToJSValue(LValue value)
    {
        LBasicBlock isInt32 = FTL_NEW_BLOCK(m_out, ("strictInt52ToJSValue isInt32 case"));
        LBasicBlock isDouble = FTL_NEW_BLOCK(m_out, ("strictInt52ToJSValue isDouble case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("strictInt52ToJSValue continuation"));
        
        Vector<ValueFromBlock, 2> results;
            
        LValue int32Value = m_out.castToInt32(value);
        m_out.branch(
            m_out.equal(m_out.signExt(int32Value, m_out.int64), value),
            unsure(isInt32), unsure(isDouble));
        
        LBasicBlock lastNext = m_out.appendTo(isInt32, isDouble);
        
        results.append(m_out.anchor(boxInt32(int32Value)));
        m_out.jump(continuation);
        
        m_out.appendTo(isDouble, continuation);
        
        results.append(m_out.anchor(boxDouble(m_out.intToDouble(value))));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        return m_out.phi(m_out.int64, results);
    }
    
    LValue strictInt52ToInt52(LValue value)
    {
        return m_out.shl(value, m_out.constInt64(JSValue::int52ShiftAmount));
    }
    
    LValue int52ToStrictInt52(LValue value)
    {
        return m_out.aShr(value, m_out.constInt64(JSValue::int52ShiftAmount));
    }
    
    LValue isInt32(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecInt32))
            return proven;
        return m_out.aboveOrEqual(jsValue, m_tagTypeNumber);
    }
    LValue isNotInt32(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~SpecInt32))
            return proven;
        return m_out.below(jsValue, m_tagTypeNumber);
    }
    LValue unboxInt32(LValue jsValue)
    {
        return m_out.castToInt32(jsValue);
    }
    LValue boxInt32(LValue value)
    {
        return m_out.add(m_out.zeroExt(value, m_out.int64), m_tagTypeNumber);
    }
    
    LValue isCellOrMisc(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecCell | SpecMisc))
            return proven;
        return m_out.testIsZero64(jsValue, m_tagTypeNumber);
    }
    LValue isNotCellOrMisc(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~(SpecCell | SpecMisc)))
            return proven;
        return m_out.testNonZero64(jsValue, m_tagTypeNumber);
    }
    
    LValue unboxDouble(LValue jsValue)
    {
        return m_out.bitCast(m_out.add(jsValue, m_tagTypeNumber), m_out.doubleType);
    }
    LValue boxDouble(LValue doubleValue)
    {
        return m_out.sub(m_out.bitCast(doubleValue, m_out.int64), m_tagTypeNumber);
    }
    
    LValue jsValueToStrictInt52(Edge edge, LValue boxedValue)
    {
        LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("jsValueToInt52 unboxing int case"));
        LBasicBlock doubleCase = FTL_NEW_BLOCK(m_out, ("jsValueToInt52 unboxing double case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("jsValueToInt52 unboxing continuation"));
            
        LValue isNotInt32;
        if (!m_interpreter.needsTypeCheck(edge, SpecInt32))
            isNotInt32 = m_out.booleanFalse;
        else if (!m_interpreter.needsTypeCheck(edge, ~SpecInt32))
            isNotInt32 = m_out.booleanTrue;
        else
            isNotInt32 = this->isNotInt32(boxedValue);
        m_out.branch(isNotInt32, unsure(doubleCase), unsure(intCase));
            
        LBasicBlock lastNext = m_out.appendTo(intCase, doubleCase);
            
        ValueFromBlock intToInt52 = m_out.anchor(
            m_out.signExt(unboxInt32(boxedValue), m_out.int64));
        m_out.jump(continuation);
            
        m_out.appendTo(doubleCase, continuation);
        
        LValue possibleResult = m_out.call(
            m_out.operation(operationConvertBoxedDoubleToInt52), boxedValue);
        FTL_TYPE_CHECK(
            jsValueValue(boxedValue), edge, SpecInt32 | SpecInt52AsDouble,
            m_out.equal(possibleResult, m_out.constInt64(JSValue::notInt52)));
            
        ValueFromBlock doubleToInt52 = m_out.anchor(possibleResult);
        m_out.jump(continuation);
            
        m_out.appendTo(continuation, lastNext);
            
        return m_out.phi(m_out.int64, intToInt52, doubleToInt52);
    }
    
    LValue doubleToStrictInt52(Edge edge, LValue value)
    {
        LValue possibleResult = m_out.call(
            m_out.operation(operationConvertDoubleToInt52), value);
        FTL_TYPE_CHECK(
            doubleValue(value), edge, SpecInt52AsDouble,
            m_out.equal(possibleResult, m_out.constInt64(JSValue::notInt52)));
        
        return possibleResult;
    }

    LValue convertDoubleToInt32(LValue value, bool shouldCheckNegativeZero)
    {
        LValue integerValue = m_out.fpToInt32(value);
        LValue integerValueConvertedToDouble = m_out.intToDouble(integerValue);
        LValue valueNotConvertibleToInteger = m_out.doubleNotEqualOrUnordered(value, integerValueConvertedToDouble);
        speculate(Overflow, FormattedValue(ValueFormatDouble, value), m_node, valueNotConvertibleToInteger);

        if (shouldCheckNegativeZero) {
            LBasicBlock valueIsZero = FTL_NEW_BLOCK(m_out, ("ConvertDoubleToInt32 on zero"));
            LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("ConvertDoubleToInt32 continuation"));
            m_out.branch(m_out.isZero32(integerValue), unsure(valueIsZero), unsure(continuation));

            LBasicBlock lastNext = m_out.appendTo(valueIsZero, continuation);

            LValue doubleBitcastToInt64 = m_out.bitCast(value, m_out.int64);
            LValue signBitSet = m_out.lessThan(doubleBitcastToInt64, m_out.constInt64(0));

            speculate(NegativeZero, FormattedValue(ValueFormatDouble, value), m_node, signBitSet);
            m_out.jump(continuation);
            m_out.appendTo(continuation, lastNext);
        }
        return integerValue;
    }

    LValue isNumber(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecFullNumber))
            return proven;
        return isNotCellOrMisc(jsValue);
    }
    LValue isNotNumber(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~SpecFullNumber))
            return proven;
        return isCellOrMisc(jsValue);
    }
    
    LValue isNotCell(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~SpecCell))
            return proven;
        return m_out.testNonZero64(jsValue, m_tagMask);
    }
    
    LValue isCell(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecCell))
            return proven;
        return m_out.testIsZero64(jsValue, m_tagMask);
    }
    
    LValue isNotMisc(LValue value, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~SpecMisc))
            return proven;
        return m_out.above(value, m_out.constInt64(TagBitTypeOther | TagBitBool | TagBitUndefined));
    }
    
    LValue isMisc(LValue value, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecMisc))
            return proven;
        return m_out.bitNot(isNotMisc(value));
    }
    
    LValue isNotBoolean(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~SpecBoolean))
            return proven;
        return m_out.testNonZero64(
            m_out.bitXor(jsValue, m_out.constInt64(ValueFalse)),
            m_out.constInt64(~1));
    }
    LValue isBoolean(LValue jsValue, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecBoolean))
            return proven;
        return m_out.bitNot(isNotBoolean(jsValue));
    }
    LValue unboxBoolean(LValue jsValue)
    {
        // We want to use a cast that guarantees that LLVM knows that even the integer
        // value is just 0 or 1. But for now we do it the dumb way.
        return m_out.notZero64(m_out.bitAnd(jsValue, m_out.constInt64(1)));
    }
    LValue boxBoolean(LValue value)
    {
        return m_out.select(
            value, m_out.constInt64(ValueTrue), m_out.constInt64(ValueFalse));
    }
    
    LValue isNotOther(LValue value, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, ~SpecOther))
            return proven;
        return m_out.notEqual(
            m_out.bitAnd(value, m_out.constInt64(~TagBitUndefined)),
            m_out.constInt64(ValueNull));
    }
    LValue isOther(LValue value, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type, SpecOther))
            return proven;
        return m_out.equal(
            m_out.bitAnd(value, m_out.constInt64(~TagBitUndefined)),
            m_out.constInt64(ValueNull));
    }
    
    LValue isProvenValue(SpeculatedType provenType, SpeculatedType wantedType)
    {
        if (!(provenType & ~wantedType))
            return m_out.booleanTrue;
        if (!(provenType & wantedType))
            return m_out.booleanFalse;
        return nullptr;
    }

    void speculate(Edge edge)
    {
        switch (edge.useKind()) {
        case UntypedUse:
            break;
        case KnownInt32Use:
        case KnownStringUse:
        case DoubleRepUse:
        case Int52RepUse:
            ASSERT(!m_interpreter.needsTypeCheck(edge));
            break;
        case Int32Use:
            speculateInt32(edge);
            break;
        case CellUse:
            speculateCell(edge);
            break;
        case KnownCellUse:
            ASSERT(!m_interpreter.needsTypeCheck(edge));
            break;
        case MachineIntUse:
            speculateMachineInt(edge);
            break;
        case ObjectUse:
            speculateObject(edge);
            break;
        case FunctionUse:
            speculateFunction(edge);
            break;
        case ObjectOrOtherUse:
            speculateObjectOrOther(edge);
            break;
        case FinalObjectUse:
            speculateFinalObject(edge);
            break;
        case StringUse:
            speculateString(edge);
            break;
        case StringIdentUse:
            speculateStringIdent(edge);
            break;
        case StringObjectUse:
            speculateStringObject(edge);
            break;
        case StringOrStringObjectUse:
            speculateStringOrStringObject(edge);
            break;
        case NumberUse:
            speculateNumber(edge);
            break;
        case RealNumberUse:
            speculateRealNumber(edge);
            break;
        case DoubleRepRealUse:
            speculateDoubleRepReal(edge);
            break;
        case DoubleRepMachineIntUse:
            speculateDoubleRepMachineInt(edge);
            break;
        case BooleanUse:
            speculateBoolean(edge);
            break;
        case NotStringVarUse:
            speculateNotStringVar(edge);
            break;
        case NotCellUse:
            speculateNotCell(edge);
            break;
        case OtherUse:
            speculateOther(edge);
            break;
        case MiscUse:
            speculateMisc(edge);
            break;
        default:
            DFG_CRASH(m_graph, m_node, "Unsupported speculation use kind");
        }
    }
    
    void speculate(Node*, Edge edge)
    {
        speculate(edge);
    }
    
    void speculateInt32(Edge edge)
    {
        lowInt32(edge);
    }
    
    void speculateCell(Edge edge)
    {
        lowCell(edge);
    }
    
    void speculateMachineInt(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        jsValueToStrictInt52(edge, lowJSValue(edge, ManualOperandSpeculation));
    }
    
    LValue isObject(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type & SpecCell, SpecObject))
            return proven;
        return m_out.aboveOrEqual(
            m_out.load8(cell, m_heaps.JSCell_typeInfoType),
            m_out.constInt8(ObjectType));
    }

    LValue isNotObject(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type & SpecCell, ~SpecObject))
            return proven;
        return m_out.below(
            m_out.load8(cell, m_heaps.JSCell_typeInfoType),
            m_out.constInt8(ObjectType));
    }

    LValue isNotString(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type & SpecCell, ~SpecString))
            return proven;
        return m_out.notEqual(
            m_out.load32(cell, m_heaps.JSCell_structureID),
            m_out.constInt32(vm().stringStructure->id()));
    }
    
    LValue isString(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type & SpecCell, SpecString))
            return proven;
        return m_out.equal(
            m_out.load32(cell, m_heaps.JSCell_structureID),
            m_out.constInt32(vm().stringStructure->id()));
    }
    
    LValue isArrayType(LValue cell, ArrayMode arrayMode)
    {
        switch (arrayMode.type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            LValue indexingType = m_out.load8(cell, m_heaps.JSCell_indexingType);
            
            switch (arrayMode.arrayClass()) {
            case Array::OriginalArray:
                DFG_CRASH(m_graph, m_node, "Unexpected original array");
                return 0;
                
            case Array::Array:
                return m_out.equal(
                    m_out.bitAnd(indexingType, m_out.constInt8(IsArray | IndexingShapeMask)),
                    m_out.constInt8(IsArray | arrayMode.shapeMask()));
                
            case Array::NonArray:
            case Array::OriginalNonArray:
                return m_out.equal(
                    m_out.bitAnd(indexingType, m_out.constInt8(IsArray | IndexingShapeMask)),
                    m_out.constInt8(arrayMode.shapeMask()));
                
            case Array::PossiblyArray:
                return m_out.equal(
                    m_out.bitAnd(indexingType, m_out.constInt8(IndexingShapeMask)),
                    m_out.constInt8(arrayMode.shapeMask()));
            }
            
            DFG_CRASH(m_graph, m_node, "Corrupt array class");
        }
            
        case Array::DirectArguments:
            return m_out.equal(
                m_out.load8(cell, m_heaps.JSCell_typeInfoType),
                m_out.constInt8(DirectArgumentsType));
            
        case Array::ScopedArguments:
            return m_out.equal(
                m_out.load8(cell, m_heaps.JSCell_typeInfoType),
                m_out.constInt8(ScopedArgumentsType));
            
        default:
            return m_out.equal(
                m_out.load8(cell, m_heaps.JSCell_typeInfoType), 
                m_out.constInt8(typeForTypedArrayType(arrayMode.typedArrayType())));
        }
    }
    
    LValue isFunction(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type & SpecCell, SpecFunction))
            return proven;
        return isType(cell, JSFunctionType);
    }
    LValue isNotFunction(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (LValue proven = isProvenValue(type & SpecCell, ~SpecFunction))
            return proven;
        return isNotType(cell, JSFunctionType);
    }
            
    LValue isExoticForTypeof(LValue cell, SpeculatedType type = SpecFullTop)
    {
        if (!(type & SpecObjectOther))
            return m_out.booleanFalse;
        return m_out.testNonZero8(
            m_out.load8(cell, m_heaps.JSCell_typeInfoFlags),
            m_out.constInt8(MasqueradesAsUndefined | TypeOfShouldCallGetCallData));
    }
    
    LValue isType(LValue cell, JSType type)
    {
        return m_out.equal(
            m_out.load8(cell, m_heaps.JSCell_typeInfoType),
            m_out.constInt8(type));
    }
    
    LValue isNotType(LValue cell, JSType type)
    {
        return m_out.bitNot(isType(cell, type));
    }
    
    void speculateObject(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(jsValueValue(cell), edge, SpecObject, isNotObject(cell));
    }
    
    void speculateObject(Edge edge)
    {
        speculateObject(edge, lowCell(edge));
    }
    
    void speculateFunction(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(jsValueValue(cell), edge, SpecFunction, isNotFunction(cell));
    }
    
    void speculateFunction(Edge edge)
    {
        speculateFunction(edge, lowCell(edge));
    }
    
    void speculateObjectOrOther(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        
        LBasicBlock cellCase = FTL_NEW_BLOCK(m_out, ("speculateObjectOrOther cell case"));
        LBasicBlock primitiveCase = FTL_NEW_BLOCK(m_out, ("speculateObjectOrOther primitive case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("speculateObjectOrOther continuation"));
        
        m_out.branch(isNotCell(value, provenType(edge)), unsure(primitiveCase), unsure(cellCase));
        
        LBasicBlock lastNext = m_out.appendTo(cellCase, primitiveCase);
        
        FTL_TYPE_CHECK(
            jsValueValue(value), edge, (~SpecCell) | SpecObject, isNotObject(value));
        
        m_out.jump(continuation);
        
        m_out.appendTo(primitiveCase, continuation);
        
        FTL_TYPE_CHECK(
            jsValueValue(value), edge, SpecCell | SpecOther, isNotOther(value));
        
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void speculateFinalObject(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(
            jsValueValue(cell), edge, SpecFinalObject, isNotType(cell, FinalObjectType));
    }
    
    void speculateFinalObject(Edge edge)
    {
        speculateFinalObject(edge, lowCell(edge));
    }
    
    void speculateString(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(jsValueValue(cell), edge, SpecString | ~SpecCell, isNotString(cell));
    }
    
    void speculateString(Edge edge)
    {
        speculateString(edge, lowCell(edge));
    }
    
    void speculateStringIdent(Edge edge, LValue string, LValue stringImpl)
    {
        if (!m_interpreter.needsTypeCheck(edge, SpecStringIdent | ~SpecString))
            return;
        
        speculate(BadType, jsValueValue(string), edge.node(), m_out.isNull(stringImpl));
        speculate(
            BadType, jsValueValue(string), edge.node(),
            m_out.testIsZero32(
                m_out.load32(stringImpl, m_heaps.StringImpl_hashAndFlags),
                m_out.constInt32(StringImpl::flagIsAtomic())));
        m_interpreter.filter(edge, SpecStringIdent | ~SpecString);
    }
    
    void speculateStringIdent(Edge edge)
    {
        lowStringIdent(edge);
    }
    
    void speculateStringObject(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge, SpecStringObject))
            return;
        
        speculateStringObjectForCell(edge, lowCell(edge));
        m_interpreter.filter(edge, SpecStringObject);
    }
    
    void speculateStringOrStringObject(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge, SpecString | SpecStringObject))
            return;
        
        LBasicBlock notString = FTL_NEW_BLOCK(m_out, ("Speculate StringOrStringObject not string case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Speculate StringOrStringObject continuation"));
        
        LValue structureID = m_out.load32(lowCell(edge), m_heaps.JSCell_structureID);
        m_out.branch(
            m_out.equal(structureID, m_out.constInt32(vm().stringStructure->id())),
            unsure(continuation), unsure(notString));
        
        LBasicBlock lastNext = m_out.appendTo(notString, continuation);
        speculateStringObjectForStructureID(edge, structureID);
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
        
        m_interpreter.filter(edge, SpecString | SpecStringObject);
    }
    
    void speculateStringObjectForCell(Edge edge, LValue cell)
    {
        speculateStringObjectForStructureID(edge, m_out.load32(cell, m_heaps.JSCell_structureID));
    }
    
    void speculateStringObjectForStructureID(Edge edge, LValue structureID)
    {
        Structure* stringObjectStructure =
            m_graph.globalObjectFor(m_node->origin.semantic)->stringObjectStructure();
        
        if (abstractStructure(edge).isSubsetOf(StructureSet(stringObjectStructure)))
            return;
        
        speculate(
            NotStringObject, noValue(), 0,
            m_out.notEqual(structureID, weakStructureID(stringObjectStructure)));
    }
    
    void speculateNonNullObject(Edge edge, LValue cell)
    {
        FTL_TYPE_CHECK(jsValueValue(cell), edge, SpecObject, isNotObject(cell));
        if (masqueradesAsUndefinedWatchpointIsStillValid())
            return;
        
        speculate(
            BadType, jsValueValue(cell), edge.node(),
            m_out.testNonZero8(
                m_out.load8(cell, m_heaps.JSCell_typeInfoFlags),
                m_out.constInt8(MasqueradesAsUndefined)));
    }
    
    void speculateNumber(Edge edge)
    {
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        FTL_TYPE_CHECK(jsValueValue(value), edge, SpecBytecodeNumber, isNotNumber(value));
    }
    
    void speculateRealNumber(Edge edge)
    {
        // Do an early return here because lowDouble() can create a lot of control flow.
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        LValue doubleValue = unboxDouble(value);
        
        LBasicBlock intCase = FTL_NEW_BLOCK(m_out, ("speculateRealNumber int case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("speculateRealNumber continuation"));
        
        m_out.branch(
            m_out.doubleEqual(doubleValue, doubleValue),
            usually(continuation), rarely(intCase));
        
        LBasicBlock lastNext = m_out.appendTo(intCase, continuation);
        
        typeCheck(
            jsValueValue(value), m_node->child1(), SpecBytecodeRealNumber,
            isNotInt32(value, provenType(m_node->child1()) & ~SpecFullDouble));
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
    }
    
    void speculateDoubleRepReal(Edge edge)
    {
        // Do an early return here because lowDouble() can create a lot of control flow.
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowDouble(edge);
        FTL_TYPE_CHECK(
            doubleValue(value), edge, SpecDoubleReal,
            m_out.doubleNotEqualOrUnordered(value, value));
    }
    
    void speculateDoubleRepMachineInt(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        doubleToStrictInt52(edge, lowDouble(edge));
    }
    
    void speculateBoolean(Edge edge)
    {
        lowBoolean(edge);
    }
    
    void speculateNotStringVar(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge, ~SpecStringVar))
            return;
        
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        
        LBasicBlock isCellCase = FTL_NEW_BLOCK(m_out, ("Speculate NotStringVar is cell case"));
        LBasicBlock isStringCase = FTL_NEW_BLOCK(m_out, ("Speculate NotStringVar is string case"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Speculate NotStringVar continuation"));
        
        m_out.branch(isCell(value, provenType(edge)), unsure(isCellCase), unsure(continuation));
        
        LBasicBlock lastNext = m_out.appendTo(isCellCase, isStringCase);
        m_out.branch(isString(value, provenType(edge)), unsure(isStringCase), unsure(continuation));
        
        m_out.appendTo(isStringCase, continuation);
        speculateStringIdent(edge, value, m_out.loadPtr(value, m_heaps.JSString_value));
        m_out.jump(continuation);
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void speculateNotCell(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        typeCheck(jsValueValue(value), edge, ~SpecCell, isCell(value));
    }
    
    void speculateOther(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        typeCheck(jsValueValue(value), edge, SpecOther, isNotOther(value));
    }
    
    void speculateMisc(Edge edge)
    {
        if (!m_interpreter.needsTypeCheck(edge))
            return;
        
        LValue value = lowJSValue(edge, ManualOperandSpeculation);
        typeCheck(jsValueValue(value), edge, SpecMisc, isNotMisc(value));
    }
    
    bool masqueradesAsUndefinedWatchpointIsStillValid()
    {
        return m_graph.masqueradesAsUndefinedWatchpointIsStillValid(m_node->origin.semantic);
    }
    
    LValue loadMarkByte(LValue base)
    {
        return m_out.load8(base, m_heaps.JSCell_gcData);
    }

    void emitStoreBarrier(LValue base)
    {
#if ENABLE(GGC)
        LBasicBlock isMarkedAndNotRemembered = FTL_NEW_BLOCK(m_out, ("Store barrier is marked block"));
        LBasicBlock bufferHasSpace = FTL_NEW_BLOCK(m_out, ("Store barrier buffer has space"));
        LBasicBlock bufferIsFull = FTL_NEW_BLOCK(m_out, ("Store barrier buffer is full"));
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Store barrier continuation"));

        // Check the mark byte. 
        m_out.branch(
            m_out.notZero8(loadMarkByte(base)), usually(continuation), rarely(isMarkedAndNotRemembered));

        // Append to the write barrier buffer.
        LBasicBlock lastNext = m_out.appendTo(isMarkedAndNotRemembered, bufferHasSpace);
        LValue currentBufferIndex = m_out.load32(m_out.absolute(vm().heap.writeBarrierBuffer().currentIndexAddress()));
        LValue bufferCapacity = m_out.constInt32(vm().heap.writeBarrierBuffer().capacity());
        m_out.branch(
            m_out.lessThan(currentBufferIndex, bufferCapacity),
            usually(bufferHasSpace), rarely(bufferIsFull));

        // Buffer has space, store to it.
        m_out.appendTo(bufferHasSpace, bufferIsFull);
        LValue writeBarrierBufferBase = m_out.constIntPtr(vm().heap.writeBarrierBuffer().buffer());
        m_out.storePtr(base, m_out.baseIndex(m_heaps.WriteBarrierBuffer_bufferContents, writeBarrierBufferBase, m_out.zeroExtPtr(currentBufferIndex)));
        m_out.store32(m_out.add(currentBufferIndex, m_out.constInt32(1)), m_out.absolute(vm().heap.writeBarrierBuffer().currentIndexAddress()));
        m_out.jump(continuation);

        // Buffer is out of space, flush it.
        m_out.appendTo(bufferIsFull, continuation);
        vmCallNoExceptions(m_out.operation(operationFlushWriteBarrierBuffer), m_callFrame, base);
        m_out.jump(continuation);

        m_out.appendTo(continuation, lastNext);
#else
        UNUSED_PARAM(base);
#endif
    }

    template<typename... Args>
    LValue vmCall(LValue function, Args... args)
    {
        callPreflight();
        LValue result = m_out.call(function, args...);
        callCheck();
        return result;
    }
    
    template<typename... Args>
    LValue vmCallNoExceptions(LValue function, Args... args)
    {
        callPreflight();
        LValue result = m_out.call(function, args...);
        return result;
    }

    void callPreflight(CodeOrigin codeOrigin)
    {
        m_out.store32(
            m_out.constInt32(
                CallFrame::Location::encodeAsCodeOriginIndex(
                    m_ftlState.jitCode->common.addCodeOrigin(codeOrigin))),
            tagFor(JSStack::ArgumentCount));
    }
    void callPreflight()
    {
        callPreflight(m_node->origin.semantic);
    }
    
    void callCheck()
    {
        if (Options::useExceptionFuzz())
            m_out.call(m_out.operation(operationExceptionFuzz));
        
        LBasicBlock continuation = FTL_NEW_BLOCK(m_out, ("Exception check continuation"));
        
        LValue exception = m_out.load64(m_out.absolute(vm().addressOfException()));
        
        m_out.branch(
            m_out.notZero64(exception), rarely(m_handleExceptions), usually(continuation));
        
        m_out.appendTo(continuation);
    }
    
    LBasicBlock lowBlock(BasicBlock* block)
    {
        return m_blocks.get(block);
    }
    
    void appendOSRExit(
        ExitKind kind, FormattedValue lowValue, Node* highValue, LValue failCondition)
    {
        if (verboseCompilationEnabled()) {
            dataLog("    OSR exit #", m_ftlState.jitCode->osrExit.size(), " with availability: ", availabilityMap(), "\n");
            if (!m_availableRecoveries.isEmpty())
                dataLog("        Available recoveries: ", listDump(m_availableRecoveries), "\n");
        }
        
        if (doOSRExitFuzzing()) {
            LValue numberOfFuzzChecks = m_out.add(
                m_out.load32(m_out.absolute(&g_numberOfOSRExitFuzzChecks)),
                m_out.int32One);
            
            m_out.store32(numberOfFuzzChecks, m_out.absolute(&g_numberOfOSRExitFuzzChecks));
            
            if (unsigned atOrAfter = Options::fireOSRExitFuzzAtOrAfter()) {
                failCondition = m_out.bitOr(
                    failCondition,
                    m_out.aboveOrEqual(numberOfFuzzChecks, m_out.constInt32(atOrAfter)));
            }
            if (unsigned at = Options::fireOSRExitFuzzAt()) {
                failCondition = m_out.bitOr(
                    failCondition,
                    m_out.equal(numberOfFuzzChecks, m_out.constInt32(at)));
            }
        }

        ASSERT(m_ftlState.jitCode->osrExit.size() == m_ftlState.finalizer->osrExit.size());
        
        m_ftlState.jitCode->osrExit.append(OSRExit(
            kind, lowValue.format(), m_graph.methodOfGettingAValueProfileFor(highValue),
            m_codeOriginForExitTarget, m_codeOriginForExitProfile,
            availabilityMap().m_locals.numberOfArguments(),
            availabilityMap().m_locals.numberOfLocals()));
        m_ftlState.finalizer->osrExit.append(OSRExitCompilationInfo());
        
        OSRExit& exit = m_ftlState.jitCode->osrExit.last();

        LBasicBlock lastNext = nullptr;
        LBasicBlock continuation = nullptr;
        
        LBasicBlock failCase = FTL_NEW_BLOCK(m_out, ("OSR exit failCase for ", m_node));
        continuation = FTL_NEW_BLOCK(m_out, ("OSR exit continuation for ", m_node));
        
        m_out.branch(failCondition, rarely(failCase), usually(continuation));
        
        lastNext = m_out.appendTo(failCase, continuation);
        
        emitOSRExitCall(exit, lowValue);
        
        m_out.unreachable();
        
        m_out.appendTo(continuation, lastNext);
    }
    
    void emitOSRExitCall(OSRExit& exit, FormattedValue lowValue)
    {
        ExitArgumentList arguments;
        
        CodeOrigin codeOrigin = exit.m_codeOrigin;
        
        buildExitArguments(exit, arguments, lowValue, codeOrigin);
        
        callStackmap(exit, arguments);
    }
    
    void buildExitArguments(
        OSRExit& exit, ExitArgumentList& arguments, FormattedValue lowValue,
        CodeOrigin codeOrigin)
    {
        if (!!lowValue)
            arguments.append(lowValue.value());
        
        AvailabilityMap availabilityMap = this->availabilityMap();
        availabilityMap.pruneByLiveness(m_graph, codeOrigin);
        
        HashMap<Node*, ExitTimeObjectMaterialization*> map;
        availabilityMap.forEachAvailability(
            [&] (Availability availability) {
                if (!availability.shouldUseNode())
                    return;
                
                Node* node = availability.node();
                if (!node->isPhantomAllocation())
                    return;
                
                auto result = map.add(node, nullptr);
                if (result.isNewEntry) {
                    result.iterator->value =
                        exit.m_materializations.add(node->op(), node->origin.semantic);
                }
            });
        
        for (unsigned i = 0; i < exit.m_values.size(); ++i) {
            int operand = exit.m_values.operandForIndex(i);
            
            Availability availability = availabilityMap.m_locals[i];
            
            if (Options::validateFTLOSRExitLiveness()) {
                DFG_ASSERT(
                    m_graph, m_node,
                    (!(availability.isDead() && m_graph.isLiveInBytecode(VirtualRegister(operand), codeOrigin))) || m_graph.m_plan.mode == FTLForOSREntryMode);
            }
            
            exit.m_values[i] = exitValueForAvailability(arguments, map, availability);
        }
        
        for (auto heapPair : availabilityMap.m_heap) {
            Node* node = heapPair.key.base();
            ExitTimeObjectMaterialization* materialization = map.get(node);
            materialization->add(
                heapPair.key.descriptor(),
                exitValueForAvailability(arguments, map, heapPair.value));
        }
        
        if (verboseCompilationEnabled()) {
            dataLog("        Exit values: ", exit.m_values, "\n");
            if (!exit.m_materializations.isEmpty()) {
                dataLog("        Materializations: \n");
                for (ExitTimeObjectMaterialization* materialization : exit.m_materializations)
                    dataLog("            ", pointerDump(materialization), "\n");
            }
        }
    }
    
    void callStackmap(OSRExit& exit, ExitArgumentList& arguments)
    {
        exit.m_stackmapID = m_stackmapIDs++;
        arguments.insert(0, m_out.constInt32(MacroAssembler::maxJumpReplacementSize()));
        arguments.insert(0, m_out.constInt64(exit.m_stackmapID));
        
        m_out.call(m_out.stackmapIntrinsic(), arguments);
    }
    
    ExitValue exitValueForAvailability(
        ExitArgumentList& arguments, const HashMap<Node*, ExitTimeObjectMaterialization*>& map,
        Availability availability)
    {
        FlushedAt flush = availability.flushedAt();
        switch (flush.format()) {
        case DeadFlush:
        case ConflictingFlush:
            if (availability.hasNode())
                return exitValueForNode(arguments, map, availability.node());
            
            // This means that the value is dead. It could be dead in bytecode or it could have
            // been killed by our DCE, which can sometimes kill things even if they were live in
            // bytecode.
            return ExitValue::dead();

        case FlushedJSValue:
        case FlushedCell:
        case FlushedBoolean:
            return ExitValue::inJSStack(flush.virtualRegister());
                
        case FlushedInt32:
            return ExitValue::inJSStackAsInt32(flush.virtualRegister());
                
        case FlushedInt52:
            return ExitValue::inJSStackAsInt52(flush.virtualRegister());
                
        case FlushedDouble:
            return ExitValue::inJSStackAsDouble(flush.virtualRegister());
        }
        
        DFG_CRASH(m_graph, m_node, "Invalid flush format");
        return ExitValue::dead();
    }
    
    ExitValue exitValueForNode(
        ExitArgumentList& arguments, const HashMap<Node*, ExitTimeObjectMaterialization*>& map,
        Node* node)
    {
        ASSERT(node->shouldGenerate());
        ASSERT(node->hasResult());

        if (node) {
            switch (node->op()) {
            case BottomValue:
                // This might arise in object materializations. I actually doubt that it would,
                // but it seems worthwhile to be conservative.
                return ExitValue::dead();
                
            case JSConstant:
            case Int52Constant:
            case DoubleConstant:
                return ExitValue::constant(node->asJSValue());
                
            default:
                if (node->isPhantomAllocation())
                    return ExitValue::materializeNewObject(map.get(node));
                break;
            }
        }
        
        for (unsigned i = 0; i < m_availableRecoveries.size(); ++i) {
            AvailableRecovery recovery = m_availableRecoveries[i];
            if (recovery.node() != node)
                continue;
            
            ExitValue result = ExitValue::recovery(
                recovery.opcode(), arguments.size(), arguments.size() + 1,
                recovery.format());
            arguments.append(recovery.left());
            arguments.append(recovery.right());
            return result;
        }
        
        LoweredNodeValue value = m_int32Values.get(node);
        if (isValid(value))
            return exitArgument(arguments, ValueFormatInt32, value.value());
        
        value = m_int52Values.get(node);
        if (isValid(value))
            return exitArgument(arguments, ValueFormatInt52, value.value());
        
        value = m_strictInt52Values.get(node);
        if (isValid(value))
            return exitArgument(arguments, ValueFormatStrictInt52, value.value());
        
        value = m_booleanValues.get(node);
        if (isValid(value)) {
            LValue valueToPass = m_out.zeroExt(value.value(), m_out.int32);
            return exitArgument(arguments, ValueFormatBoolean, valueToPass);
        }
        
        value = m_jsValueValues.get(node);
        if (isValid(value))
            return exitArgument(arguments, ValueFormatJSValue, value.value());
        
        value = m_doubleValues.get(node);
        if (isValid(value))
            return exitArgument(arguments, ValueFormatDouble, value.value());

        DFG_CRASH(m_graph, m_node, toCString("Cannot find value for node: ", node).data());
        return ExitValue::dead();
    }
    
    ExitValue exitArgument(ExitArgumentList& arguments, ValueFormat format, LValue value)
    {
        ExitValue result = ExitValue::exitArgument(ExitArgument(format, arguments.size()));
        arguments.append(value);
        return result;
    }
    
    bool doesKill(Edge edge)
    {
        if (edge.doesNotKill())
            return false;
        
        if (edge->hasConstant())
            return false;
        
        return true;
    }

    void addAvailableRecovery(
        Node* node, RecoveryOpcode opcode, LValue left, LValue right, ValueFormat format)
    {
        m_availableRecoveries.append(AvailableRecovery(node, opcode, left, right, format));
    }
    
    void addAvailableRecovery(
        Edge edge, RecoveryOpcode opcode, LValue left, LValue right, ValueFormat format)
    {
        addAvailableRecovery(edge.node(), opcode, left, right, format);
    }
    
    void setInt32(Node* node, LValue value)
    {
        m_int32Values.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setInt52(Node* node, LValue value)
    {
        m_int52Values.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setStrictInt52(Node* node, LValue value)
    {
        m_strictInt52Values.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setInt52(Node* node, LValue value, Int52Kind kind)
    {
        switch (kind) {
        case Int52:
            setInt52(node, value);
            return;
            
        case StrictInt52:
            setStrictInt52(node, value);
            return;
        }
        
        DFG_CRASH(m_graph, m_node, "Corrupt int52 kind");
    }
    void setJSValue(Node* node, LValue value)
    {
        m_jsValueValues.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setBoolean(Node* node, LValue value)
    {
        m_booleanValues.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setStorage(Node* node, LValue value)
    {
        m_storageValues.set(node, LoweredNodeValue(value, m_highBlock));
    }
    void setDouble(Node* node, LValue value)
    {
        m_doubleValues.set(node, LoweredNodeValue(value, m_highBlock));
    }

    void setInt32(LValue value)
    {
        setInt32(m_node, value);
    }
    void setInt52(LValue value)
    {
        setInt52(m_node, value);
    }
    void setStrictInt52(LValue value)
    {
        setStrictInt52(m_node, value);
    }
    void setInt52(LValue value, Int52Kind kind)
    {
        setInt52(m_node, value, kind);
    }
    void setJSValue(LValue value)
    {
        setJSValue(m_node, value);
    }
    void setBoolean(LValue value)
    {
        setBoolean(m_node, value);
    }
    void setStorage(LValue value)
    {
        setStorage(m_node, value);
    }
    void setDouble(LValue value)
    {
        setDouble(m_node, value);
    }
    
    bool isValid(const LoweredNodeValue& value)
    {
        if (!value)
            return false;
        if (!m_graph.m_dominators.dominates(value.block(), m_highBlock))
            return false;
        return true;
    }
    
    void addWeakReference(JSCell* target)
    {
        m_graph.m_plan.weakReferences.addLazily(target);
    }

    LValue loadStructure(LValue value)
    {
        LValue tableIndex = m_out.load32(value, m_heaps.JSCell_structureID);
        LValue tableBase = m_out.loadPtr(
            m_out.absolute(vm().heap.structureIDTable().base()));
        TypedPointer address = m_out.baseIndex(
            m_heaps.structureTable, tableBase, m_out.zeroExtPtr(tableIndex));
        return m_out.loadPtr(address);
    }

    LValue weakPointer(JSCell* pointer)
    {
        addWeakReference(pointer);
        return m_out.constIntPtr(pointer);
    }

    LValue weakStructureID(Structure* structure)
    {
        addWeakReference(structure);
        return m_out.constInt32(structure->id());
    }
    
    LValue weakStructure(Structure* structure)
    {
        return weakPointer(structure);
    }
    
    TypedPointer addressFor(LValue base, int operand, ptrdiff_t offset = 0)
    {
        return m_out.address(base, m_heaps.variables[operand], offset);
    }
    TypedPointer payloadFor(LValue base, int operand)
    {
        return addressFor(base, operand, PayloadOffset);
    }
    TypedPointer tagFor(LValue base, int operand)
    {
        return addressFor(base, operand, TagOffset);
    }
    TypedPointer addressFor(int operand, ptrdiff_t offset = 0)
    {
        return addressFor(VirtualRegister(operand), offset);
    }
    TypedPointer addressFor(VirtualRegister operand, ptrdiff_t offset = 0)
    {
        if (operand.isLocal())
            return addressFor(m_captured, operand.offset(), offset);
        return addressFor(m_callFrame, operand.offset(), offset);
    }
    TypedPointer payloadFor(int operand)
    {
        return payloadFor(VirtualRegister(operand));
    }
    TypedPointer payloadFor(VirtualRegister operand)
    {
        return addressFor(operand, PayloadOffset);
    }
    TypedPointer tagFor(int operand)
    {
        return tagFor(VirtualRegister(operand));
    }
    TypedPointer tagFor(VirtualRegister operand)
    {
        return addressFor(operand, TagOffset);
    }
    
    AbstractValue abstractValue(Node* node)
    {
        return m_state.forNode(node);
    }
    AbstractValue abstractValue(Edge edge)
    {
        return abstractValue(edge.node());
    }
    
    SpeculatedType provenType(Node* node)
    {
        return abstractValue(node).m_type;
    }
    SpeculatedType provenType(Edge edge)
    {
        return provenType(edge.node());
    }
    
    JSValue provenValue(Node* node)
    {
        return abstractValue(node).m_value;
    }
    JSValue provenValue(Edge edge)
    {
        return provenValue(edge.node());
    }
    
    StructureAbstractValue abstractStructure(Node* node)
    {
        return abstractValue(node).m_structure;
    }
    StructureAbstractValue abstractStructure(Edge edge)
    {
        return abstractStructure(edge.node());
    }
    
    void crash()
    {
        crash(m_highBlock->index, m_node->index());
    }
    void crash(BlockIndex blockIndex, unsigned nodeIndex)
    {
#if ASSERT_DISABLED
        m_out.call(m_out.operation(ftlUnreachable));
        UNUSED_PARAM(blockIndex);
        UNUSED_PARAM(nodeIndex);
#else
        m_out.call(
            m_out.intToPtr(
                m_out.constIntPtr(ftlUnreachable),
                pointerType(
                    functionType(
                        m_out.voidType, m_out.intPtr, m_out.int32, m_out.int32))),
            m_out.constIntPtr(codeBlock()), m_out.constInt32(blockIndex),
            m_out.constInt32(nodeIndex));
#endif
        m_out.unreachable();
    }

    AvailabilityMap& availabilityMap() { return m_availabilityCalculator.m_availability; }
    
    VM& vm() { return m_graph.m_vm; }
    CodeBlock* codeBlock() { return m_graph.m_codeBlock; }
    
    Graph& m_graph;
    State& m_ftlState;
    AbstractHeapRepository m_heaps;
    Output m_out;
    
    LBasicBlock m_prologue;
    LBasicBlock m_handleExceptions;
    HashMap<BasicBlock*, LBasicBlock> m_blocks;
    
    LValue m_execState;
    LValue m_execStorage;
    LValue m_callFrame;
    LValue m_captured;
    LValue m_tagTypeNumber;
    LValue m_tagMask;
    
    HashMap<Node*, LoweredNodeValue> m_int32Values;
    HashMap<Node*, LoweredNodeValue> m_strictInt52Values;
    HashMap<Node*, LoweredNodeValue> m_int52Values;
    HashMap<Node*, LoweredNodeValue> m_jsValueValues;
    HashMap<Node*, LoweredNodeValue> m_booleanValues;
    HashMap<Node*, LoweredNodeValue> m_storageValues;
    HashMap<Node*, LoweredNodeValue> m_doubleValues;
    
    // This is a bit of a hack. It prevents LLVM from having to do CSE on loading of arguments.
    // It's nice to have these optimizations on our end because we can guarantee them a bit better.
    // Probably also saves LLVM compile time.
    HashMap<Node*, LValue> m_loadedArgumentValues;
    
    HashMap<Node*, LValue> m_phis;
    
    LocalOSRAvailabilityCalculator m_availabilityCalculator;
    
    Vector<AvailableRecovery, 3> m_availableRecoveries;
    
    InPlaceAbstractState m_state;
    AbstractInterpreter<InPlaceAbstractState> m_interpreter;
    BasicBlock* m_highBlock;
    BasicBlock* m_nextHighBlock;
    LBasicBlock m_nextLowBlock;
    
    CodeOrigin m_codeOriginForExitTarget;
    CodeOrigin m_codeOriginForExitProfile;
    unsigned m_nodeIndex;
    Node* m_node;
    
    uint32_t m_stackmapIDs;
    unsigned m_tbaaKind;
    unsigned m_tbaaStructKind;
};

} // anonymous namespace

void lowerDFGToLLVM(State& state)
{
    LowerDFGToLLVM lowering(state);
    lowering.lower();
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

