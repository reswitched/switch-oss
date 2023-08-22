/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TextureMapper_h
#define TextureMapper_h

#if USE(TEXTURE_MAPPER)

#if USE(OPENGL_ES_2)
#define TEXMAP_OPENGL_ES_2
#endif

#include "BitmapTexture.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "IntSize.h"
#include "TransformationMatrix.h"

/*
    TextureMapper is a mechanism that enables hardware acceleration of CSS animations (accelerated compositing) without
    a need for a platform specific scene-graph library like CoreAnimations or QGraphicsView.
*/

namespace WebCore {

class BitmapTexturePool;
class GraphicsLayer;
class TextureMapper;
class FilterOperations;

class TextureMapper {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum PaintFlag {
        PaintingMirrored = 1 << 0,
    };

    enum WrapMode {
        StretchWrap,
        RepeatWrap
    };

    typedef unsigned PaintFlags;

    static std::unique_ptr<TextureMapper> create();

    explicit TextureMapper();
    virtual ~TextureMapper();

    enum ExposedEdges {
        NoEdges = 0,
        LeftEdge = 1 << 0,
        RightEdge = 1 << 1,
        TopEdge = 1 << 2,
        BottomEdge = 1 << 3,
        AllEdges = LeftEdge | RightEdge | TopEdge | BottomEdge,
    };

    virtual void drawBorder(const Color&, float borderWidth, const FloatRect&, const TransformationMatrix&) = 0;
    virtual void drawNumber(int number, const Color&, const FloatPoint&, const TransformationMatrix&) = 0;

    virtual void drawTexture(const BitmapTexture&, const FloatRect& target, const TransformationMatrix& modelViewMatrix = TransformationMatrix(), float opacity = 1.0f, unsigned exposedEdges = AllEdges) = 0;
    virtual void drawSolidColor(const FloatRect&, const TransformationMatrix&, const Color&) = 0;

    // makes a surface the target for the following drawTexture calls.
    virtual void bindSurface(BitmapTexture* surface) = 0;
    void setGraphicsContext(GraphicsContext* context) { m_context = context; }
    GraphicsContext* graphicsContext() { return m_context; }
    virtual void beginClip(const TransformationMatrix&, const FloatRect&) = 0;
    virtual void endClip() = 0;
    virtual IntRect clipBounds() = 0;
    virtual PassRefPtr<BitmapTexture> createTexture() = 0;

    void setImageInterpolationQuality(InterpolationQuality quality) { m_interpolationQuality = quality; }
    void setTextDrawingMode(TextDrawingModeFlags mode) { m_textDrawingMode = mode; }

    InterpolationQuality imageInterpolationQuality() const { return m_interpolationQuality; }
    TextDrawingModeFlags textDrawingMode() const { return m_textDrawingMode; }

    virtual void beginPainting(PaintFlags = 0) { }
    virtual void endPainting() { }

    void setMaskMode(bool m) { m_isMaskMode = m; }

    virtual IntSize maxTextureSize() const = 0;

    virtual PassRefPtr<BitmapTexture> acquireTextureFromPool(const IntSize&, const BitmapTexture::Flags = BitmapTexture::SupportsAlpha);

    void setPatternTransform(const TransformationMatrix& p) { m_patternTransform = p; }
    void setWrapMode(WrapMode m) { m_wrapMode = m; }

protected:
    GraphicsContext* m_context;
    std::unique_ptr<BitmapTexturePool> m_texturePool;

    bool isInMaskMode() const { return m_isMaskMode; }
    WrapMode wrapMode() const { return m_wrapMode; }
    const TransformationMatrix& patternTransform() const { return m_patternTransform; }

private:
#if USE(TEXTURE_MAPPER_GL)
    static std::unique_ptr<TextureMapper> platformCreateAccelerated();
#else
    static std::unique_ptr<TextureMapper> platformCreateAccelerated()
    {
        return nullptr;
    }
#endif
    InterpolationQuality m_interpolationQuality;
    TextDrawingModeFlags m_textDrawingMode;
    bool m_isMaskMode;
    TransformationMatrix m_patternTransform;
    WrapMode m_wrapMode;
};

}

#endif

#endif
