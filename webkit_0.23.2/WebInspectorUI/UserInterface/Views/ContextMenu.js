/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

WebInspector.ContextMenuItem = class ContextMenuItem extends WebInspector.Object
{
    constructor(topLevelMenu, type, label, disabled, checked)
    {
        super();

        this._type = type;
        this._label = label;
        this._disabled = disabled;
        this._checked = checked;
        this._contextMenu = topLevelMenu || this;

        if (type === "item" || type === "checkbox")
            this._id = topLevelMenu.nextId();
    }

    // Public

    id()
    {
        return this._id;
    }

    type()
    {
        return this._type;
    }

    isEnabled()
    {
        return !this._disabled;
    }

    setEnabled(enabled)
    {
        this._disabled = !enabled;
    }

    // Private

    _buildDescriptor()
    {
        switch (this._type) {
        case "item":
            return { type: "item", id: this._id, label: this._label, enabled: !this._disabled };
        case "separator":
            return { type: "separator" };
        case "checkbox":
            return { type: "checkbox", id: this._id, label: this._label, checked: !!this._checked, enabled: !this._disabled };
        }
    }
};

WebInspector.ContextSubMenuItem = class ContextSubMenuItem extends WebInspector.ContextMenuItem
{
    constructor(topLevelMenu, label, disabled)
    {
        super(topLevelMenu, "subMenu", label, disabled);

        this._items = [];
    }

    // Public

    appendItem(label, handler, disabled)
    {
        var item = new WebInspector.ContextMenuItem(this._contextMenu, "item", label, disabled);
        this._pushItem(item);
        this._contextMenu._setHandler(item.id(), handler);
        return item;
    }

    appendSubMenuItem(label, disabled)
    {
        var item = new WebInspector.ContextSubMenuItem(this._contextMenu, label, disabled);
        this._pushItem(item);
        return item;
    }

    appendCheckboxItem(label, handler, checked, disabled)
    {
        var item = new WebInspector.ContextMenuItem(this._contextMenu, "checkbox", label, disabled, checked);
        this._pushItem(item);
        this._contextMenu._setHandler(item.id(), handler);
        return item;
    }

    appendSeparator()
    {
        if (this._items.length)
            this._pendingSeparator = true;
    }

    _pushItem(item)
    {
        if (this._pendingSeparator) {
            this._items.push(new WebInspector.ContextMenuItem(this._contextMenu, "separator"));
            delete this._pendingSeparator;
        }
        this._items.push(item);
    }

    isEmpty()
    {
        return !this._items.length;
    }

    _buildDescriptor()
    {
        var result = { type: "subMenu", label: this._label, enabled: !this._disabled, subItems: [] };
        for (var i = 0; i < this._items.length; ++i)
            result.subItems.push(this._items[i]._buildDescriptor());
        return result;
    }
};

WebInspector.ContextMenu = class ContextMenu extends WebInspector.ContextSubMenuItem
{
    constructor(event)
    {
        super(null, "");

        this._event = event;
        this._handlers = {};
        this._id = 0;
    }

    // Static

    static contextMenuItemSelected(id)
    {
        if (WebInspector.ContextMenu._lastContextMenu)
            WebInspector.ContextMenu._lastContextMenu._itemSelected(id);
    }

    static contextMenuCleared()
    {
        // FIXME: Unfortunately, contextMenuCleared is invoked between show and item selected
        // so we can't delete last menu object from WebInspector. Fix the contract.
    }

    // Public

    nextId()
    {
        return this._id++;
    }

    show()
    {
        console.assert(this._event instanceof MouseEvent);

        var menuObject = this._buildDescriptor();

        if (menuObject.length) {
            WebInspector.ContextMenu._lastContextMenu = this;

            if (this._event.type !== "contextmenu" && typeof InspectorFrontendHost.dispatchEventAsContextMenuEvent === "function") {
                this._menuObject = menuObject;
                this._event.target.addEventListener("contextmenu", this, true);
                InspectorFrontendHost.dispatchEventAsContextMenuEvent(this._event);
            } else
                InspectorFrontendHost.showContextMenu(this._event, menuObject);
        }

        if (this._event)
            this._event.stopImmediatePropagation();
    }

    // Protected

    handleEvent(event)
    {
        this._event.target.removeEventListener("contextmenu", this, true);
        InspectorFrontendHost.showContextMenu(event, this._menuObject);
        delete this._menuObject;

        event.stopImmediatePropagation();
    }

    // Private

    _setHandler(id, handler)
    {
        if (handler)
            this._handlers[id] = handler;
    }

    _buildDescriptor()
    {
        var result = [];
        for (var i = 0; i < this._items.length; ++i)
            result.push(this._items[i]._buildDescriptor());
        return result;
    }

    _itemSelected(id)
    {
        if (this._handlers[id])
            this._handlers[id].call(this);
    }
};
