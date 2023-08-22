/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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
 *
 */

#ifndef FillLayer_h
#define FillLayer_h

#include "GraphicsTypes.h"
#include "LengthSize.h"
#include "RenderStyleConstants.h"
#include "StyleImage.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class RenderElement;

struct FillSize {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    FillSize()
        : type(SizeLength)
    {
    }

    FillSize(EFillSizeType type, const LengthSize& size)
        : type(type)
        , size(size)
    {
    }

    EFillSizeType type;
    LengthSize size;
};

inline bool operator==(const FillSize& a, const FillSize& b)
{
    return a.type == b.type && a.size == b.size;
}

inline bool operator!=(const FillSize& a, const FillSize& b)
{
    return !(a == b);
}

class FillLayer {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit FillLayer(EFillLayerType);
    ~FillLayer();

    StyleImage* image() const { return m_image.get(); }
    const Length& xPosition() const { return m_xPosition; }
    const Length& yPosition() const { return m_yPosition; }
    BackgroundEdgeOrigin backgroundXOrigin() const { return static_cast<BackgroundEdgeOrigin>(m_backgroundXOrigin); }
    BackgroundEdgeOrigin backgroundYOrigin() const { return static_cast<BackgroundEdgeOrigin>(m_backgroundYOrigin); }
    EFillAttachment attachment() const { return static_cast<EFillAttachment>(m_attachment); }
    EFillBox clip() const { return static_cast<EFillBox>(m_clip); }
    EFillBox origin() const { return static_cast<EFillBox>(m_origin); }
    EFillRepeat repeatX() const { return static_cast<EFillRepeat>(m_repeatX); }
    EFillRepeat repeatY() const { return static_cast<EFillRepeat>(m_repeatY); }
    CompositeOperator composite() const { return static_cast<CompositeOperator>(m_composite); }
    BlendMode blendMode() const { return static_cast<BlendMode>(m_blendMode); }
    const LengthSize& sizeLength() const { return m_sizeLength; }
    EFillSizeType sizeType() const { return static_cast<EFillSizeType>(m_sizeType); }
    FillSize size() const { return FillSize(static_cast<EFillSizeType>(m_sizeType), m_sizeLength); }
    EMaskSourceType maskSourceType() const { return static_cast<EMaskSourceType>(m_maskSourceType); }

    const FillLayer* next() const { return m_next.get(); }
    FillLayer* next() { return m_next.get(); }

    bool isImageSet() const { return m_imageSet; }
    bool isXPositionSet() const { return m_xPosSet; }
    bool isYPositionSet() const { return m_yPosSet; }
    bool isBackgroundOriginSet() const { return m_backgroundOriginSet; }
    bool isAttachmentSet() const { return m_attachmentSet; }
    bool isClipSet() const { return m_clipSet; }
    bool isOriginSet() const { return m_originSet; }
    bool isRepeatXSet() const { return m_repeatXSet; }
    bool isRepeatYSet() const { return m_repeatYSet; }
    bool isCompositeSet() const { return m_compositeSet; }
    bool isBlendModeSet() const { return m_blendModeSet; }
    bool isSizeSet() const { return m_sizeType != SizeNone; }
    bool isMaskSourceTypeSet() const { return m_maskSourceTypeSet; }

    void setImage(PassRefPtr<StyleImage> image) { m_image = image; m_imageSet = true; }
    void setXPosition(Length length) { m_xPosition = WTF::move(length); m_xPosSet = true; }
    void setYPosition(Length length) { m_yPosition = WTF::move(length); m_yPosSet = true; }
    void setBackgroundXOrigin(BackgroundEdgeOrigin o) { m_backgroundXOrigin = o; m_backgroundOriginSet = true; }
    void setBackgroundYOrigin(BackgroundEdgeOrigin o) { m_backgroundYOrigin = o; m_backgroundOriginSet = true; }
    void setAttachment(EFillAttachment attachment) { m_attachment = attachment; m_attachmentSet = true; }
    void setClip(EFillBox b) { m_clip = b; m_clipSet = true; }
    void setOrigin(EFillBox b) { m_origin = b; m_originSet = true; }
    void setRepeatX(EFillRepeat r) { m_repeatX = r; m_repeatXSet = true; }
    void setRepeatY(EFillRepeat r) { m_repeatY = r; m_repeatYSet = true; }
    void setComposite(CompositeOperator c) { m_composite = c; m_compositeSet = true; }
    void setBlendMode(BlendMode b) { m_blendMode = b; m_blendModeSet = true; }
    void setSizeType(EFillSizeType b) { m_sizeType = b; }
    void setSizeLength(LengthSize l) { m_sizeLength = l; }
    void setSize(FillSize f) { m_sizeType = f.type; m_sizeLength = f.size; }
    void setMaskSourceType(EMaskSourceType m) { m_maskSourceType = m; m_maskSourceTypeSet = true; }

    void clearImage() { m_image = nullptr; m_imageSet = false; }

    void clearXPosition() { m_xPosSet = false; m_backgroundOriginSet = false; }
    void clearYPosition() { m_yPosSet = false; m_backgroundOriginSet = false; }

    void clearAttachment() { m_attachmentSet = false; }
    void clearClip() { m_clipSet = false; }
    void clearOrigin() { m_originSet = false; }
    void clearRepeatX() { m_repeatXSet = false; }
    void clearRepeatY() { m_repeatYSet = false; }
    void clearComposite() { m_compositeSet = false; }
    void clearBlendMode() { m_blendModeSet = false; }
    void clearSize() { m_sizeType = SizeNone; }
    void clearMaskSourceType() { m_maskSourceTypeSet = false; }

    void setNext(std::unique_ptr<FillLayer> next) { m_next = WTF::move(next); }

    FillLayer& operator=(const FillLayer&);
    FillLayer(const FillLayer&);

    bool operator==(const FillLayer&) const;
    bool operator!=(const FillLayer& other) const { return !(*this == other); }

    bool containsImage(StyleImage&) const;
    bool imagesAreLoaded() const;
    bool hasImage() const;
    bool hasFixedImage() const;
    bool hasOpaqueImage(const RenderElement&) const;
    bool hasRepeatXY() const;
    bool clipOccludesNextLayers(bool firstLayer) const;

    EFillLayerType type() const { return static_cast<EFillLayerType>(m_type); }

    void fillUnsetProperties();
    void cullEmptyLayers();

    static bool imagesIdentical(const FillLayer*, const FillLayer*);

    static EFillAttachment initialFillAttachment(EFillLayerType) { return ScrollBackgroundAttachment; }
    static EFillBox initialFillClip(EFillLayerType) { return BorderFillBox; }
    static EFillBox initialFillOrigin(EFillLayerType type) { return type == BackgroundFillLayer ? PaddingFillBox : BorderFillBox; }
    static EFillRepeat initialFillRepeatX(EFillLayerType) { return RepeatFill; }
    static EFillRepeat initialFillRepeatY(EFillLayerType) { return RepeatFill; }
    static CompositeOperator initialFillComposite(EFillLayerType) { return CompositeSourceOver; }
    static BlendMode initialFillBlendMode(EFillLayerType) { return BlendModeNormal; }
    static FillSize initialFillSize(EFillLayerType) { return FillSize(); }
    static Length initialFillXPosition(EFillLayerType) { return Length(0.0f, Percent); }
    static Length initialFillYPosition(EFillLayerType) { return Length(0.0f, Percent); }
    static StyleImage* initialFillImage(EFillLayerType) { return nullptr; }
    static EMaskSourceType initialFillMaskSourceType(EFillLayerType) { return MaskAlpha; }

private:
    friend class RenderStyle;

    void computeClipMax() const;

    std::unique_ptr<FillLayer> m_next;

    RefPtr<StyleImage> m_image;

    Length m_xPosition;
    Length m_yPosition;

    LengthSize m_sizeLength;

    unsigned m_attachment : 2; // EFillAttachment
    unsigned m_clip : 2; // EFillBox
    unsigned m_origin : 2; // EFillBox
    unsigned m_repeatX : 3; // EFillRepeat
    unsigned m_repeatY : 3; // EFillRepeat
    unsigned m_composite : 4; // CompositeOperator
    unsigned m_sizeType : 2; // EFillSizeType
    unsigned m_blendMode : 5; // BlendMode
    unsigned m_maskSourceType : 1; // EMaskSourceType

    unsigned m_imageSet : 1;
    unsigned m_attachmentSet : 1;
    unsigned m_clipSet : 1;
    unsigned m_originSet : 1;
    unsigned m_repeatXSet : 1;
    unsigned m_repeatYSet : 1;
    unsigned m_xPosSet : 1;
    unsigned m_yPosSet : 1;
    unsigned m_backgroundOriginSet : 1;
    unsigned m_backgroundXOrigin : 2; // BackgroundEdgeOrigin
    unsigned m_backgroundYOrigin : 2; // BackgroundEdgeOrigin
    unsigned m_compositeSet : 1;
    unsigned m_blendModeSet : 1;
    unsigned m_maskSourceTypeSet : 1;

    unsigned m_type : 1; // EFillLayerType

    mutable unsigned m_clipMax : 2; // EFillBox, maximum m_clip value from this to bottom layer
};

} // namespace WebCore

#endif // FillLayer_h
