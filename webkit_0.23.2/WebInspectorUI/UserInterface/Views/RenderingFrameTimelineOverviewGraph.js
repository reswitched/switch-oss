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

WebInspector.RenderingFrameTimelineOverviewGraph = class RenderingFrameTimelineOverviewGraph extends WebInspector.TimelineOverviewGraph
{
    constructor(timeline, timelineOverview)
    {
        super(timelineOverview);

        this.element.classList.add("rendering-frame");
        this.element.addEventListener("click", this._mouseClicked.bind(this));

        this._renderingFrameTimeline = timeline;
        this._renderingFrameTimeline.addEventListener(WebInspector.Timeline.Event.RecordAdded, this._timelineRecordAdded, this);

        this._selectedFrameMarker = document.createElement("div");
        this._selectedFrameMarker.classList.add("frame-marker");

        this._timelineRecordFrames = [];
        this._selectedTimelineRecordFrame = null;
        this._graphHeightSeconds = NaN;
        this._framesPerSecondDividerMap = new Map;

        this.reset();
    }

    // Public

    get graphHeightSeconds()
    {
        if (!isNaN(this._graphHeightSeconds))
            return this._graphHeightSeconds;

        var maximumFrameDuration = this._renderingFrameTimeline.records.reduce(function(previousValue, currentValue) {
            return Math.max(previousValue, currentValue.duration);
        }, 0);

        this._graphHeightSeconds = maximumFrameDuration * 1.1;  // Add 10% margin above frames.
        this._graphHeightSeconds = Math.min(this._graphHeightSeconds, WebInspector.RenderingFrameTimelineOverviewGraph.MaximumGraphHeightSeconds);
        this._graphHeightSeconds = Math.max(this._graphHeightSeconds, WebInspector.RenderingFrameTimelineOverviewGraph.MinimumGraphHeightSeconds);
        return this._graphHeightSeconds;
    }

    reset()
    {
        super.reset();

        this.element.removeChildren();

        this.selectedRecord = null;

        this._framesPerSecondDividerMap.clear();
    }

    recordWasFiltered(record, filtered)
    {
        super.recordWasFiltered(record, filtered);

        if (!(record instanceof WebInspector.RenderingFrameTimelineRecord))
            return;

        record[WebInspector.RenderingFrameTimelineOverviewGraph.RecordWasFilteredSymbol] = filtered;

        // Set filtered style if the frame element is within the visible range.
        var startIndex = Math.floor(this.startTime);
        var endIndex = Math.min(Math.floor(this.endTime), this._renderingFrameTimeline.records.length - 1);
        if (record.frameIndex < startIndex || record.frameIndex > endIndex)
            return;

        var frameIndex = record.frameIndex - startIndex;
        this._timelineRecordFrames[frameIndex].filtered = filtered;
    }

    // Protected

    layout()
    {
        if (!this._renderingFrameTimeline.records.length)
            return;

        var records = this._renderingFrameTimeline.records;
        var startIndex = Math.floor(this.startTime);
        var endIndex = Math.min(Math.floor(this.endTime), records.length - 1);
        var recordFrameIndex = 0;

        for (var i = startIndex; i <= endIndex; ++i) {
            var record = records[i];
            var timelineRecordFrame = this._timelineRecordFrames[recordFrameIndex];
            if (!timelineRecordFrame)
                timelineRecordFrame = this._timelineRecordFrames[recordFrameIndex] = new WebInspector.TimelineRecordFrame(this, record);
            else
                timelineRecordFrame.record = record;

            timelineRecordFrame.refresh(this);
            if (!timelineRecordFrame.element.parentNode)
                this.element.appendChild(timelineRecordFrame.element);

            timelineRecordFrame.filtered = record[WebInspector.RenderingFrameTimelineOverviewGraph.RecordWasFilteredSymbol] || false;
            ++recordFrameIndex;
        }

        // Remove the remaining unused TimelineRecordFrames.
        for (; recordFrameIndex < this._timelineRecordFrames.length; ++recordFrameIndex) {
            this._timelineRecordFrames[recordFrameIndex].record = null;
            this._timelineRecordFrames[recordFrameIndex].element.remove();
        }

        this._updateDividers();
        this._updateFrameMarker();
    }

    updateSelectedRecord()
    {
        if (!this.selectedRecord) {
            this._updateFrameMarker();
            return;
        }

        var visibleDuration = this.timelineOverview.visibleDuration;
        var frameIndex = this.selectedRecord.frameIndex;

        // Reveal a newly selected record if it's outside the visible range.
        if (frameIndex < Math.ceil(this.timelineOverview.scrollStartTime) || frameIndex >= this.timelineOverview.scrollStartTime + visibleDuration) {
            var scrollStartTime = frameIndex;
            if (!this._selectedTimelineRecordFrame || Math.abs(this._selectedTimelineRecordFrame.record.frameIndex - this.selectedRecord.frameIndex) > 1) {
                scrollStartTime -= Math.floor(visibleDuration / 2);
                scrollStartTime = Math.max(Math.min(scrollStartTime, this.timelineOverview.endTime), this.timelineOverview.startTime);
            }

            this.timelineOverview.scrollStartTime = scrollStartTime;
            return;
        }

        this._updateFrameMarker();
    }

    // Private

    _timelineRecordAdded(event)
    {
        this._graphHeightSeconds = NaN;

        this.needsLayout();
    }

    _updateDividers()
    {
        if (this.graphHeightSeconds === 0)
            return;

        var overviewGraphHeight = this.element.offsetHeight;

        function createDividerAtPosition(framesPerSecond)
        {
            var secondsPerFrame = 1 / framesPerSecond;
            var dividerTop = 1 - secondsPerFrame / this.graphHeightSeconds;
            if (dividerTop < 0.01 || dividerTop >= 1)
                return;

            var divider = this._framesPerSecondDividerMap.get(framesPerSecond);
            if (!divider) {
                divider = document.createElement("div");
                divider.classList.add("divider");

                var label = document.createElement("div");
                label.classList.add("label");
                label.innerText = framesPerSecond + " fps";
                divider.appendChild(label);

                this.element.appendChild(divider);

                this._framesPerSecondDividerMap.set(framesPerSecond, divider);
            }

            divider.style.marginTop = (dividerTop * overviewGraphHeight).toFixed(2) + "px";
        }

        createDividerAtPosition.call(this, 60);
        createDividerAtPosition.call(this, 30);
    }

    _updateFrameMarker()
    {
        if (this._selectedTimelineRecordFrame) {
            this._selectedTimelineRecordFrame.selected = false;
            this._selectedTimelineRecordFrame = null;
        }

        if (!this.selectedRecord) {
            if (this._selectedFrameMarker.parentElement)
                this.element.removeChild(this._selectedFrameMarker);

            this.dispatchSelectedRecordChangedEvent();
            return;
        }

        var frameWidth = (1 / this.timelineOverview.secondsPerPixel);
        this._selectedFrameMarker.style.width = frameWidth + "px";

        var markerLeftPosition = this.selectedRecord.frameIndex - this.startTime;
        this._selectedFrameMarker.style.left = ((markerLeftPosition / this.timelineOverview.visibleDuration) * 100).toFixed(2) + "%";

        if (!this._selectedFrameMarker.parentElement)
            this.element.appendChild(this._selectedFrameMarker);

        // Find and update the selected frame element.
        var index = this._timelineRecordFrames.binaryIndexOf(this.selectedRecord, function(record, frame) {
            return frame.record ? record.frameIndex - frame.record.frameIndex : -1;
        });

        console.assert(index >= 0 && index < this._timelineRecordFrames.length, "Selected record not within visible graph duration.", this.selectedRecord);
        if (index < 0 || index >= this._timelineRecordFrames.length)
            return;

        this._selectedTimelineRecordFrame = this._timelineRecordFrames[index];
        this._selectedTimelineRecordFrame.selected = true;

        this.dispatchSelectedRecordChangedEvent();
    }

    _mouseClicked(event)
    {
        var position = event.pageX - this.element.getBoundingClientRect().left;
        var frameIndex = Math.floor(position * this.timelineOverview.secondsPerPixel + this.startTime);
        if (frameIndex < 0 || frameIndex >= this._renderingFrameTimeline.records.length)
            return;

        var newSelectedRecord = this._renderingFrameTimeline.records[frameIndex];
        if (newSelectedRecord[WebInspector.RenderingFrameTimelineOverviewGraph.RecordWasFilteredSymbol])
            return;

        // Clicking the selected frame causes it to be deselected.
        if (this.selectedRecord === newSelectedRecord)
            newSelectedRecord = null;

        if (frameIndex >= this.timelineOverview.selectionStartTime && frameIndex < this.timelineOverview.selectionStartTime + this.timelineOverview.selectionDuration) {
            this.selectedRecord = newSelectedRecord;
            return;
        }

        // Clicking a frame outside the current ruler selection changes the selection to include the frame.
        this.selectedRecord = newSelectedRecord;
        this.timelineOverview.selectionStartTime = frameIndex;
        this.timelineOverview.selectionDuration = 1;
    }
};

WebInspector.RenderingFrameTimelineOverviewGraph.RecordWasFilteredSymbol = Symbol("rendering-frame-overview-graph-record-was-filtered");

WebInspector.RenderingFrameTimelineOverviewGraph.MaximumGraphHeightSeconds = 0.037;
WebInspector.RenderingFrameTimelineOverviewGraph.MinimumGraphHeightSeconds = 0.0185;
