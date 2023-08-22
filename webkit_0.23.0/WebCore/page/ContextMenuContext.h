/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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
 *
 */

#ifndef ContextMenuContext_h
#define ContextMenuContext_h

#if ENABLE(CONTEXT_MENUS)

#include "HitTestResult.h"
#include "Image.h"

namespace WebCore {

class ContextMenuContext {
public:
    ContextMenuContext();
    ContextMenuContext(const HitTestResult&);

    const HitTestResult& hitTestResult() const { return m_hitTestResult; }

    void setSelectedText(const String& selectedText) { m_selectedText = selectedText; }
    const String& selectedText() const { return m_selectedText; }

#if ENABLE(SERVICE_CONTROLS)
    void setControlledImage(Image* controlledImage) { m_controlledImage = controlledImage; }
    Image* controlledImage() const { return m_controlledImage.get(); }
#endif

private:

    HitTestResult m_hitTestResult;
    String m_selectedText;

#if ENABLE(SERVICE_CONTROLS)
    RefPtr<Image> m_controlledImage;
#endif
};

} // namespace WebCore

#endif // ENABLE(CONTEXT_MENUS)
#endif // ContextMenuContext_h
