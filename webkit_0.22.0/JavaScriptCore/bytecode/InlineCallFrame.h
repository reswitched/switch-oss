/*
 * Copyright (C) 2011-2015 Apple Inc. All rights reserved.
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

#ifndef InlineCallFrame_h
#define InlineCallFrame_h

#include "CodeBlock.h"
#include "CodeBlockHash.h"
#include "CodeOrigin.h"
#include "Executable.h"
#include "ValueRecovery.h"
#include "WriteBarrier.h"
#include <wtf/BitVector.h>
#include <wtf/HashMap.h>
#include <wtf/PrintStream.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace JSC {

struct InlineCallFrame;
class ExecState;
class ScriptExecutable;
class JSFunction;

struct InlineCallFrame {
    enum Kind {
        Call,
        Construct,
        TailCall,
        CallVarargs,
        ConstructVarargs,
        TailCallVarargs,
        
        // For these, the stackOffset incorporates the argument count plus the true return PC
        // slot.
        GetterCall,
        SetterCall
    };

    static CallMode callModeFor(Kind kind)
    {
        switch (kind) {
        case Call:
        case CallVarargs:
        case GetterCall:
        case SetterCall:
            return CallMode::Regular;
        case TailCall:
        case TailCallVarargs:
            return CallMode::Tail;
        case Construct:
        case ConstructVarargs:
            return CallMode::Construct;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    static Kind kindFor(CallMode callMode)
    {
        switch (callMode) {
        case CallMode::Regular:
            return Call;
        case CallMode::Construct:
            return Construct;
        case CallMode::Tail:
            return TailCall;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    static Kind varargsKindFor(CallMode callMode)
    {
        switch (callMode) {
        case CallMode::Regular:
            return CallVarargs;
        case CallMode::Construct:
            return ConstructVarargs;
        case CallMode::Tail:
            return TailCallVarargs;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    static CodeSpecializationKind specializationKindFor(Kind kind)
    {
        switch (kind) {
        case Call:
        case CallVarargs:
        case TailCall:
        case TailCallVarargs:
        case GetterCall:
        case SetterCall:
            return CodeForCall;
        case Construct:
        case ConstructVarargs:
            return CodeForConstruct;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    static bool isVarargs(Kind kind)
    {
        switch (kind) {
        case CallVarargs:
        case TailCallVarargs:
        case ConstructVarargs:
            return true;
        default:
            return false;
        }
    }

    static bool isTail(Kind kind)
    {
        switch (kind) {
        case TailCall:
        case TailCallVarargs:
            return true;
        default:
            return false;
        }
    }
    bool isTail() const
    {
        return isTail(static_cast<Kind>(kind));
    }

    static CodeOrigin* computeCallerSkippingDeadFrames(InlineCallFrame* inlineCallFrame)
    {
        CodeOrigin* codeOrigin;
        bool tailCallee;
        do {
            tailCallee = inlineCallFrame->isTail();
            codeOrigin = &inlineCallFrame->directCaller;
            inlineCallFrame = codeOrigin->inlineCallFrame;
        } while (inlineCallFrame && tailCallee);
        if (tailCallee)
            return nullptr;
        return codeOrigin;
    }

    CodeOrigin* getCallerSkippingDeadFrames()
    {
        return computeCallerSkippingDeadFrames(this);
    }

    InlineCallFrame* getCallerInlineFrameSkippingDeadFrames()
    {
        CodeOrigin* caller = getCallerSkippingDeadFrames();
        return caller ? caller->inlineCallFrame : nullptr;
    }
    
    Vector<ValueRecovery> arguments; // Includes 'this'.
    WriteBarrier<ScriptExecutable> executable;
    ValueRecovery calleeRecovery;
    CodeOrigin directCaller;

    signed stackOffset : 28;
    unsigned kind : 3; // real type is Kind
    bool isClosureCall : 1; // If false then we know that callee/scope are constants and the DFG won't treat them as variables, i.e. they have to be recovered manually.
    VirtualRegister argumentCountRegister; // Only set when we inline a varargs call.
    
    // There is really no good notion of a "default" set of values for
    // InlineCallFrame's fields. This constructor is here just to reduce confusion if
    // we forgot to initialize explicitly.
    InlineCallFrame()
        : stackOffset(0)
        , kind(Call)
        , isClosureCall(false)
    {
    }
    
    bool isVarargs() const
    {
        return isVarargs(static_cast<Kind>(kind));
    }

    CodeSpecializationKind specializationKind() const { return specializationKindFor(static_cast<Kind>(kind)); }

    JSFunction* calleeConstant() const;
    void visitAggregate(SlotVisitor&);
    
    // Get the callee given a machine call frame to which this InlineCallFrame belongs.
    JSFunction* calleeForCallFrame(ExecState*) const;
    
    CString inferredName() const;
    CodeBlockHash hash() const;
    CString hashAsStringIfPossible() const;
    
    CodeBlock* baselineCodeBlock() const;
    
    void setStackOffset(signed offset)
    {
        stackOffset = offset;
        RELEASE_ASSERT(static_cast<signed>(stackOffset) == offset);
    }

    ptrdiff_t callerFrameOffset() const { return stackOffset * sizeof(Register) + CallFrame::callerFrameOffset(); }
    ptrdiff_t returnPCOffset() const { return stackOffset * sizeof(Register) + CallFrame::returnPCOffset(); }

    void dumpBriefFunctionInformation(PrintStream&) const;
    void dump(PrintStream&) const;
    void dumpInContext(PrintStream&, DumpContext*) const;

    MAKE_PRINT_METHOD(InlineCallFrame, dumpBriefFunctionInformation, briefFunctionInformation);

};

inline CodeBlock* baselineCodeBlockForInlineCallFrame(InlineCallFrame* inlineCallFrame)
{
    RELEASE_ASSERT(inlineCallFrame);
    ScriptExecutable* executable = inlineCallFrame->executable.get();
    RELEASE_ASSERT(executable->structure()->classInfo() == FunctionExecutable::info());
    return static_cast<FunctionExecutable*>(executable)->baselineCodeBlockFor(inlineCallFrame->specializationKind());
}

inline CodeBlock* baselineCodeBlockForOriginAndBaselineCodeBlock(const CodeOrigin& codeOrigin, CodeBlock* baselineCodeBlock)
{
    if (codeOrigin.inlineCallFrame)
        return baselineCodeBlockForInlineCallFrame(codeOrigin.inlineCallFrame);
    return baselineCodeBlock;
}

} // namespace JSC

namespace WTF {

void printInternal(PrintStream&, JSC::InlineCallFrame::Kind);

} // namespace WTF

#endif // InlineCallFrame_h
