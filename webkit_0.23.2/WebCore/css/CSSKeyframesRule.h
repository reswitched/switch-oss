/*
 * Copyright (C) 2007, 2008, 2012, 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSKeyframesRule_h
#define CSSKeyframesRule_h

#include "CSSRule.h"
#include "StyleRule.h"
#include <memory>
#include <wtf/Forward.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class CSSRuleList;
class StyleKeyframe;
class CSSKeyframeRule;

class StyleRuleKeyframes : public StyleRuleBase {
public:
    static Ref<StyleRuleKeyframes> create() { return adoptRef(*new StyleRuleKeyframes()); }
    
    ~StyleRuleKeyframes();
    
    const Vector<RefPtr<StyleKeyframe>>& keyframes() const { return m_keyframes; }
    
    void parserAppendKeyframe(PassRefPtr<StyleKeyframe>);
    void wrapperAppendKeyframe(PassRefPtr<StyleKeyframe>);
    void wrapperRemoveKeyframe(unsigned);

    String name() const { return m_name; }    
    void setName(const String& name) { m_name = AtomicString(name); }
    
    int findKeyframeIndex(const String& key) const;

    Ref<StyleRuleKeyframes> copy() const { return adoptRef(*new StyleRuleKeyframes(*this)); }

private:
    StyleRuleKeyframes();
    StyleRuleKeyframes(const StyleRuleKeyframes&);

    Vector<RefPtr<StyleKeyframe>> m_keyframes;
    AtomicString m_name;
};

class CSSKeyframesRule final : public CSSRule {
public:
    static Ref<CSSKeyframesRule> create(StyleRuleKeyframes& rule, CSSStyleSheet* sheet) { return adoptRef(*new CSSKeyframesRule(rule, sheet)); }

    virtual ~CSSKeyframesRule();

    virtual CSSRule::Type type() const override { return KEYFRAMES_RULE; }
    virtual String cssText() const override;
    virtual void reattach(StyleRuleBase&) override;

    String name() const { return m_keyframesRule->name(); }
    void setName(const String&);

    CSSRuleList& cssRules();

    void insertRule(const String& rule);
    void appendRule(const String& rule);
    void deleteRule(const String& key);
    CSSKeyframeRule* findRule(const String& key);

    // For IndexedGetter and CSSRuleList.
    unsigned length() const;
    CSSKeyframeRule* item(unsigned index) const;

private:
    CSSKeyframesRule(StyleRuleKeyframes&, CSSStyleSheet* parent);

    Ref<StyleRuleKeyframes> m_keyframesRule;
    mutable Vector<RefPtr<CSSKeyframeRule>> m_childRuleCSSOMWrappers;
    mutable std::unique_ptr<CSSRuleList> m_ruleListCSSOMWrapper;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_RULE(CSSKeyframesRule, CSSRule::KEYFRAMES_RULE)

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::StyleRuleKeyframes)
    static bool isType(const WebCore::StyleRuleBase& rule) { return rule.isKeyframesRule(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // CSSKeyframesRule_h
