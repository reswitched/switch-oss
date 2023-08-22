/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2014 Igalia S.L.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef BitmapTextureGL_h
#define BitmapTextureGL_h

#include "BitmapTexture.h"
#include "FilterOperation.h"
#include "GraphicsContext3D.h"
#include "IntSize.h"
#include "TextureMapperGL.h"

namespace WebCore {

class TextureMapper;
class TextureMapperGL;
class FilterOperation;

class BitmapTextureGL : public BitmapTexture {
public:
    BitmapTextureGL(PassRefPtr<GraphicsContext3D>);
    virtual ~BitmapTextureGL();

    virtual IntSize size() const override;
    virtual bool isValid() const override;
    virtual bool canReuseWith(const IntSize& contentsSize, Flags = 0) override;
    virtual void didReset() override;
    void bindAsSurface(GraphicsContext3D*);
    void initializeStencil();
    void initializeDepthBuffer();
    virtual uint32_t id() const { return m_id; }
    uint32_t textureTarget() const { return GraphicsContext3D::TEXTURE_2D; }
    IntSize textureSize() const { return m_textureSize; }
    virtual void updateContents(Image*, const IntRect&, const IntPoint&, UpdateContentsFlag) override;
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag) override;
    void updateContentsNoSwizzle(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine, unsigned bytesPerPixel = 4, Platform3DObject glFormat = GraphicsContext3D::RGBA);
    virtual bool isBackedByOpenGL() const override { return true; }

    virtual PassRefPtr<BitmapTexture> applyFilters(TextureMapper*, const FilterOperations&) override;
    struct FilterInfo {
        RefPtr<FilterOperation> filter;
        unsigned pass;
        RefPtr<BitmapTexture> contentTexture;

        FilterInfo(PassRefPtr<FilterOperation> f = 0, unsigned p = 0, PassRefPtr<BitmapTexture> t = 0)
            : filter(f)
            , pass(p)
            , contentTexture(t)
            { }
    };
    const FilterInfo* filterInfo() const { return &m_filterInfo; }
    TextureMapperGL::ClipStack& clipStack() { return m_clipStack; }

private:

    Platform3DObject m_id;
    IntSize m_textureSize;
    IntRect m_dirtyRect;
    Platform3DObject m_fbo;
    Platform3DObject m_rbo;
    Platform3DObject m_depthBufferObject;
    bool m_shouldClear;
    TextureMapperGL::ClipStack m_clipStack;
    RefPtr<GraphicsContext3D> m_context3D;

    BitmapTextureGL();

    void clearIfNeeded();
    void createFboIfNeeded();

    FilterInfo m_filterInfo;
};

BitmapTextureGL* toBitmapTextureGL(BitmapTexture*);

}

#endif // BitmapTextureGL_h
