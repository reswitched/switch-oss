/*
 * Copyright (C) 2011, 2012 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "config.h"

#if ENABLE(GRAPHICS_CONTEXT_3D)
#include "GraphicsContext3DPrivate.h"

#include "HostWindow.h"
#include "NotImplemented.h"
#include <wtf/StdLibExtras.h>

#if USE(CAIRO)
#include "PlatformContextCairo.h"
#endif

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <gl/GL.h>
#endif
#endif

#if USE(TEXTURE_MAPPER) && USE(TEXTURE_MAPPER_GL)
#include <texmap/TextureMapperGL.h>
#endif

using namespace std;

namespace WebCore {

GraphicsContext3DPrivate::GraphicsContext3DPrivate(GraphicsContext3D* context, GraphicsContext3D::RenderStyle renderStyle)
    : m_context(context)
    , m_renderStyle(renderStyle)
{
    switch (renderStyle) {
    case GraphicsContext3D::RenderOffscreen:
        m_glContext = GLContext::createOffscreenContext(GLContext::sharingContext());
        break;
    case GraphicsContext3D::RenderToCurrentGLContext:
        break;
    case GraphicsContext3D::RenderDirectlyToHostWindow:
        ASSERT_NOT_REACHED();
        break;
    }
}

GraphicsContext3DPrivate::~GraphicsContext3DPrivate()
{
#if USE(TEXTURE_MAPPER)
    if (client())
        client()->platformLayerWillBeDestroyed();
#endif
}

bool GraphicsContext3DPrivate::makeContextCurrent()
{
    return m_glContext ? m_glContext->makeContextCurrent() : false;
}

PlatformGraphicsContext3D GraphicsContext3DPrivate::platformContext()
{
    return m_glContext ? m_glContext->platformContext() : GLContext::getCurrent()->platformContext();
}

#if USE(TEXTURE_MAPPER)
void GraphicsContext3DPrivate::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    if (!m_glContext)
        return;

    ASSERT(m_renderStyle == GraphicsContext3D::RenderOffscreen);

    m_context->markLayerComposited();

#if USE(TEXTURE_MAPPER_GL)
    if (m_context->m_attrs.antialias && m_context->m_state.boundFBO == m_context->m_multisampleFBO) {
        GLContext* previousActiveContext = GLContext::getCurrent();
        if (previousActiveContext != m_glContext.get())
            m_context->makeContextCurrent();

        m_context->resolveMultisamplingIfNecessary();
        ::glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_state.boundFBO);

        if (previousActiveContext && previousActiveContext != m_glContext.get())
            previousActiveContext->makeContextCurrent();
    }

    TextureMapperGL* texmapGL = static_cast<TextureMapperGL*>(textureMapper);
    TextureMapperGL::Flags flags = TextureMapperGL::ShouldFlipTexture | (m_context->m_attrs.alpha ? TextureMapperGL::ShouldBlend : 0);
    IntSize textureSize(m_context->m_currentWidth, m_context->m_currentHeight);
    texmapGL->drawTexture(m_context->m_texture, flags, textureSize, targetRect, matrix, opacity);
#endif // USE(TEXTURE_MAPPER_GL)
}
#endif // USE(TEXTURE_MAPPER)

} // namespace WebCore

#endif // ENABLE(GRAPHICS_CONTEXT_3D)
