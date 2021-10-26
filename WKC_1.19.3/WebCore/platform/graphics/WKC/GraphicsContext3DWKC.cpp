/*
 * Copyright (C) 2010, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 ChangSeok Oh <shivamidow@gmail.com>
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 * Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(WEBGL)

#include "GraphicsContext3D.h"

#include "BitmapImage.h"
#include "CanvasRenderingContext.h"
#include "CString.h"
#include "Extensions3D.h"
#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#else
#include "Extensions3DOpenGL.h"
#endif
#include "ANGLEWebKitBridge.h"
#include "FastMalloc.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "ImageWKC.h"
#include "Logging.h"
#include "WebGLRenderingContext.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcglpeer.h>
#include <wkc/wkcgpeer.h>

#include "NotImplemented.h"

namespace WebCore {

class Extensions3DWKC : public Extensions3D
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    Extensions3DWKC(GraphicsContext3D* context, void* peer)
        : m_context(context)
        , m_peer(peer)
        , m_contextResetStatus(0)
        , m_contextLostProc()
        , m_requiresBuiltInFunctionEmulation(true)
    {}

    ~Extensions3DWKC() {}

    void setContextLostCallback(std::unique_ptr<GraphicsContext3D::ContextLostCallback>& cb)
    {
        m_contextLostProc = WTF::move(cb);
    }

    virtual bool supports(const String&);
    virtual void ensureEnabled(const String&);
    virtual bool isEnabled(const String&);
    virtual void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter);
    virtual void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height);
    virtual Platform3DObject createVertexArrayOES();
    virtual void deleteVertexArrayOES(Platform3DObject);
    virtual GC3Dboolean isVertexArrayOES(Platform3DObject);
    virtual void bindVertexArrayOES(Platform3DObject);
    virtual String getTranslatedShaderSourceANGLE(Platform3DObject);

    virtual int getGraphicsResetStatusARB()
    {
        if (!m_contextResetStatus)
            return m_contextResetStatus;

        wkcGLMakeContextCurrentPeer(m_peer);
        int ret = wkcGLExtGetGraphicsResetStatusARBPeer(m_peer);
        if (!ret) {
            if (m_contextLostProc)
                m_contextLostProc->onContextLost();
            m_contextResetStatus = ret;
        }
        return ret;
    }
    virtual void readnPixelsEXT(int x, int y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, GC3Dsizei bufSize, void *data)
    {
        wkcGLMakeContextCurrentPeer(m_peer);
        wkcGLReadnPixelsPeer(m_peer, x, y, width, height, format, type, bufSize, data);
    }
    virtual void getnUniformfvEXT(GC3Duint program, int location, GC3Dsizei bufSize, float *params)
    {
        wkcGLMakeContextCurrentPeer(m_peer);
        wkcGLGetnUniformfvPeer(m_peer, program, location, bufSize, params);
    }
    virtual void getnUniformivEXT(GC3Duint program, int location, GC3Dsizei bufSize, int *params)
    {
        wkcGLMakeContextCurrentPeer(m_peer);
        wkcGLGetnUniformivPeer(m_peer, program, location, bufSize, params);
    }

    virtual void insertEventMarkerEXT(const String&)
    {
        notImplemented();
    }
    virtual void pushGroupMarkerEXT(const String&)
    {
        notImplemented();
    }
    virtual void popGroupMarkerEXT(void)
    {
        notImplemented();
    }

    virtual void drawBuffersEXT(GC3Dsizei n, const GC3Denum* bufs)
    {
        wkcGLMakeContextCurrentPeer(m_peer);
        wkcGLDrawBuffersPeer(m_peer, n, bufs);
    }

    virtual void drawArraysInstanced(GC3Denum mode, GC3Dint first, GC3Dsizei count, GC3Dsizei primcount)
    {
        notImplemented();
    }
    virtual void drawElementsInstanced(GC3Denum mode, GC3Dsizei count, GC3Denum type, long long offset, GC3Dsizei primcount)
    {
        notImplemented();
    }
    virtual void vertexAttribDivisor(GC3Duint index, GC3Duint divisor)
    {
        notImplemented();
    }

    virtual String vendor()
    {
        const char* vendor = (const char *)wkcGLGetStringPeer(m_peer, GraphicsContext3D::VENDOR);
        if (!vendor)
            return String("ACCESS");
        return String(vendor);
    }
    virtual bool isNVIDIA()
    {
        return (vendor().findIgnoringCase("NVIDIA")!=WTF::notFound);
    }
    virtual bool isAMD()
    {
        return (vendor().findIgnoringCase("AMD")!=WTF::notFound) || (vendor().findIgnoringCase("ATI")!=WTF::notFound);
    }
    virtual bool isIntel()
    {
        return (vendor().findIgnoringCase("Intel")!=WTF::notFound);
    }
    virtual bool isImagination()
    { 
        return (vendor().findIgnoringCase("Imagination")!=WTF::notFound);
    }

    virtual bool maySupportMultisampling() { return false; }

    virtual bool requiresBuiltInFunctionEmulation() { return true; }
    virtual bool requiresRestrictedMaximumTextureSize() { return true; }

private:
    GraphicsContext3D* m_context;
    void* m_peer;
    int m_contextResetStatus;
    std::unique_ptr<GraphicsContext3D::ContextLostCallback> m_contextLostProc;
    bool m_requiresBuiltInFunctionEmulation;
};

class GraphicsContext3DPrivate
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static std::unique_ptr<GraphicsContext3DPrivate> create(GraphicsContext3D* parent, GraphicsContext3DAttributes& attrs, HostWindow* hostwindow, bool in_direct);
    ~GraphicsContext3DPrivate()
    {
        if (m_data)
            fastFree(m_data);
        if (m_layer)
            wkcLayerDeletePeer(m_layer);
    }

    PlatformGraphicsContext3D platformGraphicsContext3D();
    inline void* peer() { return m_peer; }

    void setContextLostCallback(std::unique_ptr<GraphicsContext3D::ContextLostCallback>& cb) { m_extensions->setContextLostCallback(cb); }
    GraphicsContext3D::ErrorMessageCallback* errorMessageCallback() const { return m_errorMessageCallback.get(); }
    void setErrorMessageCallback(std::unique_ptr<GraphicsContext3D::ErrorMessageCallback>& cb) { m_errorMessageCallback.swap(cb); }

    inline GraphicsContext3DAttributes& attrs() { return m_attrs; }
    inline Extensions3D* extensions() const { return m_extensions; }
    inline void* layer() const { return m_layer; }

    void reshape(int width, int height);

private:
    GraphicsContext3DPrivate()
        : m_peer(0)
        , m_texture(0)
        , m_errorMessageCallback()
        , m_attrs()
        , m_extensions(0)
        , m_data(0)
        , m_width(0)
        , m_height(0)
    {
        m_layer = wkcLayerNewPeer(WKC_LAYER_TYPE_3DCANVAS, 0, 1.f);
    }

private:
    void* m_peer;
    Platform3DObject m_texture;
    std::unique_ptr<GraphicsContext3D::ErrorMessageCallback> m_errorMessageCallback;
    GraphicsContext3DAttributes m_attrs;
    Extensions3DWKC* m_extensions;
    void* m_data;
    int m_width;
    int m_height;
    void* m_layer;
};

std::unique_ptr<GraphicsContext3DPrivate>
GraphicsContext3DPrivate::create(GraphicsContext3D* parent, GraphicsContext3DAttributes& attrs, HostWindow* hostwindow, bool in_direct)
{
    WKCGLAttributes ga = {
        attrs.alpha,
        attrs.depth,
        attrs.stencil,
        attrs.antialias,
        attrs.premultipliedAlpha,
        attrs.preserveDrawingBuffer,
        attrs.noExtensions,
        attrs.shareResources,
        attrs.preferLowPowerToHighPerformance,
        attrs.forceSoftwareRenderer,
        attrs.failIfMajorPerformanceCaveat,
        attrs.useGLES3,
        attrs.devicePixelRatio,
    };

    GraphicsContext3DPrivate* v = new GraphicsContext3DPrivate();

    std::unique_ptr<GraphicsContext3DPrivate> self;
    self.reset(v);

    self->m_peer = wkcGLCreateContextPeer(&ga, (void *)hostwindow, self->m_layer);
    if (!self->m_peer) {
        return nullptr;
    }
    self->m_extensions = new Extensions3DWKC(parent, self->m_peer);
    self->m_attrs = attrs;

    attrs.depth = ga.fDepth;
    attrs.stencil = ga.fStencil;
    attrs.antialias = ga.fAntialias;

    return self;
}

void
GraphicsContext3DPrivate::reshape(int width, int height)
{
    if (width==m_width && height==m_height)
        return;


    if (m_data) {
        WTF::fastFree(m_data);
        m_data = 0;
        m_width = m_height = 0;
    }

    m_width = width;
    m_height = height;
    int len = 4*width*height;
    WTF::TryMallocReturnValue ret = WTF::tryFastMalloc(len);
    if (!ret.getValue(m_data)) {
        wkcMemoryNotifyMemoryAllocationErrorPeer(len, WKC_MEMORYALLOC_TYPE_IMAGE);
    }
}

#define peer() (m_private->peer())

GraphicsContext3D::GraphicsContext3D(Attributes attrs, HostWindow* hostwindow, RenderStyle)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_compiler()
{
}

GraphicsContext3D::~GraphicsContext3D()
{
    wkcGLMakeContextCurrentPeer(0);
    if (m_private && peer()) {
        wkcGLDeleteContextPeer(peer());
    }
}

RefPtr<GraphicsContext3D>
GraphicsContext3D::create(Attributes attrs, HostWindow* hostwindow, RenderStyle style)
{
    if (style == RenderDirectlyToHostWindow)
        return 0;

    RefPtr<GraphicsContext3D> self = adoptRef(new GraphicsContext3D(attrs, hostwindow));

    std::unique_ptr<GraphicsContext3DPrivate> ctx;
    ctx = GraphicsContext3DPrivate::create(self.get(), attrs, hostwindow, false);
    if (!ctx)
        return 0;

    self->m_private = WTF::move(ctx);

    self->m_compiler.setShaderOutput(self->isGLES2Compliant() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT);

    // ANGLE initialization.
    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    self->getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    self->getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    self->getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    self->getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    self->getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    self->getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    self->getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;

    GC3Dint range[2], precision;
    self->getShaderPrecisionFormat(GraphicsContext3D::FRAGMENT_SHADER, GraphicsContext3D::HIGH_FLOAT, range, &precision);
    ANGLEResources.FragmentPrecisionHigh = (range[0] || range[1] || precision);

    self->m_compiler.setResources(ANGLEResources);

    return self.release();
}

void
GraphicsContext3D::setContextLostCallback(std::unique_ptr<ContextLostCallback> cb)
{
    m_private->setContextLostCallback(cb);
}

void
GraphicsContext3D::setErrorMessageCallback(std::unique_ptr<ErrorMessageCallback> cb)
{
    m_private->setErrorMessageCallback(cb);
}

bool
GraphicsContext3D::makeContextCurrent()
{
    wkcGLMakeContextCurrentPeer(peer());
    return true;
}

bool
GraphicsContext3D::isGLES2Compliant() const
{
    return wkcGLIsGLES20CompliantPeer(peer());
}

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
}

bool
GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    if (!m_image)
        return false;

    NativeImagePtr img = 0;
    unsigned int srcUnpackAlignment = 1;
    size_t bytesPerRow = 0;
    int bpp = 0;
    size_t bitsPerPixel = 32;
    int fmt = 0;
    unsigned padding = 0;

    // We need this to stay in scope because the native image is just a shallow copy of the data.
    ImageSource* decoder = new ImageSource(premultiplyAlpha ? ImageSource::AlphaPremultiplied : ImageSource::AlphaNotPremultiplied, ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied);
    if (!decoder)
        return false;

    m_alphaOp = AlphaDoNothing;
    if (m_image->data()) {
        decoder->setData(m_image->data(), true);
        if (!decoder->frameCount() || !decoder->frameIsCompleteAtIndex(0))
            goto error_end;
        img = decoder->createFrameAtIndex(0);
    } else {
        img = m_image->nativeImageForCurrentFrame();
        // 1. For texImage2D with HTMLVideoElment input, assume no PremultiplyAlpha had been applied and the alpha value is 0xFF for each pixel,
        // which is true at present and may be changed in the future and needs adjustment accordingly.
        // 2. For texImage2D with HTMLCanvasElement input in which Alpha is already Premultiplied in this port, 
        // do AlphaDoUnmultiply if UNPACK_PREMULTIPLY_ALPHA_WEBGL is set to false.
        if (!premultiplyAlpha && m_imageHtmlDomSource != HtmlDomVideo)
            m_alphaOp = AlphaDoUnmultiply;
    }

    if (!img)
        goto error_end;

    m_imageWidth = img->size().width();
    m_imageHeight = img->size().height();
    if (!m_imageWidth || !m_imageHeight)
        goto error_end;

    srcUnpackAlignment = 1;
    bytesPerRow = img->rowbytes();
    bpp = img->bpp();
    bitsPerPixel = 32;
    fmt = 0;
    if (bpp==4) {
        bitsPerPixel = 32;
        fmt = DataFormatBGRA8;
    } else if (bpp==2) {
        bitsPerPixel = 16;
        fmt = DataFormatRGB565;
    } else
        goto error_end;

    padding = bytesPerRow - bitsPerPixel / 8 * m_imageWidth;
    if (padding) {
        srcUnpackAlignment = padding + 1;
        while (bytesPerRow % srcUnpackAlignment)
            ++srcUnpackAlignment;
    }

    m_imagePixelData = img->bitmap();
    m_imageSourceFormat = (DataFormat)fmt;
    m_imageSourceUnpackAlignment = srcUnpackAlignment;

    delete decoder;
    return true;

error_end:
    if (decoder)
        delete decoder;
    return false;
}

// WebGL entry points

void
GraphicsContext3D::activeTexture(GC3Denum texture)
{
    makeContextCurrent();
    wkcGLActiveTexturePeer(peer(), texture);
}


void
GraphicsContext3D::attachShader(Platform3DObject program, Platform3DObject shader)
{
    ASSERT(program);
    ASSERT(shader);
    makeContextCurrent();
    wkcGLAttachShaderPeer(peer(), program, shader);
    m_shaderProgramSymbolCountMap.remove(program);
}


void
GraphicsContext3D::bindAttribLocation(Platform3DObject program, GC3Duint index, const String& name)
{
    ASSERT(program);
    makeContextCurrent();
    String mappedName = mappedSymbolName(program, SHADER_SYMBOL_TYPE_ATTRIBUTE, name);
    wkcGLBindAttribLocationPeer(peer(), program, index, mappedName.utf8().data());
}


void
GraphicsContext3D::bindBuffer(GC3Denum target, Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLBindBufferPeer(peer(), target, obj);
}


void
GraphicsContext3D::bindFramebuffer(GC3Denum target, Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLBindFramebufferPeer(peer(), target, obj);
}


void
GraphicsContext3D::bindRenderbuffer(GC3Denum target, Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLBindRenderbufferPeer(peer(), target, obj);
}


void
GraphicsContext3D::bindTexture(GC3Denum target, Platform3DObject obj)
{
    makeContextCurrent();    
    wkcGLBindTexturePeer(peer(), target, obj);
}


void
GraphicsContext3D::blendColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    makeContextCurrent();
    wkcGLBlendColorPeer(peer(), red, green, blue, alpha);
}


void
GraphicsContext3D::blendEquation(GC3Denum mode)
{
    makeContextCurrent();
    wkcGLBlendEquationPeer(peer(), mode);
}


void
GraphicsContext3D::blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha)
{
    makeContextCurrent();
    wkcGLBlendEquationSeparatePeer(peer(), modeRGB, modeAlpha);
}


void
GraphicsContext3D::blendFunc(GC3Denum sfactor, GC3Denum dfactor)
{
    makeContextCurrent();
    wkcGLBlendFuncPeer(peer(), sfactor, dfactor);
}


void
GraphicsContext3D::blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha)
{
    makeContextCurrent();
    wkcGLBlendFuncSeparatePeer(peer(), srcRGB, dstRGB, srcAlpha, dstAlpha);
}


void
GraphicsContext3D::bufferData(GC3Denum target, GC3Dsizeiptr size, GC3Denum usage)
{
    makeContextCurrent();
    wkcGLBufferDataPeer(peer(), target, size, 0, usage);
}


void
GraphicsContext3D::bufferData(GC3Denum target, GC3Dsizeiptr size, const void* data, GC3Denum usage)
{
    makeContextCurrent();
    wkcGLBufferDataPeer(peer(), target, size, data, usage);
}


void
GraphicsContext3D::bufferSubData(GC3Denum target, GC3Dintptr offset, GC3Dsizeiptr size, const void* data)
{
    makeContextCurrent();
    wkcGLBufferSubDataPeer(peer(), target, offset, size, data);
}



GC3Denum
GraphicsContext3D::checkFramebufferStatus(GC3Denum target)
{
    makeContextCurrent();
    return wkcGLCheckFramebufferStatusPeer(peer(), target);
}


void
GraphicsContext3D::clear(GC3Dbitfield mask)
{
    makeContextCurrent();
    wkcGLClearPeer(peer(), mask);
}


void
GraphicsContext3D::clearColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    makeContextCurrent();
    wkcGLClearColorPeer(peer(), red, green, blue, alpha);
}


void
GraphicsContext3D::clearDepth(GC3Dclampf depth)
{
    makeContextCurrent();
    wkcGLClearDepthPeer(peer(), depth);
}


void
GraphicsContext3D::clearStencil(GC3Dint s)
{
    makeContextCurrent();
    wkcGLClearStencilPeer(peer(), s);
}


void
GraphicsContext3D::colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha)
{
    makeContextCurrent();
    wkcGLColorMaskPeer(peer(), red, green, blue, alpha);
}

WKC_DEFINE_GLOBAL_TYPE(ShaderNameHash*, currentNameHashMapForShader, 0);

static uint64_t nameHashForShader(const char* name, size_t length)
{
    if (!length)
        return 0;

    CString nameAsCString = CString(name);

    // Look up name in our local map.
    if (currentNameHashMapForShader) {
        ShaderNameHash::iterator result = currentNameHashMapForShader->find(nameAsCString);
        if (result != currentNameHashMapForShader->end())
            return result->value;
    }

    unsigned hashValue = nameAsCString.hash();

    // Convert the 32-bit hash from CString::hash into a 64-bit result
    // by shifting then adding the size of our table. Overflow would
    // only be a problem if we're already hashing to the same value (and
    // we're hoping that over the lifetime of the context we
    // don't have that many symbols).

    uint64_t result = hashValue;
    result = (result << 32) + (currentNameHashMapForShader->size() + 1);

    currentNameHashMapForShader->set(nameAsCString, result);
    return result;
}

void
GraphicsContext3D::compileShader(Platform3DObject shader)
{
    ASSERT(shader);

    ShBuiltInResources ANGLEResources = m_compiler.getResources();
    ShHashFunction64 previousHashFunction = ANGLEResources.HashFunction;
    ANGLEResources.HashFunction = nameHashForShader;

    if (!nameHashMapForShaders)
        nameHashMapForShaders = std::make_unique<ShaderNameHash>();
    currentNameHashMapForShader = nameHashMapForShaders.get();
    m_compiler.setResources(ANGLEResources);

    String translatedShaderSource = m_private->extensions()->getTranslatedShaderSourceANGLE(shader);

    ANGLEResources.HashFunction = previousHashFunction;
    m_compiler.setResources(ANGLEResources);
    currentNameHashMapForShader = 0;

    if (!translatedShaderSource.length()) {
        LOG(WebGL, "invalid shader");
        synthesizeGLError(INVALID_VALUE);
        return;
    }

    makeContextCurrent();

    wkcGLShaderSourcePeer(peer(), shader, translatedShaderSource.utf8().data());
    wkcGLCompileShaderPeer(peer(), shader);
}

void
GraphicsContext3D::compressedTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Dsizei imageSize, const void* data)
{
    makeContextCurrent();
    wkcGLCompressedTexImage2DPeer(peer(), target, level, internalformat, width, height, border, imageSize, data);
}

void
GraphicsContext3D::compressedTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Dsizei imageSize, const void* data)
{
    makeContextCurrent();
    wkcGLCompressedTexSubImage2DPeer(peer(), target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void
GraphicsContext3D::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    makeContextCurrent();
    wkcGLCopyTexImage2DPeer(peer(), target, level, internalformat, x, y, width, height, border);
}


void
GraphicsContext3D::copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    wkcGLCopyTexSubImage2DPeer(peer(), target, level, xoffset, yoffset, x, y, width, height);
}


void
GraphicsContext3D::cullFace(GC3Denum mode)
{
    makeContextCurrent();
    wkcGLCullFacePeer(peer(), mode);
}


void
GraphicsContext3D::depthFunc(GC3Denum func)
{
    makeContextCurrent();
    wkcGLDepthFuncPeer(peer(), func);
}


void
GraphicsContext3D::depthMask(GC3Dboolean flag)
{
    makeContextCurrent();
    wkcGLDepthMaskPeer(peer(), flag);
}


void
GraphicsContext3D::depthRange(GC3Dclampf zNear, GC3Dclampf zFar)
{
    makeContextCurrent();
    wkcGLDepthRangePeer(peer(), zNear, zFar);
}


void
GraphicsContext3D::detachShader(Platform3DObject program, Platform3DObject shader)
{
    ASSERT(program);
    ASSERT(shader);
    makeContextCurrent();
    wkcGLDetachShaderPeer(peer(), program, shader);
    m_shaderProgramSymbolCountMap.remove(program);
}


void
GraphicsContext3D::disable(GC3Denum cap)
{
    makeContextCurrent();
    wkcGLDisablePeer(peer(), cap);
}


void
GraphicsContext3D::disableVertexAttribArray(GC3Duint index)
{
    makeContextCurrent();
    wkcGLDisableVertexAttribArrayPeer(peer(), index);
}


void
GraphicsContext3D::drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count)
{
    makeContextCurrent();
    wkcGLDrawArraysPeer(peer(), mode, first, count);
}


void
GraphicsContext3D::drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset)
{
    makeContextCurrent();
    wkcGLDrawElementsPeer(peer(), mode, count, type, offset);
}


void
GraphicsContext3D::enable(GC3Denum cap)
{
    makeContextCurrent();
    wkcGLEnablePeer(peer(), cap);
}


void
GraphicsContext3D::enableVertexAttribArray(GC3Duint index)
{
    makeContextCurrent();
    wkcGLEnableVertexAttribArrayPeer(peer(), index);
}


void
GraphicsContext3D::finish()
{
    makeContextCurrent();
    wkcGLFinishPeer(peer());
}


void
GraphicsContext3D::flush()
{
    makeContextCurrent();
    wkcGLFlushPeer(peer());
}


void
GraphicsContext3D::framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLFramebufferRenderbufferPeer(peer(), target, attachment, renderbuffertarget, obj);
}


void
GraphicsContext3D::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject obj, GC3Dint level)
{
    makeContextCurrent();
    wkcGLFramebufferTexture2DPeer(peer(), target, attachment, textarget, obj, level);
}


void
GraphicsContext3D::frontFace(GC3Denum mode)
{
    makeContextCurrent();
    wkcGLFrontFacePeer(peer(), mode);
}


void
GraphicsContext3D::generateMipmap(GC3Denum target)
{
    makeContextCurrent();
    wkcGLGenerateMipmapPeer(peer(), target);
}

bool
GraphicsContext3D::getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    GC3Dint symbolCount;
    ShaderProgramSymbolCountMap::iterator result = m_shaderProgramSymbolCountMap.find(program);
    if (result == m_shaderProgramSymbolCountMap.end()) {
        getNonBuiltInActiveSymbolCount(program, GraphicsContext3D::ACTIVE_ATTRIBUTES, &symbolCount);
        result = m_shaderProgramSymbolCountMap.find(program);
    }
    
    ActiveShaderSymbolCounts& symbolCounts = result->value;
    GC3Duint rawIndex = (index < symbolCounts.filteredToActualAttributeIndexMap.size()) ? symbolCounts.filteredToActualAttributeIndexMap[index] : -1;

    return getActiveAttribImpl(program, rawIndex, info);
}

bool
GraphicsContext3D::getActiveAttribImpl(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    makeContextCurrent();

    GC3Dint len = 0;
    wkcGLGetProgramivPeer(peer(), program, 0x8b8a /*GL_ACTIVE_ATTRIBUTE_MAX_LENGTH*/, &len);
    if (!len)
        return false;

    WTF::TryMallocReturnValue rv = WTF::tryFastMalloc(len);
    char* name = 0;
    GC3Dint nameLength = 0;
    if (!rv.getValue(name))
        return false;

    wkcGLGetActiveAttribPeer(peer(), program, index, len, &nameLength, &info.size, &info.type, name);
    if (!nameLength) {
        fastFree(name);
        return false;
    }

    info.name = originalSymbolName(program, SHADER_SYMBOL_TYPE_ATTRIBUTE, String(name, nameLength));
    fastFree(name);
    return true;
}


bool
GraphicsContext3D::getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    GC3Dint symbolCount;
    ShaderProgramSymbolCountMap::iterator result = m_shaderProgramSymbolCountMap.find(program);
    if (result == m_shaderProgramSymbolCountMap.end()) {
        getNonBuiltInActiveSymbolCount(program, GraphicsContext3D::ACTIVE_UNIFORMS, &symbolCount);
        result = m_shaderProgramSymbolCountMap.find(program);
    }
    
    ActiveShaderSymbolCounts& symbolCounts = result->value;
    GC3Duint rawIndex = (index < symbolCounts.filteredToActualUniformIndexMap.size()) ? symbolCounts.filteredToActualUniformIndexMap[index] : -1;
    
    return getActiveUniformImpl(program, rawIndex, info);
}

bool
GraphicsContext3D::getActiveUniformImpl(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    makeContextCurrent();

    GC3Dint len = 0;
    wkcGLGetProgramivPeer(peer(), program, 0x8b87 /*GL_ACTIVE_UNIFORM_MAX_LENGTH*/, &len);
    if (!len)
        return false;

    WTF::TryMallocReturnValue rv = WTF::tryFastMalloc(len);
    char* name = 0;
    GC3Dint nameLength = 0;
    if (!rv.getValue(name))
        return false;

    wkcGLGetActiveUniformPeer(peer(), program, index, len, &nameLength, &info.size, &info.type, name);
    if (!nameLength) {
        fastFree(name);
        return false;
    }
    info.name = originalSymbolName(program, SHADER_SYMBOL_TYPE_UNIFORM, String(name, nameLength));
    fastFree(name);
    return true;
}


void
GraphicsContext3D::getAttachedShaders(Platform3DObject program, GC3Dsizei maxCount, GC3Dsizei* count, Platform3DObject* shaders)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return;
    }
    makeContextCurrent();
    wkcGLGetAttachedShadersPeer(peer(), program, maxCount, count, shaders);
}

template<typename T>
static inline void appendUnsigned64AsHex(uint64_t number, T& destination)
{
    const unsigned char hexDigits[] = "0123456789abcdef";
    Vector<LChar, 8> result;
    do {
        result.append(hexDigits[number % 16]);
        number >>= 4;
    } while (number > 0);
    
    result.reverse();
    destination.append(result.data(), result.size());
}


static String generateHashedName(const String& name)
{
    if (name.isEmpty())
        return name;
    uint64_t number = nameHashForShader(name.utf8().data(), name.length());
    StringBuilder builder;
    builder.append("webgl_");
    appendUnsigned64AsHex(number, builder);
    return builder.toString();
}

String GraphicsContext3D::mappedSymbolName(Platform3DObject program, ANGLEShaderSymbolType symbolType, const String& name)
{
    GC3Dsizei count = 0;
    Platform3DObject shaders[2] = { };
    getAttachedShaders(program, 2, &count, shaders);

    for (GC3Dsizei i = 0; i < count; ++i) {
        ShaderSourceMap::iterator result = m_shaderSourceMap.find(shaders[i]);
        if (result == m_shaderSourceMap.end())
            continue;

        const ShaderSymbolMap& symbolMap = result->value.symbolMap(symbolType);
        ShaderSymbolMap::const_iterator symbolEntry = symbolMap.find(name);
        if (symbolEntry != symbolMap.end())
            return symbolEntry->value.mappedName;
    }

    if (symbolType == SHADER_SYMBOL_TYPE_ATTRIBUTE && !name.isEmpty()) {
        // Attributes are a special case: they may be requested before any shaders have been compiled,
        // and aren't even required to be used in any shader program.
        if (!nameHashMapForShaders)
            nameHashMapForShaders = std::make_unique<ShaderNameHash>();
        currentNameHashMapForShader = nameHashMapForShaders.get();

        String generatedName = generateHashedName(name);

        currentNameHashMapForShader = 0;

        m_possiblyUnusedAttributeMap.set(generatedName, name);

        return generatedName;
    }

    return name;
}
    
String GraphicsContext3D::originalSymbolName(Platform3DObject program, ANGLEShaderSymbolType symbolType, const String& name)
{
    GC3Dsizei count;
    Platform3DObject shaders[2];
    getAttachedShaders(program, 2, &count, shaders);
    
    for (GC3Dsizei i = 0; i < count; ++i) {
        ShaderSourceMap::iterator result = m_shaderSourceMap.find(shaders[i]);
        if (result == m_shaderSourceMap.end())
            continue;
        
        const ShaderSymbolMap& symbolMap = result->value.symbolMap(symbolType);
        for (ShaderSymbolMap::const_iterator it = symbolMap.begin(); it!=symbolMap.end(); ++it) {
            const WTF::KeyValuePair<String, SymbolInfo>& symbolEntry = *it;
            if (symbolEntry.value.mappedName == name)
                return symbolEntry.key;
        }
    }

    if (symbolType == SHADER_SYMBOL_TYPE_ATTRIBUTE && !name.isEmpty()) {
        // Attributes are a special case: they may be requested before any shaders have been compiled,
        // and aren't even required to be used in any shader program.

        const HashMap<String, String>::iterator& cached = m_possiblyUnusedAttributeMap.find(name);
        if (cached != m_possiblyUnusedAttributeMap.end())
            return cached->value;
    }

    return name;
}

String GraphicsContext3D::mappedSymbolName(Platform3DObject shaders[2], size_t count, const String& name)
{
    for (size_t symbolType = 0; symbolType <= static_cast<size_t>(SHADER_SYMBOL_TYPE_VARYING); ++symbolType) {
        for (size_t i = 0; i < count; ++i) {
            ShaderSourceMap::iterator result = m_shaderSourceMap.find(shaders[i]);
            if (result == m_shaderSourceMap.end())
                continue;
            
            const ShaderSymbolMap& symbolMap = result->value.symbolMap(static_cast<enum ANGLEShaderSymbolType>(symbolType));
            for (ShaderSymbolMap::const_iterator it=symbolMap.begin(); it!=symbolMap.end(); ++it) {
                const WTF::KeyValuePair<String, SymbolInfo>& symbolEntry = *it;
                if (symbolEntry.value.mappedName == name)
                    return symbolEntry.key;
            }
        }
    }
    return name;
}

GC3Dint
GraphicsContext3D::getAttribLocation(Platform3DObject program, const String& name)
{
    if (!program)
        return -1;
    makeContextCurrent();
    String mappedName = mappedSymbolName(program, SHADER_SYMBOL_TYPE_ATTRIBUTE, name);
    return wkcGLGetAttribLocationPeer(peer(), program, mappedName.utf8().data());
}


void
GraphicsContext3D::getBooleanv(GC3Denum pname, GC3Dboolean* value)
{
    makeContextCurrent();
    wkcGLGetBooleanvPeer(peer(), pname, value);
}


void
GraphicsContext3D::getBufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetBufferParameterivPeer(peer(), target, pname, value);
}


GraphicsContext3DAttributes
GraphicsContext3D::getContextAttributes()
{
    return m_private->attrs();
}


GC3Denum
GraphicsContext3D::getError()
{
    if (!m_syntheticErrors.isEmpty()) {
        moveErrorsToSyntheticErrorList();
        return m_syntheticErrors.takeFirst();
    }
    makeContextCurrent();
    return wkcGLGetErrorPeer(peer());
}


void
GraphicsContext3D::getFloatv(GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLGetFloatvPeer(peer(), pname, value);
}


void
GraphicsContext3D::getFramebufferAttachmentParameteriv(GC3Denum target, GC3Denum attachment, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetFramebufferAttachmentParameterivPeer(peer(), target, attachment, pname, value);
}


void
GraphicsContext3D::getIntegerv(GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetIntegervPeer(peer(), pname, value);
}

void
GraphicsContext3D::getInteger64v(GC3Denum pname, GC3Dint64* value)
{
    // for WebGL 2.0
    notImplemented();
}

void
GraphicsContext3D::getProgramiv(Platform3DObject program, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetProgramivPeer(peer(), program, pname, value);
}


String
GraphicsContext3D::getProgramInfoLog(Platform3DObject program)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return "";
    }
    makeContextCurrent();

    GC3Dint len = 0;
    wkcGLGetProgramivPeer(peer(), program, 0x8b84 /*GL_INFO_LOG_LENGTH*/, &len);
    if (!len)
        return "";

    WTF::TryMallocReturnValue rv = WTF::tryFastMalloc(len);
    char* info = 0;
    if (!rv.getValue(info))
        return "";

    wkcGLGetProgramInfoLogPeer(peer(), program, len, &len, info);
    String ret(info);
    fastFree(info);
    return ret;
}


void
GraphicsContext3D::getRenderbufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetRenderbufferParameterivPeer(peer(), target, pname, value);
}


void
GraphicsContext3D::getShaderiv(Platform3DObject obj, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetShaderivPeer(peer(), obj, pname, value);
}


String
GraphicsContext3D::getShaderInfoLog(Platform3DObject program)
{
    makeContextCurrent();

    GC3Dint len = 0;
    wkcGLGetShaderivPeer(peer(), program, 0x8b84 /*GL_INFO_LOG_LENGTH*/, &len);

    char* name = 0;
    CString cs = CString::newUninitialized(len+1, name);

    wkcGLGetShaderInfoLogPeer(peer(), program, len+1, &len, name);
    return cs.data();
}


String
GraphicsContext3D::getShaderSource(Platform3DObject program)
{
    makeContextCurrent();

    GC3Dint len = 0;
    wkcGLGetShaderivPeer(peer(), program, 0x8b88 /*GL_SHADER_SOURCE_LENGTH*/, &len);

    char* name = 0;
    CString cs = CString::newUninitialized(len+1, name);

    wkcGLGetShaderSourcePeer(peer(), program, len+1, &len, name);
    return cs.data();
}


String
GraphicsContext3D::getString(GC3Denum name)
{
    makeContextCurrent();
    return String(reinterpret_cast<const char *>(wkcGLGetStringPeer(peer(), name)));
}


void
GraphicsContext3D::getTexParameterfv(GC3Denum target, GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLGetTexParameterfvPeer(peer(), target, pname, value);
}


void
GraphicsContext3D::getTexParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetTexParameterivPeer(peer(), target, pname, value);
}


void
GraphicsContext3D::getUniformfv(Platform3DObject program, GC3Dint location, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLGetUniformfvPeer(peer(), program, location, value);
}


void
GraphicsContext3D::getUniformiv(Platform3DObject program, GC3Dint location, GC3Dint* value)
{
    wkcGLGetUniformivPeer(peer(), program, location, value);
}


GC3Dint
GraphicsContext3D::getUniformLocation(Platform3DObject obj, const String& name)
{
    makeContextCurrent();
    String mappedName = mappedSymbolName(obj, SHADER_SYMBOL_TYPE_UNIFORM, name);
    return wkcGLGetUniformLocationPeer(peer(), obj, mappedName.utf8().data());
}


void
GraphicsContext3D::getVertexAttribfv(GC3Duint index, GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLGetVertexAttribfvPeer(peer(), index, pname, value);
}


void
GraphicsContext3D::getVertexAttribiv(GC3Duint index, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    wkcGLGetVertexAttribivPeer(peer(), index, pname, value);
}

GC3Dsizeiptr
GraphicsContext3D::getVertexAttribOffset(GC3Duint index, GC3Denum pname)
{
    makeContextCurrent();
    return wkcGLGetVertexAttribOffsetPeer(peer(), index, pname);
}


void
GraphicsContext3D::hint(GC3Denum target, GC3Denum mode)
{
    makeContextCurrent();
    wkcGLHintPeer(peer(), target, mode);
}


GC3Dboolean
GraphicsContext3D::isBuffer(Platform3DObject obj)
{
    makeContextCurrent();
    return wkcGLIsBufferPeer(peer(), obj);
}


GC3Dboolean
GraphicsContext3D::isEnabled(GC3Denum cap)
{
    makeContextCurrent();
    return wkcGLIsEnabledPeer(peer(), cap);
}


GC3Dboolean
GraphicsContext3D::isFramebuffer(Platform3DObject obj)
{
    makeContextCurrent();
    return wkcGLIsFramebufferPeer(peer(), obj);
}


GC3Dboolean
GraphicsContext3D::isProgram(Platform3DObject obj)
{
    makeContextCurrent();
    return wkcGLIsProgramPeer(peer(), obj);
}


GC3Dboolean
GraphicsContext3D::isRenderbuffer(Platform3DObject obj)
{
    makeContextCurrent();
    return wkcGLIsRenderbufferPeer(peer(), obj);
}


GC3Dboolean
GraphicsContext3D::isShader(Platform3DObject obj)
{
    makeContextCurrent();
    return wkcGLIsShaderPeer(peer(), obj);
}


GC3Dboolean
GraphicsContext3D::isTexture(Platform3DObject obj)
{
    makeContextCurrent();
    return wkcGLIsTexturePeer(peer(), obj);
}


void
GraphicsContext3D::lineWidth(GC3Dfloat val)
{
    makeContextCurrent();
    wkcGLLineWidthPeer(peer(), val);
}


void
GraphicsContext3D::linkProgram(Platform3DObject program)
{
    ASSERT(program);
    makeContextCurrent();
    wkcGLLinkProgramPeer(peer(), program);
}


void
GraphicsContext3D::pixelStorei(GC3Denum pname, GC3Dint param)
{
    makeContextCurrent();
    wkcGLPixelStoreiPeer(peer(), pname, param);
}


void
GraphicsContext3D::polygonOffset(GC3Dfloat factor, GC3Dfloat units)
{
    makeContextCurrent();
    wkcGLPolygonOffsetPeer(peer(), factor, units);
}


void
GraphicsContext3D::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    makeContextCurrent();
    wkcGLReadPixelsPeer(peer(), x, y, width, height, format, type, data);
}


void
GraphicsContext3D::releaseShaderCompiler()
{
    makeContextCurrent();
    wkcGLReleaseShaderCompilerPeer(peer());
}


void
GraphicsContext3D::renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    wkcGLRenderbufferStoragePeer(peer(), target, internalformat, width, height);
}


void
GraphicsContext3D::sampleCoverage(GC3Dclampf value, GC3Dboolean invert)
{
    makeContextCurrent();
    wkcGLSampleCoveragePeer(peer(), value, invert);
}


void
GraphicsContext3D::scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    wkcGLScissorPeer(peer(), x, y, width, height);
}


void
GraphicsContext3D::shaderSource(Platform3DObject shader, const String& string)
{
    ShaderSourceEntry entry;
    entry.source = string;
    m_shaderSourceMap.set(shader, entry);
}


void
GraphicsContext3D::stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    makeContextCurrent();
    wkcGLStencilFuncPeer(peer(), func, ref, mask);
}


void
GraphicsContext3D::stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    makeContextCurrent();
    wkcGLStencilFuncSeparatePeer(peer(), face, func, ref, mask);
}


void
GraphicsContext3D::stencilMask(GC3Duint mask)
{
    makeContextCurrent();
    wkcGLStencilMaskPeer(peer(), mask);
}


void
GraphicsContext3D::stencilMaskSeparate(GC3Denum face, GC3Duint mask)
{
    makeContextCurrent();
    wkcGLStencilMaskSeparatePeer(peer(), face, mask);
}


void
GraphicsContext3D::stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    makeContextCurrent();
    wkcGLStencilOpPeer(peer(), fail, zfail, zpass);
}


void
GraphicsContext3D::stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    makeContextCurrent();
    wkcGLStencilOpSeparatePeer(peer(), face, fail, zfail, zpass);
}


bool
GraphicsContext3D::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    if (width && height && !pixels) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    makeContextCurrent();
    wkcGLTexImage2DPeer(peer(), target, level, internalformat, width, height, border, format, type, pixels);
    return true;
}

void
GraphicsContext3D::texImage2DDirect(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    if (!pixels)
        return;
    makeContextCurrent();
    wkcGLTexImage2DPeer(peer(), target, level, internalformat, width, height, border, format, type, pixels);
}

void
GraphicsContext3D::texParameterf(GC3Denum target, GC3Denum pname, GC3Dfloat param)
{
    makeContextCurrent();
    wkcGLTexParameterfPeer(peer(), target, pname, param);
}


void
GraphicsContext3D::texParameteri(GC3Denum target, GC3Denum pname, GC3Dint param)
{
    makeContextCurrent();
    wkcGLTexParameteriPeer(peer(), target, pname, param);
}


void
GraphicsContext3D::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels)
{
    makeContextCurrent();
    wkcGLTexSubImage2DPeer(peer(), target, level, xoffset, yoffset, width, height, format, type, pixels);
}



void
GraphicsContext3D::uniform1f(GC3Dint location, GC3Dfloat x)
{
    makeContextCurrent();
    wkcGLUniform1fPeer(peer(), location, x);
}


void
GraphicsContext3D::uniform1fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    wkcGLUniform1fvPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform1i(GC3Dint location, GC3Dint x)
{
    makeContextCurrent();
    wkcGLUniform1iPeer(peer(), location, x);
}


void
GraphicsContext3D::uniform1iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    wkcGLUniform1ivPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform2f(GC3Dint location, GC3Dfloat x, GC3Dfloat y)
{
    makeContextCurrent();
    wkcGLUniform2fPeer(peer(), location, x, y);
}


void
GraphicsContext3D::uniform2fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    wkcGLUniform2fvPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform2i(GC3Dint location, GC3Dint x, GC3Dint y)
{
    makeContextCurrent();
    wkcGLUniform2iPeer(peer(), location, x, y);
}


void
GraphicsContext3D::uniform2iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    wkcGLUniform2ivPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform3f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z)
{
    makeContextCurrent();
    wkcGLUniform3fPeer(peer(), location, x, y, z);
}


void
GraphicsContext3D::uniform3fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    wkcGLUniform3fvPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform3i(GC3Dint location, GC3Dint x, GC3Dint y, GC3Dint z)
{
    makeContextCurrent();
    wkcGLUniform3iPeer(peer(), location, x, y, z);
}


void
GraphicsContext3D::uniform3iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    wkcGLUniform3ivPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform4f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w)
{
    makeContextCurrent();
    wkcGLUniform4fPeer(peer(), location, x, y, z, w);
}


void
GraphicsContext3D::uniform4fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    wkcGLUniform4fvPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniform4i(GC3Dint location, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w)
{
    makeContextCurrent();
    wkcGLUniform4iPeer(peer(), location, x, y, z, w);
}


void
GraphicsContext3D::uniform4iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    wkcGLUniform4ivPeer(peer(), location, size, v);
}


void
GraphicsContext3D::uniformMatrix2fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLUniformMatrix2fvPeer(peer(), location, size, transpose, value);
}


void
GraphicsContext3D::uniformMatrix3fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLUniformMatrix3fvPeer(peer(), location, size, transpose, value);
}


void
GraphicsContext3D::uniformMatrix4fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* value)
{
    makeContextCurrent();
    wkcGLUniformMatrix4fvPeer(peer(), location, size, transpose, value);
}


void
GraphicsContext3D::useProgram(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLUseProgramPeer(peer(), obj);
}


void
GraphicsContext3D::validateProgram(Platform3DObject obj)
{
    ASSERT(obj);
    makeContextCurrent();
    wkcGLValidateProgramPeer(peer(), obj);
}

bool
GraphicsContext3D::checkVaryingsPacking(Platform3DObject vertexShader, Platform3DObject fragmentShader) const
{
    ASSERT(m_shaderSourceMap.contains(vertexShader));
    ASSERT(m_shaderSourceMap.contains(fragmentShader));
    const auto& vertexEntry = m_shaderSourceMap.find(vertexShader)->value;
    const auto& fragmentEntry = m_shaderSourceMap.find(fragmentShader)->value;

    HashMap<String, ShVariableInfo> combinedVaryings;
    for (const auto& vertexSymbol : vertexEntry.varyingMap) {
        const String& symbolName = vertexSymbol.key;
        // The varying map includes variables for each index of an array variable.
        // We only want a single variable to represent the array.
        if (symbolName.endsWith("]"))
            continue;

        // Don't count built in varyings.
        if (symbolName == "gl_FragCoord" || symbolName == "gl_FrontFacing" || symbolName == "gl_PointCoord")
            continue;

        const auto& fragmentSymbol = fragmentEntry.varyingMap.find(symbolName);
        if (fragmentSymbol != fragmentEntry.varyingMap.end()) {
            ShVariableInfo symbolInfo;
            symbolInfo.type = (fragmentSymbol->value).type;
            // The arrays are already split up.
            symbolInfo.size = (fragmentSymbol->value).size;
            combinedVaryings.add(symbolName, symbolInfo);
        }
    }

    size_t numVaryings = combinedVaryings.size();
    if (!numVaryings)
        return true;

    ShVariableInfo* variables = (ShVariableInfo *)fastZeroedMalloc(sizeof(ShVariableInfo) * numVaryings);
    int index = 0;
    for (const auto& varyingSymbol : combinedVaryings) {
        variables[index] = varyingSymbol.value;
        index++;
    }

    GC3Dint maxVaryingVectors = 0;
#if !PLATFORM(IOS) && !((PLATFORM(WIN) || PLATFORM(GTK)) && USE(OPENGL_ES_2))
    GC3Dint maxVaryingFloats = 0;
    wkcGLGetIntegervPeer(peer(), 0x8B4B/*GL_MAX_VARYING_FLOATS*/, &maxVaryingFloats);
    maxVaryingVectors = maxVaryingFloats / 4;
#else
    wkcGLGetIntegervPeer(peer(), 0x8DFC/*MAX_VARYING_VECTORS*/, &maxVaryingVectors);
#endif
    int result = ShCheckVariablesWithinPackingLimits(maxVaryingVectors, variables, numVaryings);
    WTF::fastFree(variables);
    return result;
}

void
GraphicsContext3D::vertexAttrib1f(GC3Duint index, GC3Dfloat x)
{
    makeContextCurrent();
    wkcGLVertexAttrib1fPeer(peer(), index, x);
}


void
GraphicsContext3D::vertexAttrib1fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    wkcGLVertexAttrib1fvPeer(peer(), index, values);
}


void
GraphicsContext3D::vertexAttrib2f(GC3Duint index, GC3Dfloat x, GC3Dfloat y)
{
    makeContextCurrent();
    wkcGLVertexAttrib2fPeer(peer(), index, x, y);
}


void
GraphicsContext3D::vertexAttrib2fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    wkcGLVertexAttrib2fvPeer(peer(), index, values);
}


void
GraphicsContext3D::vertexAttrib3f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z)
{
    makeContextCurrent();
    wkcGLVertexAttrib3fPeer(peer(), index, x, y, z);
}


void
GraphicsContext3D::vertexAttrib3fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    wkcGLVertexAttrib3fvPeer(peer(), index, values);
}


void
GraphicsContext3D::vertexAttrib4f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w)
{
    makeContextCurrent();
    wkcGLVertexAttrib4fPeer(peer(), index, x, y, z, w);
}


void
GraphicsContext3D::vertexAttrib4fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    wkcGLVertexAttrib4fvPeer(peer(), index, values);
}


void
GraphicsContext3D::vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized,
                                       GC3Dsizei stride, GC3Dintptr offset)
{
    makeContextCurrent();
    wkcGLVertexAttribPointerPeer(peer(), index, size, type, normalized, stride, offset);
}


void
GraphicsContext3D::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    wkcGLViewportPeer(peer(), x, y, width, height);
}


void
GraphicsContext3D::reshape(int width, int height)
{
    m_private->reshape(width, height);

    makeContextCurrent();
    wkcGLReshapePeer(peer(), width, height);
}

bool
GraphicsContext3D::isResourceSafe()
{
    return false;
}

void
GraphicsContext3D::markContextChanged()
{
    wkcGLMarkContextChangedPeer(peer());
}


void
GraphicsContext3D::markLayerComposited()
{
    wkcGLMarkLayerCompositedPeer(peer());
}


bool
GraphicsContext3D::layerComposited() const
{
    return wkcGLLayerCompositedPeer(peer());
}

Platform3DObject
GraphicsContext3D::createBuffer()
{
    makeContextCurrent();
    return wkcGLCreateBufferPeer(peer());
}


Platform3DObject
GraphicsContext3D::createFramebuffer()
{
    makeContextCurrent();
    return wkcGLCreateFramebufferPeer(peer());
}


Platform3DObject
GraphicsContext3D::createProgram()
{
    makeContextCurrent();
    return wkcGLCreateProgramPeer(peer());
}


Platform3DObject
GraphicsContext3D::createRenderbuffer()
{
    makeContextCurrent();
    return wkcGLCreateRenderbufferPeer(peer());
}


Platform3DObject
GraphicsContext3D::createShader(GC3Denum type)
{
    makeContextCurrent();
    return wkcGLCreateShaderPeer(peer(), type);
}


Platform3DObject
GraphicsContext3D::createTexture()
{
    makeContextCurrent();
    return wkcGLCreateTexturePeer(peer());
}


void
GraphicsContext3D::deleteBuffer(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLDeleteBufferPeer(peer(), obj);
}


void
GraphicsContext3D::deleteFramebuffer(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLDeleteFramebufferPeer(peer(), obj);
}


void
GraphicsContext3D::deleteProgram(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLDeleteProgramPeer(peer(), obj);
}


void
GraphicsContext3D::deleteRenderbuffer(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLDeleteRenderbufferPeer(peer(), obj);
}


void
GraphicsContext3D::deleteShader(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLDeleteShaderPeer(peer(), obj);
}


void
GraphicsContext3D::deleteTexture(Platform3DObject obj)
{
    makeContextCurrent();
    wkcGLDeleteTexturePeer(peer(), obj);
}



void
GraphicsContext3D::synthesizeGLError(GC3Denum error)
{
    moveErrorsToSyntheticErrorList();
    m_syntheticErrors.add(error);
}

Extensions3D*
GraphicsContext3D::getExtensions()
{
    return m_private->extensions();
}

bool
Extensions3DWKC::supports(const String& name)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    return wkcGLExtSupportsPeer(m_peer, name.utf8().data());
}

void
Extensions3DWKC::ensureEnabled(const String& name)
{
    if (name == "GL_OES_standard_derivatives") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.OES_standard_derivatives) {
            ANGLEResources.OES_standard_derivatives = 1;
            compiler.setResources(ANGLEResources);
        }
    } else if (name == "GL_EXT_draw_buffers") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.EXT_draw_buffers) {
            ANGLEResources.EXT_draw_buffers = 1;
            m_context->getIntegerv(Extensions3D::MAX_DRAW_BUFFERS_EXT, &ANGLEResources.MaxDrawBuffers);
            compiler.setResources(ANGLEResources);
        }
    } else if (name == "GL_EXT_shader_texture_lod") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.EXT_shader_texture_lod) {
            ANGLEResources.EXT_shader_texture_lod = 1;
            compiler.setResources(ANGLEResources);
        }
    }
}

bool
Extensions3DWKC::isEnabled(const String& name)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    return wkcGLExtIsEnabledPeer(m_peer, name.utf8().data());
}

void
Extensions3DWKC::blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    wkcGLExtBlitFramebufferPeer(m_peer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void
Extensions3DWKC::renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    wkcGLExtRenderbufferStorageMultisamplePeer(m_peer, target, samples, internalformat, width, height);
}

Platform3DObject
Extensions3DWKC::createVertexArrayOES()
{
    wkcGLMakeContextCurrentPeer(m_peer);
    return wkcGLExtCreateVertexArrayOESPeer(m_peer);
}

void
Extensions3DWKC::deleteVertexArrayOES(Platform3DObject obj)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    wkcGLExtDeleteVertexArrayOESPeer(m_peer, obj);
}

GC3Dboolean
Extensions3DWKC::isVertexArrayOES(Platform3DObject obj)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    return wkcGLExtIsVertexArrayOESPeer(m_peer, obj);
}

void
Extensions3DWKC::bindVertexArrayOES(Platform3DObject obj)
{
    wkcGLMakeContextCurrentPeer(m_peer);
    wkcGLExtBindVertexArrayOESPeer(m_peer, obj);
}

String
Extensions3DWKC::getTranslatedShaderSourceANGLE(Platform3DObject shader)
{
    ASSERT(shader);
    int GLshaderType;
    ANGLEShaderType shaderType;

    ANGLEWebKitBridge& compiler = m_context->m_compiler;

    m_context->getShaderiv(shader, GraphicsContext3D::SHADER_TYPE, &GLshaderType);

    if (GLshaderType == GraphicsContext3D::VERTEX_SHADER)
        shaderType = SHADER_TYPE_VERTEX;
    else if (GLshaderType == GraphicsContext3D::FRAGMENT_SHADER)
        shaderType = SHADER_TYPE_FRAGMENT;
    else
        return ""; // Invalid shader type.

    HashMap<Platform3DObject, GraphicsContext3D::ShaderSourceEntry>::iterator result = m_context->m_shaderSourceMap.find(shader);

    if (result == m_context->m_shaderSourceMap.end())
        return "";

    GraphicsContext3D::ShaderSourceEntry& entry = result->value;

    String translatedShaderSource;
    String shaderInfoLog;
    int extraCompileOptions = SH_CLAMP_INDIRECT_ARRAY_BOUNDS | SH_UNFOLD_SHORT_CIRCUIT | SH_ENFORCE_PACKING_RESTRICTIONS | SH_INIT_VARYINGS_WITHOUT_STATIC_USE | SH_LIMIT_EXPRESSION_COMPLEXITY | SH_LIMIT_CALL_STACK_DEPTH;

    if (m_requiresBuiltInFunctionEmulation)
        extraCompileOptions |= SH_EMULATE_BUILT_IN_FUNCTIONS;

    Vector<ANGLEShaderSymbol> symbols;
    bool isValid = compiler.compileShaderSource(entry.source.utf8().data(), shaderType, translatedShaderSource, shaderInfoLog, symbols, extraCompileOptions);

    entry.log = shaderInfoLog;
    entry.isValid = isValid;

    size_t numSymbols = symbols.size();
    for (size_t i = 0; i < numSymbols; ++i) {
        ANGLEShaderSymbol shaderSymbol = symbols[i];
        GraphicsContext3D::SymbolInfo symbolInfo(shaderSymbol.dataType, shaderSymbol.size, shaderSymbol.mappedName, shaderSymbol.precision, shaderSymbol.staticUse);
        entry.symbolMap(shaderSymbol.symbolType).set(shaderSymbol.name, symbolInfo);
    }

    if (!isValid)
        return "";

    return translatedShaderSource;
}

IntSize
GraphicsContext3D::getInternalFramebufferSize() const
{
    WKCSize s = {0};
    wkcGLGetInternalFramebufferSizePeer(peer(), &s);
    return s;
}

#if USE(WKC_CAIRO)
void
GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight,
                                    int canvasWidth, int canvasHeight, PlatformContextCairo* context)
{
    (void)wkcGLPaintsIntoCanvasBufferPeer(peer());
}
#endif

bool
GraphicsContext3D::paintCompositedResultsToCanvas(ImageBuffer*)
{
    notImplemented();
    return false;
}

void
GraphicsContext3D::paintRenderingResultsToCanvas(ImageBuffer* buf)
{
    if (!buf)
        return;

    GraphicsContext* dc = buf->context();

    int fmt = 0;
    int rowbytes = 0;
    void* mask = 0;
    int maskrowbytes = 0;
    WKCSize wsize = { 0 };
    void* bitmap = wkcGLLockImagePeer(peer(), &fmt, &rowbytes, &mask, &maskrowbytes, &wsize);

    if (!bitmap) return;

    int wtype = ImageWKC::EColors;
    switch (fmt&WKC_IMAGETYPE_TYPEMASK) {
    case WKC_IMAGETYPE_ARGB8888:
        wtype = ImageWKC::EColorARGB8888;
        break;
    case WKC_IMAGETYPE_RGB565:
        wtype = ImageWKC::EColorRGB565;
        break;
    default:
        break;
    }

    if (wtype != ImageWKC::EColors) {
        const IntSize size(wsize.fWidth, wsize.fHeight);
        Ref<ImageWKC> wimg = ImageWKC::create(wtype, bitmap, rowbytes, size, false);
        wimg->setHasAlpha(false);
        if (fmt&WKC_IMAGETYPE_FLAG_HASALPHA)
            wimg->setHasAlpha(true);
        Ref<BitmapImage> img = BitmapImage::create(WTFMove(wimg));
        dc->drawImage(img.get(), IntPoint());
    }

    wkcGLUnlockImagePeer(peer(), bitmap);
}

PlatformLayer*
GraphicsContext3D::platformLayer() const
{
    if (!m_private->layer())
        return 0;
    unsigned int w=0, h=0;
    wkcGLGetDestTextureSizePeer(peer(), &w, &h);
    wkcLayerSetTexturePeer(m_private->layer(), wkcGLDestTexturePeer(peer()), w, h);
    return (PlatformLayer *)m_private->layer();
}

RefPtr<ImageData>
GraphicsContext3D::paintRenderingResultsToImageData()
{
    int fmt = 0;
    int rowbytes = 0;
    void* mask = 0;
    int maskrowbytes = 0;
    WKCSize wsize = {0};
    void* bitmap = wkcGLLockImagePeer(peer(), &fmt, &rowbytes, &mask, &maskrowbytes, &wsize);

    if (!bitmap) {
        wkcMemoryNotifyNoMemoryPeer(wsize.fWidth*wsize.fHeight);
        return 0;
    }

    RefPtr<ImageData> img = ImageData::create(IntSize(wsize.fWidth, wsize.fHeight));
    if (!img) {
        wkcMemoryNotifyNoMemoryPeer(wsize.fWidth*wsize.fHeight);
        return 0;
    }
    unsigned char* dest = img->data()->data();

    int r=0, g=0, b=0, a=0;
    const unsigned char* src = 0;
    for (int y=0; y<wsize.fHeight; y++) {
        for (int x=0; x<wsize.fWidth; x++) {
            switch (fmt&WKC_IMAGETYPE_TYPEMASK) {
            case WKC_IMAGETYPE_ARGB8888:
            {
                src = (const unsigned char *)bitmap + y*rowbytes + x*4;
                a = src[0];
                r = src[1];
                g = src[2];
                b = src[3];
                break;
            }
            case WKC_IMAGETYPE_RGB565:
            {
                src = (const unsigned char *)bitmap + y*rowbytes + x*2;
                unsigned short v = src[0] + ((int)src[1]<<8);
                a = 255;
                r = (((v>>8)&0xf8) | ((v>>11)&0x07));
                g = (((v>>3)&0xfc) | ((v>>6)&0x03));
                b = (((v<<3)&0xf8) | (v&0x07));
                break;
            }
            default:
                break;
            }
        }
        *dest++ = r;
        *dest++ = g;
        *dest++ = b;
        *dest++ = a;
    }

    wkcGLUnlockImagePeer(peer(), bitmap);
    return img;
}

void GraphicsContext3D::getShaderPrecisionFormat(GC3Denum shaderType, GC3Denum precisionType, GC3Dint* range, GC3Dint* precision)
{
    makeContextCurrent();
    wkcGetShaderPrecisionFormatPeer(peer(), shaderType, precisionType, range, precision);
}

bool
GraphicsContext3D::moveErrorsToSyntheticErrorList()
{
    makeContextCurrent();
    bool movedAnError = false;

    for (unsigned i=0; i < 100; i++) {
        GC3Denum error = wkcGLGetErrorPeer(peer());
        if (error == NO_ERROR)
            break;
        m_syntheticErrors.add(error);
        movedAnError = true;
    }
    return movedAnError;
}

void
GraphicsContext3D::getNonBuiltInActiveSymbolCount(Platform3DObject program, GC3Denum pname, GC3Dint* value)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return;
    }
    makeContextCurrent();

    const ShaderProgramSymbolCountMap::iterator& result = m_shaderProgramSymbolCountMap.find(program);
    if (result != m_shaderProgramSymbolCountMap.end()) {
        *value = result->value.countForType(pname);
        return;
    }

    m_shaderProgramSymbolCountMap.set(program, ActiveShaderSymbolCounts());
    ActiveShaderSymbolCounts& symbolCounts = m_shaderProgramSymbolCountMap.find(program)->value;

    GC3Dint attributeCount = 0;
    wkcGLGetProgramivPeer(peer(), program, 0x8B89 /*ACTIVE_ATTRIBUTES*/, &attributeCount);
    for (int i=0; i < attributeCount; i++) {
        ActiveInfo info;
        getActiveAttribImpl(program, i, info);
        if (info.name.startsWith("gl_"))
            continue;

        symbolCounts.filteredToActualAttributeIndexMap.append(i);
    }

    GC3Dint uniformCount = 0;
    wkcGLGetProgramivPeer(peer(), program, 0x8B86/*ACTIVE_UNIFORMS*/, &uniformCount);
    for (int i=0; i < uniformCount; i++) {
        ActiveInfo info;
        getActiveUniformImpl(program, i, info);
        if (info.name.startsWith("gl_"))
            continue;
        
        symbolCounts.filteredToActualUniformIndexMap.append(i);
    }
    
    *value = symbolCounts.countForType(pname);
}

bool
GraphicsContext3D::precisionsMatch(Platform3DObject vertexShader, Platform3DObject fragmentShader) const
{
    return true;
}

void
GraphicsContext3D::drawArraysInstanced(GC3Denum mode, GC3Dint first, GC3Dsizei count, GC3Dsizei primcount)
{
    getExtensions()->drawArraysInstanced(mode, first, count, primcount);
}

void
GraphicsContext3D::drawElementsInstanced(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset, GC3Dsizei primcount)
{
    getExtensions()->drawElementsInstanced(mode, count, type, offset, primcount);
}

void
GraphicsContext3D::vertexAttribDivisor(GC3Duint index, GC3Duint divisor)
{
    getExtensions()->vertexAttribDivisor(index, divisor);
}

} // namespace

#else

#include "GraphicsContext3D.h"

#include "Extensions3D.h"
#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#else
#include "Extensions3DOpenGL.h"
#endif

namespace WebCore {

class GraphicsContext3DPrivate
{
public:
    static std::unique_ptr<GraphicsContext3DPrivate> create(GraphicsContext3DAttributes& attrs, HostWindow* hostwindow, bool in_direct)
    {
        return nullptr;
    }
    ~GraphicsContext3DPrivate() {}

private:
    GraphicsContext3DPrivate() {}
};

GraphicsContext3D::~GraphicsContext3D()
{
}

Platform3DObject
GraphicsContext3D::createFramebuffer()
{
    return 0;
}

void
GraphicsContext3D::uniform4f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w)
{
}

GC3Dint
GraphicsContext3D::getUniformLocation(Platform3DObject obj, const String& name)
{
    return 0;
}

} // namespace

#endif // ENABLE(WEBGL)
