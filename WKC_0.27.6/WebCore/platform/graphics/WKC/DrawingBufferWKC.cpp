/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "DrawingBuffer.h"


namespace WebCore {

DrawingBuffer::DrawingBuffer(GraphicsContext3D* context, const IntSize&, bool multisampleExtensionSupported, bool packedDepthStencilExtensionSupported, PreserveDrawingBuffer, AlphaRequirement)
    : m_preserveDrawingBuffer(Preserve)
    , m_alpha(Alpha)
    , m_scissorEnabled(false)
    , m_texture2DBinding(0)
    , m_framebufferBinding(0)
    , m_activeTextureUnit(0)
    , m_context(context)
    , m_size(-1, -1)
    , m_multisampleExtensionSupported(multisampleExtensionSupported)
    , m_packedDepthStencilExtensionSupported(packedDepthStencilExtensionSupported)
    , m_fbo(context->createFramebuffer())
    , m_colorBuffer(0)
    , m_frontColorBuffer(0)
    , m_separateFrontTexture(0)
    , m_depthStencilBuffer(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_multisampleFBO(0)
    , m_multisampleColorBuffer(0)
{
}

DrawingBuffer::~DrawingBuffer()
{
#if ENABLE(ACCELERATED_2D_CANVAS) || ENABLE(WEBGL)
    clear();
#endif
}

} // namespace
