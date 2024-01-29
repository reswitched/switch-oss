/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "ElementRuleCollector.h"

#include "CSSDefaultStyleSheets.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "CSSValueKeywords.h"
#include "HTMLElement.h"
#include "InspectorInstrumentation.h"
#include "RenderRegion.h"
#include "SVGElement.h"
#include "SelectorCompiler.h"
#include "StyleProperties.h"
#include "StyledElement.h"

#include <wtf/TemporaryChange.h>

namespace WebCore {

static const StyleProperties& leftToRightDeclaration()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<Ref<MutableStyleProperties>> leftToRightDecl(MutableStyleProperties::create());
    if (leftToRightDecl.get()->isEmpty())
        leftToRightDecl.get()->setProperty(CSSPropertyDirection, CSSValueLtr);
    return leftToRightDecl.get();
#else
    WKC_DEFINE_STATIC_PTR(MutableStyleProperties*, leftToRightDecl, &MutableStyleProperties::create().leakRef());
    if (leftToRightDecl->isEmpty())
        leftToRightDecl->setProperty(CSSPropertyDirection, CSSValueLtr);
    return *leftToRightDecl;
#endif
}

static const StyleProperties& rightToLeftDeclaration()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<Ref<MutableStyleProperties>> rightToLeftDecl(MutableStyleProperties::create());
    if (rightToLeftDecl.get()->isEmpty())
        rightToLeftDecl.get()->setProperty(CSSPropertyDirection, CSSValueRtl);
    return rightToLeftDecl.get();
#else
    WKC_DEFINE_STATIC_PTR(MutableStyleProperties*, rightToLeftDecl, &MutableStyleProperties::create().leakRef());
    if (rightToLeftDecl->isEmpty())
        rightToLeftDecl->setProperty(CSSPropertyDirection, CSSValueRtl);
    return *rightToLeftDecl;
#endif
}

class MatchRequest {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    MatchRequest(RuleSet* ruleSet, bool includeEmptyRules = false)
        : ruleSet(ruleSet)
        , includeEmptyRules(includeEmptyRules)
    {
    }
    const RuleSet* ruleSet;
    const bool includeEmptyRules;
};

StyleResolver::MatchResult& ElementRuleCollector::matchedResult()
{
    ASSERT(m_mode == SelectorChecker::Mode::ResolvingStyle);
    return m_result;
}

const Vector<RefPtr<StyleRule>>& ElementRuleCollector::matchedRuleList() const
{
    ASSERT(m_mode == SelectorChecker::Mode::CollectingRules);
    return m_matchedRuleList;
}

inline void ElementRuleCollector::addMatchedRule(const MatchedRule& matchedRule)
{
    if (!m_matchedRules)
        m_matchedRules = std::make_unique<Vector<MatchedRule, 32>>();
    m_matchedRules->append(matchedRule);
}

void ElementRuleCollector::clearMatchedRules()
{
    if (!m_matchedRules)
        return;
    m_matchedRules->clear();
}

inline void ElementRuleCollector::addElementStyleProperties(const StyleProperties* propertySet, bool isCacheable)
{
    if (!propertySet)
        return;
    m_result.ranges.lastAuthorRule = m_result.matchedProperties().size();
    if (m_result.ranges.firstAuthorRule == -1)
        m_result.ranges.firstAuthorRule = m_result.ranges.lastAuthorRule;
    m_result.addMatchedProperties(*propertySet);
    if (!isCacheable)
        m_result.isCacheable = false;
}

class MatchingUARulesScope {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    MatchingUARulesScope();
    ~MatchingUARulesScope();

    static bool isMatchingUARules();

private:
#if !PLATFORM(WKC)
    static bool m_matchingUARules;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(bool, m_matchingUARules);
#endif
};

MatchingUARulesScope::MatchingUARulesScope()
{
    ASSERT(!m_matchingUARules);
    m_matchingUARules = true;
}

MatchingUARulesScope::~MatchingUARulesScope()
{
    m_matchingUARules = false;
}

inline bool MatchingUARulesScope::isMatchingUARules()
{
    return m_matchingUARules;
}

#if !PLATFORM(WKC)
bool MatchingUARulesScope::m_matchingUARules = false;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(bool, MatchingUARulesScope, m_matchingUARules, false);
#endif

void ElementRuleCollector::collectMatchingRules(const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    ASSERT(matchRequest.ruleSet);
    ASSERT_WITH_MESSAGE(!(m_mode == SelectorChecker::Mode::ResolvingStyle && !m_style), "When resolving style, the SelectorChecker must have a style to set the pseudo elements and/or to do marking. The SelectorCompiler also rely on that behavior.");
    ASSERT_WITH_MESSAGE(!(m_mode == SelectorChecker::Mode::CollectingRulesIgnoringVirtualPseudoElements && m_pseudoStyleRequest.pseudoId != NOPSEUDO), "When in StyleInvalidation or SharingRules, SelectorChecker does not try to match the pseudo ID. While ElementRuleCollector supports matching a particular pseudoId in this case, this would indicate a error at the call site since matching a particular element should be unnecessary.");

#if ENABLE(VIDEO_TRACK)
    if (m_element.isWebVTTElement())
        collectMatchingRulesForList(matchRequest.ruleSet->cuePseudoRules(), matchRequest, ruleRange);
#endif

    if (m_element.isInShadowTree()) {
        const AtomicString& pseudoId = m_element.shadowPseudoId();
        if (!pseudoId.isEmpty())
            collectMatchingRulesForList(matchRequest.ruleSet->shadowPseudoElementRules(pseudoId.impl()), matchRequest, ruleRange);

        // Only match UA rules in shadow tree.
        if (!MatchingUARulesScope::isMatchingUARules())
            return;
    }

    // We need to collect the rules for id, class, tag, and everything else into a buffer and
    // then sort the buffer.
    if (m_element.hasID())
        collectMatchingRulesForList(matchRequest.ruleSet->idRules(m_element.idForStyleResolution().impl()), matchRequest, ruleRange);
    if (m_element.hasClass()) {
        for (size_t i = 0; i < m_element.classNames().size(); ++i)
            collectMatchingRulesForList(matchRequest.ruleSet->classRules(m_element.classNames()[i].impl()), matchRequest, ruleRange);
    }

    if (m_element.isLink())
        collectMatchingRulesForList(matchRequest.ruleSet->linkPseudoClassRules(), matchRequest, ruleRange);
    if (SelectorChecker::matchesFocusPseudoClass(&m_element))
        collectMatchingRulesForList(matchRequest.ruleSet->focusPseudoClassRules(), matchRequest, ruleRange);
    collectMatchingRulesForList(matchRequest.ruleSet->tagRules(m_element.localName().impl(), m_element.isHTMLElement() && m_element.document().isHTMLDocument()), matchRequest, ruleRange);
    collectMatchingRulesForList(matchRequest.ruleSet->universalRules(), matchRequest, ruleRange);
}

void ElementRuleCollector::collectMatchingRulesForRegion(const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    if (!m_regionForStyling)
        return;

    unsigned size = matchRequest.ruleSet->regionSelectorsAndRuleSets().size();
    for (unsigned i = 0; i < size; ++i) {
        const CSSSelector* regionSelector = matchRequest.ruleSet->regionSelectorsAndRuleSets().at(i).selector;
        if (checkRegionSelector(regionSelector, m_regionForStyling->generatingElement())) {
            RuleSet* regionRules = matchRequest.ruleSet->regionSelectorsAndRuleSets().at(i).ruleSet.get();
            ASSERT(regionRules);
            collectMatchingRules(MatchRequest(regionRules, matchRequest.includeEmptyRules), ruleRange);
        }
    }
}

void ElementRuleCollector::sortAndTransferMatchedRules()
{
    if (!m_matchedRules || m_matchedRules->isEmpty())
        return;

    sortMatchedRules();

    Vector<MatchedRule, 32>& matchedRules = *m_matchedRules;
    if (m_mode == SelectorChecker::Mode::CollectingRules) {
        for (const MatchedRule& matchedRule : matchedRules)
            m_matchedRuleList.append(matchedRule.ruleData->rule());
        return;
    }

    // Now transfer the set of matched rules over to our list of declarations.
    for (const MatchedRule& matchedRule : matchedRules) {
        if (m_style && matchedRule.ruleData->containsUncommonAttributeSelector())
            m_style->setUnique();
        m_result.addMatchedProperties(matchedRule.ruleData->rule()->properties(), matchedRule.ruleData->rule(), matchedRule.ruleData->linkMatchType(), matchedRule.ruleData->propertyWhitelistType(MatchingUARulesScope::isMatchingUARules()));
    }
}

void ElementRuleCollector::matchAuthorRules(bool includeEmptyRules)
{
    clearMatchedRules();
    m_result.ranges.lastAuthorRule = m_result.matchedProperties().size() - 1;

    // Match global author rules.
    MatchRequest matchRequest(m_ruleSets.authorStyle(), includeEmptyRules);
    StyleResolver::RuleRange ruleRange = m_result.ranges.authorRuleRange();
    collectMatchingRules(matchRequest, ruleRange);
    collectMatchingRulesForRegion(matchRequest, ruleRange);

    sortAndTransferMatchedRules();
}

void ElementRuleCollector::matchUserRules(bool includeEmptyRules)
{
    if (!m_ruleSets.userStyle())
        return;
    
    clearMatchedRules();

    m_result.ranges.lastUserRule = m_result.matchedProperties().size() - 1;
    MatchRequest matchRequest(m_ruleSets.userStyle(), includeEmptyRules);
    StyleResolver::RuleRange ruleRange = m_result.ranges.userRuleRange();
    collectMatchingRules(matchRequest, ruleRange);
    collectMatchingRulesForRegion(matchRequest, ruleRange);

    sortAndTransferMatchedRules();
}

void ElementRuleCollector::matchUARules()
{
    MatchingUARulesScope scope;

    // First we match rules from the user agent sheet.
    if (CSSDefaultStyleSheets::simpleDefaultStyleSheet)
        m_result.isCacheable = false;
    RuleSet* userAgentStyleSheet = m_isPrintStyle
        ? CSSDefaultStyleSheets::defaultPrintStyle : CSSDefaultStyleSheets::defaultStyle;
    matchUARules(userAgentStyleSheet);

    // In quirks mode, we match rules from the quirks user agent sheet.
    if (m_element.document().inQuirksMode())
        matchUARules(CSSDefaultStyleSheets::defaultQuirksStyle);
}

void ElementRuleCollector::matchUARules(RuleSet* rules)
{
    clearMatchedRules();
    
    m_result.ranges.lastUARule = m_result.matchedProperties().size() - 1;
    StyleResolver::RuleRange ruleRange = m_result.ranges.UARuleRange();
    collectMatchingRules(MatchRequest(rules), ruleRange);

    sortAndTransferMatchedRules();
}

inline bool ElementRuleCollector::ruleMatches(const RuleData& ruleData, unsigned& specificity)
{
    // We know a sufficiently simple single part selector matches simply because we found it from the rule hash when filtering the RuleSet.
    // This is limited to HTML only so we don't need to check the namespace (because of tag name match).
    MatchBasedOnRuleHash matchBasedOnRuleHash = ruleData.matchBasedOnRuleHash();
    if (matchBasedOnRuleHash != MatchBasedOnRuleHash::None && m_element.isHTMLElement()) {
        ASSERT_WITH_MESSAGE(m_pseudoStyleRequest.pseudoId == NOPSEUDO, "If we match based on the rule hash while collecting for a particular pseudo element ID, we would add incorrect rules for that pseudo element ID. We should never end in ruleMatches() with a pseudo element if the ruleData cannot match any pseudo element.");

        switch (matchBasedOnRuleHash) {
        case MatchBasedOnRuleHash::None:
            ASSERT_NOT_REACHED();
            break;
        case MatchBasedOnRuleHash::Universal:
            specificity = 0;
            break;
        case MatchBasedOnRuleHash::ClassA:
            specificity = static_cast<unsigned>(SelectorSpecificityIncrement::ClassA);
            break;
        case MatchBasedOnRuleHash::ClassB:
            specificity = static_cast<unsigned>(SelectorSpecificityIncrement::ClassB);
            break;
        case MatchBasedOnRuleHash::ClassC:
            specificity = static_cast<unsigned>(SelectorSpecificityIncrement::ClassC);
            break;
        }
        return true;
    }

#if ENABLE(CSS_SELECTOR_JIT)
    void* compiledSelectorChecker = ruleData.compiledSelectorCodeRef().code().executableAddress();
    if (!compiledSelectorChecker && ruleData.compilationStatus() == SelectorCompilationStatus::NotCompiled) {
        JSC::VM& vm = m_element.document().scriptExecutionContext()->vm();
        SelectorCompilationStatus compilationStatus;
        JSC::MacroAssemblerCodeRef compiledSelectorCodeRef;
        compilationStatus = SelectorCompiler::compileSelector(ruleData.selector(), &vm, SelectorCompiler::SelectorContext::RuleCollector, compiledSelectorCodeRef);

        ruleData.setCompiledSelector(compilationStatus, compiledSelectorCodeRef);
        compiledSelectorChecker = ruleData.compiledSelectorCodeRef().code().executableAddress();
    }

    if (compiledSelectorChecker && ruleData.compilationStatus() == SelectorCompilationStatus::SimpleSelectorChecker) {
        SelectorCompiler::RuleCollectorSimpleSelectorChecker selectorChecker = SelectorCompiler::ruleCollectorSimpleSelectorCheckerFunction(compiledSelectorChecker, ruleData.compilationStatus());
#if !ASSERT_MSG_DISABLED
        unsigned ignoreSpecificity;
        ASSERT_WITH_MESSAGE(!selectorChecker(&m_element, &ignoreSpecificity) || m_pseudoStyleRequest.pseudoId == NOPSEUDO, "When matching pseudo elements, we should never compile a selector checker without context unless it cannot match anything.");
#endif
#if CSS_SELECTOR_JIT_PROFILING
        ruleData.compiledSelectorUsed();
#endif
        return selectorChecker(&m_element, &specificity);
    }
#endif // ENABLE(CSS_SELECTOR_JIT)

    SelectorChecker::CheckingContext context(m_mode);
    context.elementStyle = m_style;
    context.pseudoId = m_pseudoStyleRequest.pseudoId;
    context.scrollbar = m_pseudoStyleRequest.scrollbar;
    context.scrollbarPart = m_pseudoStyleRequest.scrollbarPart;

#if ENABLE(CSS_SELECTOR_JIT)
    if (compiledSelectorChecker) {
        ASSERT(ruleData.compilationStatus() == SelectorCompilationStatus::SelectorCheckerWithCheckingContext);

        SelectorCompiler::RuleCollectorSelectorCheckerWithCheckingContext selectorChecker = SelectorCompiler::ruleCollectorSelectorCheckerFunctionWithCheckingContext(compiledSelectorChecker, ruleData.compilationStatus());

#if CSS_SELECTOR_JIT_PROFILING
        ruleData.compiledSelectorUsed();
#endif
        return selectorChecker(&m_element, &context, &specificity);
    }
#endif // ENABLE(CSS_SELECTOR_JIT)

    // Slow path.
    SelectorChecker selectorChecker(m_element.document());
    return selectorChecker.match(ruleData.selector(), &m_element, context, specificity);
}

void ElementRuleCollector::collectMatchingRulesForList(const RuleSet::RuleDataVector* rules, const MatchRequest& matchRequest, StyleResolver::RuleRange& ruleRange)
{
    if (!rules)
        return;

    for (unsigned i = 0, size = rules->size(); i < size; ++i) {
        const RuleData& ruleData = rules->data()[i];

        if (!ruleData.canMatchPseudoElement() && m_pseudoStyleRequest.pseudoId != NOPSEUDO)
            continue;

        if (m_canUseFastReject && m_selectorFilter.fastRejectSelector<RuleData::maximumIdentifierCount>(ruleData.descendantSelectorIdentifierHashes()))
            continue;

        StyleRule* rule = ruleData.rule();

        // If the rule has no properties to apply, then ignore it in the non-debug mode.
        const StyleProperties& properties = rule->properties();
        if (properties.isEmpty() && !matchRequest.includeEmptyRules)
            continue;

        // FIXME: Exposing the non-standard getMatchedCSSRules API to web is the only reason this is needed.
        if (m_sameOriginOnly && !ruleData.hasDocumentSecurityOrigin())
            continue;

        unsigned specificity;
        if (ruleMatches(ruleData, specificity)) {
            // Update our first/last rule indices in the matched rules array.
            ++ruleRange.lastRuleIndex;
            if (ruleRange.firstRuleIndex == -1)
                ruleRange.firstRuleIndex = ruleRange.lastRuleIndex;

            // Add this rule to our list of matched rules.
            addMatchedRule({&ruleData, specificity});
        }
    }
}

static inline bool compareRules(MatchedRule r1, MatchedRule r2)
{
    unsigned specificity1 = r1.specificity;
    unsigned specificity2 = r2.specificity;
    return (specificity1 == specificity2) ? r1.ruleData->position() < r2.ruleData->position() : specificity1 < specificity2;
}

void ElementRuleCollector::sortMatchedRules()
{
    ASSERT(m_matchedRules);
    std::sort(m_matchedRules->begin(), m_matchedRules->end(), compareRules);
}

void ElementRuleCollector::matchAllRules(bool matchAuthorAndUserStyles, bool includeSMILProperties)
{
    matchUARules();

    // Now we check user sheet rules.
    if (matchAuthorAndUserStyles)
        matchUserRules(false);

    // Now check author rules, beginning first with presentational attributes mapped from HTML.
    if (is<StyledElement>(m_element)) {
        StyledElement& styledElement = downcast<StyledElement>(m_element);
        addElementStyleProperties(styledElement.presentationAttributeStyle());

        // Now we check additional mapped declarations.
        // Tables and table cells share an additional mapped rule that must be applied
        // after all attributes, since their mapped style depends on the values of multiple attributes.
        addElementStyleProperties(styledElement.additionalPresentationAttributeStyle());

        if (is<HTMLElement>(styledElement)) {
            bool isAuto;
            TextDirection textDirection = downcast<HTMLElement>(styledElement).directionalityIfhasDirAutoAttribute(isAuto);
            if (isAuto)
                m_result.addMatchedProperties(textDirection == LTR ? leftToRightDeclaration() : rightToLeftDeclaration());
        }
    }
    
    // Check the rules in author sheets next.
    if (matchAuthorAndUserStyles)
        matchAuthorRules(false);

    if (matchAuthorAndUserStyles && is<StyledElement>(m_element)) {
        StyledElement& styledElement = downcast<StyledElement>(m_element);
        // Now check our inline style attribute.
        if (styledElement.inlineStyle()) {
            // Inline style is immutable as long as there is no CSSOM wrapper.
            // FIXME: Media control shadow trees seem to have problems with caching.
            bool isInlineStyleCacheable = !styledElement.inlineStyle()->isMutable() && !styledElement.isInShadowTree();
            // FIXME: Constify.
            addElementStyleProperties(styledElement.inlineStyle(), isInlineStyleCacheable);
        }

        // Now check SMIL animation override style.
        if (includeSMILProperties && is<SVGElement>(styledElement))
            addElementStyleProperties(downcast<SVGElement>(styledElement).animatedSMILStyleProperties(), false /* isCacheable */);
    }
}

bool ElementRuleCollector::hasAnyMatchingRules(RuleSet* ruleSet)
{
    clearMatchedRules();

    m_mode = SelectorChecker::Mode::CollectingRulesIgnoringVirtualPseudoElements;
    int firstRuleIndex = -1, lastRuleIndex = -1;
    StyleResolver::RuleRange ruleRange(firstRuleIndex, lastRuleIndex);
    collectMatchingRules(MatchRequest(ruleSet), ruleRange);

    return m_matchedRules && !m_matchedRules->isEmpty();
}

} // namespace WebCore
