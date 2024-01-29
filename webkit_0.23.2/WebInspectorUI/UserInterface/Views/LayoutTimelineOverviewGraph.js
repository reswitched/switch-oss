/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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

WebInspector.LayoutTimelineOverviewGraph = class LayoutTimelineOverviewGraph extends WebInspector.TimelineOverviewGraph
{
    constructor(timeline, timelineOverview)
    {
        super(timelineOverview);

        this.element.classList.add("layout");

        this._layoutTimeline = timeline;
        this._layoutTimeline.addEventListener(WebInspector.Timeline.Event.RecordAdded, this._layoutTimelineRecordAdded, this);

        this.reset();
    }

    // Public

    reset()
    {
        super.reset();

        function createRecordRow()
        {
            var element = document.createElement("div");
            element.className = "graph-row";
            this.element.appendChild(element);
            return {element, recordBars: [], records: []};
        }

        this.element.removeChildren();

        this._timelineRecordBarMap = new Map;
        this._timelineLayoutRecordRow = createRecordRow.call(this);
        this._timelinePaintRecordRow = createRecordRow.call(this);
    }

    // Protected

    layout()
    {
        this._updateRowLayout(this._timelinePaintRecordRow);
        this._updateRowLayout(this._timelineLayoutRecordRow);
    }

    // Private

    _updateRowLayout(row)
    {
        var secondsPerPixel = this.timelineOverview.secondsPerPixel;
        var recordBarIndex = 0;

        function createBar(records, renderMode)
        {
            var timelineRecordBar = row.recordBars[recordBarIndex];
            if (!timelineRecordBar)
                timelineRecordBar = row.recordBars[recordBarIndex] = new WebInspector.TimelineRecordBar(records, renderMode);
            else {
                timelineRecordBar.renderMode = renderMode;
                timelineRecordBar.records = records;
            }

            timelineRecordBar.refresh(this);
            if (!timelineRecordBar.element.parentNode)
                row.element.appendChild(timelineRecordBar.element);
            ++recordBarIndex;
        }

        WebInspector.TimelineRecordBar.createCombinedBars(row.records, secondsPerPixel, this, createBar.bind(this));

        // Remove the remaining unused TimelineRecordBars.
        for (; recordBarIndex < row.recordBars.length; ++recordBarIndex) {
            row.recordBars[recordBarIndex].records = null;
            row.recordBars[recordBarIndex].element.remove();
        }
    }

    _layoutTimelineRecordAdded(event)
    {
        var layoutTimelineRecord = event.data.record;
        console.assert(layoutTimelineRecord instanceof WebInspector.LayoutTimelineRecord);

        if (layoutTimelineRecord.eventType === WebInspector.LayoutTimelineRecord.EventType.Paint || layoutTimelineRecord.eventType === WebInspector.LayoutTimelineRecord.EventType.Composite)
            this._timelinePaintRecordRow.records.push(layoutTimelineRecord);
        else
            this._timelineLayoutRecordRow.records.push(layoutTimelineRecord);

        this.needsLayout();
    }
};
