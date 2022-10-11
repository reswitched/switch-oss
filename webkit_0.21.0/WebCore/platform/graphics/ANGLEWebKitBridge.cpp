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

#include "config.h"

#if ENABLE(GRAPHICS_CONTEXT_3D)

#include "ANGLEWebKitBridge.h"
#include "Logging.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

// FIXME: This is awful. Get rid of ANGLEWebKitBridge completely and call the libANGLE API directly to validate shaders.

#if !PLATFORM(WKC)
static void appendSymbol(const sh::ShaderVariable& variable, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols, const std::string& name, const std::string& mappedName)
{
    LOG(WebGL, "Map shader symbol %s -> %s\n", name.c_str(), mappedName.c_str());

    symbols.append(ANGLEShaderSymbol({ symbolType, name.c_str(), mappedName.c_str(), variable.type, variable.arraySize, variable.precision, variable.staticUse }));

    if (variable.isArray()) {
        for (unsigned i = 0; i < variable.elementCount(); i++) {
            std::string arrayBrackets = "[" + std::to_string(i) + "]";
            std::string arrayName = name + arrayBrackets;
            std::string arrayMappedName = mappedName + arrayBrackets;
            LOG(WebGL, "Map shader symbol %s -> %s\n", arrayName.c_str(), arrayMappedName.c_str());

            symbols.append({ symbolType, arrayName.c_str(), arrayMappedName.c_str(), variable.type, variable.arraySize, variable.precision, variable.staticUse });
        }
    }
}
#else
static void appendSymbol(const sh::ShaderVariable& variable, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols, const String& name, const String& mappedName)
{
    LOG(WebGL, "Map shader symbol %s -> %s\n", name.utf8().data(), mappedName.utf8().data());

    ANGLEShaderSymbol v = { symbolType, name.utf8().data(), mappedName.utf8().data(), variable.type, variable.arraySize, variable.precision, variable.staticUse };
    symbols.append(v);
    
    if (variable.isArray()) {
        for (unsigned i = 0; i < variable.elementCount(); i++) {
            String arrayBrackets = "[" + String::format("%d", i) + "]";
            String arrayName = name + arrayBrackets;
            String arrayMappedName = mappedName + arrayBrackets;
            LOG(WebGL, "Map shader symbol %s -> %s\n", arrayName.utf8().data(), arrayMappedName.utf8().data());
            
            ANGLEShaderSymbol v1 = {symbolType, arrayName, arrayMappedName, variable.type, variable.arraySize, variable.precision, variable.staticUse};
            symbols.append(v1);
        }
    }
}
#endif

#if !PLATFORM(WKC)
static void getStructInfo(const sh::ShaderVariable& field, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols, const std::string& namePrefix, const std::string& mappedNamePrefix)
{
    std::string name = namePrefix + '.' + field.name;
    std::string mappedName = mappedNamePrefix + '.' + field.mappedName;
    
    if (field.isStruct()) {
        for (const auto& subfield : field.fields) {
            // ANGLE restricts the depth of structs, which prevents stack overflow errors in this recursion.
            getStructInfo(subfield, symbolType, symbols, name, mappedName);
        }
    } else
        appendSymbol(field, symbolType, symbols, name, mappedName);
}
#else
static void getStructInfo(const sh::ShaderVariable& field, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols, const String& namePrefix, const String& mappedNamePrefix)
{
    String name = namePrefix + '.' + field.name.c_str();
    String mappedName = mappedNamePrefix + '.' + field.mappedName.c_str();

    if (field.isStruct()) {
        for (const auto& subfield : field.fields) {
            // ANGLE restricts the depth of structs, which prevents stack overflow errors in this recursion.
            getStructInfo(subfield, symbolType, symbols, name, mappedName);
        }
    }
    else
        appendSymbol(field, symbolType, symbols, name, mappedName);
}
#endif

#if !PLATFORM(WKC)
static void getSymbolInfo(const sh::ShaderVariable& variable, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols)
{
    if (variable.isStruct()) {
        if (variable.isArray()) {
            for (unsigned i = 0; i < variable.elementCount(); i++) {
                std::string arrayBrackets = "[" + std::to_string(i) + "]";
                std::string arrayName = variable.name + arrayBrackets;
                std::string arrayMappedName = variable.mappedName + arrayBrackets;
                for (const auto& field : variable.fields)
                    getStructInfo(field, symbolType, symbols, arrayName, arrayMappedName);
            }
        } else {
            for (const auto& field : variable.fields)
                getStructInfo(field, symbolType, symbols, variable.name, variable.mappedName);
        }
    } else
        appendSymbol(variable, symbolType, symbols, variable.name, variable.mappedName);
}
#else
static void getSymbolInfo(const sh::ShaderVariable& variable, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols)
{
    if (variable.isStruct()) {
        if (variable.isArray()) {
            for (unsigned i = 0; i < variable.elementCount(); i++) {
                String arrayBrackets = "[" + String::format("%d", i) + "]";
                String arrayName = variable.name.c_str() + arrayBrackets;
                String arrayMappedName = variable.mappedName.c_str() + arrayBrackets;
                for (const auto& field : variable.fields)
                    getStructInfo(field, symbolType, symbols, arrayName, arrayMappedName);
            }
        }
        else {
            for (const auto& field : variable.fields)
                getStructInfo(field, symbolType, symbols, variable.name.c_str(), variable.mappedName.c_str());
        }
    }
    else
        appendSymbol(variable, symbolType, symbols, variable.name.c_str(), variable.mappedName.c_str());
}
#endif

static bool getSymbolInfo(ShHandle compiler, ANGLEShaderSymbolType symbolType, Vector<ANGLEShaderSymbol>& symbols)
{
    switch (symbolType) {
    case SHADER_SYMBOL_TYPE_UNIFORM: {
        auto uniforms = ShGetUniforms(compiler);
        if (!uniforms)
            return false;
        for (const auto& uniform : *uniforms)
            getSymbolInfo(uniform, symbolType, symbols);
        break;
    }
    case SHADER_SYMBOL_TYPE_VARYING: {
        auto varyings = ShGetVaryings(compiler);
        if (!varyings)
            return false;
        for (const auto& varying : *varyings)
            getSymbolInfo(varying, symbolType, symbols);
        break;
    }
    case SHADER_SYMBOL_TYPE_ATTRIBUTE: {
        auto attributes = ShGetAttributes(compiler);
        if (!attributes)
            return false;
        for (const auto& attribute : *attributes)
            getSymbolInfo(attribute, symbolType, symbols);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
    return true;
}

ANGLEWebKitBridge::ANGLEWebKitBridge(ShShaderOutput shaderOutput, ShShaderSpec shaderSpec)
    : builtCompilers(false)
    , m_fragmentCompiler(0)
    , m_vertexCompiler(0)
    , m_shaderOutput(shaderOutput)
    , m_shaderSpec(shaderSpec)
{
    // This is a no-op if it's already initialized.
    ShInitialize();
}

ANGLEWebKitBridge::~ANGLEWebKitBridge()
{
    cleanupCompilers();
}

void ANGLEWebKitBridge::cleanupCompilers()
{
    if (m_fragmentCompiler)
        ShDestruct(m_fragmentCompiler);
    m_fragmentCompiler = nullptr;
    if (m_vertexCompiler)
        ShDestruct(m_vertexCompiler);
    m_vertexCompiler = nullptr;

    builtCompilers = false;
}
    
void ANGLEWebKitBridge::setResources(ShBuiltInResources resources)
{
    // Resources are (possibly) changing - cleanup compilers if we had them already
    cleanupCompilers();
    
    m_resources = resources;
}

bool ANGLEWebKitBridge::compileShaderSource(const char* shaderSource, ANGLEShaderType shaderType, String& translatedShaderSource, String& shaderValidationLog, Vector<ANGLEShaderSymbol>& symbols, int extraCompileOptions)
{
    if (!builtCompilers) {
        m_fragmentCompiler = ShConstructCompiler(GL_FRAGMENT_SHADER, m_shaderSpec, m_shaderOutput, &m_resources);
        m_vertexCompiler = ShConstructCompiler(GL_VERTEX_SHADER, m_shaderSpec, m_shaderOutput, &m_resources);
        if (!m_fragmentCompiler || !m_vertexCompiler) {
            cleanupCompilers();
            return false;
        }

        builtCompilers = true;
    }
    
    ShHandle compiler;

    if (shaderType == SHADER_TYPE_VERTEX)
        compiler = m_vertexCompiler;
    else
        compiler = m_fragmentCompiler;

    const char* const shaderSourceStrings[] = { shaderSource };

    bool validateSuccess = ShCompile(compiler, shaderSourceStrings, 1, SH_OBJECT_CODE | SH_VARIABLES | extraCompileOptions);
    if (!validateSuccess) {
#if !PLATFORM(WKC)
        const std::string& log = ShGetInfoLog(compiler);
#else
        const wkc_angle_string& log = ShGetInfoLog(compiler);
#endif
        if (log.length())
            shaderValidationLog = log.c_str();
        return false;
    }

#if !PLATFORM(WKC)
    const std::string& objectCode = ShGetObjectCode(compiler);
#else
    const wkc_angle_string& objectCode = ShGetObjectCode(compiler);
#endif
    if (objectCode.length())
        translatedShaderSource = objectCode.c_str();
    
    if (!getSymbolInfo(compiler, SHADER_SYMBOL_TYPE_ATTRIBUTE, symbols))
        return false;
    if (!getSymbolInfo(compiler, SHADER_SYMBOL_TYPE_UNIFORM, symbols))
        return false;
    if (!getSymbolInfo(compiler, SHADER_SYMBOL_TYPE_VARYING, symbols))
        return false;

    return true;
}

}

#endif // ENABLE(GRAPHICS_CONTEXT_3D)
