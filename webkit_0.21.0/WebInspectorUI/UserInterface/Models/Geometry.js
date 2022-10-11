/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.Point = class Point
{
    constructor(x, y)
    {
        this.x = x || 0;
        this.y = y || 0;
    }

    // Static

    static fromEvent(event)
    {
        return new WebInspector.Point(event.pageX, event.pageY);
    }

    static fromEventInElement(event, element)
    {
        var wkPoint = window.webkitConvertPointFromPageToNode(element, new WebKitPoint(event.pageX, event.pageY));
        return new WebInspector.Point(wkPoint.x, wkPoint.y);
    }

    // Public

    toString()
    {
        return "WebInspector.Point[" + this.x + "," + this.y + "]";
    }

    copy()
    {
        return new WebInspector.Point(this.x, this.y);
    }

    equals(anotherPoint)
    {
        return (this.x === anotherPoint.x && this.y === anotherPoint.y);
    }

    distance(anotherPoint)
    {
        var dx = anotherPoint.x - this.x;
        var dy = anotherPoint.y - this.y;
        return Math.sqrt(dx * dx, dy * dy);
    }
};

WebInspector.Size = class Size
{
    constructor(width, height)
    {
        this.width = width || 0;
        this.height = height || 0;
    }

    // Public

    toString()
    {
        return "WebInspector.Size[" + this.width + "," + this.height + "]";
    }

    copy()
    {
        return new WebInspector.Size(this.width, this.height);
    }

    equals(anotherSize)
    {
        return (this.width === anotherSize.width && this.height === anotherSize.height);
    }
};

WebInspector.Size.ZERO_SIZE = new WebInspector.Size(0, 0);


WebInspector.Rect = class Rect
{
    constructor(x, y, width, height)
    {
        this.origin = new WebInspector.Point(x || 0, y || 0);
        this.size = new WebInspector.Size(width || 0, height || 0);
    }

    // Static

    static rectFromClientRect(clientRect)
    {
        return new WebInspector.Rect(clientRect.left, clientRect.top, clientRect.width, clientRect.height);
    }

    static unionOfRects(rects)
    {
        var union = rects[0];
        for (var i = 1; i < rects.length; ++i)
            union = union.unionWithRect(rects[i]);
        return union;
    }

    // Public

    toString()
    {
        return "WebInspector.Rect[" + [this.origin.x, this.origin.y, this.size.width, this.size.height].join(", ") + "]";
    }

    copy()
    {
        return new WebInspector.Rect(this.origin.x, this.origin.y, this.size.width, this.size.height);
    }

    equals(anotherRect)
    {
        return (this.origin.equals(anotherRect.origin) && this.size.equals(anotherRect.size));
    }

    inset(insets)
    {
        return new WebInspector.Rect(
            this.origin.x + insets.left,
            this.origin.y + insets.top,
            this.size.width - insets.left - insets.right,
            this.size.height - insets.top - insets.bottom
        );
    }

    pad(padding)
    {
        return new WebInspector.Rect(
            this.origin.x - padding,
            this.origin.y - padding,
            this.size.width + padding * 2,
            this.size.height + padding * 2
        );
    }

    minX()
    {
        return this.origin.x;
    }

    minY()
    {
        return this.origin.y;
    }

    midX()
    {
        return this.origin.x + (this.size.width / 2);
    }

    midY()
    {
        return this.origin.y + (this.size.height / 2);
    }

    maxX()
    {
        return this.origin.x + this.size.width;
    }

    maxY()
    {
        return this.origin.y + this.size.height;
    }

    intersectionWithRect(rect)
    {
        var x1 = Math.max(this.minX(), rect.minX());
        var x2 = Math.min(this.maxX(), rect.maxX());
        if (x1 > x2)
            return WebInspector.Rect.ZERO_RECT;
        var intersection = new WebInspector.Rect;
        intersection.origin.x = x1;
        intersection.size.width = x2 - x1;
        var y1 = Math.max(this.minY(), rect.minY());
        var y2 = Math.min(this.maxY(), rect.maxY());
        if (y1 > y2)
            return WebInspector.Rect.ZERO_RECT;
        intersection.origin.y = y1;
        intersection.size.height = y2 - y1;
        return intersection;
    }

    unionWithRect(rect)
    {
        var x = Math.min(this.minX(), rect.minX());
        var y = Math.min(this.minY(), rect.minY());
        var width = Math.max(this.maxX(), rect.maxX()) - x;
        var height = Math.max(this.maxY(), rect.maxY()) - y;
        return new WebInspector.Rect(x, y, width, height);
    }

    round()
    {
        return new WebInspector.Rect(
            Math.floor(this.origin.x),
            Math.floor(this.origin.y),
            Math.ceil(this.size.width),
            Math.ceil(this.size.height)
        );
    }
};

WebInspector.Rect.ZERO_RECT = new WebInspector.Rect(0, 0, 0, 0);


WebInspector.EdgeInsets = class EdgeInsets
{
    constructor(top, right, bottom, left)
    {
        console.assert(arguments.length === 1 || arguments.length === 4);

        if (arguments.length === 1) {
            this.top = top;
            this.right = top;
            this.bottom = top;
            this.left = top;
        } else if (arguments.length === 4) {
            this.top = top;
            this.right = right;
            this.bottom = bottom;
            this.left = left;
        }
    }

    // Public

    equals(anotherInset)
    {
        return (this.top === anotherInset.top && this.right === anotherInset.right &&
                this.bottom === anotherInset.bottom && this.left === anotherInset.left);
    }

    copy()
    {
        return new WebInspector.EdgeInsets(this.top, this.right, this.bottom, this.left);
    }
};

WebInspector.RectEdge = {
    MIN_X : 0,
    MIN_Y : 1,
    MAX_X : 2,
    MAX_Y : 3
};

WebInspector.Quad = class Quad
{
    constructor(quad)
    {
        this.points = [
            new WebInspector.Point(quad[0], quad[1]), // top left
            new WebInspector.Point(quad[2], quad[3]), // top right
            new WebInspector.Point(quad[4], quad[5]), // bottom right
            new WebInspector.Point(quad[6], quad[7])  // bottom left
        ];

        this.width = Math.round(Math.sqrt(Math.pow(quad[0] - quad[2], 2) + Math.pow(quad[1] - quad[3], 2)));
        this.height = Math.round(Math.sqrt(Math.pow(quad[0] - quad[6], 2) + Math.pow(quad[1] - quad[7], 2)));
    }

    // Public

    toProtocol()
    {
        return [
            this.points[0].x, this.points[0].y,
            this.points[1].x, this.points[1].y,
            this.points[2].x, this.points[2].y,
            this.points[3].x, this.points[3].y
        ];
    }
};

WebInspector.Polygon = class Polygon
{
    constructor(points)
    {
        this.points = points;
    }

    // Public

    bounds()
    {
        var minX = Number.MAX_VALUE;
        var minY = Number.MAX_VALUE;
        var maxX = -Number.MAX_VALUE;
        var maxY = -Number.MAX_VALUE;
        for (var point of this.points) {
            minX = Math.min(minX, point.x);
            maxX = Math.max(maxX, point.x);
            minY = Math.min(minY, point.y);
            maxY = Math.max(maxY, point.y);
        }
        return new WebInspector.Rect(minX, minY, maxX - minX, maxY - minY);
    }
}

WebInspector.CubicBezier = class CubicBezier
{
    constructor(x1, y1, x2, y2)
    {
        this._inPoint = new WebInspector.Point(x1, y1);
        this._outPoint = new WebInspector.Point(x2, y2);

        // Calculate the polynomial coefficients, implicit first and last control points are (0,0) and (1,1).
        this._curveInfo = {
            x: {c: 3.0 * x1},
            y: {c: 3.0 * y1}
        }

        this._curveInfo.x.b = 3.0 * (x2 - x1) - this._curveInfo.x.c;
        this._curveInfo.x.a = 1.0 - this._curveInfo.x.c - this._curveInfo.x.b;

        this._curveInfo.y.b = 3.0 * (y2 - y1) - this._curveInfo.y.c;
        this._curveInfo.y.a = 1.0 - this._curveInfo.y.c - this._curveInfo.y.b;
    }

    // Static

    static fromCoordinates(coordinates)
    {
        if (!coordinates || coordinates.length < 4)
            return null;

        coordinates = coordinates.map(Number);
        if (coordinates.includes(NaN))
            return null;

        return new WebInspector.CubicBezier(coordinates[0], coordinates[1], coordinates[2], coordinates[3]);
    }

    static fromString(text)
    {
        if (!text || !text.length)
            return null;

        var trimmedText = text.toLowerCase().replace(/\s/g, "");
        if (!trimmedText.length)
            return null;

        if (Object.keys(WebInspector.CubicBezier.keywordValues).includes(trimmedText))
            return WebInspector.CubicBezier.fromCoordinates(WebInspector.CubicBezier.keywordValues[trimmedText]);

        var matches = trimmedText.match(/^cubic-bezier\(([-\d.]+),([-\d.]+),([-\d.]+),([-\d.]+)\)$/);
        if (!matches)
            return null;

        matches.splice(0, 1);
        return WebInspector.CubicBezier.fromCoordinates(matches);
    }

    // Public

    get inPoint()
    {
        return this._inPoint;
    }

    get outPoint()
    {
        return this._outPoint;
    }

    copy()
    {
        return new WebInspector.CubicBezier(this._inPoint.x, this._inPoint.y, this._outPoint.x, this._outPoint.y);
    }

    toString()
    {
        var values = [this._inPoint.x, this._inPoint.y, this._outPoint.x, this._outPoint.y];
        for (var key in WebInspector.CubicBezier.keywordValues) {
            if (Object.shallowEqual(WebInspector.CubicBezier.keywordValues[key], values))
                return key;
        }

        return "cubic-bezier(" + values.join(", ") + ")";
    }

    solve(x, epsilon)
    {
        return this._sampleCurveY(this._solveCurveX(x, epsilon));
    }

    // Private

    _sampleCurveX(t)
    {
        // `ax t^3 + bx t^2 + cx t' expanded using Horner's rule.
        return ((this._curveInfo.x.a * t + this._curveInfo.x.b) * t + this._curveInfo.x.c) * t;
    }

    _sampleCurveY(t)
    {
        return ((this._curveInfo.y.a * t + this._curveInfo.y.b) * t + this._curveInfo.y.c) * t;
    }

    _sampleCurveDerivativeX(t)
    {
        return (3.0 * this._curveInfo.x.a * t + 2.0 * this._curveInfo.x.b) * t + this._curveInfo.x.c;
    }

    // Given an x value, find a parametric value it came from.
    _solveCurveX(x, epsilon)
    {
        var t0, t1, t2, x2, d2, i;

        // First try a few iterations of Newton's method -- normally very fast.
        for (t2 = x, i = 0; i < 8; i++) {
            x2 = this._sampleCurveX(t2) - x;
            if (Math.abs(x2) < epsilon)
                return t2;
            d2 = this._sampleCurveDerivativeX(t2);
            if (Math.abs(d2) < 1e-6)
                break;
            t2 = t2 - x2 / d2;
        }

        // Fall back to the bisection method for reliability.
        t0 = 0.0;
        t1 = 1.0;
        t2 = x;

        if (t2 < t0)
            return t0;
        if (t2 > t1)
            return t1;

        while (t0 < t1) {
            x2 = this._sampleCurveX(t2);
            if (Math.abs(x2 - x) < epsilon)
                return t2;
            if (x > x2)
                t0 = t2;
            else
                t1 = t2;
            t2 = (t1 - t0) * 0.5 + t0;
        }

        // Failure.
        return t2;
    }
}

WebInspector.CubicBezier.keywordValues = {
    "ease":         [0.25, 0.1, 0.25, 1],
    "ease-in":      [0.42, 0, 1, 1],
    "ease-out":     [0, 0, 0.58, 1],
    "ease-in-out":  [0.42, 0, 0.58, 1],
    "linear":       [0, 0, 1, 1]
};
