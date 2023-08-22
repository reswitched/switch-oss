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

WebInspector.ScriptTimelineDataGridNode = class ScriptTimelineDataGridNode extends WebInspector.TimelineDataGridNode
{
    constructor(scriptTimelineRecord, baseStartTime, rangeStartTime, rangeEndTime)
    {
        super(false, null);

        this._record = scriptTimelineRecord;
        this._baseStartTime = baseStartTime || 0;
        this._rangeStartTime = rangeStartTime || 0;
        this._rangeEndTime = typeof rangeEndTime === "number" ? rangeEndTime : Infinity;
    }

    // Public

    get record()
    {
        return this._record;
    }

    get records()
    {
        return [this._record];
    }

    get baseStartTime()
    {
        return this._baseStartTime;
    }

    get rangeStartTime()
    {
        return this._rangeStartTime;
    }

    get rangeEndTime()
    {
        return this._rangeEndTime;
    }

    get data()
    {
        var startTime = this._record.startTime;
        var duration = this._record.startTime + this._record.duration - startTime;
        var callFrameOrSourceCodeLocation = this._record.initiatorCallFrame || this._record.sourceCodeLocation;

        // COMPATIBILITY (iOS 8): Profiles included per-call information and can be finely partitioned.
        if (this._record.profile) {
            var oneRootNode = this._record.profile.topDownRootNodes[0];
            if (oneRootNode && oneRootNode.calls) {
                startTime = Math.max(this._rangeStartTime, this._record.startTime);
                duration = Math.min(this._record.startTime + this._record.duration, this._rangeEndTime) - startTime;
            }
        }

        return {eventType: this._record.eventType, startTime, selfTime: duration, totalTime: duration,
            averageTime: duration, callCount: 1, location: callFrameOrSourceCodeLocation};
    }

    updateRangeTimes(startTime, endTime)
    {
        var oldRangeStartTime = this._rangeStartTime;
        var oldRangeEndTime = this._rangeEndTime;

        if (oldRangeStartTime === startTime && oldRangeEndTime === endTime)
            return;

        this._rangeStartTime = startTime;
        this._rangeEndTime = endTime;

        // If we have no duration the range does not matter.
        if (!this._record.duration)
            return;

        // We only need a refresh if the new range time changes the visible portion of this record.
        var recordStart = this._record.startTime;
        var recordEnd = this._record.startTime + this._record.duration;
        var oldStartBoundary = Number.constrain(oldRangeStartTime, recordStart, recordEnd);
        var oldEndBoundary = Number.constrain(oldRangeEndTime, recordStart, recordEnd);
        var newStartBoundary = Number.constrain(startTime, recordStart, recordEnd);
        var newEndBoundary = Number.constrain(endTime, recordStart, recordEnd);

        if (oldStartBoundary !== newStartBoundary || oldEndBoundary !== newEndBoundary)
            this.needsRefresh();
    }

    createCellContent(columnIdentifier, cell)
    {
        var emptyValuePlaceholderString = "\u2014";
        var value = this.data[columnIdentifier];

        switch (columnIdentifier) {
        case "eventType":
            return WebInspector.ScriptTimelineRecord.EventType.displayName(value, this._record.details);

        case "startTime":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value - this._baseStartTime, true);

        case "selfTime":
        case "totalTime":
        case "averageTime":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value, true);
        }

        return super.createCellContent(columnIdentifier, cell);
    }
};
