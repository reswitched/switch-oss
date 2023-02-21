/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef PropertySetCSSStyleDeclaration_h
#define PropertySetCSSStyleDeclaration_h

#include "CSSStyleDeclaration.h"
#include <memory>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSRule;
class CSSProperty;
class CSSValue;
class MutableStyleProperties;
class StyleSheetContents;
class StyledElement;

class PropertySetCSSStyleDeclaration : public CSSStyleDeclaration {
public:
    PropertySetCSSStyleDeclaration(MutableStyleProperties* propertySet) : m_propertySet(propertySet) { }
    
    virtual void clearParentElement() { ASSERT_NOT_REACHED(); }
    StyleSheetContents* contextStyleSheet() const;
    
    virtual void ref() override;
    virtual void deref() override;

private:
    virtual CSSRule* parentRule() const override { return nullptr; }
    virtual unsigned length() const override final;
    virtual String item(unsigned index) const override final;
    virtual PassRefPtr<CSSValue> getPropertyCSSValue(const String& propertyName) override final;
    virtual String getPropertyValue(const String& propertyName) override final;
    virtual String getPropertyPriority(const String& propertyName) override final;
    virtual String getPropertyShorthand(const String& propertyName) override final;
    virtual bool isPropertyImplicit(const String& propertyName) override final;
    virtual void setProperty(const String& propertyName, const String& value, const String& priority, ExceptionCode&) override final;
    virtual String removeProperty(const String& propertyName, ExceptionCode&) override final;
    virtual String cssText() const override final;
    virtual void setCssText(const String&, ExceptionCode&) override final;
    virtual PassRefPtr<CSSValue> getPropertyCSSValueInternal(CSSPropertyID) override final;
    virtual String getPropertyValueInternal(CSSPropertyID) override final;
    virtual bool setPropertyInternal(CSSPropertyID, const String& value, bool important, ExceptionCode&) override final;
    
    virtual Ref<MutableStyleProperties> copyProperties() const override final;

    CSSValue* cloneAndCacheForCSSOM(CSSValue*);
    
protected:
    enum MutationType { NoChanges, PropertyChanged };
    virtual bool willMutate() WARN_UNUSED_RETURN { return true; }
    virtual void didMutate(MutationType) { }

    MutableStyleProperties* m_propertySet;
    std::unique_ptr<HashMap<CSSValue*, RefPtr<CSSValue>>> m_cssomCSSValueClones;
};

class StyleRuleCSSStyleDeclaration final : public PropertySetCSSStyleDeclaration
{
public:
    static PassRefPtr<StyleRuleCSSStyleDeclaration> create(MutableStyleProperties& propertySet, CSSRule& parentRule)
    {
        return adoptRef(new StyleRuleCSSStyleDeclaration(propertySet, parentRule));
    }
    virtual ~StyleRuleCSSStyleDeclaration();

    void clearParentRule() { m_parentRule = 0; }
    
    virtual void ref() override;
    virtual void deref() override;

    void reattach(MutableStyleProperties&);

private:
    StyleRuleCSSStyleDeclaration(MutableStyleProperties&, CSSRule&);

    virtual CSSStyleSheet* parentStyleSheet() const override;

    virtual CSSRule* parentRule() const override { return m_parentRule;  }

    virtual bool willMutate() override WARN_UNUSED_RETURN;
    virtual void didMutate(MutationType) override;

    unsigned m_refCount;
    CSSRule* m_parentRule;
};

class InlineCSSStyleDeclaration final : public PropertySetCSSStyleDeclaration
{
public:
    InlineCSSStyleDeclaration(MutableStyleProperties* propertySet, StyledElement* parentElement)
        : PropertySetCSSStyleDeclaration(propertySet)
        , m_parentElement(parentElement) 
    {
    }
    
private:
    virtual CSSStyleSheet* parentStyleSheet() const override;
    virtual StyledElement* parentElement() const override { return m_parentElement; }
    virtual void clearParentElement() override { m_parentElement = 0; }

    virtual void didMutate(MutationType) override;

    StyledElement* m_parentElement;
};

} // namespace WebCore

#endif
