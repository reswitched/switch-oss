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

#ifndef WebGL2RenderingContext_h
#define WebGL2RenderingContext_h

#if ENABLE(WEBGL2)

#include "WebGLRenderingContextBase.h"

namespace WebCore {

class WebGLQuery;
class WebGLSampler;
class WebGLSync;
class WebGLTransformFeedback;
class WebGLVertexArrayObject;

class WebGL2RenderingContext final : public WebGLRenderingContextBase {
public:
    WebGL2RenderingContext(HTMLCanvasElement*, GraphicsContext3D::Attributes);
    WebGL2RenderingContext(HTMLCanvasElement*, PassRefPtr<GraphicsContext3D>, GraphicsContext3D::Attributes);
    virtual bool isWebGL2() const override { return true; }

    /* Buffer objects */
    void copyBufferSubData(GC3Denum readTarget, GC3Denum writeTarget, GC3Dint64 readOffset, GC3Dint64 writeOffset, GC3Dint64 size);
    void getBufferSubData(GC3Denum target, GC3Dint64 offset, ArrayBufferView* returnedData);
    void getBufferSubData(GC3Denum target, GC3Dint64 offset, ArrayBuffer* returnedData);
    
    /* Framebuffer objects */
    virtual WebGLGetInfo getFramebufferAttachmentParameter(GC3Denum target, GC3Denum attachment, GC3Denum pname, ExceptionCode&) override;
    void blitFramebuffer(GC3Dint srcX0, GC3Dint srcY0, GC3Dint srcX1, GC3Dint srcY1, GC3Dint dstX0, GC3Dint dstY0, GC3Dint dstX1, GC3Dint dstY1, GC3Dbitfield mask, GC3Denum filter);
    void framebufferTextureLayer(GC3Denum target, GC3Denum attachment, GC3Duint texture, GC3Dint level, GC3Dint layer);
    WebGLGetInfo getInternalformatParameter(GC3Denum target, GC3Denum internalformat, GC3Denum pname);
    void invalidateFramebuffer(GC3Denum target, Vector<GC3Denum> attachments);
    void invalidateSubFramebuffer(GC3Denum target, Vector<GC3Denum> attachments, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    void readBuffer(GC3Denum src);
    
    /* Renderbuffer objects */
    void renderbufferStorageMultisample(GC3Denum target, GC3Dsizei samples, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height);
    
    /* Texture objects */
    void texStorage2D(GC3Denum target, GC3Dsizei levels, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height);
    void texStorage3D(GC3Denum target, GC3Dsizei levels, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dsizei depth);
    void texImage3D(GC3Denum target, GC3Dint level, GC3Dint internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dsizei depth, GC3Dint border, GC3Denum format, GC3Denum type, ArrayBufferView* pixels);
    void texSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Dsizei width, GC3Dsizei height, GC3Dsizei depth, GC3Denum format, GC3Denum type, ArrayBufferView* pixels);
    void texSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Denum format, GC3Denum type, ImageData* source);
    void texSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Denum format, GC3Denum type, HTMLImageElement* source);
    void texSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Denum format, GC3Denum type, HTMLCanvasElement* source);
    void texSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Denum format, GC3Denum type, HTMLVideoElement* source);
    void copyTexSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    void compressedTexImage3D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dsizei depth, GC3Dint border, GC3Dsizei imageSize, ArrayBufferView* data);
    void compressedTexSubImage3D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint zoffset, GC3Dsizei width, GC3Dsizei height, GC3Dsizei depth, GC3Denum format, GC3Dsizei imageSize, ArrayBufferView* data);
    
    /* Programs and shaders */
    GC3Dint getFragDataLocation(WebGLProgram* program, String name);
    
    /* Uniforms and attributes */
    void uniform1ui(WebGLUniformLocation* location, GC3Duint v0);
    void uniform2ui(WebGLUniformLocation* location, GC3Duint v0, GC3Duint v1);
    void uniform3ui(WebGLUniformLocation* location, GC3Duint v0, GC3Duint v1, GC3Duint v2);
    void uniform4ui(WebGLUniformLocation* location, GC3Duint v0, GC3Duint v1, GC3Duint v2, GC3Duint v3);
    void uniform1uiv(WebGLUniformLocation* location, Uint32Array* value);
    void uniform2uiv(WebGLUniformLocation* location, Uint32Array* value);
    void uniform3uiv(WebGLUniformLocation* location, Uint32Array* value);
    void uniform4uiv(WebGLUniformLocation* location, Uint32Array* value);
    void uniformMatrix2x3fv(WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value);
    void uniformMatrix3x2fv(WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value);
    void uniformMatrix2x4fv(WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value);
    void uniformMatrix4x2fv(WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value);
    void uniformMatrix3x4fv(WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value);
    void uniformMatrix4x3fv(WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value);
    void vertexAttribI4i(GC3Duint index, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w);
    void vertexAttribI4iv(GC3Duint index, Int32Array* v);
    void vertexAttribI4ui(GC3Duint index, GC3Duint x, GC3Duint y, GC3Duint z, GC3Duint w);
    void vertexAttribI4uiv(GC3Duint index, Uint32Array* v);
    void vertexAttribIPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dsizei stride, GC3Dint64 offset);
    
    /* Writing to the drawing buffer */
    virtual void clear(GC3Dbitfield mask) override;
    void vertexAttribDivisor(GC3Duint index, GC3Duint divisor);
    void drawArraysInstanced(GC3Denum mode, GC3Dint first, GC3Dsizei count, GC3Dsizei instanceCount);
    void drawElementsInstanced(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dint64 offset, GC3Dsizei instanceCount);
    void drawRangeElements(GC3Denum mode, GC3Duint start, GC3Duint end, GC3Dsizei count, GC3Denum type, GC3Dint64 offset);
    
    /* Multiple Render Targets */
    void drawBuffers(Vector<GC3Denum> buffers);
    void clearBufferiv(GC3Denum buffer, GC3Dint drawbuffer, Int32Array* value);
    void clearBufferuiv(GC3Denum buffer, GC3Dint drawbuffer, Uint32Array* value);
    void clearBufferfv(GC3Denum buffer, GC3Dint drawbuffer, Float32Array* value);
    void clearBufferfi(GC3Denum buffer, GC3Dint drawbuffer, GC3Dfloat depth, GC3Dint stencil);
    
    /* Query Objects */
    PassRefPtr<WebGLQuery> createQuery();
    void deleteQuery(WebGLQuery* query);
    GC3Dboolean isQuery(WebGLQuery* query);
    void beginQuery(GC3Denum target, WebGLQuery* query);
    void endQuery(GC3Denum target);
    PassRefPtr<WebGLQuery> getQuery(GC3Denum target, GC3Denum pname);
    WebGLGetInfo getQueryParameter(WebGLQuery* query, GC3Denum pname);
    
    /* Sampler Objects */
    PassRefPtr<WebGLSampler> createSampler();
    void deleteSampler(WebGLSampler* sampler);
    GC3Dboolean isSampler(WebGLSampler* sampler);
    void bindSampler(GC3Duint unit, WebGLSampler* sampler);
    void samplerParameteri(WebGLSampler* sampler, GC3Denum pname, GC3Dint param);
    void samplerParameterf(WebGLSampler* sampler, GC3Denum pname, GC3Dfloat param);
    WebGLGetInfo getSamplerParameter(WebGLSampler* sampler, GC3Denum pname);
    
    /* Sync objects */
    PassRefPtr<WebGLSync> fenceSync(GC3Denum condition, GC3Dbitfield flags);
    GC3Dboolean isSync(WebGLSync* sync);
    void deleteSync(WebGLSync* sync);
    GC3Denum clientWaitSync(WebGLSync* sync, GC3Dbitfield flags, GC3Duint64 timeout);
    void waitSync(WebGLSync* sync, GC3Dbitfield flags, GC3Duint64 timeout);
    WebGLGetInfo getSyncParameter(WebGLSync* sync, GC3Denum pname);
    
    /* Transform Feedback */
    PassRefPtr<WebGLTransformFeedback> createTransformFeedback();
    void deleteTransformFeedback(WebGLTransformFeedback* id);
    GC3Dboolean isTransformFeedback(WebGLTransformFeedback* id);
    void bindTransformFeedback(GC3Denum target, WebGLTransformFeedback* id);
    void beginTransformFeedback(GC3Denum primitiveMode);
    void endTransformFeedback();
    void transformFeedbackVaryings(WebGLProgram* program, Vector<String> varyings, GC3Denum bufferMode);
    PassRefPtr<WebGLActiveInfo> getTransformFeedbackVarying(WebGLProgram* program, GC3Duint index);
    void pauseTransformFeedback();
    void resumeTransformFeedback();
    
    /* Uniform Buffer Objects and Transform Feedback Buffers */
    void bindBufferBase(GC3Denum target, GC3Duint index, WebGLBuffer* buffer);
    void bindBufferRange(GC3Denum target, GC3Duint index, WebGLBuffer* buffer, GC3Dint64 offset, GC3Dint64 size);
    WebGLGetInfo getIndexedParameter(GC3Denum target, GC3Duint index);
    Uint32Array* getUniformIndices(WebGLProgram* program, Vector<String> uniformNames);
    Int32Array* getActiveUniforms(WebGLProgram* program, Uint32Array* uniformIndices, GC3Denum pname);
    GC3Duint getUniformBlockIndex(WebGLProgram* program, String uniformBlockName);
    WebGLGetInfo getActiveUniformBlockParameter(WebGLProgram* program, GC3Duint uniformBlockIndex, GC3Denum pname);
    WebGLGetInfo getActiveUniformBlockName(WebGLProgram* program, GC3Duint uniformBlockIndex);
    void uniformBlockBinding(WebGLProgram* program, GC3Duint uniformBlockIndex, GC3Duint uniformBlockBinding);
    
    /* Vertex Array Objects */
    PassRefPtr<WebGLVertexArrayObject> createVertexArray();
    void deleteVertexArray(WebGLVertexArrayObject* vertexArray);
    GC3Dboolean isVertexArray(WebGLVertexArrayObject* vertexArray);
    void bindVertexArray(WebGLVertexArrayObject* vertexArray);
    
    /* Extensions */
    virtual WebGLExtension* getExtension(const String&) override;
    virtual Vector<String> getSupportedExtensions() override;
    virtual WebGLGetInfo getParameter(GC3Denum pname, ExceptionCode&) override;

    virtual void renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height) override;
    virtual void hint(GC3Denum target, GC3Denum mode) override;
    virtual void copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border) override;
    virtual void texSubImage2DBase(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum internalformat, GC3Denum format, GC3Denum type, const void* pixels, ExceptionCode&) override;
    virtual void texSubImage2DImpl(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Denum format, GC3Denum type, Image*, GraphicsContext3D::ImageHtmlDomSource, bool flipY, bool premultiplyAlpha, ExceptionCode&) override;
    virtual void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
        GC3Dsizei width, GC3Dsizei height,
        GC3Denum format, GC3Denum type, ArrayBufferView*, ExceptionCode&) override;
    virtual void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
        GC3Denum format, GC3Denum type, ImageData*, ExceptionCode&) override;
    virtual void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
        GC3Denum format, GC3Denum type, HTMLImageElement*, ExceptionCode&) override;
    virtual void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
        GC3Denum format, GC3Denum type, HTMLCanvasElement*, ExceptionCode&) override;
#if ENABLE(VIDEO)
    virtual void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
        GC3Denum format, GC3Denum type, HTMLVideoElement*, ExceptionCode&) override;
#endif

protected:
    virtual void initializeVertexArrayObjects() override;
    virtual GC3Dint getMaxDrawBuffers() override;
    virtual GC3Dint getMaxColorAttachments() override;
    virtual bool validateIndexArrayConservative(GC3Denum type, unsigned& numElementsRequired) override;
    virtual bool validateBlendEquation(const char* functionName, GC3Denum mode) override;
    virtual bool validateTexFuncFormatAndType(const char* functionName, GC3Denum internalformat, GC3Denum format, GC3Denum type, GC3Dint level) override;
    virtual bool validateTexFuncParameters(const char* functionName,
        TexFuncValidationFunctionType,
        GC3Denum target, GC3Dint level,
        GC3Denum internalformat,
        GC3Dsizei width, GC3Dsizei height, GC3Dint border,
        GC3Denum format, GC3Denum type) override;
    virtual bool validateTexFuncData(const char* functionName, GC3Dint level,
        GC3Dsizei width, GC3Dsizei height,
        GC3Denum internalformat, GC3Denum format, GC3Denum type,
        ArrayBufferView* pixels,
        NullDisposition) override;
    virtual bool validateCapability(const char* functionName, GC3Denum cap) override;
    virtual bool validateFramebufferFuncParameters(const char* functionName, GC3Denum target, GC3Denum attachment) override;
    
private:
    GC3Denum baseInternalFormatFromInternalFormat(GC3Denum internalformat);
    bool isIntegerFormat(GC3Denum internalformat);
    void initializeShaderExtensions();
};

} // namespace WebCore

#endif // WEBGL2

#endif
