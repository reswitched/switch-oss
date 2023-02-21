/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
#include "DFGDoesGC.h"

#if ENABLE(DFG_JIT)

#include "DFGClobberize.h"
#include "DFGGraph.h"
#include "DFGNode.h"
#include "Operations.h"

namespace JSC { namespace DFG {

bool doesGC(Graph& graph, Node* node)
{
    if (clobbersHeap(graph, node))
        return true;
    
    // Now consider nodes that don't clobber the world but that still may GC. This includes all
    // nodes. By convention we put world-clobbering nodes in the block of "false" cases but we can
    // put them anywhere.
    switch (node->op()) {
    case JSConstant:
    case DoubleConstant:
    case Int52Constant:
    case Identity:
    case GetCallee:
    case GetArgumentCount:
    case GetLocal:
    case SetLocal:
    case MovHint:
    case ZombieHint:
    case Phantom:
    case Upsilon:
    case Phi:
    case Flush:
    case PhantomLocal:
    case GetLocalUnlinked:
    case SetArgument:
    case BitAnd:
    case BitOr:
    case BitXor:
    case BitLShift:
    case BitRShift:
    case BitURShift:
    case ValueToInt32:
    case UInt32ToNumber:
    case DoubleAsInt32:
    case ArithAdd:
    case ArithClz32:
    case ArithSub:
    case ArithNegate:
    case ArithMul:
    case ArithIMul:
    case ArithDiv:
    case ArithMod:
    case ArithAbs:
    case ArithMin:
    case ArithMax:
    case ArithPow:
    case ArithSqrt:
    case ArithRound:
    case ArithFRound:
    case ArithSin:
    case ArithCos:
    case ArithLog:
    case ValueAdd:
    case GetById:
    case GetByIdFlush:
    case PutById:
    case PutByIdFlush:
    case PutByIdDirect:
    case CheckStructure:
    case GetExecutable:
    case GetButterfly:
    case CheckArray:
    case GetScope:
    case LoadArrowFunctionThis:
    case SkipScope:
    case GetClosureVar:
    case PutClosureVar:
    case GetGlobalVar:
    case PutGlobalVar:
    case VarInjectionWatchpoint:
    case CheckCell:
    case CheckNotEmpty:
    case RegExpExec:
    case RegExpTest:
    case CompareLess:
    case CompareLessEq:
    case CompareGreater:
    case CompareGreaterEq:
    case CompareEq:
    case CompareEqConstant:
    case CompareStrictEq:
    case Call:
    case TailCallInlinedCaller:
    case Construct:
    case CallVarargs:
    case TailCallVarargsInlinedCaller:
    case ConstructVarargs:
    case LoadVarargs:
    case CallForwardVarargs:
    case ConstructForwardVarargs:
    case NativeCall:
    case NativeConstruct:
    case TailCallForwardVarargs:
    case TailCallForwardVarargsInlinedCaller:
    case Breakpoint:
    case ProfileType:
    case ProfileControlFlow:
    case CheckHasInstance:
    case InstanceOf:
    case IsJSArray:
    case IsUndefined:
    case IsBoolean:
    case IsNumber:
    case IsString:
    case IsObject:
    case IsObjectOrNull:
    case IsFunction:
    case TypeOf:
    case LogicalNot:
    case ToPrimitive:
    case ToString:
    case CallStringConstructor:
    case In:
    case Jump:
    case Branch:
    case Switch:
    case Return:
    case TailCall:
    case TailCallVarargs:
    case Throw:
    case CountExecution:
    case ForceOSRExit:
    case CheckWatchdogTimer:
    case StringFromCharCode:
    case Unreachable:
    case ExtractOSREntryLocal:
    case CheckTierUpInLoop:
    case CheckTierUpAtReturn:
    case CheckTierUpAndOSREnter:
    case CheckTierUpWithNestedTriggerAndOSREnter:
    case LoopHint:
    case StoreBarrier:
    case InvalidationPoint:
    case NotifyWrite:
    case CheckInBounds:
    case ConstantStoragePointer:
    case Check:
    case MultiGetByOffset:
    case ValueRep:
    case DoubleRep:
    case Int52Rep:
    case GetGetter:
    case GetSetter:
    case GetByVal:
    case GetIndexedPropertyStorage:
    case GetArrayLength:
    case ArrayPush:
    case ArrayPop:
    case StringCharAt:
    case StringCharCodeAt:
    case GetTypedArrayByteOffset:
    case PutByValDirect:
    case PutByVal:
    case PutByValAlias:
    case PutStructure:
    case GetByOffset:
    case GetGetterSetterByOffset:
    case PutByOffset:
    case GetEnumerableLength:
    case HasGenericProperty:
    case HasStructureProperty:
    case HasIndexedProperty:
    case GetDirectPname:
    case FiatInt52:
    case BooleanToNumber:
    case CheckBadCell:
    case BottomValue:
    case PhantomNewObject:
    case PhantomNewFunction:
    case PhantomCreateActivation:
    case PhantomDirectArguments:
    case PhantomClonedArguments:
    case GetMyArgumentByVal:
    case ForwardVarargs:
    case PutHint:
    case CheckStructureImmediate:
    case PutStack:
    case KillStack:
    case GetStack:
    case GetFromArguments:
    case PutToArguments:
        return false;

    case CreateActivation:
    case CreateDirectArguments:
    case CreateScopedArguments:
    case CreateClonedArguments:
    case ToThis:
    case CreateThis:
    case AllocatePropertyStorage:
    case ReallocatePropertyStorage:
    case Arrayify:
    case ArrayifyToStructure:
    case NewObject:
    case NewArray:
    case NewArrayWithSize:
    case NewArrayBuffer:
    case NewRegexp:
    case NewStringObject:
    case MakeRope:
    case NewArrowFunction:
    case NewFunction:
    case NewTypedArray:
    case ThrowReferenceError:
    case GetPropertyEnumerator:
    case GetEnumeratorStructurePname:
    case GetEnumeratorGenericPname:
    case ToIndexString:
    case MaterializeNewObject:
    case MaterializeCreateActivation:
        return true;
        
    case MultiPutByOffset:
        return node->multiPutByOffsetData().reallocatesStorage();

    case LastNodeType:
        RELEASE_ASSERT_NOT_REACHED();
        return true;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
    return true;
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
