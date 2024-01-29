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

WebInspector.CallFrameView = class CallFrameView extends WebInspector.Object
{
    constructor(callFrame, showFunctionName)
    {
        console.assert(callFrame instanceof WebInspector.CallFrame);

        var callFrameElement = document.createElement("div");
        callFrameElement.classList.add("call-frame", WebInspector.CallFrameView.iconClassNameForCallFrame(callFrame));

        var subtitleElement = document.createElement("span");
        subtitleElement.classList.add("subtitle");

        var sourceCodeLocation = callFrame.sourceCodeLocation;
        if (sourceCodeLocation) {
            WebInspector.linkifyElement(callFrameElement, sourceCodeLocation);

            var linkElement = document.createElement("a");
            linkElement.classList.add("source-link");
            linkElement.href = sourceCodeLocation.sourceCode.url;

            if (showFunctionName) {
                var separatorElement = document.createElement("span");
                separatorElement.classList.add("separator");
                separatorElement.textContent = "  ";
                subtitleElement.appendChild(separatorElement);
            }

            subtitleElement.appendChild(linkElement);

            sourceCodeLocation.populateLiveDisplayLocationTooltip(linkElement);
            sourceCodeLocation.populateLiveDisplayLocationString(linkElement, "textContent");
        }

        var titleElement = document.createElement("span");
        titleElement.classList.add("title");

        if (showFunctionName) {
            var imgElement = document.createElement("img");
            imgElement.classList.add("icon");

            titleElement.appendChild(imgElement);
            titleElement.appendChild(document.createTextNode(callFrame.functionName || WebInspector.UIString("(anonymous function)")));
        }

        callFrameElement.appendChild(titleElement);
        callFrameElement.appendChild(subtitleElement);

        return callFrameElement;
    }

    static iconClassNameForCallFrame(callFrame)
    {
        if (callFrame.programCode)
            return WebInspector.CallFrameView.ProgramIconStyleClassName;

        // This is more than likely an event listener function with an "on" prefix and it is
        // as long or longer than the shortest event listener name -- "oncut".
        if (callFrame.functionName && callFrame.functionName.startsWith("on") && callFrame.functionName.length >= 5)
            return WebInspector.CallFrameView.EventListenerIconStyleClassName;

        if (callFrame.nativeCode)
            return WebInspector.CallFrameView.NativeIconStyleClassName;

        return WebInspector.CallFrameView.FunctionIconStyleClassName;
    }
};

WebInspector.CallFrameView.ProgramIconStyleClassName = "program-icon";
WebInspector.CallFrameView.FunctionIconStyleClassName = "function-icon";
WebInspector.CallFrameView.EventListenerIconStyleClassName = "event-listener-icon";
WebInspector.CallFrameView.NativeIconStyleClassName = "native-icon";
