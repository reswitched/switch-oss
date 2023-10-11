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

WebInspector.TypeTreeView = class TypeTreeView extends WebInspector.Object
{
    constructor(typeDescription)
    {
        super();

        console.assert(typeDescription instanceof WebInspector.TypeDescription);

        this._typeDescription = typeDescription;

        this._element = document.createElement("div");
        this._element.className = "type-tree";

        this._outlineElement = document.createElement("ol");
        this._outlineElement.className = "type-tree-outline";
        this._outline = new WebInspector.TreeOutline(this._outlineElement);
        this._element.appendChild(this._outlineElement);

        this._populate();

        // Auto-expand if there is one sub-type. TypeTreeElement handles further auto-expansion.
        if (this._outline.children.length === 1)
            this._outline.children[0].expand();
    }

    // Public

    get typeDescription()
    {
        return this._typeDescription;
    }

    get element()
    {
        return this._element;
    }

    get treeOutline()
    {
        return this._outline;
    }

    // Private

    _populate()
    {
        var types = [];
        for (var structure of this._typeDescription.structures)
            types.push({name: structure.constructorName, structure});
        for (var primitiveName of this._typeDescription.typeSet.primitiveTypeNames)
            types.push({name: primitiveName});
        types.sort(WebInspector.ObjectTreeView.comparePropertyDescriptors);

        for (var type of types)
            this._outline.appendChild(new WebInspector.TypeTreeElement(type.name, type.structure, false));

        if (this._typeDescription.truncated) {
            var truncatedMessageElement = WebInspector.ObjectTreeView.createEmptyMessageElement("\u2026");
            this._outline.appendChild(new WebInspector.TreeElement(truncatedMessageElement, null, false));
        }

        if (!this._outline.children.length) {
            var errorMessageElement = WebInspector.ObjectTreeView.createEmptyMessageElement(WebInspector.UIString("No properties."));
            this._outline.appendChild(new WebInspector.TreeElement(errorMessageElement, null, false));
        }
    }
};
