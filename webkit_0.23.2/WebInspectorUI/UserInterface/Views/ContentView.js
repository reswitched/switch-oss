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

WebInspector.ContentView = class ContentView extends WebInspector.View
{
    constructor(representedObject, extraArguments)
    {
        // Concrete object instantiation.
        console.assert(!representedObject || WebInspector.ContentView.isViewable(representedObject), representedObject);

        super();

        this._representedObject = representedObject;

        this.element.classList.add("content-view");

        this._parentContainer = null;
    }

    // Public

    static createFromRepresentedObject(representedObject, extraArguments)
    {
        console.assert(representedObject);

        if (representedObject instanceof WebInspector.Frame)
            return new WebInspector.ResourceClusterContentView(representedObject.mainResource, extraArguments);

        if (representedObject instanceof WebInspector.Resource)
            return new WebInspector.ResourceClusterContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.Script)
            return new WebInspector.ScriptContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.TimelineRecording)
            return new WebInspector.TimelineRecordingContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.Timeline) {
            var timelineType = representedObject.type;
            if (timelineType === WebInspector.TimelineRecord.Type.Network)
                return new WebInspector.NetworkTimelineView(representedObject, extraArguments);

            if (timelineType === WebInspector.TimelineRecord.Type.Layout)
                return new WebInspector.LayoutTimelineView(representedObject, extraArguments);

            if (timelineType === WebInspector.TimelineRecord.Type.Script)
                return new WebInspector.ScriptTimelineView(representedObject, extraArguments);

            if (timelineType === WebInspector.TimelineRecord.Type.RenderingFrame)
                return new WebInspector.RenderingFrameTimelineView(representedObject, extraArguments);
        }

        if (representedObject instanceof WebInspector.Breakpoint) {
            if (representedObject.sourceCodeLocation)
                return WebInspector.ContentView.createFromRepresentedObject(representedObject.sourceCodeLocation.displaySourceCode, extraArguments);
        }

        if (representedObject instanceof WebInspector.DOMStorageObject)
            return new WebInspector.DOMStorageContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.CookieStorageObject)
            return new WebInspector.CookieStorageContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.DatabaseTableObject)
            return new WebInspector.DatabaseTableContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.DatabaseObject)
            return new WebInspector.DatabaseContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.IndexedDatabaseObjectStore)
            return new WebInspector.IndexedDatabaseObjectStoreContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.IndexedDatabaseObjectStoreIndex)
            return new WebInspector.IndexedDatabaseObjectStoreContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.ApplicationCacheFrame)
            return new WebInspector.ApplicationCacheFrameContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.DOMTree)
            return new WebInspector.FrameDOMTreeContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.DOMSearchMatchObject) {
            var resultView = new WebInspector.FrameDOMTreeContentView(WebInspector.frameResourceManager.mainFrame.domTree, extraArguments);
            resultView.restoreFromCookie({nodeToSelect: representedObject.domNode});
            return resultView;
        }

        if (representedObject instanceof WebInspector.SourceCodeSearchMatchObject) {
            var resultView;
            if (representedObject.sourceCode instanceof WebInspector.Resource)
                resultView = new WebInspector.ResourceClusterContentView(representedObject.sourceCode, extraArguments);
            else if (representedObject.sourceCode instanceof WebInspector.Script)
                resultView = new WebInspector.ScriptContentView(representedObject.sourceCode, extraArguments);
            else
                console.error("Unknown SourceCode", representedObject.sourceCode);

            var textRangeToSelect = representedObject.sourceCodeTextRange.formattedTextRange;
            var startPosition = textRangeToSelect.startPosition();
            resultView.restoreFromCookie({lineNumber: startPosition.lineNumber, columnNumber: startPosition.columnNumber});

            return resultView;
        }

        if (representedObject instanceof WebInspector.LogObject)
            return new WebInspector.LogContentView(representedObject, extraArguments);

        if (representedObject instanceof WebInspector.ContentFlow)
            return new WebInspector.ContentFlowDOMTreeContentView(representedObject, extraArguments);

        if (typeof representedObject === "string" || representedObject instanceof String)
            return new WebInspector.TextContentView(representedObject, extraArguments);

        console.assert(!WebInspector.ContentView.isViewable(representedObject));

        throw new Error("Can't make a ContentView for an unknown representedObject.");
    }

    static isViewable(representedObject)
    {
        if (representedObject instanceof WebInspector.Frame)
            return true;
        if (representedObject instanceof WebInspector.Resource)
            return true;
        if (representedObject instanceof WebInspector.Script)
            return true;
        if (representedObject instanceof WebInspector.TimelineRecording)
            return true;
        if (representedObject instanceof WebInspector.Timeline)
            return true;
        if (representedObject instanceof WebInspector.Breakpoint)
            return representedObject.sourceCodeLocation;
        if (representedObject instanceof WebInspector.DOMStorageObject)
            return true;
        if (representedObject instanceof WebInspector.CookieStorageObject)
            return true;
        if (representedObject instanceof WebInspector.DatabaseTableObject)
            return true;
        if (representedObject instanceof WebInspector.DatabaseObject)
            return true;
        if (representedObject instanceof WebInspector.IndexedDatabaseObjectStore)
            return true;
        if (representedObject instanceof WebInspector.IndexedDatabaseObjectStoreIndex)
            return true;
        if (representedObject instanceof WebInspector.ApplicationCacheFrame)
            return true;
        if (representedObject instanceof WebInspector.DOMTree)
            return true;
        if (representedObject instanceof WebInspector.DOMSearchMatchObject)
            return true;
        if (representedObject instanceof WebInspector.SourceCodeSearchMatchObject)
            return true;
        if (representedObject instanceof WebInspector.LogObject)
            return true;
        if (representedObject instanceof WebInspector.ContentFlow)
            return true;
        if (typeof representedObject === "string" || representedObject instanceof String)
            return true;
        return false;
    }

    // Public

    get representedObject()
    {
        return this._representedObject;
    }

    get navigationItems()
    {
        // Navigation items that will be displayed by the ContentBrowser instance,
        // meant to be subclassed. Implemented by subclasses.
        return [];
    }

    get parentContainer()
    {
        return this._parentContainer;
    }

    get visible()
    {
        return this._visible;
    }

    set visible(flag)
    {
        this._visible = flag;
    }

    get scrollableElements()
    {
        // Implemented by subclasses.
        return [];
    }

    get shouldKeepElementsScrolledToBottom()
    {
        // Implemented by subclasses.
        return false;
    }

    get selectionPathComponents()
    {
        // Implemented by subclasses.
        return [];
    }

    get supplementalRepresentedObjects()
    {
        // Implemented by subclasses.
        return [];
    }

    get supportsSplitContentBrowser()
    {
        // Implemented by subclasses.
        return true;
    }

    shown()
    {
        // Implemented by subclasses.
    }

    hidden()
    {
        // Implemented by subclasses.
    }

    closed()
    {
        // Implemented by subclasses.
    }

    saveToCookie(cookie)
    {
        // Implemented by subclasses.
    }

    restoreFromCookie(cookie)
    {
        // Implemented by subclasses.
    }

    canGoBack()
    {
        // Implemented by subclasses.
        return false;
    }

    canGoForward()
    {
        // Implemented by subclasses.
        return false;
    }

    goBack()
    {
        // Implemented by subclasses.
    }

    goForward()
    {
        // Implemented by subclasses.
    }

    get supportsSearch()
    {
        // Implemented by subclasses.
        return false;
    }

    get numberOfSearchResults()
    {
        // Implemented by subclasses.
        return null;
    }

    get hasPerformedSearch()
    {
        // Implemented by subclasses.
        return false;
    }

    set automaticallyRevealFirstSearchResult(reveal)
    {
        // Implemented by subclasses.
    }

    performSearch(query)
    {
        // Implemented by subclasses.
    }

    searchCleared()
    {
        // Implemented by subclasses.
    }

    searchQueryWithSelection()
    {
        // Implemented by subclasses.
        return null;
    }

    revealPreviousSearchResult(changeFocus)
    {
        // Implemented by subclasses.
    }

    revealNextSearchResult(changeFocus)
    {
        // Implemented by subclasses.
    }
};

WebInspector.ContentView.Event = {
    SelectionPathComponentsDidChange: "content-view-selection-path-components-did-change",
    SupplementalRepresentedObjectsDidChange: "content-view-supplemental-represented-objects-did-change",
    NumberOfSearchResultsDidChange: "content-view-number-of-search-results-did-change",
    NavigationItemsDidChange: "content-view-navigation-items-did-change"
};
