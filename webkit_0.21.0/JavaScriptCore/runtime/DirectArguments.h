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

#ifndef DirectArguments_h
#define DirectArguments_h

#include "DirectArgumentsOffset.h"
#include "GenericArguments.h"

namespace JSC {

// This is an Arguments-class object that we create when you say "arguments" inside a function,
// and none of the arguments are captured in the function's activation. The function will copy all
// of its arguments into this object, and all subsequent accesses to the arguments will go through
// this object thereafter. Special support is in place for mischevious events like the arguments
// being deleted (something like "delete arguments[0]") or reconfigured (broadly, we say deletions
// and reconfigurations mean that the respective argument was "overridden").
//
// To speed allocation, this object will hold all of the arguments in-place. The arguments as well
// as a table of flags saying which arguments were overridden.
class DirectArguments : public GenericArguments<DirectArguments> {
private:
    DirectArguments(VM&, Structure*, unsigned length, unsigned capacity);
    
public:
    // Creates an arguments object but leaves it uninitialized. This is dangerous if we GC right
    // after allocation.
    static DirectArguments* createUninitialized(VM&, Structure*, unsigned length, unsigned capacity);
    
    // Creates an arguments object and initializes everything to the empty value. Use this if you
    // cannot guarantee that you'll immediately initialize all of the elements.
    static DirectArguments* create(VM&, Structure*, unsigned length, unsigned capacity);
    
    // Creates an arguments object by copying the argumnets from the stack.
    static DirectArguments* createByCopying(ExecState*);
    
    static void visitChildren(JSCell*, SlotVisitor&);
    static void copyBackingStore(JSCell*, CopyVisitor&, CopyToken);
    
    uint32_t internalLength() const
    {
        return m_length;
    }
    
    uint32_t length(ExecState* exec) const
    {
        if (UNLIKELY(m_overrides))
            return get(exec, exec->propertyNames().length).toUInt32(exec);
        return m_length;
    }
    
    bool canAccessIndexQuickly(uint32_t i) const
    {
        return i < m_length && (!m_overrides || !m_overrides.get()[i]);
    }

    bool canAccessArgumentIndexQuicklyInDFG(uint32_t i) const
    {
        return i < m_length && !overrodeThings();
    }

    JSValue getIndexQuickly(uint32_t i) const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(canAccessIndexQuickly(i));
        return const_cast<DirectArguments*>(this)->storage()[i].get();
    }
    
    void setIndexQuickly(VM& vm, uint32_t i, JSValue value)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(canAccessIndexQuickly(i));
        storage()[i].set(vm, this, value);
    }
    
    WriteBarrier<JSFunction>& callee()
    {
        return m_callee;
    }
    
    WriteBarrier<Unknown>& argument(DirectArgumentsOffset offset)
    {
        ASSERT(offset);
        ASSERT_WITH_SECURITY_IMPLICATION(offset.offset() < std::max(m_length, m_minCapacity));
        return storage()[offset.offset()];
    }
    
    // Methods intended for use by the GenericArguments mixin.
    bool overrodeThings() const { return !!m_overrides; }
    void overrideThings(VM&);
    void overrideThingsIfNecessary(VM&);
    void overrideArgument(VM&, unsigned index);
    
    void copyToArguments(ExecState*, VirtualRegister firstElementDest, unsigned offset, unsigned length);

    DECLARE_INFO;
    
    static Structure* createStructure(VM&, JSGlobalObject*, JSValue prototype);
    
    static ptrdiff_t offsetOfCallee() { return OBJECT_OFFSETOF(DirectArguments, m_callee); }
    static ptrdiff_t offsetOfLength() { return OBJECT_OFFSETOF(DirectArguments, m_length); }
    static ptrdiff_t offsetOfMinCapacity() { return OBJECT_OFFSETOF(DirectArguments, m_minCapacity); }
    static ptrdiff_t offsetOfOverrides() { return OBJECT_OFFSETOF(DirectArguments, m_overrides); }
    
    static size_t storageOffset()
    {
        return WTF::roundUpToMultipleOf<sizeof(WriteBarrier<Unknown>)>(sizeof(DirectArguments));
    }
    
    static size_t offsetOfSlot(uint32_t index)
    {
        return storageOffset() + sizeof(WriteBarrier<Unknown>) * index;
    }
    
    static size_t allocationSize(uint32_t capacity)
    {
        return offsetOfSlot(capacity);
    }
    
private:
    WriteBarrier<Unknown>* storage()
    {
        return bitwise_cast<WriteBarrier<Unknown>*>(bitwise_cast<char*>(this) + storageOffset());
    }
    
    unsigned overridesSize();
    
    WriteBarrier<JSFunction> m_callee;
    uint32_t m_length; // Always the actual length of captured arguments and never what was stored into the length property.
    uint32_t m_minCapacity; // The max of this and length determines the capacity of this object. It may be the actual capacity, or maybe something smaller. We arrange it this way to be kind to the JITs.
    CopyWriteBarrier<bool> m_overrides; // If non-null, it means that length, callee, and caller are fully materialized properties.
};

} // namespace JSC

#endif // DirectArguments_h

