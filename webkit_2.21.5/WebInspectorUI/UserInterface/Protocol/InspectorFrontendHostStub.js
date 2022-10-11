/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013 Seokju Kwon (seokju.kwon@gmail.com)
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (c) 2015-2019 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

if (!window.Symbol) {
    window.Symbol = function(string)
    {
        return string;
    }
}

if (!window.InspectorFrontendHost) {
    WI.InspectorFrontendHostStub = function()
    {
    };

    WI.InspectorFrontendHostStub.prototype = {
        // Public

        initializeWebSocket: function(url)
        {
            var socket = new WebSocket(url);
            socket.addEventListener("open", socketReady.bind(this));

            function socketReady()
            {
                this._socket = socket;

                this._socket.addEventListener("message", function(message) { console.log("recv: " + message.data); InspectorBackend.dispatch(message.data); });
                this._socket.addEventListener("error", function(error) { console.error(error); });

                this._sendPendingMessagesToBackendIfNeeded();
            }
        },

        loaded: function()
        {
            WI.updateVisibilityState(true);
        },

        closeWindow: function()
        {
            this._windowVisible = false;
        },

        bringToFront: function()
        {
            this._windowVisible = true;
        },

        inspectedURLChanged: function(title)
        {
            document.title = title;
        },

        setZoomFactor: function(zoom)
        {
        },

        zoomFactor: function()
        {
            return 1;
        },

        userInterfaceLayoutDirection: function()
        {
            return "ltr";
        },

        requestSetDockSide: function(side)
        {
            InspectorFrontendAPI.setDockSide(side);
        },

        setAttachedWindowHeight: function(height)
        {
        },

        setAttachedWindowWidth: function(width)
        {
        },

        startWindowDrag: function()
        {
        },

        moveWindowBy: function(x, y)
        {
        },

        localizedStringsURL: function()
        {
            return undefined;
        },

        backendCommandsURL: function()
        {
            return undefined;
        },

        debuggableType: function()
        {
            return "web";
        },

        inspectionLevel: function()
        {
            return 1;
        },

        copyText: function(text)
        {
            this._textToCopy = text;
            if (!document.execCommand("copy"))
                console.error("Clipboard access is denied");
        },

        killText: function(text, shouldStartNewSequence)
        {
        },

        openInNewTab: function(url)
        {
            window.open(url, "_blank");
        },

        canSave: function()
        {
            return false;
        },

        save: function(url, content, base64Encoded, forceSaveAs)
        {
        },

        append: function(url, content)
        {
        },

        close: function(url)
        {
        },

        platform: function()
        {
            return (navigator.platform.match(/mac|win|linux/i) || ["other"])[0].toLowerCase();
        },

        port: function()
        {
            return "unknown";
        },

        showContextMenu: function(event, items)
        {
            return new WI.SoftContextMenu(items).show(event);
        },

        dispatchEventAsContextMenuEvent: function(event)
        {
        },

        sendMessageToBackend: function(message)
        {
            console.log("send: " + message);
            if (!this._socket) {
                if (!this._pendingMessages)
                    this._pendingMessages = [];
                this._pendingMessages.push(message);
                return;
            }

            this._sendPendingMessagesToBackendIfNeeded();

            this._socket.send(message);
        },

        unbufferedLog: function(message)
        {
        },

        isUnderTest: function()
        {
            return false;
        },

        beep: function()
        {
        },

        inspectInspector: function()
        {
        },

        // Private

        _sendPendingMessagesToBackendIfNeeded: function()
        {
            if (!this._pendingMessages)
                return;

            for (var i = 0; i < this._pendingMessages.length; ++i)
                this._socket.send(this._pendingMessages[i]);

            delete this._pendingMessages;
        }
    };

    InspectorFrontendHost = new WI.InspectorFrontendHostStub;

    WI.dontLocalizeUserInterface = true;
}
