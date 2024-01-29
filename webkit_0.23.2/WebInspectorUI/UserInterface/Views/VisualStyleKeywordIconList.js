/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

WebInspector.VisualStyleKeywordIconList = class VisualStyleKeywordIconList extends WebInspector.VisualStylePropertyEditor
{
    constructor(propertyNames, text, possibleValues, layoutReversed)
    {
        super(propertyNames, text, possibleValues, null, "keyword-icon-list", layoutReversed);

        this._iconListContainer = document.createElement("div");
        this._iconListContainer.classList.add("keyword-icon-list-container");

        this._iconElements = [];
        this._computedIcon = null;
        this._selectedIcon = null;

        function dashToCapital(match) {
            return match[1].toUpperCase();
        }

        var prettyPropertyReferenceName = this._propertyReferenceName.capitalize().replace(/(-.)/g, dashToCapital);

        function createListItem(value, title) {
            var iconButtonElement = document.createElement("button");
            iconButtonElement.id = value;
            iconButtonElement.title = title;
            iconButtonElement.classList.add("keyword-icon");
            iconButtonElement.addEventListener("click", this._handleKeywordChanged.bind(this));

            var imageName = value === "none" ? "VisualStyleNone" : prettyPropertyReferenceName + title.replace(/\s/g, "");
            iconButtonElement.appendChild(useSVGSymbol("Images/" + imageName + ".svg"));

            return iconButtonElement;
        }

        for (var key in this._possibleValues.basic) {
            var iconElement = createListItem.call(this, key, this._possibleValues.basic[key]);
            this._iconListContainer.appendChild(iconElement);
            this._iconElements.push(iconElement);
        }

        this.contentElement.appendChild(this._iconListContainer);
    }

    // Public

    get value()
    {
        if (!this._selectedIcon)
            return null;

        return this._selectedIcon.id;
    }

    set value(value)
    {
        this._computedIcon = null;
        this._selectedIcon = null;
        for (var icon of this._iconElements) {
            if (icon.id === this._updatedValues.placeholder)
                this._computedIcon = icon;

            if (icon.id === value && !this._propertyMissing)
                this._selectedIcon = icon;
            else
                icon.classList.remove("selected", "computed");
        }

        if (!this._computedIcon)
            this._computedIcon = this._iconElements[0];

        var iconIsSelected = this._selectedIcon && this._selectedIcon.classList.toggle("selected");
        if (!iconIsSelected) {
            this._selectedIcon = null;
            this._propertyMissing = true;
            this._computedIcon.classList.add("computed");
        }
    }

    get synthesizedValue()
    {
        return this.value;
    }

    // Private

    _handleKeywordChanged(event)
    {
        this._propertyMissing = false;
        this.value = event.target.id;
        this._valueDidChange();
    }
};
