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

WebInspector.VisualStyleRelativeNumberSlider = class VisualStyleRelativeNumberSlider extends WebInspector.VisualStyleNumberInputBox
{
    constructor(propertyNames, text, possibleValues, possibleUnits, allowNegativeValues, layoutReversed)
    {
        super(propertyNames, text, possibleValues, possibleUnits, allowNegativeValues, layoutReversed);

        this._element.classList.add("relative-number-slider");

        this._sliderElement = document.createElement("input");
        this._sliderElement.classList.add("relative-slider");
        this._sliderElement.type = "range";
        this._sliderElement.addEventListener("input", this._sliderChanged.bind(this));
        this._element.appendChild(this._sliderElement);

        this._startingValue = 0;
        this._scale = 200;
    }

    // Public

    set scale(scale)
    {
        this._scale = scale || 1;
    }

    updateEditorValues(updatedValues)
    {
        super.updateEditorValues(updatedValues);
        this._resetSlider();
    }

    // Private

    _resetSlider()
    {
        this._startingValue = parseFloat(this._valueNumberInputElement.value);
        if (isNaN(this._startingValue))
            this._startingValue = parseFloat(this.placeholder) || 0;

        var midpoint = this._scale / 2;
        if (this._allowNegativeValues || this._startingValue > midpoint) {
            this._sliderElement.min = -midpoint;
            this._sliderElement.max = midpoint;
            this._sliderElement.value = 0;
        } else {
            this._sliderElement.min = 0;
            this._sliderElement.max = this._scale;
            this._sliderElement.value = this._startingValue;
            this._startingValue = 0;
        }
    }

    _sliderChanged()
    {
        this.value = this._startingValue + Math.round(parseFloat(this._sliderElement.value) * 100) / 100;
        this._valueDidChange();
    }

    _numberInputChanged()
    {
        super._numberInputChanged();
        this._resetSlider();
    }
};
