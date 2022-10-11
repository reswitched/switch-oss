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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CompiledContentExtension_h
#define CompiledContentExtension_h

#if ENABLE(CONTENT_EXTENSIONS)

#include "ContentExtensionRule.h"
#include "DFABytecode.h"
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {
namespace ContentExtensions {

class WEBCORE_EXPORT CompiledContentExtension : public ThreadSafeRefCounted<CompiledContentExtension> {
public:
    virtual ~CompiledContentExtension();

    virtual const DFABytecode* filtersWithoutDomainsBytecode() const = 0;
    virtual unsigned filtersWithoutDomainsBytecodeLength() const = 0;
    virtual const DFABytecode* filtersWithDomainsBytecode() const = 0;
    virtual unsigned filtersWithDomainsBytecodeLength() const = 0;
    virtual const DFABytecode* domainFiltersBytecode() const = 0;
    virtual unsigned domainFiltersBytecodeLength() const = 0;
    virtual const SerializedActionByte* actions() const = 0;
    virtual unsigned actionsLength() const = 0;
    Vector<String> globalDisplayNoneSelectors();
};

} // namespace ContentExtensions
} // namespace WebCore

#endif // ENABLE(CONTENT_EXTENSIONS)
#endif // CompiledContentExtension_h
