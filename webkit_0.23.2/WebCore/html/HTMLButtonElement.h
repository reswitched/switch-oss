/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLButtonElement_h
#define HTMLButtonElement_h

#include "HTMLFormControlElement.h"

namespace WebCore {

class HTMLButtonElement final : public HTMLFormControlElement {
public:
    static Ref<HTMLButtonElement> create(const QualifiedName&, Document&, HTMLFormElement*);

    void setType(const AtomicString&);
    
    const AtomicString& value() const;

    virtual bool willRespondToMouseClickEvents() override;

private:
    HTMLButtonElement(const QualifiedName& tagName, Document&, HTMLFormElement*);

    enum Type { SUBMIT, RESET, BUTTON };

    virtual const AtomicString& formControlType() const override;

    virtual RenderPtr<RenderElement> createElementRenderer(Ref<RenderStyle>&&, const RenderTreePosition&) override;

    // HTMLFormControlElement always creates one, but buttons don't need it.
    virtual bool alwaysCreateUserAgentShadowRoot() const override { return false; }

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual bool isPresentationAttribute(const QualifiedName&) const override;
    virtual void defaultEventHandler(Event*) override;

    virtual bool appendFormData(FormDataList&, bool) override;

    virtual bool isEnumeratable() const override { return true; }
    virtual bool supportLabels() const override { return true; }

#if PLATFORM(WKC)
    virtual bool canBeSuccessfulSubmitButton() const override;
#endif
    virtual bool isSuccessfulSubmitButton() const override;
    virtual bool isActivatedSubmit() const override;
    virtual void setActivatedSubmit(bool flag) override;

    virtual void accessKeyAction(bool sendMouseEvents) override;
    virtual bool isURLAttribute(const Attribute&) const override;

    virtual bool canStartSelection() const override { return false; }

    virtual bool isOptionalFormControl() const override { return true; }
    virtual bool computeWillValidate() const override;

    Type m_type;
    bool m_isActivatedSubmit;
};

} // namespace

#endif
