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

#ifndef FTLStackMaps_h
#define FTLStackMaps_h

#if ENABLE(FTL_JIT)

#include "DataView.h"
#include "FTLDWARFRegister.h"
#include "GPRInfo.h"
#include "RegisterSet.h"
#include <wtf/HashMap.h>

namespace JSC {

class MacroAssembler;

namespace FTL {

struct StackMaps {
    struct ParseContext {
        unsigned version;
        DataView* view;
        unsigned offset;
    };
    
    struct Constant {
        int64_t integer;
        
        void parse(ParseContext&);
        void dump(PrintStream& out) const;
    };

    struct StackSize {
        uint64_t functionOffset;
        uint64_t size;

        void parse(ParseContext&);
        void dump(PrintStream&) const;
    };

    struct Location {
        enum Kind : int8_t {
            Unprocessed,
            Register,
            Direct,
            Indirect,
            Constant,
            ConstantIndex
        };
        
        DWARFRegister dwarfReg;
        uint8_t size;
        Kind kind;
        int32_t offset;
        
        void parse(ParseContext&);
        void dump(PrintStream& out) const;
        
        GPRReg directGPR() const;
        void restoreInto(MacroAssembler&, StackMaps&, char* savedRegisters, GPRReg result) const;
    };
    
    // FIXME: Investigate how much memory this takes and possibly prune it from the
    // format we keep around in FTL::JITCode. I suspect that it would be most awesome to
    // have a CompactStackMaps struct that lossily stores only that subset of StackMaps
    // and Record that we actually need for OSR exit.
    // https://bugs.webkit.org/show_bug.cgi?id=130802
    struct LiveOut {
        DWARFRegister dwarfReg;
        uint8_t size;
        
        void parse(ParseContext&);
        void dump(PrintStream& out) const;
    };
    
    struct Record {
        uint32_t patchpointID;
        uint32_t instructionOffset;
        uint16_t flags;
        
        Vector<Location> locations;
        Vector<LiveOut> liveOuts;
        
        bool parse(ParseContext&);
        void dump(PrintStream&) const;
        
        RegisterSet liveOutsSet() const;
        RegisterSet locationSet() const;
        RegisterSet usedRegisterSet() const;
    };

    unsigned version;
    Vector<StackSize> stackSizes;
    Vector<Constant> constants;
    Vector<Record> records;
    
    bool parse(DataView*); // Returns true on parse success, false on failure. Failure means that LLVM is signaling compile failure to us.
    void dump(PrintStream&) const;
    void dumpMultiline(PrintStream&, const char* prefix) const;
    
    typedef HashMap<uint32_t, Vector<Record>, WTF::IntHash<uint32_t>, WTF::UnsignedWithZeroKeyHashTraits<uint32_t>> RecordMap;
    
    RecordMap computeRecordMap() const;

    unsigned stackSize() const;
};

} } // namespace JSC::FTL

namespace WTF {

void printInternal(PrintStream&, JSC::FTL::StackMaps::Location::Kind);

} // namespace WTF

#endif // ENABLE(FTL_JIT)

#endif // FTLStackMaps_h

