/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2015 University of Washington.
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

WebInspector.TimelineView = class TimelineView extends WebInspector.ContentView
{
    constructor(representedObject, extraArguments)
    {
        console.assert(extraArguments);
        console.assert(extraArguments.timelineSidebarPanel instanceof WebInspector.TimelineSidebarPanel);

        super(representedObject);

        // This class should not be instantiated directly. Create a concrete subclass instead.
        console.assert(this.constructor !== WebInspector.TimelineView && this instanceof WebInspector.TimelineView);

        this._timelineSidebarPanel = extraArguments.timelineSidebarPanel;

        this._contentTreeOutline = this._timelineSidebarPanel.createContentTreeOutline();
        this._contentTreeOutline.onselect = this.treeElementSelected.bind(this);
        this._contentTreeOutline.ondeselect = this.treeElementDeselected.bind(this);
        this._contentTreeOutline.__canShowContentViewForTreeElement = this.canShowContentViewForTreeElement.bind(this);

        this.element.classList.add("timeline-view");

        this._zeroTime = 0;
        this._startTime = 0;
        this._endTime = 5;
        this._currentTime = 0;
    }

    // Public

    get navigationSidebarTreeOutline()
    {
        return this._contentTreeOutline;
    }

    get navigationSidebarTreeOutlineLabel()
    {
        // Implemented by sub-classes if needed.
        return null;
    }

    get navigationSidebarTreeOutlineScopeBar()
    {
        return this._scopeBar;
    }

    get timelineSidebarPanel()
    {
        return this._timelineSidebarPanel;
    }

    get selectionPathComponents()
    {
        if (!this._contentTreeOutline.selectedTreeElement || this._contentTreeOutline.selectedTreeElement.hidden)
            return null;

        var pathComponent = new WebInspector.GeneralTreeElementPathComponent(this._contentTreeOutline.selectedTreeElement);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this.treeElementPathComponentSelected, this);
        return [pathComponent];
    }

    get zeroTime()
    {
        return this._zeroTime;
    }

    set zeroTime(x)
    {
        if (this._zeroTime === x)
            return;

        this._zeroTime = x || 0;

        this.needsLayout();
    }

    get startTime()
    {
        return this._startTime;
    }

    set startTime(x)
    {
        if (this._startTime === x)
            return;

        this._startTime = x || 0;

        this.needsLayout();
    }

    get endTime()
    {
        return this._endTime;
    }

    set endTime(x)
    {
        if (this._endTime === x)
            return;

        this._endTime = x || 0;

        this.needsLayout();
    }

    get currentTime()
    {
        return this._currentTime;
    }

    set currentTime(x)
    {
        if (this._currentTime === x)
            return;

        var oldCurrentTime = this._currentTime;

        this._currentTime = x || 0;

        function checkIfLayoutIsNeeded(currentTime)
        {
            // Include some wiggle room since the current time markers can be clipped off the ends a bit and still partially visible.
            var wiggleTime = 0.05; // 50ms
            return this._startTime - wiggleTime <= currentTime && currentTime <= this._endTime + wiggleTime;
        }

        if (checkIfLayoutIsNeeded.call(this, oldCurrentTime) || checkIfLayoutIsNeeded.call(this, this._currentTime))
            this.needsLayout();
    }

    reset()
    {
        this._contentTreeOutline.removeChildren();
        this._timelineSidebarPanel.hideEmptyContentPlaceholder();
    }


    filterDidChange()
    {
        // Implemented by sub-classes if needed.
    }

    matchTreeElementAgainstCustomFilters(treeElement)
    {
        // Implemented by sub-classes if needed.
        return true;
    }

    filterUpdated()
    {
        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
    }

    needsLayout()
    {
        // FIXME: needsLayout can be removed once <https://webkit.org/b/150741> is fixed.
        if (!this.visible)
            return;

        super.needsLayout();
    }

    // Protected

    canShowContentViewForTreeElement(treeElement)
    {
        // Implemented by sub-classes if needed.

        if (treeElement instanceof WebInspector.TimelineRecordTreeElement)
            return !!treeElement.sourceCodeLocation;
        return false;
    }

    showContentViewForTreeElement(treeElement)
    {
        // Implemented by sub-classes if needed.

        if (!(treeElement instanceof WebInspector.TimelineRecordTreeElement)) {
            console.error("Unknown tree element selected.", treeElement);
            return;
        }

        var sourceCodeLocation = treeElement.sourceCodeLocation;
        if (!sourceCodeLocation) {
            this._timelineSidebarPanel.showTimelineViewForTimeline(this.representedObject);
            return;
        }

        WebInspector.showOriginalOrFormattedSourceCodeLocation(sourceCodeLocation);
    }

    treeElementPathComponentSelected(event)
    {
        // Implemented by sub-classes if needed.
    }

    treeElementDeselected(treeElement)
    {
        // Implemented by sub-classes if needed.
    }

    treeElementSelected(treeElement, selectedByUser)
    {
        // Implemented by sub-classes if needed.

        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);

        if (!this._timelineSidebarPanel.canShowDifferentContentView())
            return;

        if (treeElement instanceof WebInspector.FolderTreeElement)
            return;

        this.showContentViewForTreeElement(treeElement);
    }
};
