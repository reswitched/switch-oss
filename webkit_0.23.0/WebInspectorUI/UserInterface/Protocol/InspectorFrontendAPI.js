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

InspectorFrontendAPI = {
    _loaded: false,
    _pendingCommands: [],

    savedURL: function(url)
    {
        // Not used yet.
    },

    appendedToURL: function(url)
    {
        // Not used yet.
    },

    isTimelineProfilingEnabled: function()
    {
        return WebInspector.timelineManager.isCapturing();
    },

    setTimelineProfilingEnabled: function(enabled)
    {
        if (WebInspector.timelineManager.isCapturing() === enabled)
            return;

        if (enabled) {
            WebInspector.showTimelineTab();
            WebInspector.timelineManager.startCapturing();
        } else {
            WebInspector.timelineManager.stopCapturing();
        }
    },

    setDockingUnavailable: function(unavailable)
    {
        WebInspector.updateDockingAvailability(!unavailable);
    },

    setDockSide: function(side)
    {
        WebInspector.updateDockedState(side);
    },

    showConsole: function()
    {
        WebInspector.showConsoleTab();

        WebInspector.quickConsole.prompt.focus();

        // If the page is still loading, focus the quick console again after tabindex autofocus.
        if (document.readyState !== "complete")
            document.addEventListener("readystatechange", this);
        if (document.visibilityState !== "visible")
            document.addEventListener("visibilitychange", this);  
    },

    handleEvent: function(event)
    {
        console.assert(event.type === "readystatechange" || event.type === "visibilitychange");

        if (document.readyState === "complete" && document.visibilityState === "visible") {
            WebInspector.quickConsole.prompt.focus();
            document.removeEventListener("readystatechange", this);
            document.removeEventListener("visibilitychange", this);
        }
    },

    showResources: function()
    {
        WebInspector.showResourcesTab();
    },

    showMainResourceForFrame: function(frameIdentifier)
    {
        WebInspector.showSourceCodeForFrame(frameIdentifier);
    },

    contextMenuItemSelected: function(id)
    {
        WebInspector.ContextMenu.contextMenuItemSelected(id);
    },

    contextMenuCleared: function()
    {
        WebInspector.ContextMenu.contextMenuCleared();
    },

    dispatchMessageAsync: function(messageObject)
    {
        WebInspector.dispatchMessageFromBackend(messageObject);
    },

    dispatchMessage: function(messageObject)
    {
        InspectorBackend.dispatch(messageObject);
    },

    dispatch: function(signature)
    {
        if (!InspectorFrontendAPI._loaded) {
            InspectorFrontendAPI._pendingCommands.push(signature);
            return null;
        }

        var methodName = signature.shift();
        console.assert(InspectorFrontendAPI[methodName], "Unexpected InspectorFrontendAPI method name: " + methodName);
        if (!InspectorFrontendAPI[methodName])
            return;

        return InspectorFrontendAPI[methodName].apply(InspectorFrontendAPI, signature);
    },

    loadCompleted: function()
    {
        InspectorFrontendAPI._loaded = true;

        for (var i = 0; i < InspectorFrontendAPI._pendingCommands.length; ++i)
            InspectorFrontendAPI.dispatch(InspectorFrontendAPI._pendingCommands[i]);

        delete InspectorFrontendAPI._pendingCommands;
    }
};
