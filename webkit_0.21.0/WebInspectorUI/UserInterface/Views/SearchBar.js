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

WebInspector.SearchBar = class SearchBar extends WebInspector.NavigationItem
{
    constructor(identifier, placeholder, delegate, suppressIncremental)
    {
        super(identifier);

        this.delegate = delegate;

        this._element.classList.add("search-bar");

        this._keyboardShortcutEsc = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Escape);
        this._keyboardShortcutEnter = new WebInspector.KeyboardShortcut(null, WebInspector.KeyboardShortcut.Key.Enter);

        this._searchInput = this._element.appendChild(document.createElement("input"));
        this._searchInput.type = "search";
        this._searchInput.spellcheck = false;
        this._searchInput.incremental = !suppressIncremental;
        this._searchInput.setAttribute("results", 5);
        this._searchInput.setAttribute("autosave", identifier + "-autosave");
        this._searchInput.setAttribute("placeholder", placeholder);
        this._searchInput.addEventListener("search", this._handleSearchEvent.bind(this));
        this._searchInput.addEventListener("keydown", this._handleKeydownEvent.bind(this));
    }

    // Public

    get text()
    {
        return this._searchInput.value;
    }

    set text(newText)
    {
        this._searchInput.value = newText;
    }

    focus()
    {
        // FIXME: Workaround for: <https://webkit.org/b/149504> Caret missing from <input> after clearing text and calling select()
        if (!this._searchInput.value.length)
            this._searchInput.focus();
        else
            this._searchInput.select();
    }

    // Private

    _handleSearchEvent(event)
    {
        this.dispatchEventToListeners(WebInspector.SearchBar.Event.TextChanged);
    }

    _handleKeydownEvent(event)
    {
        if (this._keyboardShortcutEsc.matchesEvent(event)) {
            if (this.delegate && typeof this.delegate.searchBarWantsToLoseFocus === "function") {
                this.delegate.searchBarWantsToLoseFocus(this);
                event.stopPropagation();
                event.preventDefault();
            }
        } else if (this._keyboardShortcutEnter.matchesEvent(event)) {
            if (this.delegate && typeof this.delegate.searchBarDidActivate === "function") {
                this.delegate.searchBarDidActivate(this);
                event.stopPropagation();
                event.preventDefault();
            }
        }
    }
};

WebInspector.SearchBar.Event = {
    TextChanged: "searchbar-text-did-change"
};
