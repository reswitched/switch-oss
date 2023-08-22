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

WebInspector.BezierEditor = class BezierEditor extends WebInspector.Object
{
    constructor()
    {
        super();

        this._element = document.createElement("div");
        this._element.classList.add("bezier-editor");

        var editorWidth = 184;
        var editorHeight = 200;
        this._padding = 25;
        this._controlHandleRadius = 7;
        this._bezierWidth = editorWidth - (this._controlHandleRadius * 2);
        this._bezierHeight = editorHeight - (this._controlHandleRadius * 2) - (this._padding * 2);
        this._bezierPreviewAnimationStyleText = "bezierPreview 2s 250ms forwards ";

        var bezierPreviewContainer = document.createElement("div");
        bezierPreviewContainer.id = "bezierPreview";
        bezierPreviewContainer.classList.add("bezier-preview");
        bezierPreviewContainer.title = WebInspector.UIString("Click to restart the animation");
        bezierPreviewContainer.addEventListener("mousedown", this._resetPreviewAnimation.bind(this));

        this._bezierPreview = document.createElement("div");
        bezierPreviewContainer.appendChild(this._bezierPreview);

        this._element.appendChild(bezierPreviewContainer);

        this._bezierContainer = createSVGElement("svg");
        this._bezierContainer.id = "bezierContainer";
        this._bezierContainer.setAttribute("width", editorWidth);
        this._bezierContainer.setAttribute("height", editorHeight);
        this._bezierContainer.classList.add("bezier-container");

        var svgGroup = createSVGElement("g");
        svgGroup.setAttribute("transform", "translate(0, " + this._padding + ")");

        var linearCurve = createSVGElement("line");
        linearCurve.classList.add("linear-curve");
        linearCurve.setAttribute("x1", this._controlHandleRadius);
        linearCurve.setAttribute("y1", this._bezierHeight + this._controlHandleRadius);
        linearCurve.setAttribute("x2", this._bezierWidth + this._controlHandleRadius);
        linearCurve.setAttribute("y2", this._controlHandleRadius);
        svgGroup.appendChild(linearCurve);

        this._bezierCurve = createSVGElement("path");
        this._bezierCurve.classList.add("bezier-curve");
        svgGroup.appendChild(this._bezierCurve);

        function createControl(x1, y1)
        {
            x1 += this._controlHandleRadius;
            y1 += this._controlHandleRadius;

            var line = createSVGElement("line");
            line.classList.add("control-line");
            line.setAttribute("x1", x1);
            line.setAttribute("y1", y1);
            line.setAttribute("x2", x1);
            line.setAttribute("y2", y1);
            svgGroup.appendChild(line);

            var handle = createSVGElement("circle");
            handle.classList.add("control-handle");
            svgGroup.appendChild(handle);

            return {point: null, line, handle};
        }

        this._inControl = createControl.call(this, 0, this._bezierHeight);
        this._outControl = createControl.call(this, this._bezierWidth, 0);

        this._bezierContainer.appendChild(svgGroup);
        this._element.appendChild(this._bezierContainer);

        this._bezierPreviewTiming = document.createElement("div");
        this._bezierPreviewTiming.classList.add("bezier-preview-timing");
        this._element.appendChild(this._bezierPreviewTiming);

        this._selectedControl = null;
        this._bezierContainer.addEventListener("mousedown", this);
    }

    // Public

    get element()
    {
        return this._element;
    }

    set bezier(bezier)
    {
        if (!bezier)
            return;

        var isCubicBezier = bezier instanceof WebInspector.CubicBezier;
        console.assert(isCubicBezier);
        if (!isCubicBezier)
            return;

        this._bezier = bezier;
        this._inControl.point = new WebInspector.Point(this._bezier.inPoint.x * this._bezierWidth, (1 - this._bezier.inPoint.y) * this._bezierHeight);
        this._outControl.point = new WebInspector.Point(this._bezier.outPoint.x * this._bezierWidth, (1 - this._bezier.outPoint.y) * this._bezierHeight);

        this._updateBezier();
        this._triggerPreviewAnimation();
    }

    get bezier()
    {
        return this._bezier;
    }

    // Protected

    handleEvent(event)
    {
        switch (event.type) {
        case "mousedown":
            this._handleMousedown(event);
            break;
        case "mousemove":
            this._handleMousemove(event);
            break;
        case "mouseup":
            this._handleMouseup(event);
            break;
        }
    }

    // Private

    _handleMousedown(event)
    {
        if (event.button !== 0)
            return;

        window.addEventListener("mousemove", this, true);
        window.addEventListener("mouseup", this, true);

        this._bezierPreview.style.animation = null;
        this._bezierPreviewTiming.classList.remove("animate");

        this._updateControlPointsForMouseEvent(event, true);
    }

    _handleMousemove(event)
    {
        this._updateControlPointsForMouseEvent(event);
    }

    _handleMouseup(event)
    {
        this._selectedControl.handle.classList.remove("selected");
        this._selectedControl = null;
        this._triggerPreviewAnimation();

        window.removeEventListener("mousemove", this, true);
        window.removeEventListener("mouseup", this, true);
    }

    _updateControlPointsForMouseEvent(event, calculateSelectedControlPoint)
    {
        var point = WebInspector.Point.fromEventInElement(event, this._bezierContainer);
        point.x = Number.constrain(point.x - this._controlHandleRadius, 0, this._bezierWidth);
        point.y -= this._controlHandleRadius + this._padding;

        if (calculateSelectedControlPoint) {
            if (this._inControl.point.distance(point) < this._outControl.point.distance(point))
                this._selectedControl = this._inControl;
            else
                this._selectedControl = this._outControl;
        }

        this._selectedControl.point = point;
        this._selectedControl.handle.classList.add("selected");
        this._updateValue();
    }

    _updateValue()
    {
        function round(num)
        {
            return Math.round(num * 100) / 100;
        }

        var inValueX = round(this._inControl.point.x / this._bezierWidth);
        var inValueY = round(1 - (this._inControl.point.y / this._bezierHeight));

        var outValueX = round(this._outControl.point.x / this._bezierWidth);
        var outValueY = round(1 - (this._outControl.point.y / this._bezierHeight));

        this._bezier = new WebInspector.CubicBezier(inValueX, inValueY, outValueX, outValueY);
        this._updateBezier();

        this.dispatchEventToListeners(WebInspector.BezierEditor.Event.BezierChanged, {bezier: this._bezier});
    }

    _updateBezier()
    {
        var r = this._controlHandleRadius;
        var inControlX = this._inControl.point.x + r;
        var inControlY = this._inControl.point.y + r;
        var outControlX = this._outControl.point.x + r;
        var outControlY = this._outControl.point.y + r;
        var path = `M ${r} ${this._bezierHeight + r} C ${inControlX} ${inControlY} ${outControlX} ${outControlY} ${this._bezierWidth + r} ${r}`;
        this._bezierCurve.setAttribute("d", path);
        this._updateControl(this._inControl);
        this._updateControl(this._outControl);
    }

    _updateControl(control)
    {
        control.handle.setAttribute("cx", control.point.x + this._controlHandleRadius);
        control.handle.setAttribute("cy", control.point.y + this._controlHandleRadius);

        control.line.setAttribute("x2", control.point.x + this._controlHandleRadius);
        control.line.setAttribute("y2", control.point.y + this._controlHandleRadius);
    }

    _triggerPreviewAnimation()
    {
        this._bezierPreview.style.animation = this._bezierPreviewAnimationStyleText + this._bezier.toString();
        this._bezierPreviewTiming.classList.add("animate");
    }

    _resetPreviewAnimation()
    {
        var parent = this._bezierPreview.parentNode;
        parent.removeChild(this._bezierPreview);
        parent.appendChild(this._bezierPreview);

        this._element.removeChild(this._bezierPreviewTiming);
        this._element.appendChild(this._bezierPreviewTiming);
    }
};

WebInspector.BezierEditor.Event = {
    BezierChanged: "bezier-editor-bezier-changed"
};
