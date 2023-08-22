/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.RadioButtonNavigationItem = class RadioButtonNavigationItem extends WebInspector.ButtonNavigationItem
{
    constructor(identifier, toolTip, image, imageWidth, imageHeight)
    {
        super(identifier, toolTip, image, imageWidth, imageHeight, null, "tab");
    }

    // Public

    get selected()
    {
        return this.element.classList.contains(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
    }

    set selected(flag)
    {
        if (flag) {
            this.element.classList.add(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "true");
        } else {
            this.element.classList.remove(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "false");
        }
    }

    get active()
    {
        return this.element.classList.contains(WebInspector.RadioButtonNavigationItem.ActiveStyleClassName);
    }

    set active(flag)
    {
        this.element.classList.toggle(WebInspector.RadioButtonNavigationItem.ActiveStyleClassName, flag);
    }

    updateLayout(expandOnly)
    {
        if (expandOnly)
            return;

        var isSelected = this.selected;

        if (!isSelected) {
            this.element.classList.add(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "true");
        }

        var selectedWidth = this.element.offsetWidth;
        if (selectedWidth)
            this.element.style.minWidth = selectedWidth + "px";

        if (!isSelected) {
            this.element.classList.remove(WebInspector.RadioButtonNavigationItem.SelectedStyleClassName);
            this.element.setAttribute("aria-selected", "false");
        }
    }

    // Protected

    get additionalClassNames()
    {
        return ["radio", "button"];
    }
};

WebInspector.RadioButtonNavigationItem.StyleClassName = "radio";
WebInspector.RadioButtonNavigationItem.ActiveStyleClassName = "active";
WebInspector.RadioButtonNavigationItem.SelectedStyleClassName = "selected";
