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

#ifndef ContentExtension_h
#define ContentExtension_h

#include <wtf/Ref.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if ENABLE(CONTENT_EXTENSIONS)

#include "DFABytecodeInterpreter.h"
#include "StyleSheetContents.h"

namespace WebCore {

namespace ContentExtensions {

class CompiledContentExtension;

class ContentExtension : public RefCounted<ContentExtension> {
public:
    static RefPtr<ContentExtension> create(const String& identifier, Ref<CompiledContentExtension>&&);

    const String& identifier() const { return m_identifier; }
    const CompiledContentExtension& compiledExtension() const { return m_compiledExtension.get(); }
    StyleSheetContents* globalDisplayNoneStyleSheet();
    const DFABytecodeInterpreter::Actions& cachedDomainActions(const String& domain);

private:
    ContentExtension(const String& identifier, Ref<CompiledContentExtension>&&);

    String m_identifier;
    Ref<CompiledContentExtension> m_compiledExtension;
    RefPtr<StyleSheetContents> m_globalDisplayNoneStyleSheet;
    bool m_parsedGlobalDisplayNoneStyleSheet;
    
    String m_cachedDomain;
    DFABytecodeInterpreter::Actions m_cachedDomainActions;
};

} // namespace ContentExtensions
} // namespace WebCore

#endif // ENABLE(CONTENT_EXTENSIONS)
#endif // ContentExtension_h
