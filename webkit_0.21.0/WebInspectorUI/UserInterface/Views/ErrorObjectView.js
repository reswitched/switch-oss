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

WebInspector.ErrorObjectView = class ErrorObjectView extends WebInspector.Object
{
    constructor(object)
    {
        super();

        console.assert(object instanceof WebInspector.RemoteObject && object.subtype === "error", object);

        this._object = object;

        this._element = document.createElement("div");
        this._element.classList.add("error-object");
        var previewElement = WebInspector.FormattedValue.createElementForError(this._object);
        this._element.appendChild(previewElement);
        previewElement.addEventListener("click", this._handlePreviewOrTitleElementClick.bind(this));

        this._outlineElement = this._element.appendChild(document.createElement("div"));
        this._outlineElement.classList.add("error-object-outline");
        this._outline = new WebInspector.TreeOutline(this._outlineElement);
    }

    // Static

    static makeSourceLinkWithPrefix(sourceURL, lineNumber, columnNumber)
    {
        if (!sourceURL)
            return null;

        var span = document.createElement("span");
        span.classList.add("error-object-link-container");
        span.textContent = "  ";

        var a = WebInspector.linkifyLocation(sourceURL, parseInt(lineNumber) - 1, parseInt(columnNumber));
        a.classList.add("error-object-link");
        span.appendChild(a);

        return span;
    }

    // Public

    get object()
    {
        return this._object;
    }

    get element()
    {
        return this._element;
    }

    get treeOutline()
    {
        return this._outline;
    }

    get expanded()
    {
        return this._expanded;
    }

    update()
    {
        this._object.getOwnPropertyDescriptorsAsObject(function(properties) {
            console.assert(properties && properties.stack && properties.stack.value);

            if (!this._hasStackTrace)
                this._buildStackTrace(properties.stack.value.value);

            this._hasStackTrace = true;
        }.bind(this));
    }

    expand()
    {
        if (this._expanded)
            return;

        this._expanded = true;
        this._element.classList.add("expanded");

        if (this._previewView)
            this._previewView.showTitle();

        this.update();
    }

    collapse()
    {
        if (!this._expanded)
            return;

        this._expanded = false;
        this._element.classList.remove("expanded");

        if (this._previewView)
            this._previewView.showPreview();
    }

    // Private

    _handlePreviewOrTitleElementClick(event)
    {
        if (!this._expanded)
            this.expand();
        else
            this.collapse();

        event.stopPropagation();
    }

    _buildStackTrace(stackString)
    {
        var stackTrace = WebInspector.StackTrace.fromString(stackString);
        var stackTraceElement = new WebInspector.StackTraceView(stackTrace).element;
        this._outlineElement.appendChild(stackTraceElement);
    }
};
