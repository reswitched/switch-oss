/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef ANGLEWebKitBridge_h
#define ANGLEWebKitBridge_h

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(IOS)
#import <OpenGLES/ES2/glext.h>
#elif PLATFORM(MAC)
#include <OpenGL/gl.h>
#elif PLATFORM(WIN)
#include "OpenGLESShims.h"
#elif PLATFORM(GTK) || PLATFORM(EFL)
#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#else
#include "OpenGLShims.h"
#endif
#endif

#if !PLATFORM(GTK) && !PLATFORM(EFL) && !PLATFORM(WIN)  && !PLATFORM(WKC) && !defined(BUILDING_WITH_CMAKE)
#include "ANGLE/ShaderLang.h"
#elif (PLATFORM(WIN) || PLATFORM(WKC)) && !defined(BUILDING_WITH_CMAKE)
#if PLATFORM(WKC)
#define __WKC__
#endif
#include "GLSLANG/ShaderLang.h"
#else
#include <ANGLE/ShaderLang.h>
#endif

#if PLATFORM(WKC)
#define GL_VERTEX_SHADER       0x8B31
#define GL_FRAGMENT_SHADER     0x8B30
#define GL_SAMPLER_2D          0x8B5E
#define GL_SAMPLER_CUBE        0x8B60
#define GL_SAMPLER_2D_RECT_ARB 0x8B63
#endif

namespace WebCore {

enum ANGLEShaderType {
    SHADER_TYPE_VERTEX = GL_VERTEX_SHADER,
    SHADER_TYPE_FRAGMENT = GL_FRAGMENT_SHADER,
};

enum ANGLEShaderSymbolType {
    SHADER_SYMBOL_TYPE_ATTRIBUTE,
    SHADER_SYMBOL_TYPE_UNIFORM,
    SHADER_SYMBOL_TYPE_VARYING
};

struct ANGLEShaderSymbol {
    ANGLEShaderSymbolType symbolType;
    String name;
    String mappedName;
    sh::GLenum dataType;
    unsigned size;
    sh::GLenum precision;
    int staticUse;

    bool isSampler() const
    {
        return symbolType == SHADER_SYMBOL_TYPE_UNIFORM
            && (dataType == GL_SAMPLER_2D
            || dataType == GL_SAMPLER_CUBE
#if !PLATFORM(IOS) && !((PLATFORM(EFL) || PLATFORM(GTK)) && USE(OPENGL_ES_2))
            || dataType == GL_SAMPLER_2D_RECT_ARB
#endif
            );
    }
};

class ANGLEWebKitBridge {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:

    ANGLEWebKitBridge(ShShaderOutput = SH_GLSL_OUTPUT, ShShaderSpec = SH_WEBGL_SPEC);
    ~ANGLEWebKitBridge();
    
    ShBuiltInResources getResources() { return m_resources; }
    void setResources(ShBuiltInResources);
    
    bool compileShaderSource(const char* shaderSource, ANGLEShaderType, String& translatedShaderSource, String& shaderValidationLog, Vector<ANGLEShaderSymbol>& symbols, int extraCompileOptions = 0);

#if PLATFORM(WKC)
    void setShaderOutput(ShShaderOutput output) { m_shaderOutput = output; }
#endif

private:

    void cleanupCompilers();

    bool builtCompilers;
    
    ShHandle m_fragmentCompiler;
    ShHandle m_vertexCompiler;

    ShShaderOutput m_shaderOutput;
    ShShaderSpec m_shaderSpec;

    ShBuiltInResources m_resources;
};

} // namespace WebCore

#endif
