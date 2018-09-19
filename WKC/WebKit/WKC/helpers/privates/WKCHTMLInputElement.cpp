/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "helpers/WKCHTMLInputElement.h"
#include "helpers/WKCString.h"

#include "HTMLInputElement.h"

#include "helpers/WKCString.h"
#include "helpers/privates/WKCHTMLInputElementPrivate.h"

namespace WKC {

HTMLInputElementPrivate::HTMLInputElementPrivate(WebCore::HTMLInputElement* parent)
    : HTMLFormControlElementPrivate(parent)
    , m_wkc(*this)
{
}

HTMLInputElementPrivate::~HTMLInputElementPrivate()
{
}

WebCore::HTMLInputElement*
HTMLInputElementPrivate::webcore() const
{
    return static_cast<WebCore::HTMLInputElement*>(HTMLElementPrivate::webcore());
}

String
HTMLInputElementPrivate::value() const
{
    if (!webcore())
        return String("");
    return webcore()->value();
}

int
HTMLInputElementPrivate::maxLength() const
{
    if (!webcore())
        return 0;
    return webcore()->maxLength();
}

void
HTMLInputElementPrivate::setValue(const String& str, bool sendChangeEvent)
{
    if (!webcore())
        return;
    webcore()->setValue(str, sendChangeEvent ? WebCore::DispatchInputAndChangeEvent : WebCore::DispatchNoEvent);
}
 
bool
HTMLInputElementPrivate::isTextButton() const
{
    if (!webcore())
        return false;
    return webcore()->isTextButton();
}
 
bool
HTMLInputElementPrivate::isRadioButton() const
{
    if (!webcore())
        return false;
    return webcore()->isRadioButton();
}

bool
HTMLInputElementPrivate::shouldAutocomplete() const
{
    if (!webcore())
        return false;
    return webcore()->shouldAutocomplete();
}

bool
HTMLInputElementPrivate::isTextField() const
{
    if (!webcore())
        return false;
    return webcore()->isTextField();
}

bool
HTMLInputElementPrivate::isSearchField() const
{
    if (!webcore())
        return false;
    return webcore()->isSearchField();
}

bool
HTMLInputElementPrivate::isInputTypeHidden() const
{
    if (!webcore())
        return false;
    return webcore()->isInputTypeHidden();
}

bool
HTMLInputElementPrivate::isPasswordField() const
{
    if (!webcore())
        return false;
    return webcore()->isPasswordField();
}

bool
HTMLInputElementPrivate::isCheckbox() const
{
    if (!webcore())
        return false;
    return webcore()->isCheckbox();
}

bool
HTMLInputElementPrivate::isRangeControl() const
{
    if (!webcore())
        return false;
    return webcore()->isRangeControl();
}

bool
HTMLInputElementPrivate::isSteppable() const
{
    if (!webcore())
        return false;
    return webcore()->isSteppable();
}
 
bool
HTMLInputElementPrivate::isText() const
{
    if (!webcore())
        return false;
    return webcore()->isText();
}

bool
HTMLInputElementPrivate::isEmailField() const
{
    if (!webcore())
        return false;
    return webcore()->isEmailField();
}

bool
HTMLInputElementPrivate::isFileUpload() const
{
    if (!webcore())
        return false;
    return webcore()->isFileUpload();
}

bool
HTMLInputElementPrivate::isImageButton() const
{
    if (!webcore())
        return false;
    return webcore()->isImageButton();
}

bool
HTMLInputElementPrivate::isNumberField() const
{
    if (!webcore())
        return false;
    return webcore()->isNumberField();
}

bool
HTMLInputElementPrivate::isSubmitButton() const
{
    if (!webcore())
        return false;
    return webcore()->isSubmitButton();
}

bool
HTMLInputElementPrivate::isTelephoneField() const
{
    if (!webcore())
        return false;
    return webcore()->isTelephoneField();
}

bool
HTMLInputElementPrivate::isURLField() const
{
    if (!webcore())
        return false;
    return webcore()->isURLField();
}

bool
HTMLInputElementPrivate::isSpeechEnabled() const
{
    if (!webcore())
        return false;
#if ENABLE(INPUT_SPEECH)
    return webcore()->isSpeechEnabled();
#else
    return false;
#endif
}

bool
HTMLInputElementPrivate::checked() const
{
    if (!webcore())
        return false;
    return webcore()->checked();
}

////////////////////////////////////////////////////////////////////////////////

HTMLInputElement::HTMLInputElement(HTMLInputElementPrivate& parent)
    : HTMLFormControlElement(parent)
{
}

const String
HTMLInputElement::value() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).value();
}

int
HTMLInputElement::maxLength() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).maxLength();
}

void
HTMLInputElement::setValue(const String& str, bool sendChangeEvent)
{
    static_cast<HTMLInputElementPrivate&>(priv()).setValue(str, sendChangeEvent);
}

bool
HTMLInputElement::shouldAutocomplete() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).shouldAutocomplete();
}

bool
HTMLInputElement::isTextButton() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isTextButton();
}

bool
HTMLInputElement::isRadioButton() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isRadioButton();
}

bool
HTMLInputElement::isTextField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isTextField();
}

bool
HTMLInputElement::isSearchField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isSearchField();
}

bool
HTMLInputElement::isInputTypeHidden() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isInputTypeHidden();
}

bool
HTMLInputElement::isPasswordField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isPasswordField();
}

bool
HTMLInputElement::isCheckbox() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isCheckbox();
}

bool
HTMLInputElement::isRangeControl() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isRangeControl();
}

bool
HTMLInputElement::isSteppable() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isSteppable();
}

bool
HTMLInputElement::isText() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isText();
}

bool
HTMLInputElement::isEmailField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isEmailField();
}

bool
HTMLInputElement::isFileUpload() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isFileUpload();
}

bool
HTMLInputElement::isImageButton() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isImageButton();
}

bool
HTMLInputElement::isNumberField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isNumberField();
}

bool
HTMLInputElement::isSubmitButton() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isSubmitButton();
}

bool
HTMLInputElement::isTelephoneField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isTelephoneField();
}

bool
HTMLInputElement::isURLField() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isURLField();
}

bool
HTMLInputElement::isSpeechEnabled() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).isSpeechEnabled();
}

bool
HTMLInputElement::checked() const
{
    return static_cast<HTMLInputElementPrivate&>(priv()).checked();
}

} // namespace
