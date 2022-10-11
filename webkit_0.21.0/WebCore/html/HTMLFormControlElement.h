/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLFormControlElement_h
#define HTMLFormControlElement_h

#include "FormAssociatedElement.h"
#include "LabelableElement.h"

#if ENABLE(IOS_AUTOCORRECT_AND_AUTOCAPITALIZE)
#include "Autocapitalize.h"
#endif

namespace WebCore {

class FormDataList;
class HTMLFieldSetElement;
class HTMLFormElement;
class HTMLLegendElement;
class ValidationMessage;

// HTMLFormControlElement is the default implementation of FormAssociatedElement,
// and form-associated element implementations should use HTMLFormControlElement
// unless there is a special reason.
class HTMLFormControlElement : public LabelableElement, public FormAssociatedElement {
public:
    virtual ~HTMLFormControlElement();

    HTMLFormElement* form() const { return FormAssociatedElement::form(); }

    String formEnctype() const;
    void setFormEnctype(const String&);
    String formMethod() const;
    void setFormMethod(const String&);
    bool formNoValidate() const;

    void setAncestorDisabled(bool isDisabled);

    virtual void reset() { }

    bool formControlValueMatchesRenderer() const { return m_valueMatchesRenderer; }
    void setFormControlValueMatchesRenderer(bool b) { m_valueMatchesRenderer = b; }

    bool wasChangedSinceLastFormControlChangeEvent() const { return m_wasChangedSinceLastFormControlChangeEvent; }
    void setChangedSinceLastFormControlChangeEvent(bool);

    virtual void dispatchFormControlChangeEvent();
    void dispatchChangeEvent();
    void dispatchFormControlInputEvent();

    virtual bool isDisabledFormControl() const override;

    virtual bool isFocusable() const override;
    virtual bool isEnumeratable() const override { return false; }

    bool isRequired() const;

    const AtomicString& type() const { return formControlType(); }

    virtual const AtomicString& formControlType() const = 0;

    virtual bool canTriggerImplicitSubmission() const { return false; }

    // Override in derived classes to get the encoded name=value pair for submitting.
    // Return true for a successful control (see HTML4-17.13.2).
    virtual bool appendFormData(FormDataList&, bool) override { return false; }

#if PLATFORM(WKC)
    virtual bool canBeSuccessfulSubmitButton() const { return false; }
#endif
    virtual bool isSuccessfulSubmitButton() const { return false; }
    virtual bool isActivatedSubmit() const { return false; }
    virtual void setActivatedSubmit(bool) { }

#if ENABLE(IOS_AUTOCORRECT_AND_AUTOCAPITALIZE)
    WEBCORE_EXPORT bool autocorrect() const;
    void setAutocorrect(bool);

    WEBCORE_EXPORT WebAutocapitalizeType autocapitalizeType() const;
    const AtomicString& autocapitalize() const;
    void setAutocapitalize(const AtomicString&);
#endif

    virtual bool willValidate() const override final;
    void updateVisibleValidationMessage();
    void hideVisibleValidationMessage();
    bool checkValidity(Vector<RefPtr<FormAssociatedElement>>* unhandledInvalidControls = 0);
    bool isShowingValidationMessage() const;
    // This must be called when a validation constraint or control value is changed.
    void updateValidity();
    virtual void setCustomValidity(const String&) override;

    bool isReadOnly() const { return m_isReadOnly; }
    bool isDisabledOrReadOnly() const { return isDisabledFormControl() || m_isReadOnly; }

    bool hasAutofocused() { return m_hasAutofocused; }
    void setAutofocused() { m_hasAutofocused = true; }

    static HTMLFormControlElement* enclosingFormControlElement(Node*);

    String autocomplete() const;
    void setAutocomplete(const String&);

    using Node::ref;
    using Node::deref;

protected:
    HTMLFormControlElement(const QualifiedName& tagName, Document&, HTMLFormElement*);

    bool disabledByAncestorFieldset() const { return m_disabledByAncestorFieldset; }

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual void disabledAttributeChanged();
    virtual void disabledStateChanged();
    virtual void readOnlyAttributeChanged();
    virtual void requiredAttributeChanged();
    virtual void didAttachRenderers() override;
    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;
    void finishedInsertingSubtree() override;
    virtual void removedFrom(ContainerNode&) override;
    virtual void didMoveToNewDocument(Document* oldDocument) override;

    virtual bool supportsFocus() const override;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const override;
    virtual bool isMouseFocusable() const override;

    virtual void didRecalcStyle(Style::Change) override;

    virtual void dispatchBlurEvent(RefPtr<Element>&& newFocusedElement) override;

    // This must be called any time the result of willValidate() has changed.
    void setNeedsWillValidateCheck();
    virtual bool computeWillValidate() const;

    bool validationMessageShadowTreeContains(const Node&) const;

    virtual void willChangeForm() override;
    virtual void didChangeForm() override;

private:
    virtual void refFormAssociatedElement() override { ref(); }
    virtual void derefFormAssociatedElement() override { deref(); }

    virtual bool matchesValidPseudoClass() const override;
    virtual bool matchesInvalidPseudoClass() const override;

    virtual bool isFormControlElement() const override final { return true; }
    virtual bool alwaysCreateUserAgentShadowRoot() const override { return true; }

    virtual short tabIndex() const override final;

    virtual HTMLFormElement* virtualForm() const override;
    virtual bool isDefaultButtonForForm() const override;
    bool isValidFormControlElement() const;

    bool computeIsDisabledByFieldsetAncestor() const;

    virtual HTMLElement& asHTMLElement() override final { return *this; }
    virtual const HTMLFormControlElement& asHTMLElement() const override final { return *this; }
    virtual HTMLFormControlElement* asFormNamedItem() override final { return this; }

    std::unique_ptr<ValidationMessage> m_validationMessage;
    bool m_disabled : 1;
    bool m_isReadOnly : 1;
    bool m_isRequired : 1;
    bool m_valueMatchesRenderer : 1;
    bool m_disabledByAncestorFieldset : 1;

    enum DataListAncestorState { Unknown, InsideDataList, NotInsideDataList };
    mutable enum DataListAncestorState m_dataListAncestorState;

    // The initial value of m_willValidate depends on the derived class. We can't
    // initialize it with a virtual function in the constructor. m_willValidate
    // is not deterministic as long as m_willValidateInitialized is false.
    mutable bool m_willValidateInitialized: 1;
    mutable bool m_willValidate : 1;

    // Cache of validity()->valid().
    // But "candidate for constraint validation" doesn't affect m_isValid.
    bool m_isValid : 1;

    bool m_wasChangedSinceLastFormControlChangeEvent : 1;

    bool m_hasAutofocused : 1;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::HTMLFormControlElement)
    static bool isType(const WebCore::Element& element) { return element.isFormControlElement(); }
    static bool isType(const WebCore::Node& node) { return is<WebCore::Element>(node) && isType(downcast<WebCore::Element>(node)); }
    static bool isType(const WebCore::FormAssociatedElement& element) { return element.isFormControlElement(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif
