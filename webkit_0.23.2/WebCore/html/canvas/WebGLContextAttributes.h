/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebGLContextAttributes_h
#define WebGLContextAttributes_h

#include "CanvasContextAttributes.h"
#include "GraphicsContext3D.h"

namespace WebCore {

class WebGLContextAttributes final : public CanvasContextAttributes {
  public:
    virtual ~WebGLContextAttributes();

    // Create a new attributes object
    static Ref<WebGLContextAttributes> create();

    // Create a new attributes object initialized with preexisting attributes
    static Ref<WebGLContextAttributes> create(GraphicsContext3D::Attributes);

    // Whether or not the drawing buffer has an alpha channel. Default is true.
    bool alpha() const;
    void setAlpha(bool alpha);

    // Whether or not the drawing buffer has a depth buffer. Default is true.
    bool depth() const;
    void setDepth(bool depth);

    // Whether or not the drawing buffer has a stencil buffer. Default is true.
    bool stencil() const;
    void setStencil(bool stencil);

    // Whether or not the drawing buffer is antialiased. Default is true.
    bool antialias() const;
    void setAntialias(bool antialias);

    // Whether or not to treat the values in the drawing buffer as
    // though their alpha channel has already been multiplied into the
    // color channels. Default is true.
    bool premultipliedAlpha() const;
    void setPremultipliedAlpha(bool premultipliedAlpha);

    // Whether or not to preserve the drawing buffer after presentation to the
    // screen. Default is false.
    bool preserveDrawingBuffer() const;
    void setPreserveDrawingBuffer(bool);

    // Whether or not to attempt to use a lower-power GPU if it is available.
    // Default is false.
    bool preferLowPowerToHighPerformance() const;
    void setPreferLowPowerToHighPerformance(bool);

    // Whether or not to immediately fail if a performant GPU is not available.
    // Default is false.
    bool failIfMajorPerformanceCaveat() const;
    void setFailIfMajorPerformanceCaveat(bool);

    // Fetches a copy of the attributes stored in this object in a
    // form that can be used to initialize a GraphicsContext3D.
    GraphicsContext3D::Attributes attributes() const;

  protected:
    WebGLContextAttributes();
    WebGLContextAttributes(GraphicsContext3D::Attributes attributes);

  private:
    GraphicsContext3D::Attributes m_attrs;
};

} // namespace WebCore

#endif // WebGLContextAttributes_h
