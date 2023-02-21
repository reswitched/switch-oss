/*
 * Copyright (C) 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2014 Apple Inc. All rights reserved.
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
#include "CSSParserValues.h"

#include "CSSCustomPropertyValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSFunctionValue.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "CSSVariableValue.h"
#include "SelectorPseudoTypeMap.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace WTF;

void destroy(const CSSParserValue& value)
{
    if (value.unit == CSSParserValue::Function)
        delete value.function;
    else if (value.unit == CSSParserValue::ValueList)
        delete value.valueList;
    else if (value.unit == CSSParserValue::Variable)
        delete value.variable;
}

CSSParserValueList::~CSSParserValueList()
{
    for (auto& value : m_values)
        destroy(value);
}

void CSSParserValueList::addValue(const CSSParserValue& value)
{
    m_values.append(value);
}

void CSSParserValueList::insertValueAt(unsigned i, const CSSParserValue& value)
{
    m_values.insert(i, value);
}

void CSSParserValueList::extend(CSSParserValueList& other)
{
    for (auto& value : other.m_values) {
        m_values.append(value);
        value.unit = 0; // We moved the CSSParserValue from the other list; this acts like std::move.
    }
}

bool CSSParserValueList::containsVariables() const
{
    for (unsigned i = 0; i < size(); i++) {
        auto* parserValue = &m_values[i];
        if (parserValue->unit == CSSParserValue::Variable)
            return true;
        if (parserValue->unit == CSSParserValue::Function && parserValue->function->args
            && parserValue->function->args->containsVariables())
            return true;
        if (parserValue->unit == CSSParserValue::ValueList && parserValue->valueList->containsVariables())
            return true;
    }
    return false;
}

PassRefPtr<CSSValue> CSSParserValue::createCSSValue()
{
    RefPtr<CSSValue> parsedValue;
    if (id)
        return CSSPrimitiveValue::createIdentifier(id);
    
    if (unit == CSSParserValue::Operator) {
        RefPtr<CSSPrimitiveValue> primitiveValue = CSSPrimitiveValue::createParserOperator(iValue);
        primitiveValue->setPrimitiveType(CSSPrimitiveValue::CSS_PARSER_OPERATOR);
        return primitiveValue.release();
    }
    if (unit == CSSParserValue::Function)
        return CSSFunctionValue::create(function);
    if (unit == CSSParserValue::Variable)
        return CSSVariableValue::create(variable);
    if (unit == CSSParserValue::ValueList)
        return CSSValueList::createFromParserValueList(*valueList);

    if (unit >= CSSParserValue::Q_EMS)
        return CSSPrimitiveValue::createAllowingMarginQuirk(fValue, CSSPrimitiveValue::CSS_EMS);

    CSSPrimitiveValue::UnitTypes primitiveUnit = static_cast<CSSPrimitiveValue::UnitTypes>(unit);
    switch (primitiveUnit) {
    case CSSPrimitiveValue::CSS_IDENT:
    case CSSPrimitiveValue::CSS_PROPERTY_ID:
    case CSSPrimitiveValue::CSS_VALUE_ID:
        return CSSPrimitiveValue::create(string, CSSPrimitiveValue::CSS_PARSER_IDENTIFIER);
    case CSSPrimitiveValue::CSS_NUMBER:
        return CSSPrimitiveValue::create(fValue, isInt ? CSSPrimitiveValue::CSS_PARSER_INTEGER : CSSPrimitiveValue::CSS_NUMBER);
    case CSSPrimitiveValue::CSS_STRING:
    case CSSPrimitiveValue::CSS_URI:
    case CSSPrimitiveValue::CSS_PARSER_HEXCOLOR:
    case CSSPrimitiveValue::CSS_DIMENSION:
    case CSSPrimitiveValue::CSS_UNICODE_RANGE:
    case CSSPrimitiveValue::CSS_PARSER_WHITESPACE:
        return CSSPrimitiveValue::create(string, primitiveUnit);
    case CSSPrimitiveValue::CSS_PERCENTAGE:
    case CSSPrimitiveValue::CSS_EMS:
    case CSSPrimitiveValue::CSS_EXS:
    case CSSPrimitiveValue::CSS_PX:
    case CSSPrimitiveValue::CSS_CM:
    case CSSPrimitiveValue::CSS_MM:
    case CSSPrimitiveValue::CSS_IN:
    case CSSPrimitiveValue::CSS_PT:
    case CSSPrimitiveValue::CSS_PC:
    case CSSPrimitiveValue::CSS_DEG:
    case CSSPrimitiveValue::CSS_RAD:
    case CSSPrimitiveValue::CSS_GRAD:
    case CSSPrimitiveValue::CSS_MS:
    case CSSPrimitiveValue::CSS_S:
    case CSSPrimitiveValue::CSS_HZ:
    case CSSPrimitiveValue::CSS_KHZ:
    case CSSPrimitiveValue::CSS_VW:
    case CSSPrimitiveValue::CSS_VH:
    case CSSPrimitiveValue::CSS_VMIN:
    case CSSPrimitiveValue::CSS_VMAX:
    case CSSPrimitiveValue::CSS_TURN:
    case CSSPrimitiveValue::CSS_REMS:
    case CSSPrimitiveValue::CSS_CHS:
    case CSSPrimitiveValue::CSS_FR:
        return CSSPrimitiveValue::create(fValue, primitiveUnit);
    case CSSPrimitiveValue::CSS_UNKNOWN:
    case CSSPrimitiveValue::CSS_ATTR:
    case CSSPrimitiveValue::CSS_COUNTER:
    case CSSPrimitiveValue::CSS_RECT:
    case CSSPrimitiveValue::CSS_RGBCOLOR:
    case CSSPrimitiveValue::CSS_DPPX:
    case CSSPrimitiveValue::CSS_DPI:
    case CSSPrimitiveValue::CSS_DPCM:
    case CSSPrimitiveValue::CSS_PAIR:
#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPrimitiveValue::CSS_DASHBOARD_REGION:
#endif
    case CSSPrimitiveValue::CSS_PARSER_OPERATOR:
    case CSSPrimitiveValue::CSS_PARSER_INTEGER:
    case CSSPrimitiveValue::CSS_PARSER_IDENTIFIER:
    case CSSPrimitiveValue::CSS_COUNTER_NAME:
    case CSSPrimitiveValue::CSS_SHAPE:
    case CSSPrimitiveValue::CSS_FONT_FAMILY:
    case CSSPrimitiveValue::CSS_QUAD:
#if ENABLE(CSS_SCROLL_SNAP)
    case CSSPrimitiveValue::CSS_LENGTH_REPEAT:
#endif
    case CSSPrimitiveValue::CSS_CALC:
    case CSSPrimitiveValue::CSS_CALC_PERCENTAGE_WITH_NUMBER:
    case CSSPrimitiveValue::CSS_CALC_PERCENTAGE_WITH_LENGTH:
        return 0;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

CSSParserSelector* CSSParserSelector::parsePagePseudoSelector(const CSSParserString& pseudoTypeString)
{
    CSSSelector::PagePseudoClassType pseudoType;
    if (pseudoTypeString.equalIgnoringCase("first"))
        pseudoType = CSSSelector::PagePseudoClassFirst;
    else if (pseudoTypeString.equalIgnoringCase("left"))
        pseudoType = CSSSelector::PagePseudoClassLeft;
    else if (pseudoTypeString.equalIgnoringCase("right"))
        pseudoType = CSSSelector::PagePseudoClassRight;
    else
        return nullptr;

    auto selector = std::make_unique<CSSParserSelector>();
    selector->m_selector->setMatch(CSSSelector::PagePseudoClass);
    selector->m_selector->setPagePseudoType(pseudoType);
    return selector.release();
}

CSSParserSelector* CSSParserSelector::parsePseudoElementSelector(CSSParserString& pseudoTypeString)
{
    pseudoTypeString.lower();
    AtomicString name = pseudoTypeString;

    CSSSelector::PseudoElementType pseudoType = CSSSelector::parsePseudoElementType(name);
    if (pseudoType == CSSSelector::PseudoElementUnknown)
        return nullptr;

    auto selector = std::make_unique<CSSParserSelector>();
    selector->m_selector->setMatch(CSSSelector::PseudoElement);
    selector->m_selector->setPseudoElementType(pseudoType);
    selector->m_selector->setValue(name);
    return selector.release();
}

#if ENABLE(VIDEO_TRACK)
CSSParserSelector* CSSParserSelector::parsePseudoElementCueFunctionSelector(const CSSParserString& functionIdentifier, Vector<std::unique_ptr<CSSParserSelector>>* parsedSelectorVector)
{
    ASSERT_UNUSED(functionIdentifier, String(functionIdentifier) == "cue(");

    std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> selectorVector(parsedSelectorVector);

    if (!selectorVector)
        return nullptr;

    auto selector = std::make_unique<CSSParserSelector>();
    selector->m_selector->setMatch(CSSSelector::PseudoElement);
    selector->m_selector->setPseudoElementType(CSSSelector::PseudoElementCue);
    selector->adoptSelectorVector(*selectorVector);
    return selector.release();
}
#endif

CSSParserSelector* CSSParserSelector::parsePseudoClassAndCompatibilityElementSelector(CSSParserString& pseudoTypeString)
{
    if (pseudoTypeString.length() && pseudoTypeString[pseudoTypeString.length() - 1] == '(')
        return nullptr;

    PseudoClassOrCompatibilityPseudoElement pseudoType = parsePseudoClassAndCompatibilityElementString(pseudoTypeString);
    if (pseudoType.pseudoClass != CSSSelector::PseudoClassUnknown) {
        auto selector = std::make_unique<CSSParserSelector>();
        selector->m_selector->setMatch(CSSSelector::PseudoClass);
        selector->m_selector->setPseudoClassType(pseudoType.pseudoClass);
        return selector.release();
    }
    if (pseudoType.compatibilityPseudoElement != CSSSelector::PseudoElementUnknown) {
        auto selector = std::make_unique<CSSParserSelector>();
        selector->m_selector->setMatch(CSSSelector::PseudoElement);
        selector->m_selector->setPseudoElementType(pseudoType.compatibilityPseudoElement);
        AtomicString name = pseudoTypeString;
        selector->m_selector->setValue(name);
        return selector.release();
    }
    return nullptr;
}

CSSParserSelector::CSSParserSelector()
    : m_selector(std::make_unique<CSSSelector>())
{
}

CSSParserSelector::CSSParserSelector(const QualifiedName& tagQName)
    : m_selector(std::make_unique<CSSSelector>(tagQName))
{
}

CSSParserSelector::~CSSParserSelector()
{
    if (!m_tagHistory)
        return;
    Vector<std::unique_ptr<CSSParserSelector>, 16> toDelete;
    std::unique_ptr<CSSParserSelector> selector = WTF::move(m_tagHistory);
    while (true) {
        std::unique_ptr<CSSParserSelector> next = WTF::move(selector->m_tagHistory);
        toDelete.append(WTF::move(selector));
        if (!next)
            break;
        selector = WTF::move(next);
    }
}

void CSSParserSelector::adoptSelectorVector(Vector<std::unique_ptr<CSSParserSelector>>& selectorVector)
{
    auto selectorList = std::make_unique<CSSSelectorList>();
    selectorList->adoptSelectorVector(selectorVector);
    m_selector->setSelectorList(WTF::move(selectorList));
}

void CSSParserSelector::setLangArgumentList(const Vector<CSSParserString>& stringVector)
{
    ASSERT_WITH_MESSAGE(!stringVector.isEmpty(), "No CSS Selector takes an empty argument list.");
    auto argumentList = std::make_unique<Vector<AtomicString>>();
    argumentList->reserveInitialCapacity(stringVector.size());
    for (const AtomicString& languageArgument : stringVector)
        argumentList->append(languageArgument);
    m_selector->setLangArgumentList(WTF::move(argumentList));
}

void CSSParserSelector::setPseudoClassValue(const CSSParserString& pseudoClassString)
{
    ASSERT(m_selector->match() == CSSSelector::PseudoClass);

    PseudoClassOrCompatibilityPseudoElement pseudoType = parsePseudoClassAndCompatibilityElementString(pseudoClassString);
    m_selector->setPseudoClassType(pseudoType.pseudoClass);
}

static bool selectorListMatchesPseudoElement(const CSSSelectorList* selectorList)
{
    if (!selectorList)
        return false;

    for (const CSSSelector* subSelector = selectorList->first(); subSelector; subSelector = CSSSelectorList::next(subSelector)) {
        for (const CSSSelector* selector = subSelector; selector; selector = selector->tagHistory()) {
            if (selector->matchesPseudoElement())
                return true;
            if (const CSSSelectorList* subselectorList = selector->selectorList()) {
                if (selectorListMatchesPseudoElement(subselectorList))
                    return true;
            }
        }
    }
    return false;
}

bool CSSParserSelector::matchesPseudoElement() const
{
    return m_selector->matchesPseudoElement() || selectorListMatchesPseudoElement(m_selector->selectorList());
}

void CSSParserSelector::insertTagHistory(CSSSelector::Relation before, std::unique_ptr<CSSParserSelector> selector, CSSSelector::Relation after)
{
    if (m_tagHistory)
        selector->setTagHistory(WTF::move(m_tagHistory));
    setRelation(before);
    selector->setRelation(after);
    m_tagHistory = WTF::move(selector);
}

void CSSParserSelector::appendTagHistory(CSSSelector::Relation relation, std::unique_ptr<CSSParserSelector> selector)
{
    CSSParserSelector* end = this;
    while (end->tagHistory())
        end = end->tagHistory();

    end->setRelation(relation);
    end->setTagHistory(WTF::move(selector));
}

void CSSParserSelector::appendTagHistory(CSSParserSelectorCombinator relation, std::unique_ptr<CSSParserSelector> selector)
{
    CSSParserSelector* end = this;
    while (end->tagHistory())
        end = end->tagHistory();

    CSSSelector::Relation selectorRelation;
    switch (relation) {
    case CSSParserSelectorCombinator::Child:
        selectorRelation = CSSSelector::Child;
        break;
    case CSSParserSelectorCombinator::DescendantSpace:
        selectorRelation = CSSSelector::Descendant;
        break;
#if ENABLE(CSS_SELECTORS_LEVEL4)
    case CSSParserSelectorCombinator::DescendantDoubleChild:
        selectorRelation = CSSSelector::Descendant;
        break;
#endif
    case CSSParserSelectorCombinator::DirectAdjacent:
        selectorRelation = CSSSelector::DirectAdjacent;
        break;
    case CSSParserSelectorCombinator::IndirectAdjacent:
        selectorRelation = CSSSelector::IndirectAdjacent;
        break;
    }
    end->setRelation(selectorRelation);

#if ENABLE(CSS_SELECTORS_LEVEL4)
    if (relation == CSSParserSelectorCombinator::DescendantDoubleChild)
        end->setDescendantUseDoubleChildSyntax();
#endif

    end->setTagHistory(WTF::move(selector));
}

void CSSParserSelector::prependTagSelector(const QualifiedName& tagQName, bool tagIsForNamespaceRule)
{
    auto second = std::make_unique<CSSParserSelector>();
    second->m_selector = WTF::move(m_selector);
    second->m_tagHistory = WTF::move(m_tagHistory);
    m_tagHistory = WTF::move(second);

    m_selector = std::make_unique<CSSSelector>(tagQName, tagIsForNamespaceRule);
    m_selector->setRelation(CSSSelector::SubSelector);
}

}

