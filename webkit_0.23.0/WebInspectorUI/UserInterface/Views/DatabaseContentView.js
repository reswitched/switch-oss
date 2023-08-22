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

WebInspector.DatabaseContentView = class DatabaseContentView extends WebInspector.ContentView
{
    constructor(representedObject)
    {
        super(representedObject);

        this.database = representedObject;

        this.element.classList.add("storage-view", "query", "monospace");

        this._prompt = new WebInspector.ConsolePrompt(this, "text/x-sql");
        this.addSubview(this._prompt);

        this.element.addEventListener("click", this._messagesClicked.bind(this), true);
    }

    // Public

    shown()
    {
        // FIXME: remove once <https://webkit.org/b/150741> is fixed.
        this._prompt.shown();
    }

    saveToCookie(cookie)
    {
        cookie.type = WebInspector.ContentViewCookieType.Database;
        cookie.host = this.representedObject.host;
        cookie.name = this.representedObject.name;
    }

    consolePromptCompletionsNeeded(prompt, defaultCompletions, base, prefix, suffix)
    {
        var results = [];

        prefix = prefix.toLowerCase();

        function accumulateMatches(textArray)
        {
            for (var text of textArray) {
                if (text.toLowerCase().startsWith(prefix))
                    results.push(text);
            }
        }

        function tableNamesCallback(tableNames)
        {
            accumulateMatches(tableNames);
            accumulateMatches(["SELECT", "FROM", "WHERE", "LIMIT", "DELETE FROM", "CREATE", "DROP", "TABLE", "INDEX", "UPDATE", "INSERT INTO", "VALUES"]);

            this._prompt.updateCompletions(results, " ");
        }

        this.database.getTableNames(tableNamesCallback.bind(this));
    }

    consolePromptTextCommitted(prompt, query)
    {
        this.database.executeSQL(query, this._queryFinished.bind(this, query), this._queryError.bind(this, query));
    }

    // Private

    _messagesClicked()
    {
        this._prompt.focus();
    }

    _queryFinished(query, columnNames, values)
    {
        var trimmedQuery = query.trim();
        var queryView = new WebInspector.DatabaseUserQuerySuccessView(trimmedQuery, columnNames, values);
        this.insertSubviewBefore(queryView, this._prompt);

        if (queryView.dataGrid)
            queryView.dataGrid.autoSizeColumns(5);

        this._prompt.element.scrollIntoView(false);

        if (trimmedQuery.match(/^create /i) || trimmedQuery.match(/^drop table /i))
            this.dispatchEventToListeners(WebInspector.DatabaseContentView.Event.SchemaUpdated, this.database);
    }

    _queryError(query, error)
    {
        var message;
        if (error.message)
            message = error.message;
        else if (error.code === 2)
            message = WebInspector.UIString("Database no longer has expected version.");
        else
            message = WebInspector.UIString("An unexpected error %s occurred.").format(error.code);

        var queryView = new WebInspector.DatabaseUserQueryErrorView(query, message);
        this.insertSubviewBefore(queryView, this._prompt);
        this._prompt.element.scrollIntoView(false);
    }
};

WebInspector.DatabaseContentView.Event = {
    SchemaUpdated: "SchemaUpdated"
};
