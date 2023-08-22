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

WebInspector.IssueManager = class IssueManager extends WebInspector.Object
{
    constructor()
    {
        super();

        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
        WebInspector.logManager.addEventListener(WebInspector.LogManager.Event.Cleared, this._logCleared, this);

        this.initialize();
    }

    // Public

    initialize()
    {
        this._issues = [];

        this.dispatchEventToListeners(WebInspector.IssueManager.Event.Cleared);
    }

    issueWasAdded(source, level, text, url, lineNumber, columnNumber, parameters)
    {
        var modifiedLineNumber;
        if (lineNumber) {
            console.assert(typeof lineNumber === "number");
            modifiedLineNumber = lineNumber - 1;
        }

        var issue = new WebInspector.IssueMessage(source, level, text, url, modifiedLineNumber, columnNumber, parameters);
        this._issues.push(issue);

        this.dispatchEventToListeners(WebInspector.IssueManager.Event.IssueWasAdded, {issue});
    }

    issuesForSourceCode(sourceCode)
    {
        var issues = [];

        for (var i = 0; i < this._issues.length; ++i) {
            // FIXME: Support issues based on Script identifiers too.
            var issue = this._issues[i];
            if (issue.url === sourceCode.url)
                issues.push(issue);
        }

        return issues;
    }

    // Private

    _logCleared(event)
    {
        this.initialize();
    }

    _mainResourceDidChange(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        this.initialize();
    }
};

WebInspector.IssueManager.Event = {
    IssueWasAdded: "issue-manager-issue-was-added",
    Cleared: "issue-manager-cleared"
};
