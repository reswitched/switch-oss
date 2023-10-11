/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "CSSToStyleMap.h"

#include "Animation.h"
#include "CSSAnimationTriggerScrollValue.h"
#include "CSSBorderImageSliceValue.h"
#include "CSSImageGeneratorValue.h"
#include "CSSImageSetValue.h"
#include "CSSImageValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSTimingFunctionValue.h"
#include "CSSValueKeywords.h"
#include "FillLayer.h"
#include "Pair.h"
#include "Rect.h"
#include "RenderView.h"
#include "StyleResolver.h"

namespace WebCore {

CSSToStyleMap::CSSToStyleMap(StyleResolver* resolver)
    : m_resolver(resolver)
{
}

RenderStyle* CSSToStyleMap::style() const
{
    return m_resolver->style();
}

RenderStyle* CSSToStyleMap::rootElementStyle() const
{
    return m_resolver->rootElementStyle();
}

bool CSSToStyleMap::useSVGZoomRules() const
{
    return m_resolver->useSVGZoomRules();
}

PassRefPtr<StyleImage> CSSToStyleMap::styleImage(CSSPropertyID propertyId, CSSValue& value)
{
    return m_resolver->styleImage(propertyId, value);
}

void CSSToStyleMap::mapFillAttachment(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setAttachment(FillLayer::initialFillAttachment(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).getValueID()) {
    case CSSValueFixed:
        layer.setAttachment(FixedBackgroundAttachment);
        break;
    case CSSValueScroll:
        layer.setAttachment(ScrollBackgroundAttachment);
        break;
    case CSSValueLocal:
        layer.setAttachment(LocalBackgroundAttachment);
        break;
    default:
        return;
    }
}

void CSSToStyleMap::mapFillClip(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setClip(FillLayer::initialFillClip(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setClip(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillComposite(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setComposite(FillLayer::initialFillComposite(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setComposite(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillBlendMode(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setBlendMode(FillLayer::initialFillBlendMode(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setBlendMode(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillOrigin(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setOrigin(FillLayer::initialFillOrigin(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setOrigin(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillImage(CSSPropertyID propertyID, FillLayer& layer, CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setImage(FillLayer::initialFillImage(layer.type()));
        return;
    }

    layer.setImage(styleImage(propertyID, value));
}

void CSSToStyleMap::mapFillRepeatX(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setRepeatX(FillLayer::initialFillRepeatX(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setRepeatX(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillRepeatY(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setRepeatY(FillLayer::initialFillRepeatY(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setRepeatY(downcast<CSSPrimitiveValue>(value));
}

static inline bool convertToLengthSize(const CSSPrimitiveValue& primitiveValue, CSSToLengthConversionData conversionData, LengthSize& size)
{
    if (auto* pair = primitiveValue.getPairValue()) {
        size.setWidth(pair->first()->convertToLength<AnyConversion>(conversionData));
        size.setHeight(pair->second()->convertToLength<AnyConversion>(conversionData));
    } else
        size.setWidth(primitiveValue.convertToLength<AnyConversion>(conversionData));

    return !size.width().isUndefined() && !size.height().isUndefined();
}

void CSSToStyleMap::mapFillSize(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setSize(FillLayer::initialFillSize(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    FillSize fillSize;
    switch (primitiveValue.getValueID()) {
    case CSSValueContain:
        fillSize.type = Contain;
        break;
    case CSSValueCover:
        fillSize.type = Cover;
        break;
    default:
        ASSERT(fillSize.type == SizeLength);
        if (!convertToLengthSize(primitiveValue, m_resolver->state().cssToLengthConversionData(), fillSize.size))
            return;
        break;
    }
    layer.setSize(fillSize);
}

void CSSToStyleMap::mapFillXPosition(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setXPosition(FillLayer::initialFillXPosition(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto* primitiveValue = &downcast<CSSPrimitiveValue>(value);
    Pair* pair = primitiveValue->getPairValue();
    if (pair) {
        ASSERT_UNUSED(propertyID, propertyID == CSSPropertyBackgroundPositionX || propertyID == CSSPropertyWebkitMaskPositionX);
        primitiveValue = pair->second();
    }

    Length length;
    if (primitiveValue->isLength())
        length = primitiveValue->computeLength<Length>(m_resolver->state().cssToLengthConversionData());
    else if (primitiveValue->isPercentage())
        length = Length(primitiveValue->getDoubleValue(), Percent);
    else if (primitiveValue->isCalculatedPercentageWithLength())
        length = Length(primitiveValue->cssCalcValue()->createCalculationValue(m_resolver->state().cssToLengthConversionData()));
    else
        return;

    layer.setXPosition(length);
    if (pair)
        layer.setBackgroundXOrigin(*pair->first());
}

void CSSToStyleMap::mapFillYPosition(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setYPosition(FillLayer::initialFillYPosition(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto* primitiveValue = &downcast<CSSPrimitiveValue>(value);
    Pair* pair = primitiveValue->getPairValue();
    if (pair) {
        ASSERT_UNUSED(propertyID, propertyID == CSSPropertyBackgroundPositionY || propertyID == CSSPropertyWebkitMaskPositionY);
        primitiveValue = pair->second();
    }

    Length length;
    if (primitiveValue->isLength())
        length = primitiveValue->computeLength<Length>(m_resolver->state().cssToLengthConversionData());
    else if (primitiveValue->isPercentage())
        length = Length(primitiveValue->getDoubleValue(), Percent);
    else if (primitiveValue->isCalculatedPercentageWithLength())
        length = Length(primitiveValue->cssCalcValue()->createCalculationValue(m_resolver->state().cssToLengthConversionData()));
    else
        return;

    layer.setYPosition(length);
    if (pair)
        layer.setBackgroundYOrigin(*pair->first());
}

void CSSToStyleMap::mapFillMaskSourceType(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    EMaskSourceType type = FillLayer::initialFillMaskSourceType(layer.type());
    if (value.treatAsInitialValue(propertyID)) {
        layer.setMaskSourceType(type);
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).getValueID()) {
    case CSSValueAlpha:
        type = EMaskSourceType::MaskAlpha;
        break;
    case CSSValueLuminance:
        type = EMaskSourceType::MaskLuminance;
        break;
    case CSSValueAuto:
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    layer.setMaskSourceType(type);
}

void CSSToStyleMap::mapAnimationDelay(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationDelay)) {
        animation.setDelay(Animation::initialDelay());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    animation.setDelay(downcast<CSSPrimitiveValue>(value).computeTime<double, CSSPrimitiveValue::Seconds>());
}

void CSSToStyleMap::mapAnimationDirection(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationDirection)) {
        layer.setDirection(Animation::initialDirection());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).getValueID()) {
    case CSSValueNormal:
        layer.setDirection(Animation::AnimationDirectionNormal);
        break;
    case CSSValueAlternate:
        layer.setDirection(Animation::AnimationDirectionAlternate);
        break;
    case CSSValueReverse:
        layer.setDirection(Animation::AnimationDirectionReverse);
        break;
    case CSSValueAlternateReverse:
        layer.setDirection(Animation::AnimationDirectionAlternateReverse);
        break;
    default:
        break;
    }
}

void CSSToStyleMap::mapAnimationDuration(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationDuration)) {
        animation.setDuration(Animation::initialDuration());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    animation.setDuration(downcast<CSSPrimitiveValue>(value).computeTime<double, CSSPrimitiveValue::Seconds>());
}

void CSSToStyleMap::mapAnimationFillMode(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationFillMode)) {
        layer.setFillMode(Animation::initialFillMode());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).getValueID()) {
    case CSSValueNone:
        layer.setFillMode(AnimationFillModeNone);
        break;
    case CSSValueForwards:
        layer.setFillMode(AnimationFillModeForwards);
        break;
    case CSSValueBackwards:
        layer.setFillMode(AnimationFillModeBackwards);
        break;
    case CSSValueBoth:
        layer.setFillMode(AnimationFillModeBoth);
        break;
    default:
        break;
    }
}

void CSSToStyleMap::mapAnimationIterationCount(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationIterationCount)) {
        animation.setIterationCount(Animation::initialIterationCount());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    if (primitiveValue.getValueID() == CSSValueInfinite)
        animation.setIterationCount(Animation::IterationCountInfinite);
    else
        animation.setIterationCount(primitiveValue.getFloatValue());
}

void CSSToStyleMap::mapAnimationName(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationName)) {
        layer.setName(Animation::initialName());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    if (primitiveValue.getValueID() == CSSValueNone)
        layer.setIsNoneAnimation(true);
    else
        layer.setName(primitiveValue.getStringValue());
}

void CSSToStyleMap::mapAnimationPlayState(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationPlayState)) {
        layer.setPlayState(Animation::initialPlayState());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    EAnimPlayState playState = (downcast<CSSPrimitiveValue>(value).getValueID() == CSSValuePaused) ? AnimPlayStatePaused : AnimPlayStatePlaying;
    layer.setPlayState(playState);
}

void CSSToStyleMap::mapAnimationProperty(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimation)) {
        animation.setAnimationMode(Animation::AnimateAll);
        animation.setProperty(CSSPropertyInvalid);
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    if (primitiveValue.getValueID() == CSSValueAll) {
        animation.setAnimationMode(Animation::AnimateAll);
        animation.setProperty(CSSPropertyInvalid);
    } else if (primitiveValue.getValueID() == CSSValueNone) {
        animation.setAnimationMode(Animation::AnimateNone);
        animation.setProperty(CSSPropertyInvalid);
    } else {
        animation.setAnimationMode(Animation::AnimateSingleProperty);
        animation.setProperty(primitiveValue.getPropertyID());
    }
}

void CSSToStyleMap::mapAnimationTimingFunction(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationTimingFunction)) {
        animation.setTimingFunction(Animation::initialTimingFunction());
        return;
    }

    if (is<CSSPrimitiveValue>(value)) {
        switch (downcast<CSSPrimitiveValue>(value).getValueID()) {
        case CSSValueLinear:
            animation.setTimingFunction(LinearTimingFunction::create());
            break;
        case CSSValueEase:
            animation.setTimingFunction(CubicBezierTimingFunction::create());
            break;
        case CSSValueEaseIn:
            animation.setTimingFunction(CubicBezierTimingFunction::create(CubicBezierTimingFunction::EaseIn));
            break;
        case CSSValueEaseOut:
            animation.setTimingFunction(CubicBezierTimingFunction::create(CubicBezierTimingFunction::EaseOut));
            break;
        case CSSValueEaseInOut:
            animation.setTimingFunction(CubicBezierTimingFunction::create(CubicBezierTimingFunction::EaseInOut));
            break;
        case CSSValueStepStart:
            animation.setTimingFunction(StepsTimingFunction::create(1, true));
            break;
        case CSSValueStepEnd:
            animation.setTimingFunction(StepsTimingFunction::create(1, false));
            break;
        default:
            break;
        }
        return;
    }

    if (is<CSSCubicBezierTimingFunctionValue>(value)) {
        auto& cubicTimingFunction = downcast<CSSCubicBezierTimingFunctionValue>(value);
        animation.setTimingFunction(CubicBezierTimingFunction::create(cubicTimingFunction.x1(), cubicTimingFunction.y1(), cubicTimingFunction.x2(), cubicTimingFunction.y2()));
    } else if (is<CSSStepsTimingFunctionValue>(value)) {
        auto& stepsTimingFunction = downcast<CSSStepsTimingFunctionValue>(value);
        animation.setTimingFunction(StepsTimingFunction::create(stepsTimingFunction.numberOfSteps(), stepsTimingFunction.stepAtStart()));
    }
}

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
void CSSToStyleMap::mapAnimationTrigger(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyWebkitAnimationTrigger)) {
        animation.setTrigger(Animation::initialTrigger());
        return;
    }

    if (value.isPrimitiveValue()) {
        auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
        if (primitiveValue.getValueID() == CSSValueAuto)
            animation.setTrigger(AutoAnimationTrigger::create());
        return;
    }

    if (value.isAnimationTriggerScrollValue()) {
        auto& scrollTrigger = downcast<CSSAnimationTriggerScrollValue>(value);

        const CSSPrimitiveValue* startValue = downcast<CSSPrimitiveValue>(scrollTrigger.startValue());
        Length startLength = startValue->computeLength<Length>(m_resolver->state().cssToLengthConversionData());

        Length endLength;
        if (scrollTrigger.hasEndValue()) {
            const CSSPrimitiveValue* endValue = downcast<CSSPrimitiveValue>(scrollTrigger.endValue());
            endLength = endValue->computeLength<Length>(m_resolver->state().cssToLengthConversionData());
        }

        animation.setTrigger(ScrollAnimationTrigger::create(startLength, endLength));
    }
}
#endif

void CSSToStyleMap::mapNinePieceImage(CSSPropertyID property, CSSValue* value, NinePieceImage& image)
{
    // If we're not a value list, then we are "none" and don't need to alter the empty image at all.
    if (!is<CSSValueList>(value))
        return;

    // Retrieve the border image value.
    CSSValueList& borderImage = downcast<CSSValueList>(*value);

    // Set the image (this kicks off the load).
    CSSPropertyID imageProperty;
    if (property == CSSPropertyWebkitBorderImage)
        imageProperty = CSSPropertyBorderImageSource;
    else if (property == CSSPropertyWebkitMaskBoxImage)
        imageProperty = CSSPropertyWebkitMaskBoxImageSource;
    else
        imageProperty = property;

    for (auto& current : borderImage) {
        if (is<CSSImageValue>(current.get()) || is<CSSImageGeneratorValue>(current.get())
#if ENABLE(CSS_IMAGE_SET)
            || is<CSSImageSetValue>(current.get())
#endif
            )
            image.setImage(styleImage(imageProperty, current.get()));
        else if (is<CSSBorderImageSliceValue>(current.get()))
            mapNinePieceImageSlice(current, image);
        else if (is<CSSValueList>(current.get())) {
            CSSValueList& slashList = downcast<CSSValueList>(current.get());
            // Map in the image slices.
            if (is<CSSBorderImageSliceValue>(slashList.item(0)))
                mapNinePieceImageSlice(*slashList.item(0), image);

            // Map in the border slices.
            if (slashList.item(1))
                image.setBorderSlices(mapNinePieceImageQuad(*slashList.item(1)));

            // Map in the outset.
            if (slashList.item(2))
                image.setOutset(mapNinePieceImageQuad(*slashList.item(2)));
        } else if (is<CSSPrimitiveValue>(current.get())) {
            // Set the appropriate rules for stretch/round/repeat of the slices.
            mapNinePieceImageRepeat(current, image);
        }
    }

    if (property == CSSPropertyWebkitBorderImage) {
        // We have to preserve the legacy behavior of -webkit-border-image and make the border slices
        // also set the border widths. We don't need to worry about percentages, since we don't even support
        // those on real borders yet.
        if (image.borderSlices().top().isFixed())
            style()->setBorderTopWidth(image.borderSlices().top().value());
        if (image.borderSlices().right().isFixed())
            style()->setBorderRightWidth(image.borderSlices().right().value());
        if (image.borderSlices().bottom().isFixed())
            style()->setBorderBottomWidth(image.borderSlices().bottom().value());
        if (image.borderSlices().left().isFixed())
            style()->setBorderLeftWidth(image.borderSlices().left().value());
    }
}

void CSSToStyleMap::mapNinePieceImageSlice(CSSValue& value, NinePieceImage& image)
{
    if (!is<CSSBorderImageSliceValue>(value))
        return;

    // Retrieve the border image value.
    auto& borderImageSlice = downcast<CSSBorderImageSliceValue>(value);

    // Set up a length box to represent our image slices.
    LengthBox box;
    Quad* slices = borderImageSlice.slices();
    if (slices->top()->isPercentage())
        box.top() = Length(slices->top()->getDoubleValue(), Percent);
    else
        box.top() = Length(slices->top()->getIntValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    if (slices->bottom()->isPercentage())
        box.bottom() = Length(slices->bottom()->getDoubleValue(), Percent);
    else
        box.bottom() = Length((int)slices->bottom()->getFloatValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    if (slices->left()->isPercentage())
        box.left() = Length(slices->left()->getDoubleValue(), Percent);
    else
        box.left() = Length(slices->left()->getIntValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    if (slices->right()->isPercentage())
        box.right() = Length(slices->right()->getDoubleValue(), Percent);
    else
        box.right() = Length(slices->right()->getIntValue(CSSPrimitiveValue::CSS_NUMBER), Fixed);
    image.setImageSlices(box);

    // Set our fill mode.
    image.setFill(borderImageSlice.m_fill);
}

LengthBox CSSToStyleMap::mapNinePieceImageQuad(CSSValue& value)
{
    if (!is<CSSPrimitiveValue>(value))
        return LengthBox();

    // Get our zoom value.
    CSSToLengthConversionData conversionData = useSVGZoomRules() ? m_resolver->state().cssToLengthConversionData().copyWithAdjustedZoom(1.0f) : m_resolver->state().cssToLengthConversionData();

    // Retrieve the primitive value.
    auto& borderWidths = downcast<CSSPrimitiveValue>(value);

    // Set up a length box to represent our image slices.
    LengthBox box; // Defaults to 'auto' so we don't have to handle that explicitly below.
    Quad* slices = borderWidths.getQuadValue();
    if (slices->top()->isNumber())
        box.top() = Length(slices->top()->getIntValue(), Relative);
    else if (slices->top()->isPercentage())
        box.top() = Length(slices->top()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->top()->getValueID() != CSSValueAuto)
        box.top() = slices->top()->computeLength<Length>(conversionData);

    if (slices->right()->isNumber())
        box.right() = Length(slices->right()->getIntValue(), Relative);
    else if (slices->right()->isPercentage())
        box.right() = Length(slices->right()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->right()->getValueID() != CSSValueAuto)
        box.right() = slices->right()->computeLength<Length>(conversionData);

    if (slices->bottom()->isNumber())
        box.bottom() = Length(slices->bottom()->getIntValue(), Relative);
    else if (slices->bottom()->isPercentage())
        box.bottom() = Length(slices->bottom()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->bottom()->getValueID() != CSSValueAuto)
        box.bottom() = slices->bottom()->computeLength<Length>(conversionData);

    if (slices->left()->isNumber())
        box.left() = Length(slices->left()->getIntValue(), Relative);
    else if (slices->left()->isPercentage())
        box.left() = Length(slices->left()->getDoubleValue(CSSPrimitiveValue::CSS_PERCENTAGE), Percent);
    else if (slices->left()->getValueID() != CSSValueAuto)
        box.left() = slices->left()->computeLength<Length>(conversionData);

    return box;
}

void CSSToStyleMap::mapNinePieceImageRepeat(CSSValue& value, NinePieceImage& image)
{
    if (!is<CSSPrimitiveValue>(value))
        return;

    CSSPrimitiveValue& primitiveValue = downcast<CSSPrimitiveValue>(value);
    Pair* pair = primitiveValue.getPairValue();
    if (!pair || !pair->first() || !pair->second())
        return;

    CSSValueID firstIdentifier = pair->first()->getValueID();
    CSSValueID secondIdentifier = pair->second()->getValueID();

    ENinePieceImageRule horizontalRule;
    switch (firstIdentifier) {
    case CSSValueStretch:
        horizontalRule = StretchImageRule;
        break;
    case CSSValueRound:
        horizontalRule = RoundImageRule;
        break;
    case CSSValueSpace:
        horizontalRule = SpaceImageRule;
        break;
    default: // CSSValueRepeat
        horizontalRule = RepeatImageRule;
        break;
    }
    image.setHorizontalRule(horizontalRule);

    ENinePieceImageRule verticalRule;
    switch (secondIdentifier) {
    case CSSValueStretch:
        verticalRule = StretchImageRule;
        break;
    case CSSValueRound:
        verticalRule = RoundImageRule;
        break;
    case CSSValueSpace:
        verticalRule = SpaceImageRule;
        break;
    default: // CSSValueRepeat
        verticalRule = RepeatImageRule;
        break;
    }
    image.setVerticalRule(verticalRule);
}

};
