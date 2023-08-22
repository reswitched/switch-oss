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

WebInspector.MultipleScopeBarItem = class MultipleScopeBarItem extends WebInspector.Object
{
    constructor(scopeBarItems)
    {
        super();

        this._element = document.createElement("li");
        this._element.classList.add("multiple");

        this._titleElement = document.createElement("span");
        this._element.appendChild(this._titleElement);
        this._element.addEventListener("click", this._clicked.bind(this));

        this._selectElement = document.createElement("select");
        this._selectElement.addEventListener("change", this._selectElementSelectionChanged.bind(this));
        this._element.appendChild(this._selectElement);

        this._element.appendChild(useSVGSymbol("Images/UpDownArrows.svg", "arrows"));

        this.scopeBarItems = scopeBarItems;
    }

    // Public

    get element()
    {
        return this._element;
    }

    get exclusive()
    {
        return false;
    }

    get scopeBarItems()
    {
        return this._scopeBarItems;
    }

    set scopeBarItems(scopeBarItems)
    {
        if (this._scopeBarItems) {
            for (var scopeBarItem of this._scopeBarItems)
                scopeBarItem.removeEventListener(null, null, this);
        }

        this._scopeBarItems = scopeBarItems || [];
        this._selectedScopeBarItem = null;

        this._selectElement.removeChildren();

        function createOption(scopeBarItem)
        {
            var optionElement = document.createElement("option");
            var maxPopupMenuLength = 130; // <rdar://problem/13445374> <select> with very long option has clipped text and popup menu is still very wide
            optionElement.textContent = scopeBarItem.label.length <= maxPopupMenuLength ? scopeBarItem.label : scopeBarItem.label.substring(0, maxPopupMenuLength) + "\u2026";
            return optionElement;
        }

        for (var scopeBarItem of this._scopeBarItems) {
            if (scopeBarItem.selected && !this._selectedScopeBarItem)
                this._selectedScopeBarItem = scopeBarItem;
            else if (scopeBarItem.selected) {
                // Only one selected item is allowed at a time.
                // Deselect any other items after the first.
                scopeBarItem.selected = false;
            }

            scopeBarItem.addEventListener(WebInspector.ScopeBarItem.Event.SelectionChanged, this._itemSelectionDidChange, this);

            this._selectElement.appendChild(createOption(scopeBarItem));
        }

        this._lastSelectedScopeBarItem = this._selectedScopeBarItem || this._scopeBarItems[0] || null;
        this._titleElement.textContent = this._lastSelectedScopeBarItem ? this._lastSelectedScopeBarItem.label : "";

        this._element.classList.toggle("selected", !!this._selectedScopeBarItem);
        this._selectElement.selectedIndex = this._scopeBarItems.indexOf(this._selectedScopeBarItem);
    }

    get selected()
    {
        return !!this._selectedScopeBarItem;
    }

    set selected(selected)
    {
        this.selectedScopeBarItem = selected ? this._lastSelectedScopeBarItem : null;
    }

    get selectedScopeBarItem()
    {
        return this._selectedScopeBarItem;
    }

    set selectedScopeBarItem(selectedScopeBarItem)
    {
        this._ignoreItemSelectedEvent = true;

        if (this._selectedScopeBarItem) {
            this._selectedScopeBarItem.selected = false;
            this._lastSelectedScopeBarItem = this._selectedScopeBarItem;
        } else if (!this._lastSelectedScopeBarItem)
            this._lastSelectedScopeBarItem = selectedScopeBarItem;

        this._element.classList.toggle("selected", !!selectedScopeBarItem);
        this._selectedScopeBarItem = selectedScopeBarItem || null;
        this._selectElement.selectedIndex = this._scopeBarItems.indexOf(this._selectedScopeBarItem);

        if (this._selectedScopeBarItem) {
            this._titleElement.textContent = this._selectedScopeBarItem.label;
            this._selectedScopeBarItem.selected = true;
        }

        var withModifier = WebInspector.modifierKeys.metaKey && !WebInspector.modifierKeys.ctrlKey && !WebInspector.modifierKeys.altKey && !WebInspector.modifierKeys.shiftKey;
        this.dispatchEventToListeners(WebInspector.ScopeBarItem.Event.SelectionChanged, {withModifier});

        this._ignoreItemSelectedEvent = false;
    }

    // Private

    _clicked(event)
    {
        // Only support click to select when the item is not selected yet.
        // Clicking when selected will cause the menu it appear instead.
        if (this._element.classList.contains("selected"))
            return;
        this.selected = true;
    }

    _selectElementSelectionChanged(event)
    {
        this.selectedScopeBarItem = this._scopeBarItems[this._selectElement.selectedIndex];
    }

    _itemSelectionDidChange(event)
    {
        if (this._ignoreItemSelectedEvent)
            return;
        this.selectedScopeBarItem = event.target.selected ? event.target : null;
    }
};
