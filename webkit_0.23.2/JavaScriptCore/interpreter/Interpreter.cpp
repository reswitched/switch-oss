/*
 * Copyright (C) 2008, 2009, 2010, 2012-2015 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
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

#include "config.h"
#include "Interpreter.h"

#include "BatchedTransitionOptimizer.h"
#include "CallFrameClosure.h"
#include "CallFrameInlines.h"
#include "ClonedArguments.h"
#include "CodeBlock.h"
#include "DirectArguments.h"
#include "Heap.h"
#include "Debugger.h"
#include "DebuggerCallFrame.h"
#include "ErrorInstance.h"
#include "EvalCodeCache.h"
#include "Exception.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "JSArray.h"
#include "JSArrowFunction.h"
#include "JSBoundFunction.h"
#include "JSCInlines.h"
#include "JSLexicalEnvironment.h"
#include "JSNameScope.h"
#include "JSNotAnObject.h"
#include "JSStackInlines.h"
#include "JSString.h"
#include "JSWithScope.h"
#include "LLIntCLoop.h"
#include "LLIntThunks.h"
#include "LiteralParser.h"
#include "ObjectPrototype.h"
#include "Parser.h"
#include "ProtoCallFrame.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "Register.h"
#include "SamplingTool.h"
#include "ScopedArguments.h"
#include "StackAlignment.h"
#include "StackVisitor.h"
#include "StrictEvalActivation.h"
#include "StrongInlines.h"
#include "Symbol.h"
#include "VMEntryScope.h"
#include "VirtualRegister.h"

#include <limits.h>
#include <stdio.h>
#include <wtf/StackStats.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringPrintStream.h>
#include <wtf/Threading.h>
#include <wtf/WTFThreadData.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(JIT)
#include "JIT.h"
#endif

using namespace std;

namespace JSC {

#if PLATFORM(WKC)
static bool isStackOverflow(unsigned int margin)
{
    size_t consumption;
    void* stack_top;
    bool stack_overflow;

    stack_overflow = wkcMemoryIsStackOverflowPeer(margin, &consumption, &stack_top);
    return stack_overflow;
}
#define IS_STACK_OVERFLOW(margin)   JSC::isStackOverflow((margin))
#endif

String StackFrame::friendlySourceURL() const
{
    String traceLine;
    
    switch (codeType) {
    case StackFrameEvalCode:
    case StackFrameFunctionCode:
    case StackFrameGlobalCode:
        if (!sourceURL.isEmpty())
            traceLine = sourceURL.impl();
        break;
    case StackFrameNativeCode:
        traceLine = "[native code]";
        break;
    }
    return traceLine.isNull() ? emptyString() : traceLine;
}

String StackFrame::friendlyFunctionName(CallFrame* callFrame) const
{
    String traceLine;
    JSObject* stackFrameCallee = callee.get();

    switch (codeType) {
    case StackFrameEvalCode:
        traceLine = "eval code";
        break;
    case StackFrameNativeCode:
        if (callee)
            traceLine = getCalculatedDisplayName(callFrame, stackFrameCallee).impl();
        break;
    case StackFrameFunctionCode:
        traceLine = getCalculatedDisplayName(callFrame, stackFrameCallee).impl();
        break;
    case StackFrameGlobalCode:
        traceLine = "global code";
        break;
    }
    return traceLine.isNull() ? emptyString() : traceLine;
}

JSValue eval(CallFrame* callFrame)
{
    if (!callFrame->argumentCount())
        return jsUndefined();

    JSValue program = callFrame->argument(0);
    if (!program.isString())
        return program;

    TopCallFrameSetter topCallFrame(callFrame->vm(), callFrame);
    JSGlobalObject* globalObject = callFrame->lexicalGlobalObject();
    if (!globalObject->evalEnabled()) {
        callFrame->vm().throwException(callFrame, createEvalError(callFrame, globalObject->evalDisabledErrorMessage()));
        return jsUndefined();
    }
    String programSource = asString(program)->value(callFrame);
    if (callFrame->hadException())
        return JSValue();
    
    CallFrame* callerFrame = callFrame->callerFrame();
    CodeBlock* callerCodeBlock = callerFrame->codeBlock();
    JSScope* callerScopeChain = callerFrame->uncheckedR(callerCodeBlock->scopeRegister().offset()).Register::scope();
    EvalExecutable* eval = callerCodeBlock->evalCodeCache().tryGet(callerCodeBlock->isStrictMode(), programSource, callerScopeChain);

    if (!eval) {
        if (!callerCodeBlock->isStrictMode()) {
            if (programSource.is8Bit()) {
                LiteralParser<LChar> preparser(callFrame, programSource.characters8(), programSource.length(), NonStrictJSON);
                if (JSValue parsedObject = preparser.tryLiteralParse())
                    return parsedObject;
            } else {
                LiteralParser<UChar> preparser(callFrame, programSource.characters16(), programSource.length(), NonStrictJSON);
                if (JSValue parsedObject = preparser.tryLiteralParse())
                    return parsedObject;                
            }
        }
        
        // If the literal parser bailed, it should not have thrown exceptions.
        ASSERT(!callFrame->vm().exception());

        ThisTDZMode thisTDZMode = callerCodeBlock->unlinkedCodeBlock()->constructorKind() == ConstructorKind::Derived ? ThisTDZMode::AlwaysCheck : ThisTDZMode::CheckIfNeeded;
        eval = callerCodeBlock->evalCodeCache().getSlow(callFrame, callerCodeBlock->ownerExecutable(), callerCodeBlock->isStrictMode(), thisTDZMode, programSource, callerScopeChain);
        if (!eval)
            return jsUndefined();
    }

    JSValue thisValue = callerFrame->thisValue();
    Interpreter* interpreter = callFrame->vm().interpreter;
    return interpreter->execute(eval, callFrame, thisValue, callerScopeChain);
}

unsigned sizeOfVarargs(CallFrame* callFrame, JSValue arguments, uint32_t firstVarArgOffset)
{
    if (UNLIKELY(!arguments.isCell())) {
        if (arguments.isUndefinedOrNull())
            return 0;
        
        callFrame->vm().throwException(callFrame, createInvalidFunctionApplyParameterError(callFrame, arguments));
        return 0;
    }
    
    JSCell* cell = arguments.asCell();
    unsigned length;
    switch (cell->type()) {
    case DirectArgumentsType:
        length = jsCast<DirectArguments*>(cell)->length(callFrame);
        break;
    case ScopedArgumentsType:
        length =jsCast<ScopedArguments*>(cell)->length(callFrame);
        break;
    case StringType:
        callFrame->vm().throwException(callFrame, createInvalidFunctionApplyParameterError(callFrame,  arguments));
        return 0;
    default:
        ASSERT(arguments.isObject());
        if (isJSArray(cell))
            length = jsCast<JSArray*>(cell)->length();
        else
            length = jsCast<JSObject*>(cell)->get(callFrame, callFrame->propertyNames().length).toUInt32(callFrame);
        break;
    }
    
    if (length >= firstVarArgOffset)
        length -= firstVarArgOffset;
    else
        length = 0;
    
    return length;
}

unsigned sizeFrameForForwardArguments(CallFrame* callFrame, JSStack* stack, unsigned numUsedStackSlots)
{
    unsigned length = callFrame->argumentCount();
    CallFrame* calleeFrame = calleeFrameForVarargs(callFrame, numUsedStackSlots, length + 1);
    if (!stack->ensureCapacityFor(calleeFrame->registers()))
        throwStackOverflowError(callFrame);

    return length;
}

unsigned sizeFrameForVarargs(CallFrame* callFrame, JSStack* stack, JSValue arguments, unsigned numUsedStackSlots, uint32_t firstVarArgOffset)
{
    unsigned length = sizeOfVarargs(callFrame, arguments, firstVarArgOffset);
    
    CallFrame* calleeFrame = calleeFrameForVarargs(callFrame, numUsedStackSlots, length + 1);
    if (length > maxArguments || !stack->ensureCapacityFor(calleeFrame->registers())) {
        throwStackOverflowError(callFrame);
        return 0;
    }
    
    return length;
}

void loadVarargs(CallFrame* callFrame, VirtualRegister firstElementDest, JSValue arguments, uint32_t offset, uint32_t length)
{
    if (UNLIKELY(!arguments.isCell()))
        return;
    
    JSCell* cell = arguments.asCell();
    switch (cell->type()) {
    case DirectArgumentsType:
        jsCast<DirectArguments*>(cell)->copyToArguments(callFrame, firstElementDest, offset, length);
        return;
    case ScopedArgumentsType:
        jsCast<ScopedArguments*>(cell)->copyToArguments(callFrame, firstElementDest, offset, length);
        return;
    default: {
        ASSERT(arguments.isObject());
        JSObject* object = jsCast<JSObject*>(cell);
        if (isJSArray(object)) {
            jsCast<JSArray*>(object)->copyToArguments(callFrame, firstElementDest, offset, length);
            return;
        }
        unsigned i;
        for (i = 0; i < length && object->canGetIndexQuickly(i + offset); ++i)
            callFrame->r(firstElementDest + i) = object->getIndexQuickly(i + offset);
        for (; i < length; ++i)
            callFrame->r(firstElementDest + i) = object->get(callFrame, i + offset);
        return;
    } }
}

void setupVarargsFrame(CallFrame* callFrame, CallFrame* newCallFrame, JSValue arguments, uint32_t offset, uint32_t length)
{
    VirtualRegister calleeFrameOffset(newCallFrame - callFrame);
    
    loadVarargs(
        callFrame,
        calleeFrameOffset + CallFrame::argumentOffset(0),
        arguments, offset, length);
    
    newCallFrame->setArgumentCountIncludingThis(length + 1);
}

void setupVarargsFrameAndSetThis(CallFrame* callFrame, CallFrame* newCallFrame, JSValue thisValue, JSValue arguments, uint32_t firstVarArgOffset, uint32_t length)
{
    setupVarargsFrame(callFrame, newCallFrame, arguments, firstVarArgOffset, length);
    newCallFrame->setThisValue(thisValue);
}

void setupForwardArgumentsFrame(CallFrame* execCaller, CallFrame* execCallee, uint32_t length)
{
    ASSERT(length == execCaller->argumentCount());
    unsigned offset = execCaller->argumentOffset(0) * sizeof(Register);
    memcpy(reinterpret_cast<char*>(execCallee) + offset, reinterpret_cast<char*>(execCaller) + offset, length * sizeof(Register));
    execCallee->setArgumentCountIncludingThis(length + 1);
}

void setupForwardArgumentsFrameAndSetThis(CallFrame* execCaller, CallFrame* execCallee, JSValue thisValue, uint32_t length)
{
    setupForwardArgumentsFrame(execCaller, execCallee, length);
    execCallee->setThisValue(thisValue);
}

    

Interpreter::Interpreter(VM& vm)
    : m_sampleEntryDepth(0)
    , m_vm(vm)
    , m_stack(vm)
    , m_errorHandlingModeReentry(0)
#if !ASSERT_DISABLED
    , m_initialized(false)
#endif
{
}

Interpreter::~Interpreter()
{
}

void Interpreter::initialize(bool canUseJIT)
{
    UNUSED_PARAM(canUseJIT);

#if ENABLE(COMPUTED_GOTO_OPCODES)
    m_opcodeTable = LLInt::opcodeMap();
    for (int i = 0; i < numOpcodeIDs; ++i)
        m_opcodeIDTable.add(m_opcodeTable[i], static_cast<OpcodeID>(i));
#endif

#if !ASSERT_DISABLED
    m_initialized = true;
#endif

#if ENABLE(OPCODE_SAMPLING)
    enableSampler();
#endif
}

#ifdef NDEBUG

void Interpreter::dumpCallFrame(CallFrame*)
{
}

#else

void Interpreter::dumpCallFrame(CallFrame* callFrame)
{
    callFrame->codeBlock()->dumpBytecode();
    dumpRegisters(callFrame);
}

class DumpRegisterFunctor {
public:
    DumpRegisterFunctor(const Register*& it)
        : m_hasSkippedFirstFrame(false)
        , m_it(it)
    {
    }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        if (!m_hasSkippedFirstFrame) {
            m_hasSkippedFirstFrame = true;
            return StackVisitor::Continue;
        }

        unsigned line = 0;
        unsigned unusedColumn = 0;
        visitor->computeLineAndColumn(line, unusedColumn);
        dataLogF("[ReturnVPC]                | %10p | %d (line %d)\n", m_it, visitor->bytecodeOffset(), line);
        --m_it;
        return StackVisitor::Done;
    }

private:
    bool m_hasSkippedFirstFrame;
    const Register*& m_it;
};

void Interpreter::dumpRegisters(CallFrame* callFrame)
{
    dataLogF("Register frame: \n\n");
    dataLogF("-----------------------------------------------------------------------------\n");
    dataLogF("            use            |   address  |                value               \n");
    dataLogF("-----------------------------------------------------------------------------\n");

    CodeBlock* codeBlock = callFrame->codeBlock();
    const Register* it;
    const Register* end;

    it = callFrame->registers() + JSStack::ThisArgument + callFrame->argumentCount();
    end = callFrame->registers() + JSStack::ThisArgument - 1;
    while (it > end) {
        JSValue v = it->jsValue();
        int registerNumber = it - callFrame->registers();
        String name = codeBlock->nameForRegister(VirtualRegister(registerNumber));
        dataLogF("[r% 3d %14s]      | %10p | %-16s 0x%lld \n", registerNumber, name.ascii().data(), it, toCString(v).data(), (long long)JSValue::encode(v));
        --it;
    }
    
    dataLogF("-----------------------------------------------------------------------------\n");
    dataLogF("[ArgumentCount]            | %10p | %lu \n", it, (unsigned long) callFrame->argumentCount());
    --it;
    dataLogF("[CallerFrame]              | %10p | %p \n", it, callFrame->callerFrame());
    --it;
    dataLogF("[Callee]                   | %10p | %p \n", it, callFrame->callee());
    --it;
    // FIXME: Remove the next decrement when the ScopeChain slot is removed from the call header
    --it;
#if ENABLE(JIT)
    AbstractPC pc = callFrame->abstractReturnPC(callFrame->vm());
    if (pc.hasJITReturnAddress())
        dataLogF("[ReturnJITPC]              | %10p | %p \n", it, pc.jitReturnAddress().value());
#endif

    DumpRegisterFunctor functor(it);
    callFrame->iterate(functor);

    dataLogF("[CodeBlock]                | %10p | %p \n", it, callFrame->codeBlock());
    --it;
    dataLogF("-----------------------------------------------------------------------------\n");

    end = it - codeBlock->m_numVars;
    if (it != end) {
        do {
            JSValue v = it->jsValue();
            int registerNumber = it - callFrame->registers();
            String name = codeBlock->nameForRegister(VirtualRegister(registerNumber));
            dataLogF("[r% 3d %14s]      | %10p | %-16s 0x%lld \n", registerNumber, name.ascii().data(), it, toCString(v).data(), (long long)JSValue::encode(v));
            --it;
        } while (it != end);
    }
    dataLogF("-----------------------------------------------------------------------------\n");

    end = it - codeBlock->m_numCalleeRegisters + codeBlock->m_numVars;
    if (it != end) {
        do {
            JSValue v = (*it).jsValue();
            int registerNumber = it - callFrame->registers();
            dataLogF("[r% 3d]                     | %10p | %-16s 0x%lld \n", registerNumber, it, toCString(v).data(), (long long)JSValue::encode(v));
            --it;
        } while (it != end);
    }
    dataLogF("-----------------------------------------------------------------------------\n");
}

#endif

bool Interpreter::isOpcode(Opcode opcode)
{
#if ENABLE(COMPUTED_GOTO_OPCODES)
    return opcode != HashTraits<Opcode>::emptyValue()
        && !HashTraits<Opcode>::isDeletedValue(opcode)
        && m_opcodeIDTable.contains(opcode);
#else
    return opcode >= 0 && opcode <= op_end;
#endif
}

static bool unwindCallFrame(StackVisitor& visitor)
{
    CallFrame* callFrame = visitor->callFrame();
    if (Debugger* debugger = callFrame->vmEntryGlobalObject()->debugger()) {
        SuspendExceptionScope scope(&callFrame->vm());
        if (jsDynamicCast<JSFunction*>(callFrame->callee()))
            debugger->returnEvent(callFrame);
        else
            debugger->didExecuteProgram(callFrame);
        ASSERT(!callFrame->hadException());
    }

    return !visitor->callerIsVMEntryFrame();
}

static StackFrameCodeType getStackFrameCodeType(StackVisitor& visitor)
{
    switch (visitor->codeType()) {
    case StackVisitor::Frame::Eval:
        return StackFrameEvalCode;
    case StackVisitor::Frame::Function:
        return StackFrameFunctionCode;
    case StackVisitor::Frame::Global:
        return StackFrameGlobalCode;
    case StackVisitor::Frame::Native:
        ASSERT_NOT_REACHED();
        return StackFrameNativeCode;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return StackFrameGlobalCode;
}

void StackFrame::computeLineAndColumn(unsigned& line, unsigned& column)
{
    if (!codeBlock) {
        line = 0;
        column = 0;
        return;
    }

    int divot = 0;
    int unusedStartOffset = 0;
    int unusedEndOffset = 0;
    unsigned divotLine = 0;
    unsigned divotColumn = 0;
    expressionInfo(divot, unusedStartOffset, unusedEndOffset, divotLine, divotColumn);

    line = divotLine + lineOffset;
    column = divotColumn + (divotLine ? 1 : firstLineColumnOffset);

    if (executable->hasOverrideLineNumber())
        line = executable->overrideLineNumber();
}

void StackFrame::expressionInfo(int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column)
{
    codeBlock->expressionRangeForBytecodeOffset(bytecodeOffset, divot, startOffset, endOffset, line, column);
    divot += characterOffset;
}

String StackFrame::toString(CallFrame* callFrame)
{
    StringBuilder traceBuild;
    String functionName = friendlyFunctionName(callFrame);
    String sourceURL = friendlySourceURL();
    traceBuild.append(functionName);
    if (!sourceURL.isEmpty()) {
        if (!functionName.isEmpty())
            traceBuild.append('@');
        traceBuild.append(sourceURL);
        if (codeType != StackFrameNativeCode) {
            unsigned line;
            unsigned column;
            computeLineAndColumn(line, column);

            traceBuild.append(':');
            traceBuild.appendNumber(line);
            traceBuild.append(':');
            traceBuild.appendNumber(column);
        }
    }
    return traceBuild.toString().impl();
}

class GetStackTraceFunctor {
public:
    GetStackTraceFunctor(VM& vm, Vector<StackFrame>& results, size_t remainingCapacity)
        : m_vm(vm)
        , m_results(results)
        , m_remainingCapacityForFrameCapture(remainingCapacity)
    {
    }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        VM& vm = m_vm;
        if (m_remainingCapacityForFrameCapture) {
            if (visitor->isJSFrame() && !visitor->codeBlock()->unlinkedCodeBlock()->isBuiltinFunction()) {
                CodeBlock* codeBlock = visitor->codeBlock();
                StackFrame s = {
                    Strong<JSObject>(vm, visitor->callee()),
                    getStackFrameCodeType(visitor),
                    Strong<ScriptExecutable>(vm, codeBlock->ownerExecutable()),
                    Strong<UnlinkedCodeBlock>(vm, codeBlock->unlinkedCodeBlock()),
                    codeBlock->source(),
                    codeBlock->ownerExecutable()->firstLine(),
                    codeBlock->firstLineColumnOffset(),
                    codeBlock->sourceOffset(),
                    visitor->bytecodeOffset(),
                    visitor->sourceURL()
                };
                m_results.append(s);
            } else {
                StackFrame s = { Strong<JSObject>(vm, visitor->callee()), StackFrameNativeCode, Strong<ScriptExecutable>(), Strong<UnlinkedCodeBlock>(), 0, 0, 0, 0, 0, String()};
                m_results.append(s);
            }
    
            m_remainingCapacityForFrameCapture--;
            return StackVisitor::Continue;
        }
        return StackVisitor::Done;
    }

private:
    VM& m_vm;
    Vector<StackFrame>& m_results;
    size_t m_remainingCapacityForFrameCapture;
};

void Interpreter::getStackTrace(Vector<StackFrame>& results, size_t maxStackSize)
{
    VM& vm = m_vm;
    CallFrame* callFrame = vm.topCallFrame;
    if (!callFrame)
        return;

    GetStackTraceFunctor functor(vm, results, maxStackSize);
    callFrame->iterate(functor);
}

JSString* Interpreter::stackTraceAsString(ExecState* exec, Vector<StackFrame> stackTrace)
{
    // FIXME: JSStringJoiner could be more efficient than StringBuilder here.
    StringBuilder builder;
    for (unsigned i = 0; i < stackTrace.size(); i++) {
        builder.append(String(stackTrace[i].toString(exec)));
        if (i != stackTrace.size() - 1)
            builder.append('\n');
    }
    return jsString(&exec->vm(), builder.toString());
}

class GetCatchHandlerFunctor {
public:
    GetCatchHandlerFunctor()
        : m_handler(0)
    {
    }

    HandlerInfo* handler() { return m_handler; }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        CodeBlock* codeBlock = visitor->codeBlock();
        if (!codeBlock)
            return StackVisitor::Continue;

        unsigned bytecodeOffset = visitor->bytecodeOffset();
        m_handler = codeBlock->handlerForBytecodeOffset(bytecodeOffset, CodeBlock::RequiredHandler::CatchHandler);
        if (m_handler)
            return StackVisitor::Done;

        return StackVisitor::Continue;
    }

private:
    HandlerInfo* m_handler;
};

class UnwindFunctor {
public:
    UnwindFunctor(CallFrame*& callFrame, bool isTermination, CodeBlock*& codeBlock, HandlerInfo*& handler)
        : m_callFrame(callFrame)
        , m_isTermination(isTermination)
        , m_codeBlock(codeBlock)
        , m_handler(handler)
    {
    }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        m_callFrame = visitor->callFrame();
        m_codeBlock = visitor->codeBlock();
        unsigned bytecodeOffset = visitor->bytecodeOffset();

        if (m_isTermination || !(m_handler = m_codeBlock ? m_codeBlock->handlerForBytecodeOffset(bytecodeOffset) : nullptr)) {
            if (!unwindCallFrame(visitor)) {
                copyCalleeSavesToVMCalleeSavesBuffer(visitor);

                return StackVisitor::Done;
            }
        } else
            return StackVisitor::Done;

        copyCalleeSavesToVMCalleeSavesBuffer(visitor);

        return StackVisitor::Continue;
    }

private:
    void copyCalleeSavesToVMCalleeSavesBuffer(StackVisitor& visitor)
    {
#if ENABLE(JIT) && NUMBER_OF_CALLEE_SAVES_REGISTERS > 0

        if (!visitor->isJSFrame())
            return;

#if ENABLE(DFG_JIT)
        if (visitor->inlineCallFrame())
            return;
#endif
        RegisterAtOffsetList* currentCalleeSaves = m_codeBlock ? m_codeBlock->calleeSaveRegisters() : nullptr;

        if (!currentCalleeSaves)
            return;

        VM& vm = m_callFrame->vm();
        RegisterAtOffsetList* allCalleeSaves = vm.getAllCalleeSaveRegisterOffsets();
        RegisterSet dontCopyRegisters = RegisterSet::stackRegisters();
        intptr_t* frame = reinterpret_cast<intptr_t*>(m_callFrame->registers());

        unsigned registerCount = currentCalleeSaves->size();
        for (unsigned i = 0; i < registerCount; i++) {
            RegisterAtOffset currentEntry = currentCalleeSaves->at(i);
            if (dontCopyRegisters.get(currentEntry.reg()))
                continue;
            RegisterAtOffset* vmCalleeSavesEntry = allCalleeSaves->find(currentEntry.reg());
            
            vm.calleeSaveRegistersBuffer[vmCalleeSavesEntry->offsetAsIndex()] = *(frame + currentEntry.offsetAsIndex());
        }
#else
        UNUSED_PARAM(visitor);
#endif
    }

    CallFrame*& m_callFrame;
    bool m_isTermination;
    CodeBlock*& m_codeBlock;
    HandlerInfo*& m_handler;
};

NEVER_INLINE HandlerInfo* Interpreter::unwind(VM& vm, CallFrame*& callFrame, Exception* exception, UnwindStart unwindStart)
{
    if (unwindStart == UnwindFromCallerFrame) {
        if (callFrame->callerFrameOrVMEntryFrame() == vm.topVMEntryFrame)
            return nullptr;

        callFrame = callFrame->callerFrame();
        vm.topCallFrame = callFrame;
    }

    CodeBlock* codeBlock = callFrame->codeBlock();
    bool isTermination = false;

    JSValue exceptionValue = exception->value();
    ASSERT(!exceptionValue.isEmpty());
    ASSERT(!exceptionValue.isCell() || exceptionValue.asCell());
    // This shouldn't be possible (hence the assertions), but we're already in the slowest of
    // slow cases, so let's harden against it anyway to be safe.
    if (exceptionValue.isEmpty() || (exceptionValue.isCell() && !exceptionValue.asCell()))
        exceptionValue = jsNull();

    if (exceptionValue.isObject())
        isTermination = isTerminatedExecutionException(exception);

    ASSERT(vm.exception() && vm.exception()->stack().size());

    Debugger* debugger = callFrame->vmEntryGlobalObject()->debugger();
    if (debugger && debugger->needsExceptionCallbacks() && !exception->didNotifyInspectorOfThrow()) {
        // We need to clear the exception here in order to see if a new exception happens.
        // Afterwards, the values are put back to continue processing this error.
        SuspendExceptionScope scope(&vm);
        // This code assumes that if the debugger is enabled then there is no inlining.
        // If that assumption turns out to be false then we'll ignore the inlined call
        // frames.
        // https://bugs.webkit.org/show_bug.cgi?id=121754

        bool hasCatchHandler;
        if (isTermination)
            hasCatchHandler = false;
        else {
            GetCatchHandlerFunctor functor;
            callFrame->iterate(functor);
            HandlerInfo* handler = functor.handler();
            ASSERT(!handler || handler->isCatchHandler());
            hasCatchHandler = !!handler;
        }

        debugger->exception(callFrame, exceptionValue, hasCatchHandler);
        ASSERT(!callFrame->hadException());
    }
    exception->setDidNotifyInspectorOfThrow();

    // Calculate an exception handler vPC, unwinding call frames as necessary.
    HandlerInfo* handler = nullptr;
    UnwindFunctor functor(callFrame, isTermination, codeBlock, handler);
    callFrame->iterate(functor);
    if (!handler)
        return nullptr;

    // Unwind the scope chain within the exception handler's call frame.
    int targetScopeDepth = handler->scopeDepth;
    if (codeBlock->needsActivation() && callFrame->hasActivation())
        ++targetScopeDepth;

    int scopeRegisterOffset = codeBlock->scopeRegister().offset();
    JSScope* scope = callFrame->scope(scopeRegisterOffset);
    int scopeDelta = scope->depth() - targetScopeDepth;
    RELEASE_ASSERT(scopeDelta >= 0);

    while (scopeDelta--)
        scope = scope->next();

    callFrame->setScope(scopeRegisterOffset, scope);

    return handler;
}

static inline JSValue checkedReturn(JSValue returnValue)
{
    ASSERT(returnValue);
    return returnValue;
}

static inline JSObject* checkedReturn(JSObject* returnValue)
{
    ASSERT(returnValue);
    return returnValue;
}

class SamplingScope {
public:
    SamplingScope(Interpreter* interpreter)
        : m_interpreter(interpreter)
    {
        interpreter->startSampling();
    }
    ~SamplingScope()
    {
        m_interpreter->stopSampling();
    }
private:
    Interpreter* m_interpreter;
};

JSValue Interpreter::execute(ProgramExecutable* program, CallFrame* callFrame, JSObject* thisObj)
{
    SamplingScope samplingScope(this);

    JSScope* scope = thisObj->globalObject();
    VM& vm = *scope->vm();

    ASSERT(!vm.exception());
    ASSERT(!vm.isCollectorBusy());
    RELEASE_ASSERT(vm.currentThreadIsHoldingAPILock());
    if (vm.isCollectorBusy())
        return jsNull();

    if (!vm.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(callFrame));
    }
#endif

    // First check if the "program" is actually just a JSON object. If so,
    // we'll handle the JSON object here. Else, we'll handle real JS code
    // below at failedJSONP.

    Vector<JSONPData> JSONPData;
    bool parseResult;
    const String programSource = program->source().toString();
    if (programSource.isNull())
        return jsUndefined();
    if (programSource.is8Bit()) {
        LiteralParser<LChar> literalParser(callFrame, programSource.characters8(), programSource.length(), JSONP);
        parseResult = literalParser.tryJSONPParse(JSONPData, scope->globalObject()->globalObjectMethodTable()->supportsRichSourceInfo(scope->globalObject()));
    } else {
        LiteralParser<UChar> literalParser(callFrame, programSource.characters16(), programSource.length(), JSONP);
        parseResult = literalParser.tryJSONPParse(JSONPData, scope->globalObject()->globalObjectMethodTable()->supportsRichSourceInfo(scope->globalObject()));
    }

    if (parseResult) {
        JSGlobalObject* globalObject = scope->globalObject();
        JSValue result;
        for (unsigned entry = 0; entry < JSONPData.size(); entry++) {
            Vector<JSONPPathEntry> JSONPPath;
            JSONPPath.swap(JSONPData[entry].m_path);
            JSValue JSONPValue = JSONPData[entry].m_value.get();
            if (JSONPPath.size() == 1 && JSONPPath[0].m_type == JSONPPathEntryTypeDeclare) {
                globalObject->addVar(callFrame, JSONPPath[0].m_pathEntryName);
                PutPropertySlot slot(globalObject);
                globalObject->methodTable()->put(globalObject, callFrame, JSONPPath[0].m_pathEntryName, JSONPValue, slot);
                result = jsUndefined();
                continue;
            }
            JSValue baseObject(globalObject);
            for (unsigned i = 0; i < JSONPPath.size() - 1; i++) {
                ASSERT(JSONPPath[i].m_type != JSONPPathEntryTypeDeclare);
                switch (JSONPPath[i].m_type) {
                case JSONPPathEntryTypeDot: {
                    if (i == 0) {
                        PropertySlot slot(globalObject);
                        if (!globalObject->getPropertySlot(callFrame, JSONPPath[i].m_pathEntryName, slot)) {
                            if (entry)
                                return callFrame->vm().throwException(callFrame, createUndefinedVariableError(callFrame, JSONPPath[i].m_pathEntryName));
                            goto failedJSONP;
                        }
                        baseObject = slot.getValue(callFrame, JSONPPath[i].m_pathEntryName);
                    } else
                        baseObject = baseObject.get(callFrame, JSONPPath[i].m_pathEntryName);
                    if (callFrame->hadException())
                        return jsUndefined();
                    continue;
                }
                case JSONPPathEntryTypeLookup: {
                    baseObject = baseObject.get(callFrame, JSONPPath[i].m_pathIndex);
                    if (callFrame->hadException())
                        return jsUndefined();
                    continue;
                }
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                    return jsUndefined();
                }
            }
            PutPropertySlot slot(baseObject);
            switch (JSONPPath.last().m_type) {
            case JSONPPathEntryTypeCall: {
                JSValue function = baseObject.get(callFrame, JSONPPath.last().m_pathEntryName);
                if (callFrame->hadException())
                    return jsUndefined();
                CallData callData;
                CallType callType = getCallData(function, callData);
                if (callType == CallTypeNone)
                    return callFrame->vm().throwException(callFrame, createNotAFunctionError(callFrame, function));
                MarkedArgumentBuffer jsonArg;
                jsonArg.append(JSONPValue);
                JSValue thisValue = JSONPPath.size() == 1 ? jsUndefined(): baseObject;
                JSONPValue = JSC::call(callFrame, function, callType, callData, thisValue, jsonArg);
                if (callFrame->hadException())
                    return jsUndefined();
                break;
            }
            case JSONPPathEntryTypeDot: {
                baseObject.put(callFrame, JSONPPath.last().m_pathEntryName, JSONPValue, slot);
                if (callFrame->hadException())
                    return jsUndefined();
                break;
            }
            case JSONPPathEntryTypeLookup: {
                baseObject.putByIndex(callFrame, JSONPPath.last().m_pathIndex, JSONPValue, slot.isStrictMode());
                if (callFrame->hadException())
                    return jsUndefined();
                break;
            }
            default:
                RELEASE_ASSERT_NOT_REACHED();
                    return jsUndefined();
            }
            result = JSONPValue;
        }
        return result;
    }
failedJSONP:
    // If we get here, then we have already proven that the script is not a JSON
    // object.

    VMEntryScope entryScope(vm, scope->globalObject());

    // Compile source to bytecode if necessary:
    if (JSObject* error = program->initializeGlobalProperties(vm, callFrame, scope))
        return checkedReturn(callFrame->vm().throwException(callFrame, error));

    if (JSObject* error = program->prepareForExecution(callFrame, nullptr, scope, CodeForCall))
        return checkedReturn(callFrame->vm().throwException(callFrame, error));

    ProgramCodeBlock* codeBlock = program->codeBlock();

    if (UNLIKELY(vm.watchdog && vm.watchdog->didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    ASSERT(codeBlock->numParameters() == 1); // 1 parameter for 'this'.

    ProtoCallFrame protoCallFrame;
    protoCallFrame.init(codeBlock, JSCallee::create(vm, scope->globalObject(), scope), thisObj, 1);

    // Execute the code:
    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get());
        Watchdog::Scope watchdogScope(vm.watchdog.get());

        result = program->generatedJITCode()->execute(&vm, &protoCallFrame);
    }

    return checkedReturn(result);
}

JSValue Interpreter::executeCall(CallFrame* callFrame, JSObject* function, CallType callType, const CallData& callData, JSValue thisValue, const ArgList& args)
{
    VM& vm = callFrame->vm();
    ASSERT(!callFrame->hadException());
    ASSERT(!vm.isCollectorBusy());
    if (vm.isCollectorBusy())
        return jsNull();

    bool isJSCall = (callType == CallTypeJS);
    JSScope* scope = nullptr;
    CodeBlock* newCodeBlock;
    size_t argsCount = 1 + args.size(); // implicit "this" parameter

    JSGlobalObject* globalObject;

    if (isJSCall) {
        scope = callData.js.scope;
        globalObject = scope->globalObject();
    } else {
        ASSERT(callType == CallTypeHost);
        globalObject = function->globalObject();
    }

    VMEntryScope entryScope(vm, globalObject);
    if (!vm.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(callFrame));
    }
#endif

    if (isJSCall) {
        // Compile the callee:
        JSObject* compileError = callData.js.functionExecutable->prepareForExecution(callFrame, jsCast<JSFunction*>(function), scope, CodeForCall);
        if (UNLIKELY(!!compileError)) {
            return checkedReturn(callFrame->vm().throwException(callFrame, compileError));
        }
        newCodeBlock = callData.js.functionExecutable->codeBlockForCall();
        ASSERT(!!newCodeBlock);
        newCodeBlock->m_shouldAlwaysBeInlined = false;
    } else
        newCodeBlock = 0;

    if (UNLIKELY(vm.watchdog && vm.watchdog->didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    ProtoCallFrame protoCallFrame;
    protoCallFrame.init(newCodeBlock, function, thisValue, argsCount, args.data());

    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get(), !isJSCall);
        Watchdog::Scope watchdogScope(vm.watchdog.get());

        // Execute the code:
        if (isJSCall)
            result = callData.js.functionExecutable->generatedJITCodeForCall()->execute(&vm, &protoCallFrame);
        else {
            result = JSValue::decode(vmEntryToNative(reinterpret_cast<void*>(callData.native.function), &vm, &protoCallFrame));
            if (callFrame->hadException())
                result = jsNull();
        }
    }

    return checkedReturn(result);
}

JSObject* Interpreter::executeConstruct(CallFrame* callFrame, JSObject* constructor, ConstructType constructType, const ConstructData& constructData, const ArgList& args, JSValue newTarget)
{
    VM& vm = callFrame->vm();
    ASSERT(!callFrame->hadException());
    ASSERT(!vm.isCollectorBusy());
    // We throw in this case because we have to return something "valid" but we're
    // already in an invalid state.
    if (vm.isCollectorBusy())
        return checkedReturn(throwStackOverflowError(callFrame));

    bool isJSConstruct = (constructType == ConstructTypeJS);
    JSScope* scope = nullptr;
    CodeBlock* newCodeBlock;
    size_t argsCount = 1 + args.size(); // implicit "this" parameter

    JSGlobalObject* globalObject;

    if (isJSConstruct) {
        scope = constructData.js.scope;
        globalObject = scope->globalObject();
    } else {
        ASSERT(constructType == ConstructTypeHost);
        globalObject = constructor->globalObject();
    }

    VMEntryScope entryScope(vm, globalObject);
    if (!vm.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        return checkedReturn(throwStackOverflowError(callFrame));
    }
#endif

    if (isJSConstruct) {
        // Compile the callee:
        JSObject* compileError = constructData.js.functionExecutable->prepareForExecution(callFrame, jsCast<JSFunction*>(constructor), scope, CodeForConstruct);
        if (UNLIKELY(!!compileError)) {
            return checkedReturn(callFrame->vm().throwException(callFrame, compileError));
        }
        newCodeBlock = constructData.js.functionExecutable->codeBlockForConstruct();
        ASSERT(!!newCodeBlock);
        newCodeBlock->m_shouldAlwaysBeInlined = false;
    } else
        newCodeBlock = 0;

    if (UNLIKELY(vm.watchdog && vm.watchdog->didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    ProtoCallFrame protoCallFrame;
    protoCallFrame.init(newCodeBlock, constructor, newTarget, argsCount, args.data());

    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get(), !isJSConstruct);
        Watchdog::Scope watchdogScope(vm.watchdog.get());

        // Execute the code.
        if (isJSConstruct)
            result = constructData.js.functionExecutable->generatedJITCodeForConstruct()->execute(&vm, &protoCallFrame);
        else {
            result = JSValue::decode(vmEntryToNative(reinterpret_cast<void*>(constructData.native.function), &vm, &protoCallFrame));

            if (!callFrame->hadException())
                RELEASE_ASSERT(result.isObject());
        }
    }

    if (callFrame->hadException())
        return 0;
    ASSERT(result.isObject());
    return checkedReturn(asObject(result));
}

CallFrameClosure Interpreter::prepareForRepeatCall(FunctionExecutable* functionExecutable, CallFrame* callFrame, ProtoCallFrame* protoCallFrame, JSFunction* function, int argumentCountIncludingThis, JSScope* scope, JSValue* args)
{
    VM& vm = *scope->vm();
    ASSERT(!vm.exception());
    
    if (vm.isCollectorBusy())
        return CallFrameClosure();

#if PLATFORM(WKC)
    if (IS_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT)) {
        throwStackOverflowError(callFrame);
        return CallFrameClosure();
    }
#endif

    // Compile the callee:
    JSObject* error = functionExecutable->prepareForExecution(callFrame, function, scope, CodeForCall);
    if (error) {
        callFrame->vm().throwException(callFrame, error);
        return CallFrameClosure();
    }
    CodeBlock* newCodeBlock = functionExecutable->codeBlockForCall();
    newCodeBlock->m_shouldAlwaysBeInlined = false;

    size_t argsCount = argumentCountIncludingThis;

    protoCallFrame->init(newCodeBlock, function, jsUndefined(), argsCount, args);
    // Return the successful closure:
    CallFrameClosure result = { callFrame, protoCallFrame, function, functionExecutable, &vm, scope, newCodeBlock->numParameters(), argumentCountIncludingThis };
    return result;
}

JSValue Interpreter::execute(CallFrameClosure& closure) 
{
    VM& vm = *closure.vm;
    SamplingScope samplingScope(this);
    
    ASSERT(!vm.isCollectorBusy());
    RELEASE_ASSERT(vm.currentThreadIsHoldingAPILock());
    if (vm.isCollectorBusy())
        return jsNull();

    StackStats::CheckPoint stackCheckPoint;

    if (UNLIKELY(vm.watchdog && vm.watchdog->didFire(closure.oldCallFrame)))
        return throwTerminatedExecutionException(closure.oldCallFrame);

    // Execute the code:
    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get());
        Watchdog::Scope watchdogScope(vm.watchdog.get());

        result = closure.functionExecutable->generatedJITCodeForCall()->execute(&vm, closure.protoCallFrame);
    }

    return checkedReturn(result);
}

JSValue Interpreter::execute(EvalExecutable* eval, CallFrame* callFrame, JSValue thisValue, JSScope* scope)
{
    VM& vm = *scope->vm();
    SamplingScope samplingScope(this);
    
    ASSERT(scope->vm() == &callFrame->vm());
    ASSERT(!vm.exception());
    ASSERT(!vm.isCollectorBusy());
    RELEASE_ASSERT(vm.currentThreadIsHoldingAPILock());
    if (vm.isCollectorBusy())
        return jsNull();

    VMEntryScope entryScope(vm, scope->globalObject());
    if (!vm.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));        

    unsigned numVariables = eval->numVariables();
    int numFunctions = eval->numberOfFunctionDecls();

    JSScope* variableObject;
    if ((numVariables || numFunctions) && eval->isStrictMode()) {
        scope = StrictEvalActivation::create(callFrame, scope);
        variableObject = scope;
    } else {
        for (JSScope* node = scope; ; node = node->next()) {
            RELEASE_ASSERT(node);
            if (node->isVariableObject()) {
                ASSERT(!node->isNameScopeObject());
                variableObject = node;
                break;
            }
        }
    }

    JSObject* compileError = eval->prepareForExecution(callFrame, nullptr, scope, CodeForCall);
    if (UNLIKELY(!!compileError))
        return checkedReturn(callFrame->vm().throwException(callFrame, compileError));
    EvalCodeBlock* codeBlock = eval->codeBlock();

    if (numVariables || numFunctions) {
        BatchedTransitionOptimizer optimizer(vm, variableObject);
        if (variableObject->next())
            variableObject->globalObject()->varInjectionWatchpoint()->fireAll("Executed eval, fired VarInjection watchpoint");

        for (unsigned i = 0; i < numVariables; ++i) {
            const Identifier& ident = codeBlock->variable(i);
            if (!variableObject->hasProperty(callFrame, ident)) {
                PutPropertySlot slot(variableObject);
                variableObject->methodTable()->put(variableObject, callFrame, ident, jsUndefined(), slot);
            }
        }

        for (int i = 0; i < numFunctions; ++i) {
            FunctionExecutable* function = codeBlock->functionDecl(i);
            PutPropertySlot slot(variableObject);
            variableObject->methodTable()->put(variableObject, callFrame, function->name(), JSFunction::create(vm, function, scope), slot);
        }
    }

    if (UNLIKELY(vm.watchdog && vm.watchdog->didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    ASSERT(codeBlock->numParameters() == 1); // 1 parameter for 'this'.

    ProtoCallFrame protoCallFrame;
    protoCallFrame.init(codeBlock, JSCallee::create(vm, scope->globalObject(), scope), thisValue, 1);

    // Execute the code:
    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get());
        Watchdog::Scope watchdogScope(vm.watchdog.get());

        result = eval->generatedJITCode()->execute(&vm, &protoCallFrame);
    }

    return checkedReturn(result);
}

NEVER_INLINE void Interpreter::debug(CallFrame* callFrame, DebugHookID debugHookID)
{
    Debugger* debugger = callFrame->vmEntryGlobalObject()->debugger();
    if (!debugger)
        return;

    ASSERT(callFrame->codeBlock()->hasDebuggerRequests());
    ASSERT(!callFrame->hadException());

    switch (debugHookID) {
        case DidEnterCallFrame:
            debugger->callEvent(callFrame);
            break;
        case WillLeaveCallFrame:
            debugger->returnEvent(callFrame);
            break;
        case WillExecuteStatement:
            debugger->atStatement(callFrame);
            break;
        case WillExecuteProgram:
            debugger->willExecuteProgram(callFrame);
            break;
        case DidExecuteProgram:
            debugger->didExecuteProgram(callFrame);
            break;
        case DidReachBreakpoint:
            debugger->didReachBreakpoint(callFrame);
            break;
    }
    ASSERT(!callFrame->hadException());
}    

void Interpreter::enableSampler()
{
#if ENABLE(OPCODE_SAMPLING)
    if (!m_sampler) {
        m_sampler = std::make_unique<SamplingTool>(this);
        m_sampler->setup();
    }
#endif
}
void Interpreter::dumpSampleData(ExecState* exec)
{
#if ENABLE(OPCODE_SAMPLING)
    if (m_sampler)
        m_sampler->dump(exec);
#else
    UNUSED_PARAM(exec);
#endif
}
void Interpreter::startSampling()
{
#if ENABLE(SAMPLING_THREAD)
    if (!m_sampleEntryDepth)
        SamplingThread::start();

    m_sampleEntryDepth++;
#endif
}
void Interpreter::stopSampling()
{
#if ENABLE(SAMPLING_THREAD)
    m_sampleEntryDepth--;
    if (!m_sampleEntryDepth)
        SamplingThread::stop();
#endif
}

} // namespace JSC
