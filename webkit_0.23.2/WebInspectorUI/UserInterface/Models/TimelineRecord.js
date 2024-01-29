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

WebInspector.TimelineRecord = class TimelineRecord extends WebInspector.Object
{
    constructor(type, startTime, endTime, callFrames, sourceCodeLocation)
    {
        super();

        console.assert(type);

        if (type in WebInspector.TimelineRecord.Type)
            type = WebInspector.TimelineRecord.Type[type];

        this._type = type;
        this._startTime = startTime || NaN;
        this._endTime = endTime || NaN;
        this._callFrames = callFrames || null;
        this._sourceCodeLocation = sourceCodeLocation || null;
        this._children = [];
    }

    // Public

    get type()
    {
        return this._type;
    }

    get startTime()
    {
        // Implemented by subclasses if needed.
        return this._startTime;
    }

    get activeStartTime()
    {
        // Implemented by subclasses if needed.
        return this._startTime;
    }

    get endTime()
    {
        // Implemented by subclasses if needed.
        return this._endTime;
    }

    get duration()
    {
        // Use the getters instead of the properties so this works for subclasses that override the getters.
        return this.endTime - this.startTime;
    }

    get inactiveDuration()
    {
        // Use the getters instead of the properties so this works for subclasses that override the getters.
        return this.activeStartTime - this.startTime;
    }

    get activeDuration()
    {
        // Use the getters instead of the properties so this works for subclasses that override the getters.
        return this.endTime - this.activeStartTime;
    }

    get updatesDynamically()
    {
        // Implemented by subclasses if needed.
        return false;
    }

    get usesActiveStartTime()
    {
        // Implemented by subclasses if needed.
        return false;
    }

    get callFrames()
    {
        return this._callFrames;
    }

    get initiatorCallFrame()
    {
        if (!this._callFrames || !this._callFrames.length)
            return null;

        // Return the first non-native code call frame as the initiator.
        for (var i = 0; i < this._callFrames.length; ++i) {
            if (this._callFrames[i].nativeCode)
                continue;
            return this._callFrames[i];
        }

        return null;
    }

    get sourceCodeLocation()
    {
        return this._sourceCodeLocation;
    }

    get parent()
    {
        return this._parent;
    }

    set parent(x)
    {
        if (this._parent === x)
            return;

        this._parent = x;
    }

    get children()
    {
        return this._children;
    }

    saveIdentityToCookie(cookie)
    {
        cookie[WebInspector.TimelineRecord.SourceCodeURLCookieKey] = this._sourceCodeLocation ? this._sourceCodeLocation.sourceCode.url ? this._sourceCodeLocation.sourceCode.url.hash : null : null;
        cookie[WebInspector.TimelineRecord.SourceCodeLocationLineCookieKey] = this._sourceCodeLocation ? this._sourceCodeLocation.lineNumber : null;
        cookie[WebInspector.TimelineRecord.SourceCodeLocationColumnCookieKey] = this._sourceCodeLocation ? this._sourceCodeLocation.columnNumber : null;
        cookie[WebInspector.TimelineRecord.TypeCookieKey] = this._type || null;
    }
};

WebInspector.TimelineRecord.Event = {
    Updated: "timeline-record-updated"
};

WebInspector.TimelineRecord.Type = {
    Network: "timeline-record-type-network",
    Layout: "timeline-record-type-layout",
    Script: "timeline-record-type-script",
    RenderingFrame: "timeline-record-type-rendering-frame"
};

WebInspector.TimelineRecord.TypeIdentifier = "timeline-record";
WebInspector.TimelineRecord.SourceCodeURLCookieKey = "timeline-record-source-code-url";
WebInspector.TimelineRecord.SourceCodeLocationLineCookieKey = "timeline-record-source-code-location-line";
WebInspector.TimelineRecord.SourceCodeLocationColumnCookieKey = "timeline-record-source-code-location-column";
WebInspector.TimelineRecord.TypeCookieKey = "timeline-record-type";
