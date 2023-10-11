/*
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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

#ifndef RenderSVGImage_h
#define RenderSVGImage_h

#include "AffineTransform.h"
#include "FloatRect.h"
#include "RenderSVGModelObject.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore {

class RenderImageResource;
class SVGImageElement;

class RenderSVGImage final : public RenderSVGModelObject {
public:
    RenderSVGImage(SVGImageElement&, Ref<RenderStyle>&&);
    virtual ~RenderSVGImage();

    SVGImageElement& imageElement() const;

    bool updateImageViewport();
    virtual void setNeedsBoundariesUpdate() override { m_needsBoundariesUpdate = true; }
    virtual bool needsBoundariesUpdate() override { return m_needsBoundariesUpdate; }
    virtual void setNeedsTransformUpdate() override { m_needsTransformUpdate = true; }

    RenderImageResource& imageResource() { return *m_imageResource; }
    const RenderImageResource& imageResource() const { return *m_imageResource; }

    // Note: Assumes the PaintInfo context has had all local transforms applied.
    void paintForeground(PaintInfo&);

private:
    void element() const  = delete;

    virtual const char* renderName() const override { return "RenderSVGImage"; }
    virtual bool isSVGImage() const override { return true; }
    virtual bool canHaveChildren() const override { return false; }

    virtual const AffineTransform& localToParentTransform() const override { return m_localTransform; }

    virtual FloatRect objectBoundingBox() const override { return m_objectBoundingBox; }
    virtual FloatRect strokeBoundingBox() const override { return m_objectBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinates() const override { return m_repaintBoundingBox; }
    virtual FloatRect repaintRectInLocalCoordinatesExcludingSVGShadow() const override { return m_repaintBoundingBoxExcludingShadow; }

    virtual void addFocusRingRects(Vector<IntRect>&, const LayoutPoint& additionalOffset, const RenderLayerModelObject* paintContainer = 0) override;

    virtual void imageChanged(WrappedImagePtr, const IntRect* = nullptr) override;

    virtual void layout() override;
    virtual void paint(PaintInfo&, const LayoutPoint&) override;

    void invalidateBufferedForeground();

    virtual bool nodeAtFloatPoint(const HitTestRequest&, HitTestResult&, const FloatPoint& pointInParent, HitTestAction) override;

    virtual AffineTransform localTransform() const override { return m_localTransform; }
    void calculateImageViewport();

    bool m_needsBoundariesUpdate : 1;
    bool m_needsTransformUpdate : 1;
    AffineTransform m_localTransform;
    FloatRect m_objectBoundingBox;
    FloatRect m_repaintBoundingBox;
    FloatRect m_repaintBoundingBoxExcludingShadow;
    std::unique_ptr<RenderImageResource> m_imageResource;
    std::unique_ptr<ImageBuffer> m_bufferedForeground;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderSVGImage, isSVGImage())

#endif // RenderSVGImage_h
