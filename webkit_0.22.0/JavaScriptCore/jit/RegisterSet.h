/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

#ifndef RegisterSet_h
#define RegisterSet_h

#if ENABLE(JIT)

#include "FPRInfo.h"
#include "GPRInfo.h"
#include "MacroAssembler.h"
#include "Reg.h"
#include "TempRegisterSet.h"
#include <wtf/BitVector.h>

namespace JSC {

class RegisterSet {
public:
    template<typename... Regs>
    explicit RegisterSet(Regs... regs)
    {
        setMany(regs...);
    }
    
    static RegisterSet stackRegisters();
    static RegisterSet reservedHardwareRegisters();
    static RegisterSet runtimeRegisters();
    static RegisterSet specialRegisters(); // The union of stack, reserved hardware, and runtime registers.
    static RegisterSet calleeSaveRegisters();
    static RegisterSet vmCalleeSaveRegisters(); // Callee save registers that might be saved and used by any tier.
    static RegisterSet llintBaselineCalleeSaveRegisters(); // Registers saved and used by the LLInt.
    static RegisterSet dfgCalleeSaveRegisters(); // Registers saved and used by the DFG JIT.
    static RegisterSet ftlCalleeSaveRegisters(); // Registers that might be saved and used by the FTL JIT.
    static RegisterSet stubUnavailableRegisters(); // The union of callee saves and special registers.
    static RegisterSet allGPRs();
    static RegisterSet allFPRs();
    static RegisterSet allRegisters();
    
    void set(Reg reg, bool value = true)
    {
        ASSERT(!!reg);
        m_vector.set(reg.index(), value);
    }
    
    void set(JSValueRegs regs)
    {
        if (regs.tagGPR() != InvalidGPRReg)
            set(regs.tagGPR());
        set(regs.payloadGPR());
    }
    
    void clear(Reg reg)
    {
        ASSERT(!!reg);
        set(reg, false);
    }
    
    bool get(Reg reg) const
    {
        ASSERT(!!reg);
        return m_vector.get(reg.index());
    }
    
    void merge(const RegisterSet& other) { m_vector.merge(other.m_vector); }
    void filter(const RegisterSet& other) { m_vector.filter(other.m_vector); }
    void exclude(const RegisterSet& other) { m_vector.exclude(other.m_vector); }
    
    size_t numberOfSetGPRs() const;
    size_t numberOfSetFPRs() const;
    size_t numberOfSetRegisters() const { return m_vector.bitCount(); }
    
    void dump(PrintStream&) const;
    
    enum EmptyValueTag { EmptyValue };
    enum DeletedValueTag { DeletedValue };
    
    RegisterSet(EmptyValueTag)
        : m_vector(BitVector::EmptyValue)
    {
    }
    
    RegisterSet(DeletedValueTag)
        : m_vector(BitVector::DeletedValue)
    {
    }
    
    bool isEmptyValue() const { return m_vector.isEmptyValue(); }
    bool isDeletedValue() const { return m_vector.isDeletedValue(); }
    
    bool operator==(const RegisterSet& other) const { return m_vector == other.m_vector; }
    unsigned hash() const { return m_vector.hash(); }
    
private:
    void setAny(Reg reg) { set(reg); }
    void setAny(const RegisterSet& set) { merge(set); }
    void setMany() { }
    template<typename RegType, typename... Regs>
    void setMany(RegType reg, Regs... regs)
    {
        setAny(reg);
        setMany(regs...);
    }

    BitVector m_vector;
};

struct RegisterSetHash {
    static unsigned hash(const RegisterSet& set) { return set.hash(); }
    static bool equal(const RegisterSet& a, const RegisterSet& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = false;
};

} // namespace JSC

namespace WTF {

template<typename T> struct DefaultHash;
template<> struct DefaultHash<JSC::RegisterSet> {
    typedef JSC::RegisterSetHash Hash;
};

template<typename T> struct HashTraits;
template<> struct HashTraits<JSC::RegisterSet> : public CustomHashTraits<JSC::RegisterSet> { };

} // namespace WTF

#endif // ENABLE(JIT)

#endif // RegisterSet_h

