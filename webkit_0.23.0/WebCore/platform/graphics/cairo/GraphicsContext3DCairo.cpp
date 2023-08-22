/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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

#if USE(CAIRO)

#if ENABLE(GRAPHICS_CONTEXT_3D)
#include "GraphicsContext3D.h"

#include "CairoUtilities.h"
#include "GraphicsContext3DPrivate.h"
#include "Image.h"
#include "ImageSource.h"
#include "NotImplemented.h"
#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include <cairo.h>

#if PLATFORM(WIN)
#include <GLSLANG/ShaderLang.h>
#else
#include <ANGLE/ShaderLang.h>
#endif

#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#else
#include "Extensions3DOpenGL.h"
#include "OpenGLShims.h"
#endif

namespace WebCore {

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attributes, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to the HostWindow.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;

    static bool initialized = false;
    static bool success = true;
    if (!initialized) {
#if !USE(OPENGL_ES_2)
        success = initializeOpenGLShims();
#endif
        initialized = true;
    }
    if (!success)
        return 0;

    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attributes, hostWindow, renderStyle));
    return context.release();
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attributes, HostWindow*, GraphicsContext3D::RenderStyle renderStyle)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_compiler(isGLES2Compliant() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT)
    , m_attrs(attributes)
    , m_texture(0)
    , m_compositorTexture(0)
    , m_fbo(0)
    , m_depthStencilBuffer(0)
    , m_multisampleFBO(0)
    , m_multisampleDepthStencilBuffer(0)
    , m_multisampleColorBuffer(0)
    , m_private(std::make_unique<GraphicsContext3DPrivate>(this, renderStyle))
{
    makeContextCurrent();

    validateAttributes();

    if (renderStyle == RenderOffscreen) {
        // Create a texture to render into.
        ::glGenTextures(1, &m_texture);
        ::glBindTexture(GL_TEXTURE_2D, m_texture);
        ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ::glBindTexture(GL_TEXTURE_2D, 0);

        // Create an FBO.
        ::glGenFramebuffers(1, &m_fbo);
        ::glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        m_state.boundFBO = m_fbo;
        if (!m_attrs.antialias && (m_attrs.stencil || m_attrs.depth))
            ::glGenRenderbuffers(1, &m_depthStencilBuffer);

        // Create a multisample FBO.
        if (m_attrs.antialias) {
            ::glGenFramebuffers(1, &m_multisampleFBO);
            ::glBindFramebuffer(GL_FRAMEBUFFER, m_multisampleFBO);
            m_state.boundFBO = m_multisampleFBO;
            ::glGenRenderbuffers(1, &m_multisampleColorBuffer);
            if (m_attrs.stencil || m_attrs.depth)
                ::glGenRenderbuffers(1, &m_multisampleDepthStencilBuffer);
        }
    }

    // ANGLE initialization.
    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;

    GC3Dint range[2], precision;
    getShaderPrecisionFormat(GraphicsContext3D::FRAGMENT_SHADER, GraphicsContext3D::HIGH_FLOAT, range, &precision);
    ANGLEResources.FragmentPrecisionHigh = (range[0] || range[1] || precision);

    m_compiler.setResources(ANGLEResources);

#if !USE(OPENGL_ES_2)
    ::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    ::glEnable(GL_POINT_SPRITE);
#endif

    ::glClearColor(0, 0, 0, 0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    if (m_private->renderStyle() == RenderToCurrentGLContext)
        return;

    makeContextCurrent();
    if (m_texture)
        ::glDeleteTextures(1, &m_texture);
    if (m_compositorTexture)
        ::glDeleteTextures(1, &m_compositorTexture);

    if (m_attrs.antialias) {
        ::glDeleteRenderbuffers(1, &m_multisampleColorBuffer);
        if (m_attrs.stencil || m_attrs.depth)
            ::glDeleteRenderbuffers(1, &m_multisampleDepthStencilBuffer);
        ::glDeleteFramebuffers(1, &m_multisampleFBO);
    } else {
        if (m_attrs.stencil || m_attrs.depth)
            ::glDeleteRenderbuffers(1, &m_depthStencilBuffer);
    }
    ::glDeleteFramebuffers(1, &m_fbo);
}

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
    if (m_decoder)
        delete m_decoder;
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    if (!m_image)
        return false;
    // We need this to stay in scope because the native image is just a shallow copy of the data.
    m_decoder = new ImageSource(premultiplyAlpha ? ImageSource::AlphaPremultiplied : ImageSource::AlphaNotPremultiplied, ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied);
    if (!m_decoder)
        return false;
    ImageSource& decoder = *m_decoder;

    m_alphaOp = AlphaDoNothing;
    if (m_image->data()) {
        decoder.setData(m_image->data(), true);
        if (!decoder.frameCount() || !decoder.frameIsCompleteAtIndex(0))
            return false;
        m_imageSurface = decoder.createFrameAtIndex(0);
    } else {
        m_imageSurface = m_image->nativeImageForCurrentFrame();
        // 1. For texImage2D with HTMLVideoElment input, assume no PremultiplyAlpha had been applied and the alpha value is 0xFF for each pixel,
        // which is true at present and may be changed in the future and needs adjustment accordingly.
        // 2. For texImage2D with HTMLCanvasElement input in which Alpha is already Premultiplied in this port, 
        // do AlphaDoUnmultiply if UNPACK_PREMULTIPLY_ALPHA_WEBGL is set to false.
        if (!premultiplyAlpha && m_imageHtmlDomSource != HtmlDomVideo)
            m_alphaOp = AlphaDoUnmultiply;

        // if m_imageSurface is not an image, extract a copy of the surface
        if (m_imageSurface && cairo_surface_get_type(m_imageSurface.get()) != CAIRO_SURFACE_TYPE_IMAGE) {
            RefPtr<cairo_surface_t> tmpSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_imageWidth, m_imageHeight));
            copyRectFromOneSurfaceToAnother(m_imageSurface.get(), tmpSurface.get(), IntSize(), IntRect(0, 0, m_imageWidth, m_imageHeight), IntSize(), CAIRO_OPERATOR_SOURCE);
            m_imageSurface = tmpSurface.release();
        }
    }

    if (!m_imageSurface)
        return false;

    ASSERT(cairo_surface_get_type(m_imageSurface.get()) == CAIRO_SURFACE_TYPE_IMAGE);

    IntSize imageSize = cairoSurfaceSize(m_imageSurface.get());
    m_imageWidth = imageSize.width();
    m_imageHeight = imageSize.height();
    if (!m_imageWidth || !m_imageHeight)
        return false;

    if (cairo_image_surface_get_format(m_imageSurface.get()) != CAIRO_FORMAT_ARGB32)
        return false;

    unsigned int srcUnpackAlignment = 1;
    size_t bytesPerRow = cairo_image_surface_get_stride(m_imageSurface.get());
    size_t bitsPerPixel = 32;
    unsigned padding = bytesPerRow - bitsPerPixel / 8 * m_imageWidth;
    if (padding) {
        srcUnpackAlignment = padding + 1;
        while (bytesPerRow % srcUnpackAlignment)
            ++srcUnpackAlignment;
    }

    m_imagePixelData = cairo_image_surface_get_data(m_imageSurface.get());
    m_imageSourceFormat = DataFormatBGRA8;
    m_imageSourceUnpackAlignment = srcUnpackAlignment;
    return true;
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight, PlatformContextCairo* context)
{
    if (!imagePixels || imageWidth <= 0 || imageHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0 || !context)
        return;

    cairo_t *cr = context->cr();
    context->save();

    cairo_rectangle(cr, 0, 0, canvasWidth, canvasHeight);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(
        const_cast<unsigned char*>(imagePixels), CAIRO_FORMAT_ARGB32, imageWidth, imageHeight, imageWidth * 4));

    // OpenGL keeps the pixels stored bottom up, so we need to flip the image here.
    cairo_translate(cr, 0, imageHeight);
    cairo_scale(cr, 1, -1);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cr, imageSurface.get(), 0, 0);
    cairo_rectangle(cr, 0, 0, canvasWidth, -canvasHeight);

    cairo_fill(cr);
    context->restore();
}

void GraphicsContext3D::setContextLostCallback(std::unique_ptr<ContextLostCallback>)
{
}

void GraphicsContext3D::setErrorMessageCallback(std::unique_ptr<ErrorMessageCallback>)
{
}

bool GraphicsContext3D::makeContextCurrent()
{
    if (!m_private)
        return false;
    return m_private->makeContextCurrent();
}

void GraphicsContext3D::checkGPUStatusIfNecessary()
{
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D()
{
    return m_private->platformContext();
}

Platform3DObject GraphicsContext3D::platformTexture() const
{
    return m_texture;
}

bool GraphicsContext3D::isGLES2Compliant() const
{
#if USE(OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

PlatformLayer* GraphicsContext3D::platformLayer() const
{
    return m_private.get();
}

} // namespace WebCore

#endif // ENABLE(GRAPHICS_CONTEXT_3D)

#endif // USE(CAIRO)
