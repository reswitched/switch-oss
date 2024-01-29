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

WebInspector.VisualStyleKeywordPicker = class VisualStyleKeywordPicker extends WebInspector.VisualStylePropertyEditor
{
    constructor(propertyNames, text, possibleValues, layoutReversed)
    {
        super(propertyNames, text, possibleValues, null, "keyword-picker", layoutReversed);

        this._keywordSelectElement = document.createElement("select");
        this._keywordSelectElement.classList.add("keyword-picker-select");
        if (this._possibleValues.advanced)
            this._keywordSelectElement.title = WebInspector.UIString("Option-click to show all values");

        this._unchangedOptionElement = document.createElement("option");
        this._unchangedOptionElement.value = "";
        this._unchangedOptionElement.text = WebInspector.UIString("Unchanged");
        this._keywordSelectElement.appendChild(this._unchangedOptionElement);

        this._keywordSelectElement.appendChild(document.createElement("hr"));

        this._createValueOptions(this._possibleValues.basic);
        this._advancedValuesElements = null;

        this._keywordSelectElement.addEventListener("mousedown", this._keywordSelectMouseDown.bind(this));
        this._keywordSelectElement.addEventListener("change", this._handleKeywordChanged.bind(this));
        this.contentElement.appendChild(this._keywordSelectElement);
    }

    // Public

    get value()
    {
        // FIXME: <https://webkit.org/b/147064> Getter and setter on super are called with wrong "this" object
        return this._getValue();
    }

    set value(value)
    {
        // FIXME: <https://webkit.org/b/147064> Getter and setter on super are called with wrong "this" object
        this._setValue(value);
    }

    set placeholder(placeholder)
    {
        if (this._updatedValues.conflictingValues)
            return;

        this.specialPropertyPlaceholderElement.textContent = this._canonicalizedKeywordForKey(placeholder) || placeholder;
    }

    get synthesizedValue()
    {
        // FIXME: <https://webkit.org/b/147064> Getter and setter on super are called with wrong "this" object
        return this._generateSynthesizedValue();
    }

    updateEditorValues(updatedValues)
    {   if (!updatedValues.conflictingValues) {
            var missing = this._propertyMissing || !updatedValues.value;
            this._unchangedOptionElement.selected = missing;
            this.specialPropertyPlaceholderElement.hidden = !missing;
        }

        super.updateEditorValues(updatedValues);
    }

    // Private

    _getValue()
    {
        return this._keywordSelectElement.value;
    }

    _setValue(value)
    {
        if (!value || !value.length) {
            this._unchangedOptionElement.selected = true;
            this.specialPropertyPlaceholderElement.hidden = false;
            return;
        }

        if (this._propertyMissing || !this.valueIsSupportedKeyword(value))
            return;

        if (value === this._keywordSelectElement.value)
            return;

        if (this._valueIsSupportedAdvancedKeyword(value))
            this._addAdvancedValues();

        this._keywordSelectElement.value = value;
    }

    _generateSynthesizedValue()
    {
        return this._unchangedOptionElement.selected ? null : this._keywordSelectElement.value;
    }

    _handleKeywordChanged()
    {
        this._valueDidChange();
        this.specialPropertyPlaceholderElement.hidden = !this._unchangedOptionElement.selected;
    }

    _keywordSelectMouseDown(event)
    {
        if (event.altKey)
            this._addAdvancedValues();
        else if (!this._valueIsSupportedAdvancedKeyword())
            this._removeAdvancedValues();
    }

    _createValueOptions(values)
    {
        var addedElements = [];
        for (var key in values) {
            var option = document.createElement("option");
            option.value = key;
            option.text = values[key];
            this._keywordSelectElement.appendChild(option);
            addedElements.push(option);
        }
        return addedElements;
    }

    _addAdvancedValues()
    {
        if (this._advancedValuesElements)
            return;

        this._keywordSelectElement.appendChild(document.createElement("hr"));
        this._advancedValuesElements = this._createValueOptions(this._possibleValues.advanced);
    }

    _removeAdvancedValues()
    {
        if (!this._advancedValuesElements)
            return;

        this._keywordSelectElement.removeChild(this._advancedValuesElements[0].previousSibling);
        for (var element of this._advancedValuesElements)
            this._keywordSelectElement.removeChild(element);

        this._advancedValuesElements = null;
    }

    _toggleTabbingOfSelectableElements(disabled)
    {
        this._keywordSelectElement.tabIndex = disabled ? "-1" : null;
    }
};
