/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (c) 2010-2018 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_HTMLINPUTELEMENT_H_
#define _WKC_HELPERS_WKC_HTMLINPUTELEMENT_H_

#include <wkc/wkcbase.h>

#include "WKCHTMLFormControlElement.h"

namespace WKC {
class HTMLInputElementPrivate;
class RenderObject;
class String;

class WKC_API HTMLInputElement : public WKC::HTMLFormControlElement {
public:
    const String value() const;
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

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    HTMLInputElement(HTMLInputElementPrivate&);
    virtual ~HTMLInputElement() {}

private:
    HTMLInputElement(const HTMLInputElement&);
    HTMLInputElement& operator=(const HTMLInputElement&);
};
} // namespace

#endif // _WKC_HELPERS_WKC_HTMLINPUTELEMENT_H_

