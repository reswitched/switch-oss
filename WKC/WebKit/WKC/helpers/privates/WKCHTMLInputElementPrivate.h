/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef _WKC_HELPERS_PRIVATE_HTMLINPUTELEMENT_H_
#define _WKC_HELPERS_PRIVATE_HTMLINPUTELEMENT_H_

#include "helpers/WKCHTMLInputElement.h"
#include "helpers/privates/WKCHTMLFormControlElementPrivate.h"

namespace WebCore {
class HTMLInputElement;
} // namespace

namespace WKC {

class HTMLInputElementWrap : public HTMLInputElement {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLInputElementWrap(HTMLInputElementPrivate& parent) : HTMLInputElement(parent) {}
    ~HTMLInputElementWrap() {}
};

class HTMLInputElementPrivate : public HTMLFormControlElementPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLInputElementPrivate(WebCore::HTMLInputElement*);
    virtual ~HTMLInputElementPrivate();

    WebCore::HTMLInputElement* webcore() const;
    HTMLInputElement& wkc() { return m_wkc; }

    String value() const;
    bool readOnly() const;
    bool disabled() const;
    int maxLength() const;
    void setValue(const String&, bool sendChangeEvent = false);
    bool shouldAutocomplete() const;

    bool isTextButton() const;

    bool isRadioButton() const;
    bool isTextField() const;
    bool isSearchField() const;
    bool isInputTypeHidden() const;
    bool isPasswordField() const;
    bool isCheckbox() const;
    bool isRangeControl() const;

    bool isSteppable() const;

    bool isText() const;

    bool isEmailField() const;
    bool isFileUpload() const;
    bool isImageButton() const;
    bool isNumberField() const;
    bool isSubmitButton() const;
    bool isTelephoneField() const;
    bool isURLField() const;

    bool isSpeechEnabled() const;

    bool checked() const;

private:
    HTMLInputElementWrap m_wkc;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_HTMLINPUTELEMENT_H_

