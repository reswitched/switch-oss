/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.DOMNode = class DOMNode extends WebInspector.Object
{
    constructor(domAgent, doc, isInShadowTree, payload)
    {
        super();

        this._domAgent = domAgent;
        this._isInShadowTree = isInShadowTree;

        this.id = payload.nodeId;
        domAgent._idToDOMNode[this.id] = this;

        this._nodeType = payload.nodeType;
        this._nodeName = payload.nodeName;
        this._localName = payload.localName;
        this._nodeValue = payload.nodeValue;
        this._pseudoType = payload.pseudoType;
        this._computedRole = payload.role;

        if (this._nodeType === Node.DOCUMENT_NODE)
            this.ownerDocument = this;
        else
            this.ownerDocument = doc;

        this._attributes = [];
        this._attributesMap = {};
        if (payload.attributes)
            this._setAttributesPayload(payload.attributes);

        this._childNodeCount = payload.childNodeCount;
        this._children = null;
        this._filteredChildren = null;
        this._filteredChildrenNeedsUpdating = true;

        this._nextSibling = null;
        this._previousSibling = null;
        this.parentNode = null;

        this._enabledPseudoClasses = [];

        this._shadowRoots = [];
        if (payload.shadowRoots) {
            for (var i = 0; i < payload.shadowRoots.length; ++i) {
                var root = payload.shadowRoots[i];
                var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, true, root);
                this._shadowRoots.push(node);
            }
        }

        if (payload.templateContent) {
            this._templateContent = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, true, payload.templateContent);
            this._templateContent.parentNode = this;
        }

        if (payload.children)
            this._setChildrenPayload(payload.children);

        this._pseudoElements = new Map;
        if (payload.pseudoElements) {
            for (var i = 0; i < payload.pseudoElements.length; ++i) {
                var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, this._isInShadowTree, payload.pseudoElements[i]);
                node.parentNode = this;
                this._pseudoElements.set(node.pseudoType(), node);
            }
        }

        if (payload.contentDocument) {
            this._contentDocument = new WebInspector.DOMNode(domAgent, null, false, payload.contentDocument);
            this._children = [this._contentDocument];
            this._renumber();
        }

        if (payload.frameId)
            this._frameIdentifier = payload.frameId;

        if (this._nodeType === Node.ELEMENT_NODE) {
            // HTML and BODY from internal iframes should not overwrite top-level ones.
            if (this.ownerDocument && !this.ownerDocument.documentElement && this._nodeName === "HTML")
                this.ownerDocument.documentElement = this;
            if (this.ownerDocument && !this.ownerDocument.body && this._nodeName === "BODY")
                this.ownerDocument.body = this;
            if (payload.documentURL)
                this.documentURL = payload.documentURL;
        } else if (this._nodeType === Node.DOCUMENT_TYPE_NODE) {
            this.publicId = payload.publicId;
            this.systemId = payload.systemId;
            this.internalSubset = payload.internalSubset;
        } else if (this._nodeType === Node.DOCUMENT_NODE) {
            this.documentURL = payload.documentURL;
            this.xmlVersion = payload.xmlVersion;
        } else if (this._nodeType === Node.ATTRIBUTE_NODE) {
            this.name = payload.name;
            this.value = payload.value;
        }
    }

    // Public

    get frameIdentifier()
    {
        return this._frameIdentifier || this.ownerDocument.frameIdentifier;
    }

    get frame()
    {
        if (!this._frame)
            this._frame = WebInspector.frameResourceManager.frameForIdentifier(this.frameIdentifier);
        return this._frame;
    }

    get children()
    {
        if (!this._children)
            return null;

        if (WebInspector.showShadowDOMSetting.value)
            return this._children;

        if (this._filteredChildrenNeedsUpdating) {
            this._filteredChildrenNeedsUpdating = false;
            this._filteredChildren = this._children.filter(function(node) {
                return !node._isInShadowTree;
            });
        }

        return this._filteredChildren;
    }

    get firstChild()
    {
        var children = this.children;

        if (children && children.length > 0)
            return children[0];

        return null;
    }

    get lastChild()
    {
        var children = this.children;

        if (children && children.length > 0)
            return children.lastValue;

        return null;
    }

    get nextSibling()
    {
        if (WebInspector.showShadowDOMSetting.value)
            return this._nextSibling;

        var node = this._nextSibling;
        while (node) {
            if (!node._isInShadowTree)
                return node;
            node = node._nextSibling;
        }
        return null;
    }

    get previousSibling()
    {
        if (WebInspector.showShadowDOMSetting.value)
            return this._previousSibling;

        var node = this._previousSibling;
        while (node) {
            if (!node._isInShadowTree)
                return node;
            node = node._previousSibling;
        }
        return null;
    }

    get childNodeCount()
    {
        var children = this.children;
        if (children)
            return children.length;

        if (WebInspector.showShadowDOMSetting.value)
            return this._childNodeCount + this._shadowRoots.length;

        return this._childNodeCount;
    }

    set childNodeCount(count)
    {
        this._childNodeCount = count;
    }

    computedRole()
    {
        return this._computedRole;
    }

    hasAttributes()
    {
        return this._attributes.length > 0;
    }

    hasChildNodes()
    {
        return this.childNodeCount > 0;
    }

    hasShadowRoots()
    {
        return !!this._shadowRoots.length;
    }

    isInShadowTree()
    {
        return this._isInShadowTree;
    }

    isPseudoElement()
    {
        return this._pseudoType !== undefined;
    }

    nodeType()
    {
        return this._nodeType;
    }

    nodeName()
    {
        return this._nodeName;
    }

    nodeNameInCorrectCase()
    {
        return this.isXMLNode() ? this.nodeName() : this.nodeName().toLowerCase();
    }

    setNodeName(name, callback)
    {
        DOMAgent.setNodeName(this.id, name, this._makeUndoableCallback(callback));
    }

    localName()
    {
        return this._localName;
    }

    templateContent()
    {
        return this._templateContent || null;
    }

    pseudoType()
    {
        return this._pseudoType;
    }

    hasPseudoElements()
    {
        return this._pseudoElements.size > 0;
    }

    pseudoElements()
    {
        return this._pseudoElements;
    }

    beforePseudoElement()
    {
        return this._pseudoElements.get(WebInspector.DOMNode.PseudoElementType.Before) || null;
    }

    afterPseudoElement()
    {
        return this._pseudoElements.get(WebInspector.DOMNode.PseudoElementType.After) || null;
    }

    nodeValue()
    {
        return this._nodeValue;
    }

    setNodeValue(value, callback)
    {
        DOMAgent.setNodeValue(this.id, value, this._makeUndoableCallback(callback));
    }

    getAttribute(name)
    {
        var attr = this._attributesMap[name];
        return attr ? attr.value : undefined;
    }

    setAttribute(name, text, callback)
    {
        DOMAgent.setAttributesAsText(this.id, text, name, this._makeUndoableCallback(callback));
    }

    setAttributeValue(name, value, callback)
    {
        DOMAgent.setAttributeValue(this.id, name, value, this._makeUndoableCallback(callback));
    }

    attributes()
    {
        return this._attributes;
    }

    removeAttribute(name, callback)
    {
        function mycallback(error, success)
        {
            if (!error) {
                delete this._attributesMap[name];
                for (var i = 0;  i < this._attributes.length; ++i) {
                    if (this._attributes[i].name === name) {
                        this._attributes.splice(i, 1);
                        break;
                    }
                }
            }

            this._makeUndoableCallback(callback)(error);
        }
        DOMAgent.removeAttribute(this.id, name, mycallback.bind(this));
    }

    getChildNodes(callback)
    {
        if (this.children) {
            if (callback)
                callback(this.children);
            return;
        }

        function mycallback(error) {
            if (!error && callback)
                callback(this.children);
        }

        DOMAgent.requestChildNodes(this.id, mycallback.bind(this));
    }

    getSubtree(depth, callback)
    {
        function mycallback(error)
        {
            if (callback)
                callback(error ? null : this.children);
        }

        DOMAgent.requestChildNodes(this.id, depth, mycallback.bind(this));
    }

    getOuterHTML(callback)
    {
        DOMAgent.getOuterHTML(this.id, callback);
    }

    setOuterHTML(html, callback)
    {
        DOMAgent.setOuterHTML(this.id, html, this._makeUndoableCallback(callback));
    }

    removeNode(callback)
    {
        DOMAgent.removeNode(this.id, this._makeUndoableCallback(callback));
    }

    copyNode()
    {
        function copy(error, text)
        {
            if (!error)
                InspectorFrontendHost.copyText(text);
        }
        DOMAgent.getOuterHTML(this.id, copy);
    }

    eventListeners(callback)
    {
        DOMAgent.getEventListenersForNode(this.id, callback);
    }

    accessibilityProperties(callback)
    {
        function accessibilityPropertiesCallback(error, accessibilityProperties)
        {
            if (!error && callback && accessibilityProperties) {
                callback({
                    activeDescendantNodeId: accessibilityProperties.activeDescendantNodeId,
                    busy: accessibilityProperties.busy,
                    checked: accessibilityProperties.checked,
                    childNodeIds: accessibilityProperties.childNodeIds,
                    controlledNodeIds: accessibilityProperties.controlledNodeIds,
                    disabled: accessibilityProperties.disabled,
                    exists: accessibilityProperties.exists,
                    expanded: accessibilityProperties.expanded,
                    flowedNodeIds: accessibilityProperties.flowedNodeIds,
                    focused: accessibilityProperties.focused,
                    ignored: accessibilityProperties.ignored,
                    ignoredByDefault: accessibilityProperties.ignoredByDefault,
                    invalid: accessibilityProperties.invalid,
                    hidden: accessibilityProperties.hidden,
                    label: accessibilityProperties.label,
                    liveRegionAtomic: accessibilityProperties.liveRegionAtomic,
                    liveRegionRelevant: accessibilityProperties.liveRegionRelevant,
                    liveRegionStatus: accessibilityProperties.liveRegionStatus,
                    mouseEventNodeId: accessibilityProperties.mouseEventNodeId,
                    nodeId: accessibilityProperties.nodeId,
                    ownedNodeIds: accessibilityProperties.ownedNodeIds,
                    parentNodeId: accessibilityProperties.parentNodeId,
                    pressed: accessibilityProperties.pressed,
                    readonly: accessibilityProperties.readonly,
                    required: accessibilityProperties.required,
                    role: accessibilityProperties.role,
                    selected: accessibilityProperties.selected,
                    selectedChildNodeIds: accessibilityProperties.selectedChildNodeIds
                });
            }
        }
        DOMAgent.getAccessibilityPropertiesForNode(this.id, accessibilityPropertiesCallback.bind(this));
    }

    path()
    {
        var path = [];
        var node = this;
        while (node && "index" in node && node._nodeName.length) {
            path.push([node.index, node._nodeName]);
            node = node.parentNode;
        }
        path.reverse();
        return path.join(",");
    }

    appropriateSelectorFor(justSelector)
    {
        if (this.isPseudoElement())
            return this.parentNode.appropriateSelectorFor() + "::" + this._pseudoType;

        var lowerCaseName = this.localName() || this.nodeName().toLowerCase();

        var id = this.getAttribute("id");
        if (id) {
            if (/[\s'"]/.test(id)) {
                id = id.replace(/\\/g, "\\\\").replace(/\"/g, "\\\"");
                selector = lowerCaseName + "[id=\"" + id + "\"]";
            } else
                selector = "#" + id;
            return (justSelector ? selector : lowerCaseName + selector);
        }

        var className = this.getAttribute("class");
        if (className) {
            var selector = "." + className.trim().replace(/\s+/g, ".");
            return (justSelector ? selector : lowerCaseName + selector);
        }

        if (lowerCaseName === "input" && this.getAttribute("type"))
            return lowerCaseName + "[type=\"" + this.getAttribute("type") + "\"]";

        return lowerCaseName;
    }

    isAncestor(node)
    {
        if (!node)
            return false;

        var currentNode = node.parentNode;
        while (currentNode) {
            if (this === currentNode)
                return true;
            currentNode = currentNode.parentNode;
        }
        return false;
    }

    isDescendant(descendant)
    {
        return descendant !== null && descendant.isAncestor(this);
    }

    _setAttributesPayload(attrs)
    {
        this._attributes = [];
        this._attributesMap = {};
        for (var i = 0; i < attrs.length; i += 2)
            this._addAttribute(attrs[i], attrs[i + 1]);
    }

    _insertChild(prev, payload)
    {
        var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, this._isInShadowTree, payload);
        if (!prev) {
            if (!this._children) {
                // First node
                this._children = this._shadowRoots.concat([node]);
            } else
                this._children.unshift(node);
        } else
            this._children.splice(this._children.indexOf(prev) + 1, 0, node);
        this._renumber();
        return node;
    }

    _removeChild(node)
    {
        // FIXME: Handle removal if this is a shadow root.
        if (node.isPseudoElement()) {
            this._pseudoElements.delete(node.pseudoType());
            node.parentNode = null;
        } else {
            this._children.splice(this._children.indexOf(node), 1);
            node.parentNode = null;
            this._renumber();
        }
    }

    _setChildrenPayload(payloads)
    {
        // We set children in the constructor.
        if (this._contentDocument)
            return;

        this._children = this._shadowRoots.slice();
        for (var i = 0; i < payloads.length; ++i) {
            var node = new WebInspector.DOMNode(this._domAgent, this.ownerDocument, this._isInShadowTree, payloads[i]);
            this._children.push(node);
        }
        this._renumber();
    }

    _renumber()
    {
        this._filteredChildrenNeedsUpdating = true;

        var childNodeCount = this._children.length;
        if (childNodeCount === 0)
            return;

        for (var i = 0; i < childNodeCount; ++i) {
            var child = this._children[i];
            child.index = i;
            child._nextSibling = i + 1 < childNodeCount ? this._children[i + 1] : null;
            child._previousSibling = i - 1 >= 0 ? this._children[i - 1] : null;
            child.parentNode = this;
        }
    }

    _addAttribute(name, value)
    {
        var attr = {name, value, _node: this};
        this._attributesMap[name] = attr;
        this._attributes.push(attr);
    }

    _setAttribute(name, value)
    {
        var attr = this._attributesMap[name];
        if (attr)
            attr.value = value;
        else
            this._addAttribute(name, value);
    }

    _removeAttribute(name)
    {
        var attr = this._attributesMap[name];
        if (attr) {
            this._attributes.remove(attr);
            delete this._attributesMap[name];
        }
    }

    moveTo(targetNode, anchorNode, callback)
    {
        DOMAgent.moveTo(this.id, targetNode.id, anchorNode ? anchorNode.id : undefined, this._makeUndoableCallback(callback));
    }

    isXMLNode()
    {
        return !!this.ownerDocument && !!this.ownerDocument.xmlVersion;
    }

    get enabledPseudoClasses()
    {
        return this._enabledPseudoClasses;
    }

    setPseudoClassEnabled(pseudoClass, enabled)
    {
        var pseudoClasses = this._enabledPseudoClasses;
        if (enabled) {
            if (pseudoClasses.includes(pseudoClass))
                return;
            pseudoClasses.push(pseudoClass);
        } else {
            if (!pseudoClasses.includes(pseudoClass))
                return;
            pseudoClasses.remove(pseudoClass);
        }

        function changed(error)
        {
            if (!error)
                this.dispatchEventToListeners(WebInspector.DOMNode.Event.EnabledPseudoClassesChanged);
        }

        CSSAgent.forcePseudoState(this.id, pseudoClasses, changed.bind(this));
    }

    _makeUndoableCallback(callback)
    {
        return function(error)
        {
            if (!error)
                DOMAgent.markUndoableState();

            if (callback)
                callback.apply(null, arguments);
        };
    }
};

WebInspector.DOMNode.Event = {
    EnabledPseudoClassesChanged: "dom-node-enabled-pseudo-classes-did-change",
    AttributeModified: "dom-node-attribute-modified",
    AttributeRemoved: "dom-node-attribute-removed"
};

WebInspector.DOMNode.PseudoElementType = {
    Before: "before",
    After: "after",
};
