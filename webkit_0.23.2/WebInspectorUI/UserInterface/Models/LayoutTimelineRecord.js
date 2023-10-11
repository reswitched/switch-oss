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

WebInspector.LayoutTimelineRecord = class LayoutTimelineRecord extends WebInspector.TimelineRecord
{
    constructor(eventType, startTime, endTime, callFrames, sourceCodeLocation, quad)
    {
        super(WebInspector.TimelineRecord.Type.Layout, startTime, endTime, callFrames, sourceCodeLocation);

        console.assert(eventType);
        console.assert(!quad || quad instanceof WebInspector.Quad);

        if (eventType in WebInspector.LayoutTimelineRecord.EventType)
            eventType = WebInspector.LayoutTimelineRecord.EventType[eventType];

        this._eventType = eventType;
        this._quad = quad || null;
    }

    // Static

    static displayNameForEventType(eventType)
    {
        switch(eventType) {
        case WebInspector.LayoutTimelineRecord.EventType.InvalidateStyles:
            return WebInspector.UIString("Styles Invalidated");
        case WebInspector.LayoutTimelineRecord.EventType.RecalculateStyles:
            return WebInspector.UIString("Styles Recalculated");
        case WebInspector.LayoutTimelineRecord.EventType.InvalidateLayout:
            return WebInspector.UIString("Layout Invalidated");
        case WebInspector.LayoutTimelineRecord.EventType.ForcedLayout:
            return WebInspector.UIString("Forced Layout");
        case WebInspector.LayoutTimelineRecord.EventType.Layout:
            return WebInspector.UIString("Layout");
        case WebInspector.LayoutTimelineRecord.EventType.Paint:
            return WebInspector.UIString("Paint");
        case WebInspector.LayoutTimelineRecord.EventType.Composite:
            return WebInspector.UIString("Composite");
        }
    }

    // Public

    get eventType()
    {
        return this._eventType;
    }

    get width()
    {
        return this._quad ? this._quad.width : NaN;
    }

    get height()
    {
        return this._quad ? this._quad.height : NaN;
    }

    get area()
    {
        return this.width * this.height;
    }

    get quad()
    {
        return this._quad;
    }

    saveIdentityToCookie(cookie)
    {
        super.saveIdentityToCookie(cookie);

        cookie[WebInspector.LayoutTimelineRecord.EventTypeCookieKey] = this._eventType;
    }
};

WebInspector.LayoutTimelineRecord.EventType = {
    InvalidateStyles: "layout-timeline-record-invalidate-styles",
    RecalculateStyles: "layout-timeline-record-recalculate-styles",
    InvalidateLayout: "layout-timeline-record-invalidate-layout",
    ForcedLayout: "layout-timeline-record-forced-layout",
    Layout: "layout-timeline-record-layout",
    Paint: "layout-timeline-record-paint",
    Composite: "layout-timeline-record-composite"
};

WebInspector.LayoutTimelineRecord.TypeIdentifier = "layout-timeline-record";
WebInspector.LayoutTimelineRecord.EventTypeCookieKey = "layout-timeline-record-event-type";
