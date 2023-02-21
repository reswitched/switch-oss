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
#include "FTLOSRExitCompiler.h"

#if ENABLE(FTL_JIT)

#include "DFGOSRExitCompilerCommon.h"
#include "DFGOSRExitPreparation.h"
#include "FTLExitArgumentForOperand.h"
#include "FTLJITCode.h"
#include "FTLOSRExit.h"
#include "FTLOperations.h"
#include "FTLState.h"
#include "FTLSaveRestore.h"
#include "LinkBuffer.h"
#include "MaxFrameExtentForSlowPathCall.h"
#include "OperandsInlines.h"
#include "JSCInlines.h"
#include "RegisterPreservationWrapperGenerator.h"
#include "RepatchBuffer.h"

namespace JSC { namespace FTL {

using namespace DFG;

static void compileRecovery(
    CCallHelpers& jit, const ExitValue& value, StackMaps::Record* record, StackMaps& stackmaps,
    char* registerScratch,
    const HashMap<ExitTimeObjectMaterialization*, EncodedJSValue*>& materializationToPointer)
{
    switch (value.kind()) {
    case ExitValueDead:
        jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsUndefined())), GPRInfo::regT0);
        break;
            
    case ExitValueConstant:
        jit.move(MacroAssembler::TrustedImm64(JSValue::encode(value.constant())), GPRInfo::regT0);
        break;
            
    case ExitValueArgument:
        record->locations[value.exitArgument().argument()].restoreInto(
            jit, stackmaps, registerScratch, GPRInfo::regT0);
        break;
            
    case ExitValueInJSStack:
    case ExitValueInJSStackAsInt32:
    case ExitValueInJSStackAsInt52:
    case ExitValueInJSStackAsDouble:
        jit.load64(AssemblyHelpers::addressFor(value.virtualRegister()), GPRInfo::regT0);
        break;
            
    case ExitValueRecovery:
        record->locations[value.rightRecoveryArgument()].restoreInto(
            jit, stackmaps, registerScratch, GPRInfo::regT1);
        record->locations[value.leftRecoveryArgument()].restoreInto(
            jit, stackmaps, registerScratch, GPRInfo::regT0);
        switch (value.recoveryOpcode()) {
        case AddRecovery:
            switch (value.recoveryFormat()) {
            case ValueFormatInt32:
                jit.add32(GPRInfo::regT1, GPRInfo::regT0);
                break;
            case ValueFormatInt52:
                jit.add64(GPRInfo::regT1, GPRInfo::regT0);
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            break;
        case SubRecovery:
            switch (value.recoveryFormat()) {
            case ValueFormatInt32:
                jit.sub32(GPRInfo::regT1, GPRInfo::regT0);
                break;
            case ValueFormatInt52:
                jit.sub64(GPRInfo::regT1, GPRInfo::regT0);
                break;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
        
    case ExitValueMaterializeNewObject:
        jit.loadPtr(materializationToPointer.get(value.objectMaterialization()), GPRInfo::regT0);
        break;
            
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
        
    reboxAccordingToFormat(
        value.valueFormat(), jit, GPRInfo::regT0, GPRInfo::regT1, GPRInfo::regT2);
}

static void compileStub(
    unsigned exitID, JITCode* jitCode, OSRExit& exit, VM* vm, CodeBlock* codeBlock)
{
    StackMaps::Record* record = nullptr;
    
    for (unsigned i = jitCode->stackmaps.records.size(); i--;) {
        record = &jitCode->stackmaps.records[i];
        if (record->patchpointID == exit.m_stackmapID)
            break;
    }
    
    RELEASE_ASSERT(record->patchpointID == exit.m_stackmapID);
    
    // This code requires framePointerRegister is the same as callFrameRegister
    static_assert(MacroAssembler::framePointerRegister == GPRInfo::callFrameRegister, "MacroAssembler::framePointerRegister and GPRInfo::callFrameRegister must be the same");

    CCallHelpers jit(vm, codeBlock);
    
    // We need scratch space to save all registers, to build up the JS stack, to deal with unwind
    // fixup, pointers to all of the objects we materialize, and the elements inside those objects
    // that we materialize.
    
    // Figure out how much space we need for those object allocations.
    unsigned numMaterializations = 0;
    size_t maxMaterializationNumArguments = 0;
    for (ExitTimeObjectMaterialization* materialization : exit.m_materializations) {
        numMaterializations++;
        
        maxMaterializationNumArguments = std::max(
            maxMaterializationNumArguments,
            materialization->properties().size());
    }
    
    ScratchBuffer* scratchBuffer = vm->scratchBufferForSize(
        sizeof(EncodedJSValue) * (
            exit.m_values.size() + numMaterializations + maxMaterializationNumArguments) +
        requiredScratchMemorySizeInBytes() +
        codeBlock->calleeSaveRegisters()->size() * sizeof(uint64_t));
    EncodedJSValue* scratch = scratchBuffer ? static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer()) : 0;
    EncodedJSValue* materializationPointers = scratch + exit.m_values.size();
    EncodedJSValue* materializationArguments = materializationPointers + numMaterializations;
    char* registerScratch = bitwise_cast<char*>(materializationArguments + maxMaterializationNumArguments);
    uint64_t* unwindScratch = bitwise_cast<uint64_t*>(registerScratch + requiredScratchMemorySizeInBytes());
    
    HashMap<ExitTimeObjectMaterialization*, EncodedJSValue*> materializationToPointer;
    unsigned materializationCount = 0;
    for (ExitTimeObjectMaterialization* materialization : exit.m_materializations) {
        materializationToPointer.add(
            materialization, materializationPointers + materializationCount++);
    }
    
    // Note that we come in here, the stack used to be as LLVM left it except that someone called pushToSave().
    // We don't care about the value they saved. But, we do appreciate the fact that they did it, because we use
    // that slot for saveAllRegisters().

    saveAllRegisters(jit, registerScratch);
    
    // Bring the stack back into a sane form and assert that it's sane.
    jit.popToRestore(GPRInfo::regT0);
    jit.checkStackPointerAlignment();
    
    if (vm->m_perBytecodeProfiler && codeBlock->jitCode()->dfgCommon()->compilation) {
        Profiler::Database& database = *vm->m_perBytecodeProfiler;
        Profiler::Compilation* compilation = codeBlock->jitCode()->dfgCommon()->compilation.get();
        
        Profiler::OSRExit* profilerExit = compilation->addOSRExit(
            exitID, Profiler::OriginStack(database, codeBlock, exit.m_codeOrigin),
            exit.m_kind, exit.m_kind == UncountableInvalidation);
        jit.add64(CCallHelpers::TrustedImm32(1), CCallHelpers::AbsoluteAddress(profilerExit->counterAddress()));
    }

    // The remaining code assumes that SP/FP are in the same state that they were in the FTL's
    // call frame.
    
    // Get the call frame and tag thingies.
    // Restore the exiting function's callFrame value into a regT4
    jit.move(MacroAssembler::TrustedImm64(TagTypeNumber), GPRInfo::tagTypeNumberRegister);
    jit.move(MacroAssembler::TrustedImm64(TagMask), GPRInfo::tagMaskRegister);
    
    // Do some value profiling.
    if (exit.m_profileValueFormat != InvalidValueFormat) {
        record->locations[0].restoreInto(jit, jitCode->stackmaps, registerScratch, GPRInfo::regT0);
        reboxAccordingToFormat(
            exit.m_profileValueFormat, jit, GPRInfo::regT0, GPRInfo::regT1, GPRInfo::regT2);
        
        if (exit.m_kind == BadCache || exit.m_kind == BadIndexingType) {
            CodeOrigin codeOrigin = exit.m_codeOriginForExitProfile;
            if (ArrayProfile* arrayProfile = jit.baselineCodeBlockFor(codeOrigin)->getArrayProfile(codeOrigin.bytecodeIndex)) {
                jit.load32(MacroAssembler::Address(GPRInfo::regT0, JSCell::structureIDOffset()), GPRInfo::regT1);
                jit.store32(GPRInfo::regT1, arrayProfile->addressOfLastSeenStructureID());
                jit.load8(MacroAssembler::Address(GPRInfo::regT0, JSCell::indexingTypeOffset()), GPRInfo::regT1);
                jit.move(MacroAssembler::TrustedImm32(1), GPRInfo::regT2);
                jit.lshift32(GPRInfo::regT1, GPRInfo::regT2);
                jit.or32(GPRInfo::regT2, MacroAssembler::AbsoluteAddress(arrayProfile->addressOfArrayModes()));
            }
        }
        
        if (!!exit.m_valueProfile)
            jit.store64(GPRInfo::regT0, exit.m_valueProfile.getSpecFailBucket(0));
    }
    
    // Materialize all objects. Don't materialize an object until all of the objects it needs
    // have been materialized. Curiously, this is the only place that we have an algorithm that prevents
    // OSR exit from handling cyclic object materializations. Of course, object allocation sinking
    // currently wouldn't recognize a cycle as being sinkable - but if it did then the only thing that
    // would ahve to change is this fixpoint. Instead we would allocate the objects first and populate
    // them with data later.
    HashSet<ExitTimeObjectMaterialization*> toMaterialize;
    for (ExitTimeObjectMaterialization* materialization : exit.m_materializations)
        toMaterialize.add(materialization);
    
    while (!toMaterialize.isEmpty()) {
        unsigned previousToMaterializeSize = toMaterialize.size();
        
        Vector<ExitTimeObjectMaterialization*> worklist;
        worklist.appendRange(toMaterialize.begin(), toMaterialize.end());
        for (ExitTimeObjectMaterialization* materialization : worklist) {
            // Check if we can do anything about this right now.
            bool allGood = true;
            for (ExitPropertyValue value : materialization->properties()) {
                if (!value.value().isObjectMaterialization())
                    continue;
                if (toMaterialize.contains(value.value().objectMaterialization())) {
                    // Gotta skip this one, since one of its fields points to a materialization
                    // that hasn't been materialized.
                    allGood = false;
                    break;
                }
            }
            if (!allGood)
                continue;
            
            // All systems go for materializing the object. First we recover the values of all of
            // its fields and then we call a function to actually allocate the beast.
            for (unsigned propertyIndex = materialization->properties().size(); propertyIndex--;) {
                const ExitValue& value = materialization->properties()[propertyIndex].value();
                compileRecovery(
                    jit, value, record, jitCode->stackmaps, registerScratch,
                    materializationToPointer);
                jit.storePtr(GPRInfo::regT0, materializationArguments + propertyIndex);
            }
            
            // This call assumes that we don't pass arguments on the stack.
            jit.setupArgumentsWithExecState(
                CCallHelpers::TrustedImmPtr(materialization),
                CCallHelpers::TrustedImmPtr(materializationArguments));
            jit.move(CCallHelpers::TrustedImmPtr(bitwise_cast<void*>(operationMaterializeObjectInOSR)), GPRInfo::nonArgGPR0);
            jit.call(GPRInfo::nonArgGPR0);
            jit.storePtr(GPRInfo::returnValueGPR, materializationToPointer.get(materialization));
            
            // Let everyone know that we're done.
            toMaterialize.remove(materialization);
        }
        
        // We expect progress! This ensures that we crash rather than looping infinitely if there
        // is something broken about this fixpoint. Or, this could happen if we ever violate the
        // "materializations form a DAG" rule.
        RELEASE_ASSERT(toMaterialize.size() < previousToMaterializeSize);
    }

    // Save all state from wherever the exit data tells us it was, into the appropriate place in
    // the scratch buffer. This also does the reboxing.
    
    for (unsigned index = exit.m_values.size(); index--;) {
        compileRecovery(
            jit, exit.m_values[index], record, jitCode->stackmaps, registerScratch,
            materializationToPointer);
        jit.store64(GPRInfo::regT0, scratch + index);
    }
    
    // Henceforth we make it look like the exiting function was called through a register
    // preservation wrapper. This implies that FP must be nudged down by a certain amount. Then
    // we restore the various things according to either exit.m_values or by copying from the
    // old frame, and finally we save the various callee-save registers into where the
    // restoration thunk would restore them from.
    
    ptrdiff_t offset = registerPreservationOffset();
    RegisterSet toSave = registersToPreserve();
    
    // Before we start messing with the frame, we need to set aside any registers that the
    // FTL code was preserving.
    for (unsigned i = codeBlock->calleeSaveRegisters()->size(); i--;) {
        RegisterAtOffset entry = codeBlock->calleeSaveRegisters()->at(i);
        jit.load64(
            MacroAssembler::Address(MacroAssembler::framePointerRegister, entry.offset()),
            GPRInfo::regT0);
        jit.store64(GPRInfo::regT0, unwindScratch + i);
    }
    
    jit.load32(CCallHelpers::payloadFor(JSStack::ArgumentCount), GPRInfo::regT2);
    
    // Let's say that the FTL function had failed its arity check. In that case, the stack will
    // contain some extra stuff.
    //
    // First we compute the padded stack space:
    //
    //     paddedStackSpace = roundUp(codeBlock->numParameters - regT2 + 1)
    //
    // The stack will have regT2 + CallFrameHeaderSize stuff, but above it there will be
    // paddedStackSpace gunk used by the arity check fail restoration thunk. When that happens
    // we want to make the stack look like this, from higher addresses down:
    //
    //     - register preservation return PC
    //     - preserved registers
    //     - arity check fail return PC
    //     - argument padding
    //     - actual arguments
    //     - call frame header
    //
    // So that the actual call frame header appears to return to the arity check fail return
    // PC, and that then returns to the register preservation thunk. The arity check thunk that
    // we return to will have the padding size encoded into it. It will then know to return
    // into the register preservation thunk, which uses the argument count to figure out where
    // registers are preserved.

    // This code assumes that we're dealing with FunctionCode.
    RELEASE_ASSERT(codeBlock->codeType() == FunctionCode);
    
    jit.add32(
        MacroAssembler::TrustedImm32(-codeBlock->numParameters()), GPRInfo::regT2,
        GPRInfo::regT3);
    MacroAssembler::Jump arityIntact = jit.branch32(
        MacroAssembler::GreaterThanOrEqual, GPRInfo::regT3, MacroAssembler::TrustedImm32(0));
    jit.neg32(GPRInfo::regT3);
    jit.add32(MacroAssembler::TrustedImm32(1 + stackAlignmentRegisters() - 1), GPRInfo::regT3);
    jit.and32(MacroAssembler::TrustedImm32(-stackAlignmentRegisters()), GPRInfo::regT3);
    jit.add32(GPRInfo::regT3, GPRInfo::regT2);
    arityIntact.link(&jit);

    CodeBlock* baselineCodeBlock = jit.baselineCodeBlockFor(exit.m_codeOrigin);

    // First set up SP so that our data doesn't get clobbered by signals.
    unsigned conservativeStackDelta =
        registerPreservationOffset() +
        (exit.m_values.numberOfLocals() + baselineCodeBlock->calleeSaveSpaceAsVirtualRegisters()) * sizeof(Register) +
        maxFrameExtentForSlowPathCall;
    conservativeStackDelta = WTF::roundUpToMultipleOf(
        stackAlignmentBytes(), conservativeStackDelta);
    jit.addPtr(
        MacroAssembler::TrustedImm32(-conservativeStackDelta),
        MacroAssembler::framePointerRegister, MacroAssembler::stackPointerRegister);
    jit.checkStackPointerAlignment();
    
    jit.subPtr(
        MacroAssembler::TrustedImm32(registerPreservationOffset()),
        MacroAssembler::framePointerRegister);
    
    // Copy the old frame data into its new location.
    jit.add32(MacroAssembler::TrustedImm32(JSStack::CallFrameHeaderSize), GPRInfo::regT2);
    jit.move(MacroAssembler::framePointerRegister, GPRInfo::regT1);
    MacroAssembler::Label loop = jit.label();
    jit.sub32(MacroAssembler::TrustedImm32(1), GPRInfo::regT2);
    jit.load64(MacroAssembler::Address(GPRInfo::regT1, offset), GPRInfo::regT0);
    jit.store64(GPRInfo::regT0, GPRInfo::regT1);
    jit.addPtr(MacroAssembler::TrustedImm32(sizeof(Register)), GPRInfo::regT1);
    jit.branchTest32(MacroAssembler::NonZero, GPRInfo::regT2).linkTo(loop, &jit);

    RegisterAtOffsetList* baselineCalleeSaves = baselineCodeBlock->calleeSaveRegisters();

    for (Reg reg = Reg::first(); reg <= Reg::last(); reg = reg.next()) {
        if (!toSave.get(reg) || !reg.isGPR())
            continue;
        unsigned unwindIndex = codeBlock->calleeSaveRegisters()->indexOf(reg);
        RegisterAtOffset* baselineRegisterOffset = baselineCalleeSaves->find(reg);

        if (reg.isGPR()) {
            GPRReg regToLoad = baselineRegisterOffset ? GPRInfo::regT0 : reg.gpr();

            if (unwindIndex == UINT_MAX) {
                // The FTL compilation didn't preserve this register. This means that it also
                // didn't use the register. So its value at the beginning of OSR exit should be
                // preserved by the thunk. Luckily, we saved all registers into the register
                // scratch buffer, so we can restore them from there.
                jit.load64(registerScratch + offsetOfReg(reg), regToLoad);
            } else {
                // The FTL compilation preserved the register. Its new value is therefore
                // irrelevant, but we can get the value that was preserved by using the unwind
                // data. We've already copied all unwind-able preserved registers into the unwind
                // scratch buffer, so we can get it from there.
                jit.load64(unwindScratch + unwindIndex, regToLoad);
            }

            if (baselineRegisterOffset)
                jit.store64(regToLoad, MacroAssembler::Address(MacroAssembler::framePointerRegister, baselineRegisterOffset->offset()));
        } else {
            FPRReg fpRegToLoad = baselineRegisterOffset ? FPRInfo::fpRegT0 : reg.fpr();

            if (unwindIndex == UINT_MAX)
                jit.loadDouble(MacroAssembler::TrustedImmPtr(registerScratch + offsetOfReg(reg)), fpRegToLoad);
            else
                jit.loadDouble(MacroAssembler::TrustedImmPtr(unwindScratch + unwindIndex), fpRegToLoad);

            if (baselineRegisterOffset)
                jit.storeDouble(fpRegToLoad, MacroAssembler::Address(MacroAssembler::framePointerRegister, baselineRegisterOffset->offset()));
        }
    }

    size_t baselineVirtualRegistersForCalleeSaves = baselineCodeBlock->calleeSaveSpaceAsVirtualRegisters();

    // Now get state out of the scratch buffer and place it back into the stack. The values are
    // already reboxed so we just move them.
    for (unsigned index = exit.m_values.size(); index--;) {
        VirtualRegister reg = exit.m_values.virtualRegisterForIndex(index);

        if (reg.isLocal() && reg.toLocal() < static_cast<int>(baselineVirtualRegistersForCalleeSaves))
            continue;

        jit.load64(scratch + index, GPRInfo::regT0);
        jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(reg));
    }
    
    handleExitCounts(jit, exit);
    reifyInlinedCallFrames(jit, exit);
    adjustAndJumpToTarget(jit, exit);
    
    LinkBuffer patchBuffer(*vm, jit, codeBlock);
    exit.m_code = FINALIZE_CODE_IF(
        shouldDumpDisassembly() || Options::verboseOSR() || Options::verboseFTLOSRExit(),
        patchBuffer,
        ("FTL OSR exit #%u (%s, %s) from %s, with operands = %s, and record = %s",
            exitID, toCString(exit.m_codeOrigin).data(),
            exitKindToString(exit.m_kind), toCString(*codeBlock).data(),
            toCString(ignoringContext<DumpContext>(exit.m_values)).data(),
            toCString(*record).data()));
}

extern "C" void* compileFTLOSRExit(ExecState* exec, unsigned exitID)
{
    SamplingRegion samplingRegion("FTL OSR Exit Compilation");

    if (shouldDumpDisassembly() || Options::verboseOSR() || Options::verboseFTLOSRExit())
        dataLog("Compiling OSR exit with exitID = ", exitID, "\n");
    
    CodeBlock* codeBlock = exec->codeBlock();
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->jitType() == JITCode::FTLJIT);
    
    VM* vm = &exec->vm();
    
    // It's sort of preferable that we don't GC while in here. Anyways, doing so wouldn't
    // really be profitable.
    DeferGCForAWhile deferGC(vm->heap);

    JITCode* jitCode = codeBlock->jitCode()->ftl();
    OSRExit& exit = jitCode->osrExit[exitID];
    
    if (shouldDumpDisassembly() || Options::verboseOSR() || Options::verboseFTLOSRExit()) {
        dataLog("    Owning block: ", pointerDump(codeBlock), "\n");
        dataLog("    Origin: ", exit.m_codeOrigin, "\n");
        if (exit.m_codeOriginForExitProfile != exit.m_codeOrigin)
            dataLog("    Origin for exit profile: ", exit.m_codeOriginForExitProfile, "\n");
        dataLog("    Exit values: ", exit.m_values, "\n");
        if (!exit.m_materializations.isEmpty()) {
            dataLog("    Materializations:\n");
            for (ExitTimeObjectMaterialization* materialization : exit.m_materializations)
                dataLog("        ", pointerDump(materialization), "\n");
        }
    }

    prepareCodeOriginForOSRExit(exec, exit.m_codeOrigin);
    
    compileStub(exitID, jitCode, exit, vm, codeBlock);
    
    RepatchBuffer repatchBuffer(codeBlock);
    repatchBuffer.relink(
        exit.codeLocationForRepatch(codeBlock), CodeLocationLabel(exit.m_code.code()));
    
    return exit.m_code.code().executableAddress();
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

