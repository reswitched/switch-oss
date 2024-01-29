/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef EvasGLContext_h
#define EvasGLContext_h

#include <Evas_GL.h>

namespace WebCore {

class EvasGLContext {
public:
    static std::unique_ptr<EvasGLContext> create(Evas_GL* evasGL)
    {
        ASSERT(evasGL);
        Evas_GL_Context* context = evas_gl_context_create(evasGL, 0);
        if (!context)
            return nullptr;

        // Ownership of context is passed to EvasGLContext.
        return std::make_unique<EvasGLContext>(evasGL, context);
    }

    EvasGLContext(Evas_GL*, Evas_GL_Context* passContext);
    ~EvasGLContext();

    Evas_GL_Context* context() { return m_context; }

private:
    Evas_GL* m_evasGL;
    Evas_GL_Context* m_context;
};

} // namespace WebCore

#endif // EvasGLContext_h
