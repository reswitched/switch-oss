/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#ifndef RegisterAtOffsetList_h
#define RegisterAtOffsetList_h

#if ENABLE(JIT)

#include "RegisterAtOffset.h"
#include "RegisterSet.h"

namespace JSC {

class RegisterAtOffsetList {
public:
    enum OffsetBaseType { FramePointerBased, ZeroBased };

    RegisterAtOffsetList();
    RegisterAtOffsetList(RegisterSet, OffsetBaseType = FramePointerBased);

    void dump(PrintStream&) const;

    void clear()
    {
        m_registers.clear();
    }

    size_t size()
    {
        return m_registers.size();
    }

    RegisterAtOffset& at(size_t index)
    {
        return m_registers.at(index);
    }
    
    void append(RegisterAtOffset registerAtOffset)
    {
        m_registers.append(registerAtOffset);
    }

    void sort();
    RegisterAtOffset* find(Reg) const;
    unsigned indexOf(Reg) const; // Returns UINT_MAX if not found.

private:
    Vector<RegisterAtOffset> m_registers;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // RegisterAtOffsetList_h

