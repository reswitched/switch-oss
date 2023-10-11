/*
 * Copyright (C) 2007, 2008, 2009, 2013 Apple Inc.
 * Copyright (C) 2010, 2011 Google Inc. All rights reserved.
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

#include "config.h"
#include "EditingStyle.h"

#include "ApplyStyleCommand.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#include "Editor.h"
#include "Frame.h"
#include "HTMLFontElement.h"
#include "HTMLInterchange.h"
#include "HTMLNames.h"
#include "Node.h"
#include "NodeTraversal.h"
#include "QualifiedName.h"
#include "RenderStyle.h"
#include "StyleFontSizeFunctions.h"
#include "StyleProperties.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "StyledElement.h"
#include "VisibleUnits.h"
#include "htmlediting.h"

namespace WebCore {

// Editing style properties must be preserved during editing operation.
// e.g. when a user inserts a new paragraph, all properties listed here must be copied to the new paragraph.
static const CSSPropertyID editingProperties[] = {
    CSSPropertyColor,
    CSSPropertyFontFamily,
    CSSPropertyFontSize,
    CSSPropertyFontStyle,
    CSSPropertyFontVariantCaps,
    CSSPropertyFontWeight,
    CSSPropertyLetterSpacing,
    CSSPropertyOrphans,
    CSSPropertyTextAlign,
    CSSPropertyTextIndent,
    CSSPropertyTextTransform,
    CSSPropertyWhiteSpace,
    CSSPropertyWidows,
    CSSPropertyWordSpacing,
#if ENABLE(TOUCH_EVENTS)
    CSSPropertyWebkitTapHighlightColor,
#endif
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyWebkitTextFillColor,
#if ENABLE(IOS_TEXT_AUTOSIZING)
    CSSPropertyWebkitTextSizeAdjust,
#endif
    CSSPropertyWebkitTextStrokeColor,
    CSSPropertyWebkitTextStrokeWidth,

    // Non-inheritable properties
    CSSPropertyBackgroundColor,
    CSSPropertyTextDecoration,
};

const unsigned numAllEditingProperties = WTF_ARRAY_LENGTH(editingProperties);
const unsigned numInheritableEditingProperties = numAllEditingProperties - 2;

enum EditingPropertiesToInclude { OnlyInheritableEditingProperties, AllEditingProperties };
template <class StyleDeclarationType>
static PassRefPtr<MutableStyleProperties> copyEditingProperties(StyleDeclarationType* style, EditingPropertiesToInclude type)
{
    if (type == AllEditingProperties)
        return style->copyPropertiesInSet(editingProperties, numAllEditingProperties);
    return style->copyPropertiesInSet(editingProperties, numInheritableEditingProperties);
}

static inline bool isEditingProperty(int id)
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(editingProperties); ++i) {
        if (editingProperties[i] == id)
            return true;
    }
    return false;
}

static PassRefPtr<MutableStyleProperties> copyPropertiesFromComputedStyle(ComputedStyleExtractor& computedStyle, EditingStyle::PropertiesToInclude propertiesToInclude)
{
    switch (propertiesToInclude) {
    case EditingStyle::AllProperties:
        return computedStyle.copyProperties();
    case EditingStyle::OnlyEditingInheritableProperties:
        return copyEditingProperties(&computedStyle, OnlyInheritableEditingProperties);
    case EditingStyle::EditingPropertiesInEffect:
        return copyEditingProperties(&computedStyle, AllEditingProperties);
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static PassRefPtr<MutableStyleProperties> copyPropertiesFromComputedStyle(Node* node, EditingStyle::PropertiesToInclude propertiesToInclude)
{
    ComputedStyleExtractor computedStyle(node);
    return copyPropertiesFromComputedStyle(computedStyle, propertiesToInclude);
}

static PassRefPtr<CSSValue> extractPropertyValue(const StyleProperties& style, CSSPropertyID propertyID)
{
    return style.getPropertyCSSValue(propertyID);
}

static PassRefPtr<CSSValue> extractPropertyValue(ComputedStyleExtractor& computedStyle, CSSPropertyID propertyID)
{
    return computedStyle.propertyValue(propertyID);
}

template<typename T>
int identifierForStyleProperty(T& style, CSSPropertyID propertyID)
{
    RefPtr<CSSValue> value = extractPropertyValue(style, propertyID);
    if (!is<CSSPrimitiveValue>(value.get()))
        return 0;
    return downcast<CSSPrimitiveValue>(*value).getValueID();
}

template<typename T> PassRefPtr<MutableStyleProperties> getPropertiesNotIn(StyleProperties& styleWithRedundantProperties, T& baseStyle);
enum LegacyFontSizeMode { AlwaysUseLegacyFontSize, UseLegacyFontSizeOnlyIfPixelValuesMatch };
static int legacyFontSizeFromCSSValue(Document*, CSSPrimitiveValue*, bool shouldUseFixedFontDefaultSize, LegacyFontSizeMode);
static bool isTransparentColorValue(CSSValue*);
static bool hasTransparentBackgroundColor(StyleProperties*);
static PassRefPtr<CSSValue> backgroundColorInEffect(Node*);

class HTMLElementEquivalent {
    WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLElementEquivalent(CSSPropertyID, CSSValueID primitiveValue, const QualifiedName& tagName);

    virtual ~HTMLElementEquivalent() { }
    virtual bool matches(const Element& element) const { return !m_tagName || element.hasTagName(*m_tagName); }
    virtual bool hasAttribute() const { return false; }
    virtual bool propertyExistsInStyle(const EditingStyle& style) const { return style.m_mutableStyle && style.m_mutableStyle->getPropertyCSSValue(m_propertyID); }
    virtual bool valueIsPresentInStyle(Element&, const EditingStyle&) const;
    virtual void addToStyle(Element*, EditingStyle*) const;

protected:
    HTMLElementEquivalent(CSSPropertyID);
    HTMLElementEquivalent(CSSPropertyID, const QualifiedName& tagName);
    const CSSPropertyID m_propertyID;
    const RefPtr<CSSPrimitiveValue> m_primitiveValue;
    const QualifiedName* m_tagName; // We can store a pointer because HTML tag names are const global.
};

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id)
    : m_propertyID(id)
    , m_tagName(0)
{
}

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id, const QualifiedName& tagName)
    : m_propertyID(id)
    , m_tagName(&tagName)
{
}

HTMLElementEquivalent::HTMLElementEquivalent(CSSPropertyID id, CSSValueID primitiveValue, const QualifiedName& tagName)
    : m_propertyID(id)
    , m_primitiveValue(CSSPrimitiveValue::createIdentifier(primitiveValue))
    , m_tagName(&tagName)
{
    ASSERT(primitiveValue != CSSValueInvalid);
}

bool HTMLElementEquivalent::valueIsPresentInStyle(Element& element, const EditingStyle& style) const
{
    RefPtr<CSSValue> value = style.m_mutableStyle->getPropertyCSSValue(m_propertyID);
    return matches(element) && is<CSSPrimitiveValue>(value.get()) && downcast<CSSPrimitiveValue>(*value).getValueID() == m_primitiveValue->getValueID();
}

void HTMLElementEquivalent::addToStyle(Element*, EditingStyle* style) const
{
    style->setProperty(m_propertyID, m_primitiveValue->cssText());
}

class HTMLTextDecorationEquivalent : public HTMLElementEquivalent {
public:
    HTMLTextDecorationEquivalent(CSSValueID primitiveValue, const QualifiedName& tagName)
        : HTMLElementEquivalent(CSSPropertyTextDecoration, primitiveValue, tagName)
        , m_isUnderline(primitiveValue == CSSValueUnderline)
    {
    }

    bool propertyExistsInStyle(const EditingStyle& style) const override
    {
        if (changeInStyle(style) != TextDecorationChange::None)
            return true;

        if (!style.m_mutableStyle)
            return false;

        auto& mutableStyle = *style.m_mutableStyle;
        return mutableStyle.getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect)
            || mutableStyle.getPropertyCSSValue(CSSPropertyTextDecoration);
    }

    bool valueIsPresentInStyle(Element& element, const EditingStyle& style) const override
    {
        if (!matches(element))
            return false;
        auto change = changeInStyle(style);
        if (change != TextDecorationChange::None)
            return change == TextDecorationChange::Add;
        RefPtr<CSSValue> styleValue = style.m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
        if (!styleValue)
            styleValue = style.m_mutableStyle->getPropertyCSSValue(CSSPropertyTextDecoration);
        return is<CSSValueList>(styleValue.get()) && downcast<CSSValueList>(*styleValue).hasValue(m_primitiveValue.get());
    }

private:
    TextDecorationChange changeInStyle(const EditingStyle& style) const
    {
        return m_isUnderline ? style.underlineChange() : style.strikeThroughChange();
    }

    bool m_isUnderline;
};

class HTMLAttributeEquivalent : public HTMLElementEquivalent {
public:
    HTMLAttributeEquivalent(CSSPropertyID, const QualifiedName& tagName, const QualifiedName& attrName);
    HTMLAttributeEquivalent(CSSPropertyID, const QualifiedName& attrName);

    bool matches(const Element& element) const { return HTMLElementEquivalent::matches(element) && element.hasAttribute(m_attrName); }
    virtual bool hasAttribute() const { return true; }
    virtual bool valueIsPresentInStyle(Element&, const EditingStyle&) const;
    virtual void addToStyle(Element*, EditingStyle*) const;
    virtual PassRefPtr<CSSValue> attributeValueAsCSSValue(Element*) const;
    inline const QualifiedName& attributeName() const { return m_attrName; }

protected:
    const QualifiedName& m_attrName; // We can store a reference because HTML attribute names are const global.
};

HTMLAttributeEquivalent::HTMLAttributeEquivalent(CSSPropertyID id, const QualifiedName& tagName, const QualifiedName& attrName)
    : HTMLElementEquivalent(id, tagName)
    , m_attrName(attrName)
{
}

HTMLAttributeEquivalent::HTMLAttributeEquivalent(CSSPropertyID id, const QualifiedName& attrName)
    : HTMLElementEquivalent(id)
    , m_attrName(attrName)
{
}

bool HTMLAttributeEquivalent::valueIsPresentInStyle(Element& element, const EditingStyle& style) const
{
    RefPtr<CSSValue> value = attributeValueAsCSSValue(&element);
    RefPtr<CSSValue> styleValue = style.m_mutableStyle->getPropertyCSSValue(m_propertyID);
    
    return compareCSSValuePtr(value, styleValue);
}

void HTMLAttributeEquivalent::addToStyle(Element* element, EditingStyle* style) const
{
    if (RefPtr<CSSValue> value = attributeValueAsCSSValue(element))
        style->setProperty(m_propertyID, value->cssText());
}

PassRefPtr<CSSValue> HTMLAttributeEquivalent::attributeValueAsCSSValue(Element* element) const
{
    ASSERT(element);
    const AtomicString& value = element->getAttribute(m_attrName);
    if (value.isNull())
        return nullptr;
    
    RefPtr<MutableStyleProperties> dummyStyle;
    dummyStyle = MutableStyleProperties::create();
    dummyStyle->setProperty(m_propertyID, value);
    return dummyStyle->getPropertyCSSValue(m_propertyID);
}

class HTMLFontSizeEquivalent : public HTMLAttributeEquivalent {
public:
    HTMLFontSizeEquivalent();

    virtual PassRefPtr<CSSValue> attributeValueAsCSSValue(Element*) const;
};

HTMLFontSizeEquivalent::HTMLFontSizeEquivalent()
    : HTMLAttributeEquivalent(CSSPropertyFontSize, HTMLNames::fontTag, HTMLNames::sizeAttr)
{
}

PassRefPtr<CSSValue> HTMLFontSizeEquivalent::attributeValueAsCSSValue(Element* element) const
{
    ASSERT(element);
    const AtomicString& value = element->getAttribute(m_attrName);
    if (value.isNull())
        return nullptr;
    CSSValueID size;
    if (!HTMLFontElement::cssValueFromFontSizeNumber(value, size))
        return nullptr;
    return CSSPrimitiveValue::createIdentifier(size);
}

#if !PLATFORM(WKC)
float EditingStyle::NoFontDelta = 0.0f;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(float, EditingStyle, NoFontDelta, 0.0f);
#endif

EditingStyle::EditingStyle()
    : m_shouldUseFixedDefaultFontSize(false)
    , m_underlineChange(static_cast<unsigned>(TextDecorationChange::None))
    , m_strikeThroughChange(static_cast<unsigned>(TextDecorationChange::None))
{
}

EditingStyle::EditingStyle(Node* node, PropertiesToInclude propertiesToInclude)
    : EditingStyle()
{
    init(node, propertiesToInclude);
}

EditingStyle::EditingStyle(const Position& position, PropertiesToInclude propertiesToInclude)
    : EditingStyle()
{
    init(position.deprecatedNode(), propertiesToInclude);
}

EditingStyle::EditingStyle(const CSSStyleDeclaration* style)
    : EditingStyle()
{
    if (style)
        m_mutableStyle = style->copyProperties();
    extractFontSizeDelta();
}

EditingStyle::EditingStyle(const StyleProperties* style)
    : EditingStyle()
{
    if (style)
        m_mutableStyle = style->mutableCopy();
    extractFontSizeDelta();
}

EditingStyle::EditingStyle(CSSPropertyID propertyID, const String& value)
    : EditingStyle()
{
    setProperty(propertyID, value);
    extractFontSizeDelta();
}

EditingStyle::EditingStyle(CSSPropertyID propertyID, CSSValueID value)
    : EditingStyle()
{
    m_mutableStyle = MutableStyleProperties::create();
    m_mutableStyle->setProperty(propertyID, value);
    extractFontSizeDelta();
}

EditingStyle::~EditingStyle()
{
}

static RGBA32 cssValueToRGBA(CSSValue* colorValue)
{
    if (!is<CSSPrimitiveValue>(colorValue))
        return Color::transparent;
    
    CSSPrimitiveValue& primitiveColor = downcast<CSSPrimitiveValue>(*colorValue);
    if (primitiveColor.isRGBColor())
        return primitiveColor.getRGBA32Value();
    
    RGBA32 rgba = 0;
    CSSParser::parseColor(rgba, colorValue->cssText());
    return rgba;
}

template<typename T>
static inline RGBA32 textColorFromStyle(T& style)
{
    return cssValueToRGBA(extractPropertyValue(style, CSSPropertyColor).get());
}

template<typename T>
static inline RGBA32 backgroundColorFromStyle(T& style)
{
    return cssValueToRGBA(extractPropertyValue(style, CSSPropertyBackgroundColor).get());
}

static inline RGBA32 rgbaBackgroundColorInEffect(Node* node)
{
    return cssValueToRGBA(backgroundColorInEffect(node).get());
}

static int textAlignResolvingStartAndEnd(int textAlign, int direction)
{
    switch (textAlign) {
    case CSSValueCenter:
    case CSSValueWebkitCenter:
        return CSSValueCenter;
    case CSSValueJustify:
        return CSSValueJustify;
    case CSSValueLeft:
    case CSSValueWebkitLeft:
        return CSSValueLeft;
    case CSSValueRight:
    case CSSValueWebkitRight:
        return CSSValueRight;
    case CSSValueStart:
        return direction != CSSValueRtl ? CSSValueLeft : CSSValueRight;
    case CSSValueEnd:
        return direction == CSSValueRtl ? CSSValueRight : CSSValueLeft;
    }
    return CSSValueInvalid;
}

template<typename T>
static int textAlignResolvingStartAndEnd(T& style)
{
    return textAlignResolvingStartAndEnd(identifierForStyleProperty(style, CSSPropertyTextAlign), identifierForStyleProperty(style, CSSPropertyDirection));
}

void EditingStyle::init(Node* node, PropertiesToInclude propertiesToInclude)
{
    if (isTabSpanTextNode(node))
        node = tabSpanNode(node)->parentNode();
    else if (isTabSpanNode(node))
        node = node->parentNode();

    ComputedStyleExtractor computedStyleAtPosition(node);
    // FIXME: It's strange to not set background-color and text-decoration when propertiesToInclude is EditingPropertiesInEffect.
    // However editing/selection/contains-boundaries.html fails without this ternary.
    m_mutableStyle = copyPropertiesFromComputedStyle(computedStyleAtPosition,
        propertiesToInclude == EditingPropertiesInEffect ? OnlyEditingInheritableProperties : propertiesToInclude);

    if (propertiesToInclude == EditingPropertiesInEffect) {
        if (RefPtr<CSSValue> value = backgroundColorInEffect(node))
            m_mutableStyle->setProperty(CSSPropertyBackgroundColor, value->cssText());
        if (RefPtr<CSSValue> value = computedStyleAtPosition.propertyValue(CSSPropertyWebkitTextDecorationsInEffect))
            m_mutableStyle->setProperty(CSSPropertyTextDecoration, value->cssText());
    }

    if (node && node->computedStyle()) {
        RenderStyle* renderStyle = node->computedStyle();
        removeTextFillAndStrokeColorsIfNeeded(renderStyle);
        if (renderStyle->fontDescription().keywordSize())
            m_mutableStyle->setProperty(CSSPropertyFontSize, computedStyleAtPosition.getFontSizeCSSValuePreferringKeyword()->cssText());
    }

    m_shouldUseFixedDefaultFontSize = computedStyleAtPosition.useFixedFontDefaultSize();
    extractFontSizeDelta();
}

void EditingStyle::removeTextFillAndStrokeColorsIfNeeded(RenderStyle* renderStyle)
{
    // If a node's text fill color is invalid, then its children use 
    // their font-color as their text fill color (they don't
    // inherit it).  Likewise for stroke color.
    if (!renderStyle->textFillColor().isValid())
        m_mutableStyle->removeProperty(CSSPropertyWebkitTextFillColor);
    if (!renderStyle->textStrokeColor().isValid())
        m_mutableStyle->removeProperty(CSSPropertyWebkitTextStrokeColor);
}

void EditingStyle::setProperty(CSSPropertyID propertyID, const String& value, bool important)
{
    if (!m_mutableStyle)
        m_mutableStyle = MutableStyleProperties::create();

    m_mutableStyle->setProperty(propertyID, value, important);
}

void EditingStyle::extractFontSizeDelta()
{
    if (!m_mutableStyle)
        return;

    if (m_mutableStyle->getPropertyCSSValue(CSSPropertyFontSize)) {
        // Explicit font size overrides any delta.
        m_mutableStyle->removeProperty(CSSPropertyWebkitFontSizeDelta);
        return;
    }

    // Get the adjustment amount out of the style.
    RefPtr<CSSValue> value = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitFontSizeDelta);
    if (!is<CSSPrimitiveValue>(value.get()))
        return;

    CSSPrimitiveValue& primitiveValue = downcast<CSSPrimitiveValue>(*value);

    // Only PX handled now. If we handle more types in the future, perhaps
    // a switch statement here would be more appropriate.
    if (!primitiveValue.isPx())
        return;

    m_fontSizeDelta = primitiveValue.getFloatValue();
    m_mutableStyle->removeProperty(CSSPropertyWebkitFontSizeDelta);
}

bool EditingStyle::isEmpty() const
{
    return (!m_mutableStyle || m_mutableStyle->isEmpty()) && m_fontSizeDelta == NoFontDelta
        && underlineChange() == TextDecorationChange::None && strikeThroughChange() == TextDecorationChange::None;
}

Ref<MutableStyleProperties> EditingStyle::styleWithResolvedTextDecorations() const
{
    bool hasTextDecorationChanges = underlineChange() != TextDecorationChange::None || strikeThroughChange() != TextDecorationChange::None;
    if (m_mutableStyle && !hasTextDecorationChanges)
        return *m_mutableStyle;

    Ref<MutableStyleProperties> style = m_mutableStyle ? m_mutableStyle->mutableCopy() : MutableStyleProperties::create();

    Ref<CSSValueList> valueList = CSSValueList::createSpaceSeparated();
    if (underlineChange() == TextDecorationChange::Add)
        valueList->append(cssValuePool().createIdentifierValue(CSSValueUnderline));
    if (strikeThroughChange() == TextDecorationChange::Add)
        valueList->append(cssValuePool().createIdentifierValue(CSSValueLineThrough));

    if (valueList->length())
        style->setProperty(CSSPropertyTextDecoration, valueList.ptr());
    else
        style->setProperty(CSSPropertyTextDecoration, cssValuePool().createIdentifierValue(CSSValueNone));

    return style;
}

bool EditingStyle::textDirection(WritingDirection& writingDirection) const
{
    if (!m_mutableStyle)
        return false;

    RefPtr<CSSValue> unicodeBidi = m_mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi);
    if (!is<CSSPrimitiveValue>(unicodeBidi.get()))
        return false;

    CSSValueID unicodeBidiValue = downcast<CSSPrimitiveValue>(*unicodeBidi).getValueID();
    if (unicodeBidiValue == CSSValueEmbed) {
        RefPtr<CSSValue> direction = m_mutableStyle->getPropertyCSSValue(CSSPropertyDirection);
        if (!is<CSSPrimitiveValue>(direction.get()))
            return false;

        writingDirection = downcast<CSSPrimitiveValue>(*direction).getValueID() == CSSValueLtr ? LeftToRightWritingDirection : RightToLeftWritingDirection;

        return true;
    }

    if (unicodeBidiValue == CSSValueNormal) {
        writingDirection = NaturalWritingDirection;
        return true;
    }

    return false;
}

void EditingStyle::setStyle(PassRefPtr<MutableStyleProperties> style)
{
    m_mutableStyle = style;
    // FIXME: We should be able to figure out whether or not font is fixed width for mutable style.
    // We need to check font-family is monospace as in FontDescription but we don't want to duplicate code here.
    m_shouldUseFixedDefaultFontSize = false;
    extractFontSizeDelta();
}

void EditingStyle::overrideWithStyle(const StyleProperties* style)
{
    return mergeStyle(style, OverrideValues);
}

static void applyTextDecorationChangeToValueList(CSSValueList& valueList, TextDecorationChange change, Ref<CSSPrimitiveValue>&& value)
{
    switch (change) {
    case TextDecorationChange::None:
        break;
    case TextDecorationChange::Add:
        valueList.append(WTF::move(value));
        break;
    case TextDecorationChange::Remove:
        valueList.removeAll(&value.get());
        break;
    }
}

void EditingStyle::overrideTypingStyleAt(const EditingStyle& style, const Position& position)
{
    mergeStyle(style.m_mutableStyle.get(), OverrideValues);

    m_fontSizeDelta += style.m_fontSizeDelta;

    prepareToApplyAt(position, EditingStyle::PreserveWritingDirection);

    auto underlineChange = style.underlineChange();
    auto strikeThroughChange = style.strikeThroughChange();
    if (underlineChange == TextDecorationChange::None && strikeThroughChange == TextDecorationChange::None)
        return;

    if (!m_mutableStyle)
        m_mutableStyle = MutableStyleProperties::create();

    Ref<CSSPrimitiveValue> underline = cssValuePool().createIdentifierValue(CSSValueUnderline);
    Ref<CSSPrimitiveValue> lineThrough = cssValuePool().createIdentifierValue(CSSValueLineThrough);
    RefPtr<CSSValue> value = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    RefPtr<CSSValueList> valueList;
    if (value && value->isValueList()) {
        valueList = downcast<CSSValueList>(*value).copy();
        applyTextDecorationChangeToValueList(*valueList, underlineChange, WTF::move(underline));
        applyTextDecorationChangeToValueList(*valueList, strikeThroughChange, WTF::move(lineThrough));
    } else {
        valueList = CSSValueList::createSpaceSeparated();
        if (underlineChange == TextDecorationChange::Add)
            valueList->append(WTF::move(underline));
        if (strikeThroughChange == TextDecorationChange::Add)
            valueList->append(WTF::move(lineThrough));
    }
    m_mutableStyle->setProperty(CSSPropertyWebkitTextDecorationsInEffect, valueList.get());
}

void EditingStyle::clear()
{
    m_mutableStyle = nullptr;
    m_shouldUseFixedDefaultFontSize = false;
    m_fontSizeDelta = NoFontDelta;
    setUnderlineChange(TextDecorationChange::None);
    setStrikeThroughChange(TextDecorationChange::None);
}

PassRefPtr<EditingStyle> EditingStyle::copy() const
{
    RefPtr<EditingStyle> copy = EditingStyle::create();
    if (m_mutableStyle)
        copy->m_mutableStyle = m_mutableStyle->mutableCopy();
    copy->m_shouldUseFixedDefaultFontSize = m_shouldUseFixedDefaultFontSize;
    copy->m_underlineChange = m_underlineChange;
    copy->m_strikeThroughChange = m_strikeThroughChange;
    copy->m_fontSizeDelta = m_fontSizeDelta;
    return copy;
}

PassRefPtr<EditingStyle> EditingStyle::extractAndRemoveBlockProperties()
{
    RefPtr<EditingStyle> blockProperties = EditingStyle::create();
    if (!m_mutableStyle)
        return blockProperties;

    blockProperties->m_mutableStyle = m_mutableStyle->copyBlockProperties();
    m_mutableStyle->removeBlockProperties();

    return blockProperties;
}

PassRefPtr<EditingStyle> EditingStyle::extractAndRemoveTextDirection()
{
    RefPtr<EditingStyle> textDirection = EditingStyle::create();
    textDirection->m_mutableStyle = MutableStyleProperties::create();
    textDirection->m_mutableStyle->setProperty(CSSPropertyUnicodeBidi, CSSValueEmbed, m_mutableStyle->propertyIsImportant(CSSPropertyUnicodeBidi));
    textDirection->m_mutableStyle->setProperty(CSSPropertyDirection, m_mutableStyle->getPropertyValue(CSSPropertyDirection),
        m_mutableStyle->propertyIsImportant(CSSPropertyDirection));

    m_mutableStyle->removeProperty(CSSPropertyUnicodeBidi);
    m_mutableStyle->removeProperty(CSSPropertyDirection);

    return textDirection;
}

void EditingStyle::removeBlockProperties()
{
    if (!m_mutableStyle)
        return;

    m_mutableStyle->removeBlockProperties();
}

void EditingStyle::removeStyleAddedByNode(Node* node)
{
    if (!node || !node->parentNode())
        return;
    RefPtr<MutableStyleProperties> parentStyle = copyPropertiesFromComputedStyle(node->parentNode(), EditingPropertiesInEffect);
    RefPtr<MutableStyleProperties> nodeStyle = copyPropertiesFromComputedStyle(node, EditingPropertiesInEffect);
    removeEquivalentProperties(*parentStyle);
    removeEquivalentProperties(*nodeStyle);
}

void EditingStyle::removeStyleConflictingWithStyleOfNode(Node* node)
{
    if (!node || !node->parentNode() || !m_mutableStyle)
        return;

    RefPtr<MutableStyleProperties> parentStyle = copyPropertiesFromComputedStyle(node->parentNode(), EditingPropertiesInEffect);
    RefPtr<EditingStyle> nodeStyle = EditingStyle::create(node, EditingPropertiesInEffect);
    nodeStyle->removeEquivalentProperties(*parentStyle);

    MutableStyleProperties* style = nodeStyle->style();
    unsigned propertyCount = style->propertyCount();
    for (unsigned i = 0; i < propertyCount; ++i)
        m_mutableStyle->removeProperty(style->propertyAt(i).id());
}

void EditingStyle::collapseTextDecorationProperties()
{
    if (!m_mutableStyle)
        return;

    RefPtr<CSSValue> textDecorationsInEffect = m_mutableStyle->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    if (!textDecorationsInEffect)
        return;

    if (textDecorationsInEffect->isValueList())
        m_mutableStyle->setProperty(CSSPropertyTextDecoration, textDecorationsInEffect->cssText(), m_mutableStyle->propertyIsImportant(CSSPropertyTextDecoration));
    else
        m_mutableStyle->removeProperty(CSSPropertyTextDecoration);
    m_mutableStyle->removeProperty(CSSPropertyWebkitTextDecorationsInEffect);
}

// CSS properties that create a visual difference only when applied to text.
static const CSSPropertyID textOnlyProperties[] = {
    CSSPropertyTextDecoration,
    CSSPropertyWebkitTextDecorationsInEffect,
    CSSPropertyFontStyle,
    CSSPropertyFontWeight,
    CSSPropertyColor,
};

TriState EditingStyle::triStateOfStyle(EditingStyle* style) const
{
    if (!style || !style->m_mutableStyle)
        return FalseTriState;
    return triStateOfStyle(*style->m_mutableStyle, DoNotIgnoreTextOnlyProperties);
}

template<typename T>
TriState EditingStyle::triStateOfStyle(T& styleToCompare, ShouldIgnoreTextOnlyProperties shouldIgnoreTextOnlyProperties) const
{
    if (!m_mutableStyle)
        return TrueTriState;

    RefPtr<MutableStyleProperties> difference = getPropertiesNotIn(*m_mutableStyle, styleToCompare);

    if (shouldIgnoreTextOnlyProperties == IgnoreTextOnlyProperties)
        difference->removePropertiesInSet(textOnlyProperties, WTF_ARRAY_LENGTH(textOnlyProperties));

    if (difference->isEmpty())
        return TrueTriState;
    if (difference->propertyCount() == m_mutableStyle->propertyCount())
        return FalseTriState;

    return MixedTriState;
}

TriState EditingStyle::triStateOfStyle(const VisibleSelection& selection) const
{
    if (!selection.isCaretOrRange())
        return FalseTriState;

    if (selection.isCaret())
        return triStateOfStyle(EditingStyle::styleAtSelectionStart(selection).get());

    TriState state = FalseTriState;
    bool nodeIsStart = true;
    for (Node* node = selection.start().deprecatedNode(); node; node = NodeTraversal::next(*node)) {
        if (node->renderer() && node->hasEditableStyle()) {
            ComputedStyleExtractor computedStyle(node);
            TriState nodeState = triStateOfStyle(computedStyle, node->isTextNode() ? EditingStyle::DoNotIgnoreTextOnlyProperties : EditingStyle::IgnoreTextOnlyProperties);
            if (nodeIsStart) {
                state = nodeState;
                nodeIsStart = false;
            } else if (state != nodeState && node->isTextNode()) {
                state = MixedTriState;
                break;
            }
        }

        if (node == selection.end().deprecatedNode())
            break;
    }

    return state;
}

static RefPtr<CSSValueList> textDecorationValueList(const StyleProperties& properties)
{
    RefPtr<CSSValue> value = properties.getPropertyCSSValue(CSSPropertyTextDecoration);
    if (!is<CSSValueList>(value.get()))
        return nullptr;
    return downcast<CSSValueList>(value.get());
}

bool EditingStyle::conflictsWithInlineStyleOfElement(StyledElement* element, RefPtr<MutableStyleProperties>* newInlineStylePtr, EditingStyle* extractedStyle) const
{
    ASSERT(element);

    const StyleProperties* inlineStyle = element->inlineStyle();
    if (!inlineStyle)
        return false;
    bool conflicts = false;
    RefPtr<MutableStyleProperties> newInlineStyle;
    if (newInlineStylePtr) {
        newInlineStyle = inlineStyle->mutableCopy();
        *newInlineStylePtr = newInlineStyle;
    }

    bool shouldRemoveUnderline = underlineChange() == TextDecorationChange::Remove;
    bool shouldRemoveStrikeThrough = strikeThroughChange() == TextDecorationChange::Remove;
    if (shouldRemoveUnderline || shouldRemoveStrikeThrough) {
        if (RefPtr<CSSValueList> valueList = textDecorationValueList(*inlineStyle)) {
            RefPtr<CSSValueList> newValueList = valueList->copy();
            RefPtr<CSSValueList> extractedValueList = CSSValueList::createSpaceSeparated();

            Ref<CSSPrimitiveValue> underline = cssValuePool().createIdentifierValue(CSSValueUnderline);
            if (shouldRemoveUnderline && valueList->hasValue(underline.ptr())) {
                if (!newInlineStyle)
                    return true;
                newValueList->removeAll(underline.ptr());
                extractedValueList->append(WTF::move(underline));
            }

            Ref<CSSPrimitiveValue> lineThrough = cssValuePool().createIdentifierValue(CSSValueLineThrough);
            if (shouldRemoveStrikeThrough && valueList->hasValue(lineThrough.ptr())) {
                if (!newInlineStyle)
                    return true;
                newValueList->removeAll(lineThrough.ptr());
                extractedValueList->append(WTF::move(lineThrough));
            }

            if (extractedValueList->length()) {
                conflicts = true;
                if (newValueList->length())
                    newInlineStyle->setProperty(CSSPropertyTextDecoration, newValueList);
                else
                    newInlineStyle->removeProperty(CSSPropertyTextDecoration);

                if (extractedStyle) {
                    bool isImportant = inlineStyle->propertyIsImportant(CSSPropertyTextDecoration);
                    extractedStyle->setProperty(CSSPropertyTextDecoration, extractedValueList->cssText(), isImportant);
                }
            }
        }
    }

    unsigned propertyCount = m_mutableStyle ? m_mutableStyle->propertyCount() : 0;
    for (unsigned i = 0; i < propertyCount; ++i) {
        CSSPropertyID propertyID = m_mutableStyle->propertyAt(i).id();

        // We don't override whitespace property of a tab span because that would collapse the tab into a space.
        if (propertyID == CSSPropertyWhiteSpace && isTabSpanNode(element))
            continue;

        if (propertyID == CSSPropertyWebkitTextDecorationsInEffect && inlineStyle->getPropertyCSSValue(CSSPropertyTextDecoration)) {
            if (!newInlineStyle)
                return true;
            conflicts = true;
            newInlineStyle->removeProperty(CSSPropertyTextDecoration);
            if (extractedStyle)
                extractedStyle->setProperty(CSSPropertyTextDecoration, inlineStyle->getPropertyValue(CSSPropertyTextDecoration), inlineStyle->propertyIsImportant(CSSPropertyTextDecoration));
        }

        if (!inlineStyle->getPropertyCSSValue(propertyID))
            continue;

        if (propertyID == CSSPropertyUnicodeBidi && inlineStyle->getPropertyCSSValue(CSSPropertyDirection)) {
            if (!newInlineStyle)
                return true;
            conflicts = true;
            newInlineStyle->removeProperty(CSSPropertyDirection);
            if (extractedStyle)
                extractedStyle->setProperty(propertyID, inlineStyle->getPropertyValue(propertyID), inlineStyle->propertyIsImportant(propertyID));
        }

        if (!newInlineStyle)
            return true;

        conflicts = true;
        newInlineStyle->removeProperty(propertyID);
        if (extractedStyle)
            extractedStyle->setProperty(propertyID, inlineStyle->getPropertyValue(propertyID), inlineStyle->propertyIsImportant(propertyID));
    }

    return conflicts;
}

static const Vector<std::unique_ptr<HTMLElementEquivalent>>& htmlElementEquivalents()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<Vector<std::unique_ptr<HTMLElementEquivalent>>> HTMLElementEquivalents;
#else
    WKC_DEFINE_STATIC_PTR(Vector<std::unique_ptr<HTMLElementEquivalent>>*, HTMLElementEquivalents, 0);
    if (!HTMLElementEquivalents)
        HTMLElementEquivalents = new Vector<std::unique_ptr<HTMLElementEquivalent>>();
#endif

#if !PLATFORM(WKC)
    if (!HTMLElementEquivalents.get().size()) {
        HTMLElementEquivalents.get().append(std::make_unique<HTMLElementEquivalent>(CSSPropertyFontWeight, CSSValueBold, HTMLNames::bTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLElementEquivalent>(CSSPropertyFontWeight, CSSValueBold, HTMLNames::strongTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLElementEquivalent>(CSSPropertyVerticalAlign, CSSValueSub, HTMLNames::subTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLElementEquivalent>(CSSPropertyVerticalAlign, CSSValueSuper, HTMLNames::supTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLElementEquivalent>(CSSPropertyFontStyle, CSSValueItalic, HTMLNames::iTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLElementEquivalent>(CSSPropertyFontStyle, CSSValueItalic, HTMLNames::emTag));

        HTMLElementEquivalents.get().append(std::make_unique<HTMLTextDecorationEquivalent>(CSSValueUnderline, HTMLNames::uTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLTextDecorationEquivalent>(CSSValueLineThrough, HTMLNames::sTag));
        HTMLElementEquivalents.get().append(std::make_unique<HTMLTextDecorationEquivalent>(CSSValueLineThrough, HTMLNames::strikeTag));
    }

    return HTMLElementEquivalents;
#else
    if (!HTMLElementEquivalents->size()) {
        CSSPropertyID v1;
        CSSValueID v2;
        v1 = CSSPropertyFontWeight; v2 = CSSValueBold;
        HTMLElementEquivalents->append(std::make_unique<HTMLElementEquivalent>(v1, v2, HTMLNames::bTag));
        v1 = CSSPropertyFontWeight; v2 = CSSValueBold;
        HTMLElementEquivalents->append(std::make_unique<HTMLElementEquivalent>(v1, v2, HTMLNames::strongTag));
        v1 = CSSPropertyVerticalAlign; v2 = CSSValueSub;
        HTMLElementEquivalents->append(std::make_unique<HTMLElementEquivalent>(v1, v2, HTMLNames::subTag));
        v1 = CSSPropertyVerticalAlign; v2 = CSSValueSuper;
        HTMLElementEquivalents->append(std::make_unique<HTMLElementEquivalent>(v1, v2, HTMLNames::supTag));
        v1 = CSSPropertyFontStyle; v2 = CSSValueItalic;
        HTMLElementEquivalents->append(std::make_unique<HTMLElementEquivalent>(v1, v2, HTMLNames::iTag));
        v1 = CSSPropertyFontStyle; v2 = CSSValueItalic;
        HTMLElementEquivalents->append(std::make_unique<HTMLElementEquivalent>(v1, v2, HTMLNames::emTag));

        v2 = CSSValueUnderline;
        HTMLElementEquivalents->append(std::make_unique<HTMLTextDecorationEquivalent>(v2, HTMLNames::uTag));
        v2 = CSSValueLineThrough;
        HTMLElementEquivalents->append(std::make_unique<HTMLTextDecorationEquivalent>(v2, HTMLNames::sTag));
        v2 = CSSValueLineThrough;
        HTMLElementEquivalents->append(std::make_unique<HTMLTextDecorationEquivalent>(v2, HTMLNames::strikeTag));
    }

    return *HTMLElementEquivalents;
#endif
}


bool EditingStyle::conflictsWithImplicitStyleOfElement(HTMLElement* element, EditingStyle* extractedStyle, ShouldExtractMatchingStyle shouldExtractMatchingStyle) const
{
    if (isEmpty())
        return false;

    const Vector<std::unique_ptr<HTMLElementEquivalent>>& HTMLElementEquivalents = htmlElementEquivalents();
    for (auto& equivalent : HTMLElementEquivalents) {
        if (equivalent->matches(*element) && equivalent->propertyExistsInStyle(*this)
            && (shouldExtractMatchingStyle == ExtractMatchingStyle || !equivalent->valueIsPresentInStyle(*element, *this))) {
            if (extractedStyle)
                equivalent->addToStyle(element, extractedStyle);
            return true;
        }
    }
    return false;
}

static const Vector<std::unique_ptr<HTMLAttributeEquivalent>>& htmlAttributeEquivalents()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<Vector<std::unique_ptr<HTMLAttributeEquivalent>>> HTMLAttributeEquivalents;

    if (!HTMLAttributeEquivalents.get().size()) {
        // elementIsStyledSpanOrHTMLEquivalent depends on the fact each HTMLAttriuteEquivalent matches exactly one attribute
        // of exactly one element except dirAttr.
        HTMLAttributeEquivalents.get().append(std::make_unique<HTMLAttributeEquivalent>(CSSPropertyColor, HTMLNames::fontTag, HTMLNames::colorAttr));
        HTMLAttributeEquivalents.get().append(std::make_unique<HTMLAttributeEquivalent>(CSSPropertyFontFamily, HTMLNames::fontTag, HTMLNames::faceAttr));
        HTMLAttributeEquivalents.get().append(std::make_unique<HTMLFontSizeEquivalent>());

        HTMLAttributeEquivalents.get().append(std::make_unique<HTMLAttributeEquivalent>(CSSPropertyDirection, HTMLNames::dirAttr));
        HTMLAttributeEquivalents.get().append(std::make_unique<HTMLAttributeEquivalent>(CSSPropertyUnicodeBidi, HTMLNames::dirAttr));
    }

    return HTMLAttributeEquivalents;
#else
    WKC_DEFINE_STATIC_PTR(Vector<std::unique_ptr<HTMLAttributeEquivalent>>*, HTMLAttributeEquivalents, 0);
    if (!HTMLAttributeEquivalents)
        HTMLAttributeEquivalents = new Vector<std::unique_ptr<HTMLAttributeEquivalent>>();

    if (!HTMLAttributeEquivalents->size()) {
        // elementIsStyledSpanOrHTMLEquivalent depends on the fact each HTMLAttriuteEquivalent matches exactly one attribute
        // of exactly one element except dirAttr.
        CSSPropertyID v1;
        v1 = CSSPropertyColor;
        HTMLAttributeEquivalents->append(std::make_unique<HTMLAttributeEquivalent>(v1, HTMLNames::fontTag, HTMLNames::colorAttr));
        v1 = CSSPropertyFontFamily;
        HTMLAttributeEquivalents->append(std::make_unique<HTMLAttributeEquivalent>(v1, HTMLNames::fontTag, HTMLNames::faceAttr));
        HTMLAttributeEquivalents->append(std::make_unique<HTMLFontSizeEquivalent>());

        HTMLAttributeEquivalents->append(std::make_unique<HTMLAttributeEquivalent>(CSSPropertyDirection, HTMLNames::dirAttr));
        HTMLAttributeEquivalents->append(std::make_unique<HTMLAttributeEquivalent>(CSSPropertyUnicodeBidi, HTMLNames::dirAttr));
    }

    return *HTMLAttributeEquivalents;
#endif
}

bool EditingStyle::conflictsWithImplicitStyleOfAttributes(HTMLElement* element) const
{
    ASSERT(element);
    if (isEmpty())
        return false;

    const Vector<std::unique_ptr<HTMLAttributeEquivalent>>& HTMLAttributeEquivalents = htmlAttributeEquivalents();
    for (auto& equivalent : HTMLAttributeEquivalents) {
        if (equivalent->matches(*element) && equivalent->propertyExistsInStyle(*this) && !equivalent->valueIsPresentInStyle(*element, *this))
            return true;
    }

    return false;
}

bool EditingStyle::extractConflictingImplicitStyleOfAttributes(HTMLElement* element, ShouldPreserveWritingDirection shouldPreserveWritingDirection,
    EditingStyle* extractedStyle, Vector<QualifiedName>& conflictingAttributes, ShouldExtractMatchingStyle shouldExtractMatchingStyle) const
{
    ASSERT(element);
    // HTMLAttributeEquivalent::addToStyle doesn't support unicode-bidi and direction properties
    ASSERT(!extractedStyle || shouldPreserveWritingDirection == PreserveWritingDirection);
    if (!m_mutableStyle)
        return false;

    const Vector<std::unique_ptr<HTMLAttributeEquivalent>>& HTMLAttributeEquivalents = htmlAttributeEquivalents();
    bool removed = false;
    for (size_t i = 0; i < HTMLAttributeEquivalents.size(); ++i) {
        const HTMLAttributeEquivalent* equivalent = HTMLAttributeEquivalents[i].get();

        // unicode-bidi and direction are pushed down separately so don't push down with other styles.
        if (shouldPreserveWritingDirection == PreserveWritingDirection && equivalent->attributeName() == HTMLNames::dirAttr)
            continue;

        if (!equivalent->matches(*element) || !equivalent->propertyExistsInStyle(*this)
            || (shouldExtractMatchingStyle == DoNotExtractMatchingStyle && equivalent->valueIsPresentInStyle(*element, *this)))
            continue;

        if (extractedStyle)
            equivalent->addToStyle(element, extractedStyle);
        conflictingAttributes.append(equivalent->attributeName());
        removed = true;
    }

    return removed;
}

bool EditingStyle::styleIsPresentInComputedStyleOfNode(Node* node) const
{
    if (isEmpty())
        return true;
    ComputedStyleExtractor computedStyle(node);

    bool shouldAddUnderline = underlineChange() == TextDecorationChange::Add;
    bool shouldAddLineThrough = strikeThroughChange() == TextDecorationChange::Add;
    if (shouldAddUnderline || shouldAddLineThrough) {
        bool hasUnderline = false;
        bool hasLineThrough = false;
        if (RefPtr<CSSValue> value = computedStyle.propertyValue(CSSPropertyTextDecoration)) {
            if (value->isValueList()) {
                const CSSValueList& valueList = downcast<CSSValueList>(*value);
                hasUnderline = valueList.hasValue(&cssValuePool().createIdentifierValue(CSSValueUnderline).get());
                hasLineThrough = valueList.hasValue(&cssValuePool().createIdentifierValue(CSSValueLineThrough).get());
            }
        }
        if ((shouldAddUnderline && !hasUnderline) || (shouldAddLineThrough && !hasLineThrough))
            return false;
    }

    return !m_mutableStyle || getPropertiesNotIn(*m_mutableStyle, computedStyle)->isEmpty();
}

bool EditingStyle::elementIsStyledSpanOrHTMLEquivalent(const HTMLElement* element)
{
    bool elementIsSpanOrElementEquivalent = false;
    if (element->hasTagName(HTMLNames::spanTag))
        elementIsSpanOrElementEquivalent = true;
    else {
        const Vector<std::unique_ptr<HTMLElementEquivalent>>& HTMLElementEquivalents = htmlElementEquivalents();
        size_t i;
        for (i = 0; i < HTMLElementEquivalents.size(); ++i) {
            if (HTMLElementEquivalents[i]->matches(*element)) {
                elementIsSpanOrElementEquivalent = true;
                break;
            }
        }
    }

    if (!element->hasAttributes())
        return elementIsSpanOrElementEquivalent; // span, b, etc... without any attributes

    unsigned matchedAttributes = 0;
    const Vector<std::unique_ptr<HTMLAttributeEquivalent>>& HTMLAttributeEquivalents = htmlAttributeEquivalents();
    for (size_t i = 0; i < HTMLAttributeEquivalents.size(); ++i) {
        if (HTMLAttributeEquivalents[i]->matches(*element) && HTMLAttributeEquivalents[i]->attributeName() != HTMLNames::dirAttr)
            matchedAttributes++;
    }

    if (!elementIsSpanOrElementEquivalent && !matchedAttributes)
        return false; // element is not a span, a html element equivalent, or font element.
    
    if (element->getAttribute(HTMLNames::classAttr) == AppleStyleSpanClass)
        matchedAttributes++;

    if (element->hasAttribute(HTMLNames::styleAttr)) {
        if (const StyleProperties* style = element->inlineStyle()) {
            unsigned propertyCount = style->propertyCount();
            for (unsigned i = 0; i < propertyCount; ++i) {
                if (!isEditingProperty(style->propertyAt(i).id()))
                    return false;
            }
        }
        matchedAttributes++;
    }

    // font with color attribute, span with style attribute, etc...
    ASSERT(matchedAttributes <= element->attributeCount());
    return matchedAttributes >= element->attributeCount();
}

void EditingStyle::prepareToApplyAt(const Position& position, ShouldPreserveWritingDirection shouldPreserveWritingDirection)
{
    if (!m_mutableStyle)
        return;

    // ReplaceSelectionCommand::handleStyleSpans() requires that this function only removes the editing style.
    // If this function was modified in the future to delete all redundant properties, then add a boolean value to indicate
    // which one of editingStyleAtPosition or computedStyle is called.
    RefPtr<EditingStyle> editingStyleAtPosition = EditingStyle::create(position, EditingPropertiesInEffect);
    StyleProperties* styleAtPosition = editingStyleAtPosition->m_mutableStyle.get();

    RefPtr<CSSValue> unicodeBidi;
    RefPtr<CSSValue> direction;
    if (shouldPreserveWritingDirection == PreserveWritingDirection) {
        unicodeBidi = m_mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi);
        direction = m_mutableStyle->getPropertyCSSValue(CSSPropertyDirection);
    }

    removeEquivalentProperties(*styleAtPosition);

    if (textAlignResolvingStartAndEnd(*m_mutableStyle) == textAlignResolvingStartAndEnd(*styleAtPosition))
        m_mutableStyle->removeProperty(CSSPropertyTextAlign);

    if (textColorFromStyle(*m_mutableStyle) == textColorFromStyle(*styleAtPosition))
        m_mutableStyle->removeProperty(CSSPropertyColor);

    if (hasTransparentBackgroundColor(m_mutableStyle.get())
        || cssValueToRGBA(m_mutableStyle->getPropertyCSSValue(CSSPropertyBackgroundColor).get()) == rgbaBackgroundColorInEffect(position.containerNode()))
        m_mutableStyle->removeProperty(CSSPropertyBackgroundColor);

    if (is<CSSPrimitiveValue>(unicodeBidi.get())) {
        m_mutableStyle->setProperty(CSSPropertyUnicodeBidi, static_cast<CSSValueID>(downcast<CSSPrimitiveValue>(*unicodeBidi).getValueID()));
        if (is<CSSPrimitiveValue>(direction.get()))
            m_mutableStyle->setProperty(CSSPropertyDirection, static_cast<CSSValueID>(downcast<CSSPrimitiveValue>(*direction).getValueID()));
    }
}

void EditingStyle::mergeTypingStyle(Document& document)
{
    RefPtr<EditingStyle> typingStyle = document.frame()->selection().typingStyle();
    if (!typingStyle || typingStyle == this)
        return;

    mergeStyle(typingStyle->style(), OverrideValues);
}

void EditingStyle::mergeInlineStyleOfElement(StyledElement* element, CSSPropertyOverrideMode mode, PropertiesToInclude propertiesToInclude)
{
    ASSERT(element);
    if (!element->inlineStyle())
        return;

    switch (propertiesToInclude) {
    case AllProperties:
        mergeStyle(element->inlineStyle(), mode);
        return;
    case OnlyEditingInheritableProperties:
        mergeStyle(copyEditingProperties(element->inlineStyle(), OnlyInheritableEditingProperties).get(), mode);
        return;
    case EditingPropertiesInEffect:
        mergeStyle(copyEditingProperties(element->inlineStyle(), AllEditingProperties).get(), mode);
        return;
    }
}

static inline bool elementMatchesAndPropertyIsNotInInlineStyleDecl(const HTMLElementEquivalent* equivalent, const StyledElement* element,
    EditingStyle::CSSPropertyOverrideMode mode, EditingStyle& style)
{
    if (!equivalent->matches(*element))
        return false;
    if (mode != EditingStyle::OverrideValues && equivalent->propertyExistsInStyle(style))
        return false;

    return !element->inlineStyle() || !equivalent->propertyExistsInStyle(EditingStyle::create(element->inlineStyle()).get());
}

static PassRefPtr<MutableStyleProperties> extractEditingProperties(const StyleProperties* style, EditingStyle::PropertiesToInclude propertiesToInclude)
{
    if (!style)
        return 0;

    switch (propertiesToInclude) {
    case EditingStyle::AllProperties:
    case EditingStyle::EditingPropertiesInEffect:
        return copyEditingProperties(style, AllEditingProperties);
    case EditingStyle::OnlyEditingInheritableProperties:
        return copyEditingProperties(style, OnlyInheritableEditingProperties);
    }

    ASSERT_NOT_REACHED();
    return 0;
}

void EditingStyle::mergeInlineAndImplicitStyleOfElement(StyledElement* element, CSSPropertyOverrideMode mode, PropertiesToInclude propertiesToInclude)
{
    RefPtr<EditingStyle> styleFromRules = EditingStyle::create();
    styleFromRules->mergeStyleFromRulesForSerialization(element);

    if (element->inlineStyle())
        styleFromRules->m_mutableStyle->mergeAndOverrideOnConflict(*element->inlineStyle());

    styleFromRules->m_mutableStyle = extractEditingProperties(styleFromRules->m_mutableStyle.get(), propertiesToInclude);
    mergeStyle(styleFromRules->m_mutableStyle.get(), mode);

    const Vector<std::unique_ptr<HTMLElementEquivalent>>& elementEquivalents = htmlElementEquivalents();
    for (size_t i = 0; i < elementEquivalents.size(); ++i) {
        if (elementMatchesAndPropertyIsNotInInlineStyleDecl(elementEquivalents[i].get(), element, mode, *this))
            elementEquivalents[i]->addToStyle(element, this);
    }

    const Vector<std::unique_ptr<HTMLAttributeEquivalent>>& attributeEquivalents = htmlAttributeEquivalents();
    for (size_t i = 0; i < attributeEquivalents.size(); ++i) {
        if (attributeEquivalents[i]->attributeName() == HTMLNames::dirAttr)
            continue; // We don't want to include directionality
        if (elementMatchesAndPropertyIsNotInInlineStyleDecl(attributeEquivalents[i].get(), element, mode, *this))
            attributeEquivalents[i]->addToStyle(element, this);
    }
}

PassRefPtr<EditingStyle> EditingStyle::wrappingStyleForSerialization(Node* context, bool shouldAnnotate)
{
    RefPtr<EditingStyle> wrappingStyle;
    if (shouldAnnotate) {
        wrappingStyle = EditingStyle::create(context, EditingStyle::EditingPropertiesInEffect);

        // Styles that Mail blockquotes contribute should only be placed on the Mail blockquote,
        // to help us differentiate those styles from ones that the user has applied.
        // This helps us get the color of content pasted into blockquotes right.
        wrappingStyle->removeStyleAddedByNode(enclosingNodeOfType(firstPositionInOrBeforeNode(context), isMailBlockquote, CanCrossEditingBoundary));

        // Call collapseTextDecorationProperties first or otherwise it'll copy the value over from in-effect to text-decorations.
        wrappingStyle->collapseTextDecorationProperties();
        
        return wrappingStyle.release();
    }

    wrappingStyle = EditingStyle::create();

    // When not annotating for interchange, we only preserve inline style declarations.
    for (Node* node = context; node && !node->isDocumentNode(); node = node->parentNode()) {
        if (is<StyledElement>(*node) && !isMailBlockquote(node)) {
            wrappingStyle->mergeInlineAndImplicitStyleOfElement(downcast<StyledElement>(node), EditingStyle::DoNotOverrideValues,
                EditingStyle::EditingPropertiesInEffect);
        }
    }

    return wrappingStyle.release();
}


static void mergeTextDecorationValues(CSSValueList& mergedValue, const CSSValueList& valueToMerge)
{
    Ref<CSSPrimitiveValue> underline = cssValuePool().createIdentifierValue(CSSValueUnderline);
    Ref<CSSPrimitiveValue> lineThrough = cssValuePool().createIdentifierValue(CSSValueLineThrough);

    if (valueToMerge.hasValue(underline.ptr()) && !mergedValue.hasValue(underline.ptr()))
        mergedValue.append(WTF::move(underline));

    if (valueToMerge.hasValue(lineThrough.ptr()) && !mergedValue.hasValue(lineThrough.ptr()))
        mergedValue.append(WTF::move(lineThrough));
}

void EditingStyle::mergeStyle(const StyleProperties* style, CSSPropertyOverrideMode mode)
{
    if (!style)
        return;

    if (!m_mutableStyle) {
        m_mutableStyle = style->mutableCopy();
        return;
    }

    unsigned propertyCount = style->propertyCount();
    for (unsigned i = 0; i < propertyCount; ++i) {
        StyleProperties::PropertyReference property = style->propertyAt(i);
        RefPtr<CSSValue> value = m_mutableStyle->getPropertyCSSValue(property.id());

        // text decorations never override values.
        if ((property.id() == CSSPropertyTextDecoration || property.id() == CSSPropertyWebkitTextDecorationsInEffect)
            && is<CSSValueList>(*property.value()) && value) {
            if (is<CSSValueList>(*value)) {
                RefPtr<CSSValueList> newValue = downcast<CSSValueList>(*value).copy();
                mergeTextDecorationValues(*newValue, downcast<CSSValueList>(*property.value()));
                m_mutableStyle->setProperty(property.id(), newValue.release(), property.isImportant());
                continue;
            }
            value = nullptr; // text-decoration: none is equivalent to not having the property.
        }

        if (mode == OverrideValues || (mode == DoNotOverrideValues && !value))
            m_mutableStyle->setProperty(property.id(), property.value(), property.isImportant());
    }

    int oldFontSizeDelta = m_fontSizeDelta;
    extractFontSizeDelta();
    m_fontSizeDelta += oldFontSizeDelta;
}

static PassRefPtr<MutableStyleProperties> styleFromMatchedRulesForElement(Element* element, unsigned rulesToInclude)
{
    RefPtr<MutableStyleProperties> style = MutableStyleProperties::create();
    auto matchedRules = element->document().ensureStyleResolver().styleRulesForElement(element, rulesToInclude);
    for (unsigned i = 0; i < matchedRules.size(); ++i) {
        if (matchedRules[i]->isStyleRule())
            style->mergeAndOverrideOnConflict(static_pointer_cast<StyleRule>(matchedRules[i])->properties());
    }
    
    return style.release();
}

void EditingStyle::mergeStyleFromRules(StyledElement* element)
{
    RefPtr<MutableStyleProperties> styleFromMatchedRules = styleFromMatchedRulesForElement(element,
        StyleResolver::AuthorCSSRules | StyleResolver::CrossOriginCSSRules);
    // Styles from the inline style declaration, held in the variable "style", take precedence 
    // over those from matched rules.
    if (m_mutableStyle)
        styleFromMatchedRules->mergeAndOverrideOnConflict(*m_mutableStyle);

    clear();
    m_mutableStyle = styleFromMatchedRules;
}

void EditingStyle::mergeStyleFromRulesForSerialization(StyledElement* element)
{
    mergeStyleFromRules(element);

    // The property value, if it's a percentage, may not reflect the actual computed value.  
    // For example: style="height: 1%; overflow: visible;" in quirksmode
    // FIXME: There are others like this, see <rdar://problem/5195123> Slashdot copy/paste fidelity problem
    RefPtr<MutableStyleProperties> fromComputedStyle = MutableStyleProperties::create();
    ComputedStyleExtractor computedStyle(element);

    {
        unsigned propertyCount = m_mutableStyle->propertyCount();
        for (unsigned i = 0; i < propertyCount; ++i) {
            StyleProperties::PropertyReference property = m_mutableStyle->propertyAt(i);
            CSSValue* value = property.value();
            if (!is<CSSPrimitiveValue>(*value))
                continue;
            if (downcast<CSSPrimitiveValue>(*value).isPercentage()) {
                if (RefPtr<CSSValue> computedPropertyValue = computedStyle.propertyValue(property.id()))
                    fromComputedStyle->addParsedProperty(CSSProperty(property.id(), computedPropertyValue.release()));
            }
        }
    }
    m_mutableStyle->mergeAndOverrideOnConflict(*fromComputedStyle);
}

static void removePropertiesInStyle(MutableStyleProperties* styleToRemovePropertiesFrom, MutableStyleProperties* style)
{
    unsigned propertyCount = style->propertyCount();
    Vector<CSSPropertyID> propertiesToRemove(propertyCount);
    for (unsigned i = 0; i < propertyCount; ++i)
        propertiesToRemove[i] = style->propertyAt(i).id();

    styleToRemovePropertiesFrom->removePropertiesInSet(propertiesToRemove.data(), propertiesToRemove.size());
}

void EditingStyle::removeStyleFromRulesAndContext(StyledElement* element, Node* context)
{
    ASSERT(element);
    if (!m_mutableStyle)
        return;

    // 1. Remove style from matched rules because style remain without repeating it in inline style declaration
    RefPtr<MutableStyleProperties> styleFromMatchedRules = styleFromMatchedRulesForElement(element, StyleResolver::AllButEmptyCSSRules);
    if (styleFromMatchedRules && !styleFromMatchedRules->isEmpty())
        m_mutableStyle = getPropertiesNotIn(*m_mutableStyle, *styleFromMatchedRules);

    // 2. Remove style present in context and not overriden by matched rules.
    RefPtr<EditingStyle> computedStyle = EditingStyle::create(context, EditingPropertiesInEffect);
    if (computedStyle->m_mutableStyle) {
        if (!computedStyle->m_mutableStyle->getPropertyCSSValue(CSSPropertyBackgroundColor))
            computedStyle->m_mutableStyle->setProperty(CSSPropertyBackgroundColor, CSSValueTransparent);

        removePropertiesInStyle(computedStyle->m_mutableStyle.get(), styleFromMatchedRules.get());
        m_mutableStyle = getPropertiesNotIn(*m_mutableStyle, *computedStyle->m_mutableStyle);
    }

    // 3. If this element is a span and has display: inline or float: none, remove them unless they are overriden by rules.
    // These rules are added by serialization code to wrap text nodes.
    if (isStyleSpanOrSpanWithOnlyStyleAttribute(element)) {
        if (!styleFromMatchedRules->getPropertyCSSValue(CSSPropertyDisplay) && identifierForStyleProperty(*m_mutableStyle, CSSPropertyDisplay) == CSSValueInline)
            m_mutableStyle->removeProperty(CSSPropertyDisplay);
        if (!styleFromMatchedRules->getPropertyCSSValue(CSSPropertyFloat) && identifierForStyleProperty(*m_mutableStyle, CSSPropertyFloat) == CSSValueNone)
            m_mutableStyle->removeProperty(CSSPropertyFloat);
    }
}

void EditingStyle::removePropertiesInElementDefaultStyle(Element* element)
{
    if (!m_mutableStyle || m_mutableStyle->isEmpty())
        return;

    RefPtr<MutableStyleProperties> defaultStyle = styleFromMatchedRulesForElement(element, StyleResolver::UAAndUserCSSRules);

    removePropertiesInStyle(m_mutableStyle.get(), defaultStyle.get());
}

template<typename T>
void EditingStyle::removeEquivalentProperties(const T& style)
{
    Vector<CSSPropertyID> propertiesToRemove;
    for (auto& property : m_mutableStyle->m_propertyVector) {
        if (style.propertyMatches(property.id(), property.value()))
            propertiesToRemove.append(property.id());
    }
    // FIXME: This should use mass removal.
    for (auto& property : propertiesToRemove)
        m_mutableStyle->removeProperty(property);
}

void EditingStyle::forceInline()
{
    if (!m_mutableStyle)
        m_mutableStyle = MutableStyleProperties::create();
    const bool propertyIsImportant = true;
    m_mutableStyle->setProperty(CSSPropertyDisplay, CSSValueInline, propertyIsImportant);
}

bool EditingStyle::convertPositionStyle()
{
    if (!m_mutableStyle)
        return false;

    RefPtr<CSSPrimitiveValue> sticky = cssValuePool().createIdentifierValue(CSSValueWebkitSticky);
    if (m_mutableStyle->propertyMatches(CSSPropertyPosition, sticky.get())) {
        m_mutableStyle->setProperty(CSSPropertyPosition, cssValuePool().createIdentifierValue(CSSValueStatic), m_mutableStyle->propertyIsImportant(CSSPropertyPosition));
        return false;
    }
    RefPtr<CSSPrimitiveValue> fixed = cssValuePool().createIdentifierValue(CSSValueFixed);
    if (m_mutableStyle->propertyMatches(CSSPropertyPosition, fixed.get())) {
        m_mutableStyle->setProperty(CSSPropertyPosition, cssValuePool().createIdentifierValue(CSSValueAbsolute), m_mutableStyle->propertyIsImportant(CSSPropertyPosition));
        return true;
    }
    RefPtr<CSSPrimitiveValue> absolute = cssValuePool().createIdentifierValue(CSSValueAbsolute);
    if (m_mutableStyle->propertyMatches(CSSPropertyPosition, absolute.get()))
        return true;
    return false;
}

bool EditingStyle::isFloating()
{
    RefPtr<CSSValue> v = m_mutableStyle->getPropertyCSSValue(CSSPropertyFloat);
    RefPtr<CSSPrimitiveValue> noneValue = cssValuePool().createIdentifierValue(CSSValueNone);
    return v && !v->equals(*noneValue);
}

int EditingStyle::legacyFontSize(Document* document) const
{
    RefPtr<CSSValue> cssValue = m_mutableStyle->getPropertyCSSValue(CSSPropertyFontSize);
    if (!is<CSSPrimitiveValue>(cssValue.get()))
        return 0;
    return legacyFontSizeFromCSSValue(document, downcast<CSSPrimitiveValue>(cssValue.get()),
        m_shouldUseFixedDefaultFontSize, AlwaysUseLegacyFontSize);
}

PassRefPtr<EditingStyle> EditingStyle::styleAtSelectionStart(const VisibleSelection& selection, bool shouldUseBackgroundColorInEffect)
{
    if (selection.isNone())
        return 0;

    Position position = adjustedSelectionStartForStyleComputation(selection);

    // If the pos is at the end of a text node, then this node is not fully selected. 
    // Move it to the next deep equivalent position to avoid removing the style from this node. 
    // e.g. if pos was at Position("hello", 5) in <b>hello<div>world</div></b>, we want Position("world", 0) instead. 
    // We only do this for range because caret at Position("hello", 5) in <b>hello</b>world should give you font-weight: bold. 
    Node* positionNode = position.containerNode(); 
    if (selection.isRange() && positionNode && positionNode->isTextNode() && position.computeOffsetInContainerNode() == positionNode->maxCharacterOffset()) 
        position = nextVisuallyDistinctCandidate(position); 

    Element* element = position.element();
    if (!element)
        return 0;

    RefPtr<EditingStyle> style = EditingStyle::create(element, EditingStyle::AllProperties);
    style->mergeTypingStyle(element->document());

    // If background color is transparent, traverse parent nodes until we hit a different value or document root
    // Also, if the selection is a range, ignore the background color at the start of selection,
    // and find the background color of the common ancestor.
    if (shouldUseBackgroundColorInEffect && (selection.isRange() || hasTransparentBackgroundColor(style->m_mutableStyle.get()))) {
        if (RefPtr<Range> range = selection.toNormalizedRange()) {
            if (PassRefPtr<CSSValue> value = backgroundColorInEffect(range->commonAncestorContainer(IGNORE_EXCEPTION)))
                style->setProperty(CSSPropertyBackgroundColor, value->cssText());
        }
    }

    return style;
}

WritingDirection EditingStyle::textDirectionForSelection(const VisibleSelection& selection, EditingStyle* typingStyle, bool& hasNestedOrMultipleEmbeddings)
{
    hasNestedOrMultipleEmbeddings = true;

    if (selection.isNone())
        return NaturalWritingDirection;

    Position position = selection.start().downstream();

    Node* node = position.deprecatedNode();
    if (!node)
        return NaturalWritingDirection;

    Position end;
    if (selection.isRange()) {
        end = selection.end().upstream();

        Node* pastLast = Range::create(*end.document(), position.parentAnchoredEquivalent(), end.parentAnchoredEquivalent())->pastLastNode();
        for (Node* n = node; n && n != pastLast; n = NodeTraversal::next(*n)) {
            if (!n->isStyledElement())
                continue;

            RefPtr<CSSValue> unicodeBidi = ComputedStyleExtractor(n).propertyValue(CSSPropertyUnicodeBidi);
            if (!is<CSSPrimitiveValue>(unicodeBidi.get()))
                continue;

            CSSValueID unicodeBidiValue = downcast<CSSPrimitiveValue>(*unicodeBidi).getValueID();
            if (unicodeBidiValue == CSSValueEmbed || unicodeBidiValue == CSSValueBidiOverride)
                return NaturalWritingDirection;
        }
    }

    if (selection.isCaret()) {
        WritingDirection direction;
        if (typingStyle && typingStyle->textDirection(direction)) {
            hasNestedOrMultipleEmbeddings = false;
            return direction;
        }
        node = selection.visibleStart().deepEquivalent().deprecatedNode();
    }

    // The selection is either a caret with no typing attributes or a range in which no embedding is added, so just use the start position
    // to decide.
    Node* block = enclosingBlock(node);
    WritingDirection foundDirection = NaturalWritingDirection;

    for (; node != block; node = node->parentNode()) {
        if (!node->isStyledElement())
            continue;

        ComputedStyleExtractor computedStyle(node);
        RefPtr<CSSValue> unicodeBidi = computedStyle.propertyValue(CSSPropertyUnicodeBidi);
        if (!is<CSSPrimitiveValue>(unicodeBidi.get()))
            continue;

        CSSValueID unicodeBidiValue = downcast<CSSPrimitiveValue>(*unicodeBidi).getValueID();
        if (unicodeBidiValue == CSSValueNormal)
            continue;

        if (unicodeBidiValue == CSSValueBidiOverride)
            return NaturalWritingDirection;

        ASSERT(unicodeBidiValue == CSSValueEmbed);
        RefPtr<CSSValue> direction = computedStyle.propertyValue(CSSPropertyDirection);
        if (!is<CSSPrimitiveValue>(direction.get()))
            continue;

        CSSValueID directionValue = downcast<CSSPrimitiveValue>(*direction).getValueID();
        if (directionValue != CSSValueLtr && directionValue != CSSValueRtl)
            continue;

        if (foundDirection != NaturalWritingDirection)
            return NaturalWritingDirection;

        // In the range case, make sure that the embedding element persists until the end of the range.
        if (selection.isRange() && !end.deprecatedNode()->isDescendantOf(node))
            return NaturalWritingDirection;
        
        foundDirection = directionValue == CSSValueLtr ? LeftToRightWritingDirection : RightToLeftWritingDirection;
    }
    hasNestedOrMultipleEmbeddings = false;
    return foundDirection;
}

static void reconcileTextDecorationProperties(MutableStyleProperties* style)
{    
    RefPtr<CSSValue> textDecorationsInEffect = style->getPropertyCSSValue(CSSPropertyWebkitTextDecorationsInEffect);
    RefPtr<CSSValue> textDecoration = style->getPropertyCSSValue(CSSPropertyTextDecoration);
    // We shouldn't have both text-decoration and -webkit-text-decorations-in-effect because that wouldn't make sense.
    ASSERT(!textDecorationsInEffect || !textDecoration);
    if (textDecorationsInEffect) {
        style->setProperty(CSSPropertyTextDecoration, textDecorationsInEffect->cssText());
        style->removeProperty(CSSPropertyWebkitTextDecorationsInEffect);
        textDecoration = textDecorationsInEffect;
    }

    // If text-decoration is set to "none", remove the property because we don't want to add redundant "text-decoration: none".
    if (textDecoration && !textDecoration->isValueList())
        style->removeProperty(CSSPropertyTextDecoration);
}

StyleChange::StyleChange(EditingStyle* style, const Position& position)
    : m_applyBold(false)
    , m_applyItalic(false)
    , m_applyUnderline(false)
    , m_applyLineThrough(false)
    , m_applySubscript(false)
    , m_applySuperscript(false)
{
    Document* document = position.deprecatedNode() ? &position.deprecatedNode()->document() : 0;
    if (!style || style->isEmpty() || !document || !document->frame())
        return;

    Node* node = position.containerNode();
    if (!node)
        return;

    ComputedStyleExtractor computedStyle(node);

    // FIXME: take care of background-color in effect
    RefPtr<MutableStyleProperties> mutableStyle = style->style() ?
        getPropertiesNotIn(*style->style(), computedStyle) : MutableStyleProperties::create();

    reconcileTextDecorationProperties(mutableStyle.get());
    bool shouldStyleWithCSS = document->frame()->editor().shouldStyleWithCSS();
    if (!shouldStyleWithCSS)
        extractTextStyles(document, *mutableStyle, computedStyle.useFixedFontDefaultSize());

    bool shouldAddUnderline = style->underlineChange() == TextDecorationChange::Add;
    bool shouldAddStrikeThrough = style->strikeThroughChange() == TextDecorationChange::Add;
    if (shouldAddUnderline || shouldAddStrikeThrough) {
        RefPtr<CSSValue> value = computedStyle.propertyValue(CSSPropertyWebkitTextDecorationsInEffect);
        if (!is<CSSValueList>(value.get()))
            value = computedStyle.propertyValue(CSSPropertyTextDecoration);

        RefPtr<CSSValueList> valueList;
        if (is<CSSValueList>(value.get()))
            valueList = downcast<CSSValueList>(value.get());

        Ref<CSSValue> underline = cssValuePool().createIdentifierValue(CSSValueUnderline);
        bool hasUnderline = valueList && valueList->hasValue(underline.ptr());

        Ref<CSSValue> lineThrough = cssValuePool().createIdentifierValue(CSSValueLineThrough);
        bool hasLineThrough = valueList && valueList->hasValue(lineThrough.ptr());

        if (shouldStyleWithCSS) {
            valueList = valueList ? valueList->copy() : CSSValueList::createSpaceSeparated();
            if (shouldAddUnderline && !hasUnderline)
                valueList->append(WTF::move(underline));
            if (shouldAddStrikeThrough && !hasLineThrough)
                valueList->append(WTF::move(lineThrough));
            mutableStyle->setProperty(CSSPropertyTextDecoration, valueList.get());
        } else {
            m_applyUnderline = shouldAddUnderline && !hasUnderline;
            m_applyLineThrough = shouldAddStrikeThrough && !hasLineThrough;
        }
    }

    // Changing the whitespace style in a tab span would collapse the tab into a space.
    if (isTabSpanTextNode(position.deprecatedNode()) || isTabSpanNode((position.deprecatedNode())))
        mutableStyle->removeProperty(CSSPropertyWhiteSpace);

    // If unicode-bidi is present in mutableStyle and direction is not, then add direction to mutableStyle.
    // FIXME: Shouldn't this be done in getPropertiesNotIn?
    if (mutableStyle->getPropertyCSSValue(CSSPropertyUnicodeBidi) && !style->style()->getPropertyCSSValue(CSSPropertyDirection))
        mutableStyle->setProperty(CSSPropertyDirection, style->style()->getPropertyValue(CSSPropertyDirection));

    if (!mutableStyle->isEmpty())
        m_cssStyle = mutableStyle;
}

bool StyleChange::operator==(const StyleChange& other)
{
    if (m_applyBold != other.m_applyBold
        || m_applyItalic != other.m_applyItalic
        || m_applyUnderline != other.m_applyUnderline
        || m_applyLineThrough != other.m_applyLineThrough
        || m_applySubscript != other.m_applySubscript
        || m_applySuperscript != other.m_applySuperscript
        || m_applyFontColor != other.m_applyFontColor
        || m_applyFontFace != other.m_applyFontFace
        || m_applyFontSize != other.m_applyFontSize)
        return false;

    return (!m_cssStyle && !other.m_cssStyle)
        || (m_cssStyle && other.m_cssStyle && m_cssStyle->asText() == other.m_cssStyle->asText());
}

static void setTextDecorationProperty(MutableStyleProperties& style, const CSSValueList* newTextDecoration, CSSPropertyID propertyID)
{
    if (newTextDecoration->length())
        style.setProperty(propertyID, newTextDecoration->cssText(), style.propertyIsImportant(propertyID));
    else {
        // text-decoration: none is redundant since it does not remove any text decorations.
        style.removeProperty(propertyID);
    }
}

void StyleChange::extractTextStyles(Document* document, MutableStyleProperties& style, bool shouldUseFixedFontDefaultSize)
{
    if (identifierForStyleProperty(style, CSSPropertyFontWeight) == CSSValueBold) {
        style.removeProperty(CSSPropertyFontWeight);
        m_applyBold = true;
    }

    int fontStyle = identifierForStyleProperty(style, CSSPropertyFontStyle);
    if (fontStyle == CSSValueItalic || fontStyle == CSSValueOblique) {
        style.removeProperty(CSSPropertyFontStyle);
        m_applyItalic = true;
    }

    // Assuming reconcileTextDecorationProperties has been called, there should not be -webkit-text-decorations-in-effect
    // Furthermore, text-decoration: none has been trimmed so that text-decoration property is always a CSSValueList.
    RefPtr<CSSValue> textDecoration = style.getPropertyCSSValue(CSSPropertyTextDecoration);
    if (is<CSSValueList>(textDecoration.get())) {
        RefPtr<CSSPrimitiveValue> underline = cssValuePool().createIdentifierValue(CSSValueUnderline);
        RefPtr<CSSPrimitiveValue> lineThrough = cssValuePool().createIdentifierValue(CSSValueLineThrough);

        RefPtr<CSSValueList> newTextDecoration = downcast<CSSValueList>(*textDecoration).copy();
        if (newTextDecoration->removeAll(underline.get()))
            m_applyUnderline = true;
        if (newTextDecoration->removeAll(lineThrough.get()))
            m_applyLineThrough = true;

        // If trimTextDecorations, delete underline and line-through
        setTextDecorationProperty(style, newTextDecoration.get(), CSSPropertyTextDecoration);
    }

    int verticalAlign = identifierForStyleProperty(style, CSSPropertyVerticalAlign);
    switch (verticalAlign) {
    case CSSValueSub:
        style.removeProperty(CSSPropertyVerticalAlign);
        m_applySubscript = true;
        break;
    case CSSValueSuper:
        style.removeProperty(CSSPropertyVerticalAlign);
        m_applySuperscript = true;
        break;
    }

    if (style.getPropertyCSSValue(CSSPropertyColor)) {
        m_applyFontColor = Color(textColorFromStyle(style)).serialized();
        style.removeProperty(CSSPropertyColor);
    }

    m_applyFontFace = style.getPropertyValue(CSSPropertyFontFamily);
    // Remove single quotes for Outlook 2007 compatibility. See https://bugs.webkit.org/show_bug.cgi?id=79448
    m_applyFontFace.replaceWithLiteral('\'', "");
    style.removeProperty(CSSPropertyFontFamily);

    if (RefPtr<CSSValue> fontSize = style.getPropertyCSSValue(CSSPropertyFontSize)) {
        if (!is<CSSPrimitiveValue>(*fontSize))
            style.removeProperty(CSSPropertyFontSize); // Can't make sense of the number. Put no font size.
        else if (int legacyFontSize = legacyFontSizeFromCSSValue(document, downcast<CSSPrimitiveValue>(fontSize.get()),
                shouldUseFixedFontDefaultSize, UseLegacyFontSizeOnlyIfPixelValuesMatch)) {
            m_applyFontSize = String::number(legacyFontSize);
            style.removeProperty(CSSPropertyFontSize);
        }
    }
}

static void diffTextDecorations(MutableStyleProperties& style, CSSPropertyID propertID, CSSValue* refTextDecoration)
{
    RefPtr<CSSValue> textDecoration = style.getPropertyCSSValue(propertID);
    if (!is<CSSValueList>(textDecoration.get()) || !is<CSSValueList>(refTextDecoration))
        return;

    RefPtr<CSSValueList> newTextDecoration = downcast<CSSValueList>(*textDecoration).copy();
    CSSValueList& valuesInRefTextDecoration = downcast<CSSValueList>(*refTextDecoration);

    for (size_t i = 0; i < valuesInRefTextDecoration.length(); ++i)
        newTextDecoration->removeAll(valuesInRefTextDecoration.item(i));

    setTextDecorationProperty(style, newTextDecoration.get(), propertID);
}

static bool fontWeightIsBold(CSSValue& fontWeight)
{
    if (!is<CSSPrimitiveValue>(fontWeight))
        return false;

    // Because b tag can only bold text, there are only two states in plain html: bold and not bold.
    // Collapse all other values to either one of these two states for editing purposes.
    switch (downcast<CSSPrimitiveValue>(fontWeight).getValueID()) {
        case CSSValue100:
        case CSSValue200:
        case CSSValue300:
        case CSSValue400:
        case CSSValue500:
        case CSSValueNormal:
            return false;
        case CSSValueBold:
        case CSSValue600:
        case CSSValue700:
        case CSSValue800:
        case CSSValue900:
            return true;
        default:
            break;
    }

    ASSERT_NOT_REACHED(); // For CSSValueBolder and CSSValueLighter
    return false;
}

template<typename T>
static bool fontWeightIsBold(T& style)
{
    RefPtr<CSSValue> fontWeight = extractPropertyValue(style, CSSPropertyFontWeight);
    return fontWeight && fontWeightIsBold(*fontWeight);
}

template<typename T>
static PassRefPtr<MutableStyleProperties> extractPropertiesNotIn(StyleProperties& styleWithRedundantProperties, T& baseStyle)
{
    RefPtr<EditingStyle> result = EditingStyle::create(&styleWithRedundantProperties);
    result->removeEquivalentProperties(baseStyle);
    RefPtr<MutableStyleProperties> mutableStyle = result->style();

    RefPtr<CSSValue> baseTextDecorationsInEffect = extractPropertyValue(baseStyle, CSSPropertyWebkitTextDecorationsInEffect);
    diffTextDecorations(*mutableStyle, CSSPropertyTextDecoration, baseTextDecorationsInEffect.get());
    diffTextDecorations(*mutableStyle, CSSPropertyWebkitTextDecorationsInEffect, baseTextDecorationsInEffect.get());

    if (extractPropertyValue(baseStyle, CSSPropertyFontWeight) && fontWeightIsBold(*mutableStyle) == fontWeightIsBold(baseStyle))
        mutableStyle->removeProperty(CSSPropertyFontWeight);

    if (extractPropertyValue(baseStyle, CSSPropertyColor) && textColorFromStyle(*mutableStyle) == textColorFromStyle(baseStyle))
        mutableStyle->removeProperty(CSSPropertyColor);

    if (extractPropertyValue(baseStyle, CSSPropertyTextAlign)
        && textAlignResolvingStartAndEnd(*mutableStyle) == textAlignResolvingStartAndEnd(baseStyle))
        mutableStyle->removeProperty(CSSPropertyTextAlign);

    if (extractPropertyValue(baseStyle, CSSPropertyBackgroundColor) && backgroundColorFromStyle(*mutableStyle) == backgroundColorFromStyle(baseStyle))
        mutableStyle->removeProperty(CSSPropertyBackgroundColor);

    return mutableStyle.release();
}

template<typename T>
PassRefPtr<MutableStyleProperties> getPropertiesNotIn(StyleProperties& styleWithRedundantProperties, T& baseStyle)
{
    return extractPropertiesNotIn(styleWithRedundantProperties, baseStyle);
}

static bool isCSSValueLength(CSSPrimitiveValue* value)
{
    return value->isFontIndependentLength();
}

int legacyFontSizeFromCSSValue(Document* document, CSSPrimitiveValue* value, bool shouldUseFixedFontDefaultSize, LegacyFontSizeMode mode)
{
    ASSERT(document); // FIXME: This method should take a Document&

    if (isCSSValueLength(value)) {
        int pixelFontSize = value->getIntValue(CSSPrimitiveValue::CSS_PX);
        int legacyFontSize = Style::legacyFontSizeForPixelSize(pixelFontSize, shouldUseFixedFontDefaultSize, *document);
        // Use legacy font size only if pixel value matches exactly to that of legacy font size.
        int cssPrimitiveEquivalent = legacyFontSize - 1 + CSSValueXSmall;
        if (mode == AlwaysUseLegacyFontSize || Style::fontSizeForKeyword(cssPrimitiveEquivalent, shouldUseFixedFontDefaultSize, *document) == pixelFontSize)
            return legacyFontSize;

        return 0;
    }

    if (CSSValueXSmall <= value->getValueID() && value->getValueID() <= CSSValueWebkitXxxLarge)
        return value->getValueID() - CSSValueXSmall + 1;

    return 0;
}

bool isTransparentColorValue(CSSValue* cssValue)
{
    if (!cssValue)
        return true;
    if (!is<CSSPrimitiveValue>(*cssValue))
        return false;
    CSSPrimitiveValue& value = downcast<CSSPrimitiveValue>(*cssValue);
    if (value.isRGBColor())
        return !alphaChannel(value.getRGBA32Value());
    return value.getValueID() == CSSValueTransparent;
}

bool hasTransparentBackgroundColor(StyleProperties* style)
{
    RefPtr<CSSValue> cssValue = style->getPropertyCSSValue(CSSPropertyBackgroundColor);
    return isTransparentColorValue(cssValue.get());
}

PassRefPtr<CSSValue> backgroundColorInEffect(Node* node)
{
    for (Node* ancestor = node; ancestor; ancestor = ancestor->parentNode()) {
        if (RefPtr<CSSValue> value = ComputedStyleExtractor(ancestor).propertyValue(CSSPropertyBackgroundColor)) {
            if (!isTransparentColorValue(value.get()))
                return value.release();
        }
    }
    return 0;
}

}
