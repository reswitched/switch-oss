/*
 * Copyright (C) 2008, 2012, 2014, 2015 Apple Inc. All rights reserved.
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

#ifndef AbstractMacroAssembler_h
#define AbstractMacroAssembler_h

#include "AbortReason.h"
#include "AssemblerBuffer.h"
#include "CodeLocation.h"
#include "MacroAssemblerCodeRef.h"
#include "Options.h"
#include "WeakRandom.h"
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/Noncopyable.h>

#if ENABLE(ASSEMBLER)

namespace JSC {

inline bool isARMv7IDIVSupported()
{
#if HAVE(ARM_IDIV_INSTRUCTIONS)
    return true;
#else
    return false;
#endif
}

inline bool isARM64()
{
#if CPU(ARM64)
    return true;
#else
    return false;
#endif
}

inline bool isX86()
{
#if CPU(X86_64) || CPU(X86)
    return true;
#else
    return false;
#endif
}

inline bool optimizeForARMv7IDIVSupported()
{
    return isARMv7IDIVSupported() && Options::useArchitectureSpecificOptimizations();
}

inline bool optimizeForARM64()
{
    return isARM64() && Options::useArchitectureSpecificOptimizations();
}

inline bool optimizeForX86()
{
    return isX86() && Options::useArchitectureSpecificOptimizations();
}

class LinkBuffer;
class RepatchBuffer;
class Watchpoint;
namespace DFG {
struct OSRExit;
}

template <class AssemblerType, class MacroAssemblerType>
class AbstractMacroAssembler {
public:
    friend class JITWriteBarrierBase;
    typedef AbstractMacroAssembler<AssemblerType, MacroAssemblerType> AbstractMacroAssemblerType;
    typedef AssemblerType AssemblerType_T;

    typedef MacroAssemblerCodePtr CodePtr;
    typedef MacroAssemblerCodeRef CodeRef;

    class Jump;

    typedef typename AssemblerType::RegisterID RegisterID;
    typedef typename AssemblerType::FPRegisterID FPRegisterID;
    
    static RegisterID firstRegister() { return AssemblerType::firstRegister(); }
    static RegisterID lastRegister() { return AssemblerType::lastRegister(); }

    static FPRegisterID firstFPRegister() { return AssemblerType::firstFPRegister(); }
    static FPRegisterID lastFPRegister() { return AssemblerType::lastFPRegister(); }

    // Section 1: MacroAssembler operand types
    //
    // The following types are used as operands to MacroAssembler operations,
    // describing immediate  and memory operands to the instructions to be planted.

    enum Scale {
        TimesOne,
        TimesTwo,
        TimesFour,
        TimesEight,
    };
    
    static Scale timesPtr()
    {
        if (sizeof(void*) == 4)
            return TimesFour;
        return TimesEight;
    }

    // Address:
    //
    // Describes a simple base-offset address.
    struct Address {
        explicit Address(RegisterID base, int32_t offset = 0)
            : base(base)
            , offset(offset)
        {
        }
        
        Address withOffset(int32_t additionalOffset)
        {
            return Address(base, offset + additionalOffset);
        }
        
        RegisterID base;
        int32_t offset;
    };

    struct ExtendedAddress {
        explicit ExtendedAddress(RegisterID base, intptr_t offset = 0)
            : base(base)
            , offset(offset)
        {
        }
        
        RegisterID base;
        intptr_t offset;
    };

    // ImplicitAddress:
    //
    // This class is used for explicit 'load' and 'store' operations
    // (as opposed to situations in which a memory operand is provided
    // to a generic operation, such as an integer arithmetic instruction).
    //
    // In the case of a load (or store) operation we want to permit
    // addresses to be implicitly constructed, e.g. the two calls:
    //
    //     load32(Address(addrReg), destReg);
    //     load32(addrReg, destReg);
    //
    // Are equivalent, and the explicit wrapping of the Address in the former
    // is unnecessary.
    struct ImplicitAddress {
        ImplicitAddress(RegisterID base)
            : base(base)
            , offset(0)
        {
        }

        ImplicitAddress(Address address)
            : base(address.base)
            , offset(address.offset)
        {
        }

        RegisterID base;
        int32_t offset;
    };

    // BaseIndex:
    //
    // Describes a complex addressing mode.
    struct BaseIndex {
        BaseIndex(RegisterID base, RegisterID index, Scale scale, int32_t offset = 0)
            : base(base)
            , index(index)
            , scale(scale)
            , offset(offset)
        {
        }

        RegisterID base;
        RegisterID index;
        Scale scale;
        int32_t offset;
        
        BaseIndex withOffset(int32_t additionalOffset)
        {
            return BaseIndex(base, index, scale, offset + additionalOffset);
        }
    };

    // AbsoluteAddress:
    //
    // Describes an memory operand given by a pointer.  For regular load & store
    // operations an unwrapped void* will be used, rather than using this.
    struct AbsoluteAddress {
        explicit AbsoluteAddress(const void* ptr)
            : m_ptr(ptr)
        {
        }

        const void* m_ptr;
    };

    // TrustedImmPtr:
    //
    // A pointer sized immediate operand to an instruction - this is wrapped
    // in a class requiring explicit construction in order to differentiate
    // from pointers used as absolute addresses to memory operations
    struct TrustedImmPtr {
        TrustedImmPtr() { }
        
        explicit TrustedImmPtr(const void* value)
            : m_value(value)
        {
        }
        
        // This is only here so that TrustedImmPtr(0) does not confuse the C++
        // overload handling rules.
        explicit TrustedImmPtr(int value)
            : m_value(0)
        {
            ASSERT_UNUSED(value, !value);
        }

        explicit TrustedImmPtr(size_t value)
            : m_value(reinterpret_cast<void*>(value))
        {
        }

        intptr_t asIntptr()
        {
            return reinterpret_cast<intptr_t>(m_value);
        }

        const void* m_value;
    };

    struct ImmPtr : private TrustedImmPtr
    {
        explicit ImmPtr(const void* value)
            : TrustedImmPtr(value)
        {
        }

        TrustedImmPtr asTrustedImmPtr() { return *this; }
    };

    // TrustedImm32:
    //
    // A 32bit immediate operand to an instruction - this is wrapped in a
    // class requiring explicit construction in order to prevent RegisterIDs
    // (which are implemented as an enum) from accidentally being passed as
    // immediate values.
    struct TrustedImm32 {
        TrustedImm32() { }
        
        explicit TrustedImm32(int32_t value)
            : m_value(value)
        {
        }

#if !CPU(X86_64)
        explicit TrustedImm32(TrustedImmPtr ptr)
            : m_value(ptr.asIntptr())
        {
        }
#endif

        int32_t m_value;
    };


    struct Imm32 : private TrustedImm32 {
        explicit Imm32(int32_t value)
            : TrustedImm32(value)
        {
        }
#if !CPU(X86_64)
        explicit Imm32(TrustedImmPtr ptr)
            : TrustedImm32(ptr)
        {
        }
#endif
        const TrustedImm32& asTrustedImm32() const { return *this; }

    };
    
    // TrustedImm64:
    //
    // A 64bit immediate operand to an instruction - this is wrapped in a
    // class requiring explicit construction in order to prevent RegisterIDs
    // (which are implemented as an enum) from accidentally being passed as
    // immediate values.
    struct TrustedImm64 {
        TrustedImm64() { }
        
        explicit TrustedImm64(int64_t value)
            : m_value(value)
        {
        }

#if CPU(X86_64) || CPU(ARM64)
        explicit TrustedImm64(TrustedImmPtr ptr)
            : m_value(ptr.asIntptr())
        {
        }
#endif

        int64_t m_value;
    };

    struct Imm64 : private TrustedImm64
    {
        explicit Imm64(int64_t value)
            : TrustedImm64(value)
        {
        }
#if CPU(X86_64) || CPU(ARM64)
        explicit Imm64(TrustedImmPtr ptr)
            : TrustedImm64(ptr)
        {
        }
#endif
        const TrustedImm64& asTrustedImm64() const { return *this; }
    };
    
    // Section 2: MacroAssembler code buffer handles
    //
    // The following types are used to reference items in the code buffer
    // during JIT code generation.  For example, the type Jump is used to
    // track the location of a jump instruction so that it may later be
    // linked to a label marking its destination.


    // Label:
    //
    // A Label records a point in the generated instruction stream, typically such that
    // it may be used as a destination for a jump.
    class Label {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;
        friend struct DFG::OSRExit;
        friend class Jump;
        friend class MacroAssemblerCodeRef;
        friend class LinkBuffer;
        friend class Watchpoint;

    public:
        Label()
        {
        }

        Label(AbstractMacroAssemblerType* masm)
            : m_label(masm->m_assembler.label())
        {
            masm->invalidateAllTempRegisters();
        }

        bool isSet() const { return m_label.isSet(); }
    private:
        AssemblerLabel m_label;
    };
    
    // ConvertibleLoadLabel:
    //
    // A ConvertibleLoadLabel records a loadPtr instruction that can be patched to an addPtr
    // so that:
    //
    // loadPtr(Address(a, i), b)
    //
    // becomes:
    //
    // addPtr(TrustedImmPtr(i), a, b)
    class ConvertibleLoadLabel {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
        
    public:
        ConvertibleLoadLabel()
        {
        }
        
        ConvertibleLoadLabel(AbstractMacroAssemblerType* masm)
            : m_label(masm->m_assembler.labelIgnoringWatchpoints())
        {
        }
        
        bool isSet() const { return m_label.isSet(); }
    private:
        AssemblerLabel m_label;
    };

    // DataLabelPtr:
    //
    // A DataLabelPtr is used to refer to a location in the code containing a pointer to be
    // patched after the code has been generated.
    class DataLabelPtr {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabelPtr()
        {
        }

        DataLabelPtr(AbstractMacroAssemblerType* masm)
            : m_label(masm->m_assembler.label())
        {
        }

        bool isSet() const { return m_label.isSet(); }
        
    private:
        AssemblerLabel m_label;
    };

    // DataLabel32:
    //
    // A DataLabel32 is used to refer to a location in the code containing a 32-bit constant to be
    // patched after the code has been generated.
    class DataLabel32 {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabel32()
        {
        }

        DataLabel32(AbstractMacroAssemblerType* masm)
            : m_label(masm->m_assembler.label())
        {
        }

        AssemblerLabel label() const { return m_label; }

    private:
        AssemblerLabel m_label;
    };

    // DataLabelCompact:
    //
    // A DataLabelCompact is used to refer to a location in the code containing a
    // compact immediate to be patched after the code has been generated.
    class DataLabelCompact {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabelCompact()
        {
        }
        
        DataLabelCompact(AbstractMacroAssemblerType* masm)
            : m_label(masm->m_assembler.label())
        {
        }

        DataLabelCompact(AssemblerLabel label)
            : m_label(label)
        {
        }

        AssemblerLabel label() const { return m_label; }

    private:
        AssemblerLabel m_label;
    };

    // Call:
    //
    // A Call object is a reference to a call instruction that has been planted
    // into the code buffer - it is typically used to link the call, setting the
    // relative offset such that when executed it will call to the desired
    // destination.
    class Call {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;

    public:
        enum Flags {
            None = 0x0,
            Linkable = 0x1,
            Near = 0x2,
            Tail = 0x4,
            LinkableNear = 0x3,
            LinkableNearTail = 0x7,
        };

        Call()
            : m_flags(None)
        {
        }
        
        Call(AssemblerLabel jmp, Flags flags)
            : m_label(jmp)
            , m_flags(flags)
        {
        }

        bool isFlagSet(Flags flag)
        {
            return m_flags & flag;
        }

        static Call fromTailJump(Jump jump)
        {
            return Call(jump.m_label, Linkable);
        }

        AssemblerLabel m_label;
    private:
        Flags m_flags;
    };

    // Jump:
    //
    // A jump object is a reference to a jump instruction that has been planted
    // into the code buffer - it is typically used to link the jump, setting the
    // relative offset such that when executed it will jump to the desired
    // destination.
    class Jump {
        template<class TemplateAssemblerType, class TemplateMacroAssemblerType>
        friend class AbstractMacroAssembler;
        friend class Call;
        friend struct DFG::OSRExit;
        friend class LinkBuffer;
    public:
        Jump()
        {
        }
        
#if CPU(ARM_THUMB2)
        // Fixme: this information should be stored in the instruction stream, not in the Jump object.
        Jump(AssemblerLabel jmp, ARMv7Assembler::JumpType type = ARMv7Assembler::JumpNoCondition, ARMv7Assembler::Condition condition = ARMv7Assembler::ConditionInvalid)
            : m_label(jmp)
            , m_type(type)
            , m_condition(condition)
        {
        }
#elif CPU(ARM64)
        Jump(AssemblerLabel jmp, ARM64Assembler::JumpType type = ARM64Assembler::JumpNoCondition, ARM64Assembler::Condition condition = ARM64Assembler::ConditionInvalid)
            : m_label(jmp)
            , m_type(type)
            , m_condition(condition)
        {
        }

        Jump(AssemblerLabel jmp, ARM64Assembler::JumpType type, ARM64Assembler::Condition condition, bool is64Bit, ARM64Assembler::RegisterID compareRegister)
            : m_label(jmp)
            , m_type(type)
            , m_condition(condition)
            , m_is64Bit(is64Bit)
            , m_compareRegister(compareRegister)
        {
            ASSERT((type == ARM64Assembler::JumpCompareAndBranch) || (type == ARM64Assembler::JumpCompareAndBranchFixedSize));
        }

        Jump(AssemblerLabel jmp, ARM64Assembler::JumpType type, ARM64Assembler::Condition condition, unsigned bitNumber, ARM64Assembler::RegisterID compareRegister)
            : m_label(jmp)
            , m_type(type)
            , m_condition(condition)
            , m_bitNumber(bitNumber)
            , m_compareRegister(compareRegister)
        {
            ASSERT((type == ARM64Assembler::JumpTestBit) || (type == ARM64Assembler::JumpTestBitFixedSize));
        }
#elif CPU(SH4)
        Jump(AssemblerLabel jmp, SH4Assembler::JumpType type = SH4Assembler::JumpFar)
            : m_label(jmp)
            , m_type(type)
        {
        }
#else
        Jump(AssemblerLabel jmp)    
            : m_label(jmp)
        {
        }
#endif
        
        Label label() const
        {
            Label result;
            result.m_label = m_label;
            return result;
        }

        void link(AbstractMacroAssemblerType* masm) const
        {
            masm->invalidateAllTempRegisters();

#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
            masm->checkRegisterAllocationAgainstBranchRange(m_label.m_offset, masm->debugOffset());
#endif

#if CPU(ARM_THUMB2)
            masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type, m_condition);
#elif CPU(ARM64)
            if ((m_type == ARM64Assembler::JumpCompareAndBranch) || (m_type == ARM64Assembler::JumpCompareAndBranchFixedSize))
                masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type, m_condition, m_is64Bit, m_compareRegister);
            else if ((m_type == ARM64Assembler::JumpTestBit) || (m_type == ARM64Assembler::JumpTestBitFixedSize))
                masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type, m_condition, m_bitNumber, m_compareRegister);
            else
                masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type, m_condition);
#elif CPU(SH4)
            masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type);
#else
            masm->m_assembler.linkJump(m_label, masm->m_assembler.label());
#endif
        }
        
        void linkTo(Label label, AbstractMacroAssemblerType* masm) const
        {
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
            masm->checkRegisterAllocationAgainstBranchRange(label.m_label.m_offset, m_label.m_offset);
#endif

#if CPU(ARM_THUMB2)
            masm->m_assembler.linkJump(m_label, label.m_label, m_type, m_condition);
#elif CPU(ARM64)
            if ((m_type == ARM64Assembler::JumpCompareAndBranch) || (m_type == ARM64Assembler::JumpCompareAndBranchFixedSize))
                masm->m_assembler.linkJump(m_label, label.m_label, m_type, m_condition, m_is64Bit, m_compareRegister);
            else if ((m_type == ARM64Assembler::JumpTestBit) || (m_type == ARM64Assembler::JumpTestBitFixedSize))
                masm->m_assembler.linkJump(m_label, label.m_label, m_type, m_condition, m_bitNumber, m_compareRegister);
            else
                masm->m_assembler.linkJump(m_label, label.m_label, m_type, m_condition);
#else
            masm->m_assembler.linkJump(m_label, label.m_label);
#endif
        }

        bool isSet() const { return m_label.isSet(); }

    private:
        AssemblerLabel m_label;
#if CPU(ARM_THUMB2)
        ARMv7Assembler::JumpType m_type;
        ARMv7Assembler::Condition m_condition;
#elif CPU(ARM64)
        ARM64Assembler::JumpType m_type;
        ARM64Assembler::Condition m_condition;
        bool m_is64Bit;
        unsigned m_bitNumber;
        ARM64Assembler::RegisterID m_compareRegister;
#endif
#if CPU(SH4)
        SH4Assembler::JumpType m_type;
#endif
    };

    struct PatchableJump {
        PatchableJump()
        {
        }

        explicit PatchableJump(Jump jump)
            : m_jump(jump)
        {
        }

        operator Jump&() { return m_jump; }

        Jump m_jump;
    };

    // JumpList:
    //
    // A JumpList is a set of Jump objects.
    // All jumps in the set will be linked to the same destination.
    class JumpList {
        friend class LinkBuffer;

    public:
        typedef Vector<Jump, 2> JumpVector;
        
        JumpList() { }
        
        JumpList(Jump jump)
        {
            if (jump.isSet())
                append(jump);
        }

        void link(AbstractMacroAssemblerType* masm)
        {
            size_t size = m_jumps.size();
            for (size_t i = 0; i < size; ++i)
                m_jumps[i].link(masm);
            m_jumps.clear();
        }
        
        void linkTo(Label label, AbstractMacroAssemblerType* masm)
        {
            size_t size = m_jumps.size();
            for (size_t i = 0; i < size; ++i)
                m_jumps[i].linkTo(label, masm);
            m_jumps.clear();
        }
        
        void append(Jump jump)
        {
            m_jumps.append(jump);
        }
        
        void append(const JumpList& other)
        {
            m_jumps.append(other.m_jumps.begin(), other.m_jumps.size());
        }

        bool empty()
        {
            return !m_jumps.size();
        }
        
        void clear()
        {
            m_jumps.clear();
        }
        
        const JumpVector& jumps() const { return m_jumps; }

    private:
        JumpVector m_jumps;
    };


    // Section 3: Misc admin methods
#if ENABLE(DFG_JIT)
    Label labelIgnoringWatchpoints()
    {
        Label result;
        result.m_label = m_assembler.labelIgnoringWatchpoints();
        return result;
    }
#else
    Label labelIgnoringWatchpoints()
    {
        return label();
    }
#endif
    
    Label label()
    {
        return Label(this);
    }
    
    void padBeforePatch()
    {
        // Rely on the fact that asking for a label already does the padding.
        (void)label();
    }
    
    Label watchpointLabel()
    {
        Label result;
        result.m_label = m_assembler.labelForWatchpoint();
        return result;
    }
    
    Label align()
    {
        m_assembler.align(16);
        return Label(this);
    }

#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    class RegisterAllocationOffset {
    public:
        RegisterAllocationOffset(unsigned offset)
            : m_offset(offset)
        {
        }

        void checkOffsets(unsigned low, unsigned high)
        {
            RELEASE_ASSERT_WITH_MESSAGE(!(low <= m_offset && m_offset <= high), "Unsafe branch over register allocation at instruction offset %u in jump offset range %u..%u", m_offset, low, high);
        }

    private:
        unsigned m_offset;
    };

    void addRegisterAllocationAtOffset(unsigned offset)
    {
        m_registerAllocationForOffsets.append(RegisterAllocationOffset(offset));
    }

    void clearRegisterAllocationOffsets()
    {
        m_registerAllocationForOffsets.clear();
    }

    void checkRegisterAllocationAgainstBranchRange(unsigned offset1, unsigned offset2)
    {
        if (offset1 > offset2)
            std::swap(offset1, offset2);

        size_t size = m_registerAllocationForOffsets.size();
        for (size_t i = 0; i < size; ++i)
            m_registerAllocationForOffsets[i].checkOffsets(offset1, offset2);
    }
#endif

    template<typename T, typename U>
    static ptrdiff_t differenceBetween(T from, U to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    static ptrdiff_t differenceBetweenCodePtr(const MacroAssemblerCodePtr& a, const MacroAssemblerCodePtr& b)
    {
        return reinterpret_cast<ptrdiff_t>(b.executableAddress()) - reinterpret_cast<ptrdiff_t>(a.executableAddress());
    }

    unsigned debugOffset() { return m_assembler.debugOffset(); }

    ALWAYS_INLINE static void cacheFlush(void* code, size_t size)
    {
        AssemblerType::cacheFlush(code, size);
    }

#if ENABLE(MASM_PROBE)

    struct CPUState {
        #define DECLARE_REGISTER(_type, _regName) \
            _type _regName;
        FOR_EACH_CPU_REGISTER(DECLARE_REGISTER)
        #undef DECLARE_REGISTER

        static const char* registerName(RegisterID regID)
        {
            switch (regID) {
                #define DECLARE_REGISTER(_type, _regName) \
                case RegisterID::_regName: \
                    return #_regName;
                FOR_EACH_CPU_GPREGISTER(DECLARE_REGISTER)
                #undef DECLARE_REGISTER
            }
            RELEASE_ASSERT_NOT_REACHED();
        }

        static const char* registerName(FPRegisterID regID)
        {
            switch (regID) {
                #define DECLARE_REGISTER(_type, _regName) \
                case FPRegisterID::_regName: \
                    return #_regName;
                FOR_EACH_CPU_FPREGISTER(DECLARE_REGISTER)
                #undef DECLARE_REGISTER
            }
            RELEASE_ASSERT_NOT_REACHED();
        }

        void* registerValue(RegisterID regID)
        {
            switch (regID) {
                #define DECLARE_REGISTER(_type, _regName) \
                case RegisterID::_regName: \
                    return _regName;
                FOR_EACH_CPU_GPREGISTER(DECLARE_REGISTER)
                #undef DECLARE_REGISTER
            }
            RELEASE_ASSERT_NOT_REACHED();
        }

        double registerValue(FPRegisterID regID)
        {
            switch (regID) {
                #define DECLARE_REGISTER(_type, _regName) \
                case FPRegisterID::_regName: \
                    return _regName;
                FOR_EACH_CPU_FPREGISTER(DECLARE_REGISTER)
                #undef DECLARE_REGISTER
            }
            RELEASE_ASSERT_NOT_REACHED();
        }

    };

    struct ProbeContext;
    typedef void (*ProbeFunction)(struct ProbeContext*);

    struct ProbeContext {
        ProbeFunction probeFunction;
        void* arg1;
        void* arg2;
        CPUState cpu;

        void print(int indentation = 0)
        {
            #define INDENT MacroAssemblerType::printIndent(indentation)

            INDENT, dataLogF("ProbeContext %p {\n", this);
            indentation++;
            {
                INDENT, dataLogF("probeFunction: %p\n", probeFunction);
                INDENT, dataLogF("arg1: %p %llu\n", arg1, reinterpret_cast<int64_t>(arg1));
                INDENT, dataLogF("arg2: %p %llu\n", arg2, reinterpret_cast<int64_t>(arg2));
                MacroAssemblerType::printCPU(cpu, indentation);
            }
            indentation--;
            INDENT, dataLog("}\n");

            #undef INDENT
        }
    };

    static void printIndent(int indentation)
    {
        for (; indentation > 0; indentation--)
            dataLog("    ");
    }

    static void printCPU(CPUState& cpu, int indentation = 0)
    {
        #define INDENT printIndent(indentation)

        INDENT, dataLog("cpu: {\n");
        MacroAssemblerType::printCPURegisters(cpu, indentation + 1);
        INDENT, dataLog("}\n");

        #undef INDENT
    }

    // This is a marker type only used with print(). See print() below for details.
    struct AllRegisters { };

    // Emits code which will print debugging info at runtime. The type of values that
    // can be printed is encapsulated in the PrintArg struct below. Here are some
    // examples:
    //
    //      print("Hello world\n"); // Emits code to print the string.
    //
    //      CodeBlock* cb = ...;
    //      print(cb);              // Emits code to print the pointer value.
    //
    //      RegisterID regID = ...;
    //      print(regID);           // Emits code to print the register value (not the id).
    //
    //      // Emits code to print all registers.  Unlike other items, this prints
    //      // multiple lines as follows:
    //      //      cpu {
    //      //          eax: 0x123456789
    //      //          ebx: 0x000000abc
    //      //          ...
    //      //      }
    //      print(AllRegisters());
    //
    //      // Print multiple things at once. This incurs the probe overhead only once
    //      // to print all the items.
    //      print("cb:", cb, " regID:", regID, " cpu:\n", AllRegisters());

    template<typename... Arguments>
    void print(Arguments... args)
    {
        printInternal(static_cast<MacroAssemblerType*>(this), args...);
    }

    // This function will be called by printCPU() to print the contents of the
    // target specific registers which are saved away in the CPUState struct.
    // printCPURegisters() should make use of printIndentation() to print the
    // registers with the appropriate amount of indentation.
    //
    // Note: printCPURegisters() should be implemented by the target specific
    // MacroAssembler. This prototype is only provided here to document the
    // interface.

    static void printCPURegisters(CPUState&, int indentation = 0);

    // This function will be called by print() to print the contents of a
    // specific register (from the CPUState) in line with other items in the
    // print stream. Hence, no indentation is needed.
    //
    // Note: printRegister() should be implemented by the target specific
    // MacroAssembler. These prototypes are only provided here to document their
    // interface.

    static void printRegister(CPUState&, RegisterID);
    static void printRegister(CPUState&, FPRegisterID);

    // This function emits code to preserve the CPUState (e.g. registers),
    // call a user supplied probe function, and restore the CPUState before
    // continuing with other JIT generated code.
    //
    // The user supplied probe function will be called with a single pointer to
    // a ProbeContext struct (defined above) which contains, among other things,
    // the preserved CPUState. This allows the user probe function to inspect
    // the CPUState at that point in the JIT generated code.
    //
    // If the user probe function alters the register values in the ProbeContext,
    // the altered values will be loaded into the CPU registers when the probe
    // returns.
    //
    // The ProbeContext is stack allocated and is only valid for the duration
    // of the call to the user probe function.
    //
    // Note: probe() should be implemented by the target specific MacroAssembler.
    // This prototype is only provided here to document the interface.

    void probe(ProbeFunction, void* arg1 = 0, void* arg2 = 0);

#endif // ENABLE(MASM_PROBE)

    AssemblerType m_assembler;
    
protected:
    AbstractMacroAssembler()
        : m_randomSource(cryptographicallyRandomNumber())
    {
        invalidateAllTempRegisters();
    }

    uint32_t random()
    {
        return m_randomSource.getUint32();
    }

    WeakRandom m_randomSource;

#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    Vector<RegisterAllocationOffset, 10> m_registerAllocationForOffsets;
#endif

    static bool haveScratchRegisterForBlinding()
    {
        return false;
    }
    static RegisterID scratchRegisterForBlinding()
    {
        UNREACHABLE_FOR_PLATFORM();
        return firstRegister();
    }
    static bool canBlind() { return false; }
    static bool shouldBlindForSpecificArch(uint32_t) { return false; }
    static bool shouldBlindForSpecificArch(uint64_t) { return false; }

    class CachedTempRegister {
        friend class DataLabelPtr;
        friend class DataLabel32;
        friend class DataLabelCompact;
        friend class Jump;
        friend class Label;

    public:
        CachedTempRegister(AbstractMacroAssemblerType* masm, RegisterID registerID)
            : m_masm(masm)
            , m_registerID(registerID)
            , m_value(0)
            , m_validBit(1 << static_cast<unsigned>(registerID))
        {
            ASSERT(static_cast<unsigned>(registerID) < (sizeof(unsigned) * 8));
        }

        ALWAYS_INLINE RegisterID registerIDInvalidate() { invalidate(); return m_registerID; }

        ALWAYS_INLINE RegisterID registerIDNoInvalidate() { return m_registerID; }

        bool value(intptr_t& value)
        {
            value = m_value;
            return m_masm->isTempRegisterValid(m_validBit);
        }

        void setValue(intptr_t value)
        {
            m_value = value;
            m_masm->setTempRegisterValid(m_validBit);
        }

        ALWAYS_INLINE void invalidate() { m_masm->clearTempRegisterValid(m_validBit); }

    private:
        AbstractMacroAssemblerType* m_masm;
        RegisterID m_registerID;
        intptr_t m_value;
        unsigned m_validBit;
    };

    ALWAYS_INLINE void invalidateAllTempRegisters()
    {
        m_tempRegistersValidBits = 0;
    }

    ALWAYS_INLINE bool isTempRegisterValid(unsigned registerMask)
    {
        return (m_tempRegistersValidBits & registerMask);
    }

    ALWAYS_INLINE void clearTempRegisterValid(unsigned registerMask)
    {
        m_tempRegistersValidBits &=  ~registerMask;
    }

    ALWAYS_INLINE void setTempRegisterValid(unsigned registerMask)
    {
        m_tempRegistersValidBits |= registerMask;
    }

    unsigned m_tempRegistersValidBits;

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkJump(void* code, Jump jump, CodeLocationLabel target)
    {
        AssemblerType::linkJump(code, jump.m_label, target.dataLocation());
    }

    static void linkPointer(void* code, AssemblerLabel label, void* value)
    {
        AssemblerType::linkPointer(code, label, value);
    }

    static void* getLinkerAddress(void* code, AssemblerLabel label)
    {
        return AssemblerType::getRelocatedAddress(code, label);
    }

    static unsigned getLinkerCallReturnOffset(Call call)
    {
        return AssemblerType::getCallReturnOffset(call.m_label);
    }

    static void repatchJump(CodeLocationJump jump, CodeLocationLabel destination)
    {
        AssemblerType::relinkJump(jump.dataLocation(), destination.dataLocation());
    }

    static void repatchNearCall(CodeLocationNearCall nearCall, CodeLocationLabel destination)
    {
        switch (nearCall.callMode()) {
        case NearCallMode::Tail:
            AssemblerType::relinkJump(nearCall.dataLocation(), destination.executableAddress());
            return;
        case NearCallMode::Regular:
            AssemblerType::relinkCall(nearCall.dataLocation(), destination.executableAddress());
            return;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    static void repatchCompact(CodeLocationDataLabelCompact dataLabelCompact, int32_t value)
    {
        AssemblerType::repatchCompact(dataLabelCompact.dataLocation(), value);
    }
    
    static void repatchInt32(CodeLocationDataLabel32 dataLabel32, int32_t value)
    {
        AssemblerType::repatchInt32(dataLabel32.dataLocation(), value);
    }

    static void repatchPointer(CodeLocationDataLabelPtr dataLabelPtr, void* value)
    {
        AssemblerType::repatchPointer(dataLabelPtr.dataLocation(), value);
    }
    
    static void* readPointer(CodeLocationDataLabelPtr dataLabelPtr)
    {
        return AssemblerType::readPointer(dataLabelPtr.dataLocation());
    }
    
    static void replaceWithLoad(CodeLocationConvertibleLoad label)
    {
        AssemblerType::replaceWithLoad(label.dataLocation());
    }
    
    static void replaceWithAddressComputation(CodeLocationConvertibleLoad label)
    {
        AssemblerType::replaceWithAddressComputation(label.dataLocation());
    }

private:

#if ENABLE(MASM_PROBE)

    struct PrintArg {
    
        enum class Type {
            AllRegisters,
            RegisterID,
            FPRegisterID,
            ConstCharPtr,
            ConstVoidPtr,
            IntptrValue,
            UintptrValue,
        };

        PrintArg(AllRegisters&)
            : type(Type::AllRegisters)
        {
        }

        PrintArg(RegisterID regID)
            : type(Type::RegisterID)
        {
            u.gpRegisterID = regID;
        }

        PrintArg(FPRegisterID regID)
            : type(Type::FPRegisterID)
        {
            u.fpRegisterID = regID;
        }

        PrintArg(const char* ptr)
            : type(Type::ConstCharPtr)
        {
            u.constCharPtr = ptr;
        }

        PrintArg(const void* ptr)
            : type(Type::ConstVoidPtr)
        {
            u.constVoidPtr = ptr;
        }

        PrintArg(int value)
            : type(Type::IntptrValue)
        {
            u.intptrValue = value;
        }

        PrintArg(unsigned value)
            : type(Type::UintptrValue)
        {
            u.intptrValue = value;
        }

        PrintArg(intptr_t value)
            : type(Type::IntptrValue)
        {
            u.intptrValue = value;
        }

        PrintArg(uintptr_t value)
            : type(Type::UintptrValue)
        {
            u.uintptrValue = value;
        }

        Type type;
        union {
            RegisterID gpRegisterID;
            FPRegisterID fpRegisterID;
            const char* constCharPtr;
            const void* constVoidPtr;
            intptr_t intptrValue;
            uintptr_t uintptrValue;
        } u;
    };

    typedef Vector<PrintArg> PrintArgsList;

    template<typename FirstArg, typename... Arguments>
    static void appendPrintArg(PrintArgsList* argsList, FirstArg& firstArg, Arguments... otherArgs)
    {
        argsList->append(PrintArg(firstArg));
        appendPrintArg(argsList, otherArgs...);
    }

    static void appendPrintArg(PrintArgsList*) { }

    
    template<typename... Arguments>
    static void printInternal(MacroAssemblerType* masm, Arguments... args)
    {
        auto argsList = std::make_unique<PrintArgsList>();
        appendPrintArg(argsList.get(), args...);
        masm->probe(printCallback, argsList.release());
    }

    static void printCallback(ProbeContext* context)
    {
        typedef PrintArg Arg;
        PrintArgsList& argsList =
            *reinterpret_cast<PrintArgsList*>(context->arg1);
        for (size_t i = 0; i < argsList.size(); i++) {
            auto& arg = argsList[i];
            switch (arg.type) {
            case Arg::Type::AllRegisters:
                MacroAssemblerType::printCPU(context->cpu);
                break;
            case Arg::Type::RegisterID:
                MacroAssemblerType::printRegister(context->cpu, arg.u.gpRegisterID);
                break;
            case Arg::Type::FPRegisterID:
                MacroAssemblerType::printRegister(context->cpu, arg.u.fpRegisterID);
                break;
            case Arg::Type::ConstCharPtr:
                dataLog(arg.u.constCharPtr);
                break;
            case Arg::Type::ConstVoidPtr:
                dataLogF("%p", arg.u.constVoidPtr);
                break;
            case Arg::Type::IntptrValue:
                dataLog(arg.u.intptrValue);
                break;
            case Arg::Type::UintptrValue:
                dataLog(arg.u.uintptrValue);
                break;
            }
        }
    }

#endif // ENABLE(MASM_PROBE)

}; // class AbstractMacroAssembler

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // AbstractMacroAssembler_h
