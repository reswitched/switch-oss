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

WebInspector.ObjectTreePropertyTreeElement = class ObjectTreePropertyTreeElement extends WebInspector.ObjectTreeBaseTreeElement
{
    constructor(property, propertyPath, mode, prototypeName)
    {
        super(property, propertyPath, property);

        this._mode = mode || WebInspector.ObjectTreeView.Mode.Properties;
        this._prototypeName = prototypeName;

        this.mainTitle = this._titleFragment();
        this.addClassName("object-tree-property");

        if (this.property.hasValue()) {
            this.addClassName(this.property.value.type);
            if (this.property.value.subtype)
                this.addClassName(this.property.value.subtype);
        } else
            this.addClassName("accessor");

        if (this.property.wasThrown)
            this.addClassName("had-error");
        if (this.property.name === "__proto__")
            this.addClassName("prototype-property");

        this._updateTooltips();
        this._updateHasChildren();
    }

    // Protected

    onpopulate()
    {
        this._updateChildren();
    }

    onexpand()
    {
        if (this._previewView)
            this._previewView.showTitle();
    }

    oncollapse()
    {
        if (this._previewView)
            this._previewView.showPreview();
    }

    invokedGetter()
    {
        this.mainTitle = this._titleFragment();

        var resolvedValue = this.resolvedValue();
        this.addClassName(resolvedValue.type);
        if (resolvedValue.subtype)
            this.addClassName(resolvedValue.subtype);
        if (this.hadError())
            this.addClassName("had-error");
        this.removeClassName("accessor");

        this._updateHasChildren();
    }

    // Private

    _updateHasChildren()
    {
        var resolvedValue = this.resolvedValue();
        var valueHasChildren = (resolvedValue && resolvedValue.hasChildren);
        var wasThrown = this.hadError();

        if (this._mode === WebInspector.ObjectTreeView.Mode.Properties)
            this.hasChildren = !wasThrown && valueHasChildren;
        else
            this.hasChildren = !wasThrown && valueHasChildren && (this.property.name === "__proto__" || this._alwaysDisplayAsProperty());
    }

    _updateTooltips()
    {
        var attributes = [];

        if (this.property.configurable)
            attributes.push("configurable");
        if (this.property.enumerable)
            attributes.push("enumerable");
        if (this.property.writable)
            attributes.push("writable");

        this.iconElement.title = attributes.join(" ");
    }

    _titleFragment()
    {
        if (this.property.name === "__proto__")
            return this._createTitlePrototype();

        if (this._mode === WebInspector.ObjectTreeView.Mode.Properties)
            return this._createTitlePropertyStyle();
        else
            return this._createTitleAPIStyle();
    }

    _createTitlePrototype()
    {
        console.assert(this.property.hasValue());
        console.assert(this.property.name === "__proto__");

        var nameElement = document.createElement("span");
        nameElement.className = "prototype-name";
        nameElement.textContent = WebInspector.UIString("%s Prototype").format(this._sanitizedPrototypeString(this.property.value));
        nameElement.title = this.propertyPathString(this.thisPropertyPath());
        return nameElement;
    }

    _createTitlePropertyStyle()
    {
        var container = document.createDocumentFragment();

        // Property name.
        var nameElement = document.createElement("span");
        nameElement.className = "property-name";
        nameElement.textContent = this.property.name + ": ";
        nameElement.title = this.propertyPathString(this.thisPropertyPath());

        // Property attributes.
        if (this._mode === WebInspector.ObjectTreeView.Mode.Properties) {
            if (!this.property.enumerable)
                nameElement.classList.add("not-enumerable");
        }

        // Value / Getter Value / Getter.
        var valueOrGetterElement;
        var resolvedValue = this.resolvedValue();
        if (resolvedValue) {
            if (resolvedValue.preview) {
                this._previewView = new WebInspector.ObjectPreviewView(resolvedValue.preview);
                valueOrGetterElement = this._previewView.element;
            } else {
                valueOrGetterElement = WebInspector.FormattedValue.createElementForRemoteObject(resolvedValue, this.hadError());

                // Special case a function property string.
                if (resolvedValue.type === "function")
                    valueOrGetterElement.textContent = this._functionPropertyString();
            }
        } else {
            valueOrGetterElement = document.createElement("span");
            if (this.property.hasGetter())
                valueOrGetterElement.appendChild(this.createGetterElement(this._mode !== WebInspector.ObjectTreeView.Mode.ClassAPI));
            if (this.property.hasSetter())
                valueOrGetterElement.appendChild(this.createSetterElement());
        }

        valueOrGetterElement.classList.add("value");
        if (this.hadError())
            valueOrGetterElement.classList.add("error");

        container.appendChild(nameElement);
        container.appendChild(valueOrGetterElement);
        return container;
    }

    _createTitleAPIStyle()
    {
        // Fixed values and special properties display like a property.
        if (this._alwaysDisplayAsProperty())
            return this._createTitlePropertyStyle();

        // No API to display.
        var isFunction = this.property.hasValue() && this.property.value.type === "function";
        if (!isFunction && !this.property.hasGetter() && !this.property.hasSetter())
            return null;

        var container = document.createDocumentFragment();

        // Function / Getter / Setter.
        var nameElement = document.createElement("span");
        nameElement.className = "property-name";
        nameElement.textContent = this.property.name;
        nameElement.title = this.propertyPathString(this.thisPropertyPath());
        container.appendChild(nameElement);

        if (isFunction) {
            var paramElement = document.createElement("span");
            paramElement.className = "function-parameters";
            paramElement.textContent = this._functionParameterString();
            container.appendChild(paramElement);
        } else {
            var spacer = container.appendChild(document.createElement("span"));
            spacer.className = "spacer";
            if (this.property.hasGetter())
                container.appendChild(this.createGetterElement(this._mode !== WebInspector.ObjectTreeView.Mode.ClassAPI));
            if (this.property.hasSetter())
                container.appendChild(this.createSetterElement());
        }

        return container;
    }

    _alwaysDisplayAsProperty()
    {
        // Constructor, though a function, is often better treated as an expandable object.
        if (this.property.name === "constructor")
            return true;

        // Non-function objects are often better treated as properties.
        if (this.property.hasValue() && this.property.value.type !== "function")
            return true;

        // Fetched getter value.
        if (this._getterValue)
            return true;

        return false;
    }

    _functionPropertyString()
    {
        return "function" + this._functionParameterString();
    }

    _functionParameterString()
    {
        var resolvedValue = this.resolvedValue();
        console.assert(resolvedValue.type === "function");

        // For Native methods, the toString is poor. We try to provide good function parameter strings.
        if (isFunctionStringNativeCode(resolvedValue.description)) {
            // Native function on a prototype, likely "Foo.prototype.method".
            if (this._prototypeName) {
                if (WebInspector.NativePrototypeFunctionParameters[this._prototypeName]) {
                    var params = WebInspector.NativePrototypeFunctionParameters[this._prototypeName][this._property.name];
                    return params ? "(" + params + ")" : "()";
                }
            }

            var parentDescription = this._propertyPath.object.description;

            // Native function property on a native function is likely a "Foo.method".
            if (isFunctionStringNativeCode(parentDescription)) {
                var match = parentDescription.match(/^function\s+([^)]+?)\(/);
                if (match) {
                    var name = match[1];
                    if (WebInspector.NativeConstructorFunctionParameters[name]) {
                        var params = WebInspector.NativeConstructorFunctionParameters[name][this._property.name];
                        return params ? "(" + params + ")" : "()";
                    }
                }
            }

            // Native DOM constructor or on native objects that are not functions.
            if (parentDescription.endsWith("Constructor") || parentDescription === "Math" || parentDescription === "JSON") {
                var name = parentDescription;
                if (WebInspector.NativeConstructorFunctionParameters[name]) {
                    var params = WebInspector.NativeConstructorFunctionParameters[name][this._property.name];
                    return params ? "(" + params + ")" : "()";
                }
            }
        }

        var match = resolvedValue.functionDescription.match(/^function.*?(\([^)]*?\))/);
        return match ? match[1] : "()";
    }

    _sanitizedPrototypeString(value)
    {
        // FIXME: <https://webkit.org/b/141610> For many X, X.prototype is an X when it must be a plain object
        if (value.type === "function")
            return "Function";
        if (value.subtype === "date")
            return "Date";
        if (value.subtype === "regexp")
            return "RegExp";

        return value.description.replace(/Prototype$/, "");
    }

    _updateChildren()
    {
        if (this.children.length && !this.shouldRefreshChildren)
            return;

        var resolvedValue = this.resolvedValue();
        if (resolvedValue.isCollectionType() && this._mode === WebInspector.ObjectTreeView.Mode.Properties)
            resolvedValue.getCollectionEntries(0, 100, this._updateChildrenInternal.bind(this, this._updateEntries, this._mode));
        else if (this._mode === WebInspector.ObjectTreeView.Mode.ClassAPI)
            resolvedValue.getOwnPropertyDescriptors(this._updateChildrenInternal.bind(this, this._updateProperties, WebInspector.ObjectTreeView.Mode.ClassAPI));
        else if (this.property.name === "__proto__")
            resolvedValue.getOwnPropertyDescriptors(this._updateChildrenInternal.bind(this, this._updateProperties, WebInspector.ObjectTreeView.Mode.PrototypeAPI));
        else
            resolvedValue.getDisplayablePropertyDescriptors(this._updateChildrenInternal.bind(this, this._updateProperties, this._mode));
    }

    _updateChildrenInternal(handler, mode, list)
    {
        this.removeChildren();

        if (!list) {
            var errorMessageElement = WebInspector.ObjectTreeView.createEmptyMessageElement(WebInspector.UIString("Could not fetch properties. Object may no longer exist."));
            this.appendChild(new WebInspector.TreeElement(errorMessageElement, null, false));
            return;
        }

        handler.call(this, list, this.resolvedValuePropertyPath(), mode);
    }

    _updateEntries(entries, propertyPath, mode)
    {
        for (var entry of entries) {
            if (entry.key) {
                this.appendChild(new WebInspector.ObjectTreeMapKeyTreeElement(entry.key, propertyPath));
                this.appendChild(new WebInspector.ObjectTreeMapValueTreeElement(entry.value, propertyPath, entry.key));
            } else
                this.appendChild(new WebInspector.ObjectTreeSetIndexTreeElement(entry.value, propertyPath));
        }

        if (!this.children.length) {
            var emptyMessageElement = WebInspector.ObjectTreeView.createEmptyMessageElement(WebInspector.UIString("No Entries."));
            this.appendChild(new WebInspector.TreeElement(emptyMessageElement, null, false));
        }

        // Show the prototype so users can see the API.
        var resolvedValue = this.resolvedValue();
        resolvedValue.getOwnPropertyDescriptor("__proto__", function(propertyDescriptor) {
            if (propertyDescriptor)
                this.appendChild(new WebInspector.ObjectTreePropertyTreeElement(propertyDescriptor, propertyPath, mode));
        }.bind(this));
    }

    _updateProperties(properties, propertyPath, mode)
    {
        properties.sort(WebInspector.ObjectTreeView.comparePropertyDescriptors);

        var resolvedValue = this.resolvedValue();
        var isArray = resolvedValue.isArray();
        var isPropertyMode = mode === WebInspector.ObjectTreeView.Mode.Properties || this._getterValue;
        var isAPI = mode !== WebInspector.ObjectTreeView.Mode.Properties;

        var prototypeName;
        if (this.property.name === "__proto__") {
            if (resolvedValue.description)
                prototypeName = this._sanitizedPrototypeString(resolvedValue);
        }

        var hadProto = false;
        for (var propertyDescriptor of properties) {
            // FIXME: If this is a pure API ObjectTree, we should show the native getters.
            // For now, just skip native binding getters in API mode, since we likely
            // already showed them in the Properties section.
            if (isAPI && propertyDescriptor.nativeGetter)
                continue;

            // COMPATIBILITY (iOS 8): Sometimes __proto__ is not a value, but a get/set property.
            // In those cases it is actually not useful to show.
            if (propertyDescriptor.name === "__proto__" && !propertyDescriptor.hasValue())
                continue;

            if (isArray && isPropertyMode) {
                if (propertyDescriptor.isIndexProperty())
                    this.appendChild(new WebInspector.ObjectTreeArrayIndexTreeElement(propertyDescriptor, propertyPath));
                else if (propertyDescriptor.name === "__proto__")
                    this.appendChild(new WebInspector.ObjectTreePropertyTreeElement(propertyDescriptor, propertyPath, mode, prototypeName));
            } else
                this.appendChild(new WebInspector.ObjectTreePropertyTreeElement(propertyDescriptor, propertyPath, mode, prototypeName));

            if (propertyDescriptor.name === "__proto__")
                hadProto = true;
        }

        if (!this.children.length || (hadProto && this.children.length === 1)) {
            var emptyMessageElement = WebInspector.ObjectTreeView.createEmptyMessageElement(WebInspector.UIString("No Properties."));
            this.insertChild(new WebInspector.TreeElement(emptyMessageElement, null, false), 0);
        }
    }
};
