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

WebInspector.ScopeChainDetailsSidebarPanel = class ScopeChainDetailsSidebarPanel extends WebInspector.DetailsSidebarPanel
{
    constructor()
    {
        super("scope-chain", WebInspector.UIString("Scope Chain"), WebInspector.UIString("Scope Chain"));

        this._callFrame = null;

        this._watchExpressionsSetting = new WebInspector.Setting("watch-expressions", []);
        this._watchExpressionsSetting.addEventListener(WebInspector.Setting.Event.Changed, this._updateWatchExpressionsNavigationBar, this);

        this._watchExpressionOptionsElement = document.createElement("div");
        this._watchExpressionOptionsElement.classList.add("options");

        this._navigationBar = new WebInspector.NavigationBar;
        this._watchExpressionOptionsElement.appendChild(this._navigationBar.element);

        var addWatchExpressionButton = new WebInspector.ButtonNavigationItem("add-watch-expression", WebInspector.UIString("Add watch expression"), "Images/Plus13.svg", 13, 13);
        addWatchExpressionButton.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._addWatchExpressionButtonClicked, this);
        this._navigationBar.addNavigationItem(addWatchExpressionButton);

        this._clearAllWatchExpressionButton = new WebInspector.ButtonNavigationItem("clear-watch-expressions", WebInspector.UIString("Clear watch expressions"), "Images/NavigationItemTrash.svg", 14, 14);
        this._clearAllWatchExpressionButton.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._clearAllWatchExpressionsButtonClicked, this);
        this._navigationBar.addNavigationItem(this._clearAllWatchExpressionButton);

        this._refreshAllWatchExpressionButton = new WebInspector.ButtonNavigationItem("refresh-watch-expressions", WebInspector.UIString("Refresh watch expressions"), "Images/ReloadFull.svg", 13, 13);
        this._refreshAllWatchExpressionButton.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._refreshAllWatchExpressionsButtonClicked, this);
        this._navigationBar.addNavigationItem(this._refreshAllWatchExpressionButton);

        this._watchExpressionsSectionGroup = new WebInspector.DetailsSectionGroup;
        this._watchExpressionsSection = new WebInspector.DetailsSection("watch-expressions", WebInspector.UIString("Watch Expressions"), [this._watchExpressionsSectionGroup], this._watchExpressionOptionsElement);
        this.contentElement.appendChild(this._watchExpressionsSection.element);

        this._updateWatchExpressionsNavigationBar();

        this.needsRefresh();

        // Update on console prompt eval as objects in the scope chain may have changed.
        WebInspector.runtimeManager.addEventListener(WebInspector.RuntimeManager.Event.DidEvaluate, this._didEvaluateExpression, this);

        // Update watch expressions on navigations.
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
    }

    // Public

    inspect(objects)
    {
        // Convert to a single item array if needed.
        if (!(objects instanceof Array))
            objects = [objects];

        var callFrameToInspect = null;

        // Iterate over the objects to find a WebInspector.CallFrame to inspect.
        for (var i = 0; i < objects.length; ++i) {
            if (!(objects[i] instanceof WebInspector.CallFrame))
                continue;
            callFrameToInspect = objects[i];
            break;
        }

        this.callFrame = callFrameToInspect;

        return true;
    }

    get callFrame()
    {
        return this._callFrame;
    }

    set callFrame(callFrame)
    {
        if (callFrame === this._callFrame)
            return;

        this._callFrame = callFrame;

        this.needsRefresh();
    }

    refresh()
    {
        var callFrame = this._callFrame;

        Promise.all([this._generateWatchExpressionsSection(), this._generateCallFramesSection()]).then(function(sections) {
            var [watchExpressionsSection, callFrameSections] = sections;

            function delayedWork()
            {
                // Clear the timeout so we don't update the interface twice.
                clearTimeout(timeout);

                if (watchExpressionsSection)
                    this._watchExpressionsSectionGroup.rows = [watchExpressionsSection];
                else {
                    var emptyRow = new WebInspector.DetailsSectionRow(WebInspector.UIString("No Watch Expressions"));
                    this._watchExpressionsSectionGroup.rows = [emptyRow];
                    emptyRow.showEmptyMessage();
                }

                this.contentElement.removeChildren();
                this.contentElement.appendChild(this._watchExpressionsSection.element);

                // Bail if the call frame changed while we were waiting for the async response.
                if (this._callFrame !== callFrame)
                    return;

                if (!callFrameSections)
                    return;

                for (var callFrameSection of callFrameSections)
                    this.contentElement.appendChild(callFrameSection.element);
            }

            // We need a timeout in place in case there are long running, pending backend dispatches. This can happen
            // if the debugger is paused in code that was executed from the console. The console will be waiting for
            // the result of the execution and without a timeout we would never update the scope variables.
            var delay = WebInspector.ScopeChainDetailsSidebarPanel._autoExpandProperties.size === 0 ? 50 : 250;
            var timeout = setTimeout(delayedWork.bind(this), delay);

            // Since ObjectTreeView populates asynchronously, we want to wait to replace the existing content
            // until after all the pending asynchronous requests are completed. This prevents severe flashing while stepping.
            InspectorBackend.runAfterPendingDispatches(delayedWork.bind(this));
        }.bind(this)).catch(function(e) { console.error(e); });
    }

    _generateCallFramesSection()
    {
        var callFrame = this._callFrame;
        if (!callFrame)
            return Promise.resolve(null);

        var detailsSections = [];
        var foundLocalScope = false;

        var sectionCountByType = new Map;
        for (var type in WebInspector.ScopeChainNode.Type)
            sectionCountByType.set(WebInspector.ScopeChainNode.Type[type], 0);

        var scopeChain = callFrame.scopeChain;
        for (var scope of scopeChain) {
            var title = null;
            var extraPropertyDescriptor = null;
            var collapsedByDefault = false;

            var count = sectionCountByType.get(scope.type);
            sectionCountByType.set(scope.type, ++count);

            switch (scope.type) {
                case WebInspector.ScopeChainNode.Type.Local:
                    foundLocalScope = true;
                    collapsedByDefault = false;

                    title = WebInspector.UIString("Local Variables");

                    if (callFrame.thisObject)
                        extraPropertyDescriptor = new WebInspector.PropertyDescriptor({name: "this", value: callFrame.thisObject});
                    break;

                case WebInspector.ScopeChainNode.Type.Closure:
                    title = WebInspector.UIString("Closure Variables");
                    collapsedByDefault = false;
                    break;

                case WebInspector.ScopeChainNode.Type.Catch:
                    title = WebInspector.UIString("Catch Variables");
                    collapsedByDefault = false;
                    break;

                case WebInspector.ScopeChainNode.Type.FunctionName:
                    title = WebInspector.UIString("Function Name Variable");
                    collapsedByDefault = true;
                    break;

                case WebInspector.ScopeChainNode.Type.With:
                    title = WebInspector.UIString("With Object Properties");
                    collapsedByDefault = foundLocalScope;
                    break;

                case WebInspector.ScopeChainNode.Type.Global:
                    title = WebInspector.UIString("Global Variables");
                    collapsedByDefault = true;
                    break;
            }

            var detailsSectionIdentifier = scope.type + "-" + sectionCountByType.get(scope.type);

            var scopePropertyPath = WebInspector.PropertyPath.emptyPropertyPathForScope(scope.object);
            var objectTree = new WebInspector.ObjectTreeView(scope.object, WebInspector.ObjectTreeView.Mode.Properties, scopePropertyPath);

            objectTree.showOnlyProperties();

            if (extraPropertyDescriptor)
                objectTree.appendExtraPropertyDescriptor(extraPropertyDescriptor);

            var treeOutline = objectTree.treeOutline;
            treeOutline.onadd = this._objectTreeAddHandler.bind(this, detailsSectionIdentifier);
            treeOutline.onexpand = this._objectTreeExpandHandler.bind(this, detailsSectionIdentifier);
            treeOutline.oncollapse = this._objectTreeCollapseHandler.bind(this, detailsSectionIdentifier);

            var detailsSection = new WebInspector.DetailsSection(detailsSectionIdentifier, title, null, null, collapsedByDefault);
            detailsSection.groups[0].rows = [new WebInspector.DetailsSectionPropertiesRow(objectTree)];
            detailsSections.push(detailsSection);
        }

        return Promise.resolve(detailsSections);
    }

    _generateWatchExpressionsSection()
    {
        var watchExpressions = this._watchExpressionsSetting.value;
        if (!watchExpressions.length) {
            if (this._usedWatchExpressionsObjectGroup) {
                this._usedWatchExpressionsObjectGroup = false;
                RuntimeAgent.releaseObjectGroup(WebInspector.ScopeChainDetailsSidebarPanel.WatchExpressionsObjectGroupName);
            }
            return Promise.resolve(null);
        }

        RuntimeAgent.releaseObjectGroup(WebInspector.ScopeChainDetailsSidebarPanel.WatchExpressionsObjectGroupName);
        this._usedWatchExpressionsObjectGroup = true;

        var watchExpressionsRemoteObject = WebInspector.RemoteObject.createFakeRemoteObject();
        var fakePropertyPath = WebInspector.PropertyPath.emptyPropertyPathForScope(watchExpressionsRemoteObject);
        var objectTree = new WebInspector.ObjectTreeView(watchExpressionsRemoteObject, WebInspector.ObjectTreeView.Mode.Properties, fakePropertyPath);
        objectTree.showOnlyProperties();

        var treeOutline = objectTree.treeOutline;
        var watchExpressionSectionIdentifier = "watch-expressions";
        treeOutline.onadd = this._objectTreeAddHandler.bind(this, watchExpressionSectionIdentifier);
        treeOutline.onexpand = this._objectTreeExpandHandler.bind(this, watchExpressionSectionIdentifier);
        treeOutline.oncollapse = this._objectTreeCollapseHandler.bind(this, watchExpressionSectionIdentifier);
        treeOutline.objectTreeElementAddContextMenuItems = this._objectTreeElementAddContextMenuItems.bind(this);

        var promises = [];
        for (var expression of watchExpressions) {
            promises.push(new Promise(function(expression, resolve, reject) {
                WebInspector.runtimeManager.evaluateInInspectedWindow(expression, WebInspector.ScopeChainDetailsSidebarPanel.WatchExpressionsObjectGroupName, false, true, false, true, false, function(object, wasThrown) {
                    var propertyDescriptor = new WebInspector.PropertyDescriptor({name: expression, value: object}, undefined, undefined, wasThrown);
                    objectTree.appendExtraPropertyDescriptor(propertyDescriptor);
                    resolve(propertyDescriptor);
                });
            }.bind(null, expression)));
        }

        return Promise.all(promises).then(function() {
            return Promise.resolve(new WebInspector.DetailsSectionPropertiesRow(objectTree));
        });
    }

    _addWatchExpression(expression)
    {
        var watchExpressions = this._watchExpressionsSetting.value.slice(0);
        watchExpressions.push(expression);
        this._watchExpressionsSetting.value = watchExpressions;

        this.needsRefresh();
    }

    _removeWatchExpression(expression)
    {
        var watchExpressions = this._watchExpressionsSetting.value.slice(0);
        watchExpressions.remove(expression, true);
        this._watchExpressionsSetting.value = watchExpressions;

        this.needsRefresh();
    }

    _clearAllWatchExpressions()
    {
        this._watchExpressionsSetting.value = [];

        this.needsRefresh();
    }

    _addWatchExpressionButtonClicked(event)
    {
        function presentPopoverOverTargetElement()
        {
            var target = WebInspector.Rect.rectFromClientRect(event.target.element.getBoundingClientRect());
            popover.present(target, [WebInspector.RectEdge.MAX_Y, WebInspector.RectEdge.MIN_Y, WebInspector.RectEdge.MAX_X]);
        }

        var popover = new WebInspector.Popover(this);
        var content = document.createElement("div");
        content.classList.add("watch-expression");
        content.appendChild(document.createElement("div")).textContent = WebInspector.UIString("Add New Watch Expression");

        var editorElement = content.appendChild(document.createElement("div"));
        editorElement.classList.add("watch-expression-editor", WebInspector.SyntaxHighlightedStyleClassName);

        this._codeMirror = WebInspector.CodeMirrorEditor.create(editorElement, {
            lineWrapping: true,
            mode: "text/javascript",
            indentWithTabs: true,
            indentUnit: 4,
            matchBrackets: true,
            value: "",
        });

        this._popoverCommitted = false;

        this._codeMirror.addKeyMap({
            "Enter": function() { this._popoverCommitted = true; popover.dismiss(); }.bind(this),
        });

        var completionController = new WebInspector.CodeMirrorCompletionController(this._codeMirror);
        completionController.addExtendedCompletionProvider("javascript", WebInspector.javaScriptRuntimeCompletionProvider);

        // Resize the popover as best we can when the CodeMirror editor changes size.
        var previousHeight = 0;
        this._codeMirror.on("changes", function(cm, event) {
            var height = cm.getScrollInfo().height;
            if (previousHeight !== height) {
                previousHeight = height;
                popover.update(false);
            }
        });

        // Reposition the popover when the window resizes.
        this._windowResizeListener = presentPopoverOverTargetElement;
        window.addEventListener("resize", this._windowResizeListener);

        popover.content = content;
        presentPopoverOverTargetElement();

        // CodeMirror needs a refresh after the popover displays, to layout, otherwise it doesn't appear.
        setTimeout(function() {
            this._codeMirror.refresh();
            this._codeMirror.focus();
            popover.update();
        }.bind(this), 0);
    }

    willDismissPopover(popover)
    {
        if (this._popoverCommitted) {
            var expression = this._codeMirror.getValue().trim();
            if (expression)
                this._addWatchExpression(expression);
        }

        window.removeEventListener("resize", this._windowResizeListener);
        this._windowResizeListener = null;
        this._codeMirror = null;
    }

    _refreshAllWatchExpressionsButtonClicked(event)
    {
        this.needsRefresh();
    }

    _clearAllWatchExpressionsButtonClicked(event)
    {
        this._clearAllWatchExpressions();
    }

    _didEvaluateExpression(event)
    {
        if (event.data.objectGroup === WebInspector.ScopeChainDetailsSidebarPanel.WatchExpressionsObjectGroupName)
            return;

        this.needsRefresh();
    }

    _mainResourceDidChange(event)
    {
        if (!event.target.isMainFrame())
            return;

        this.needsRefresh();
    }

    _objectTreeElementAddContextMenuItems(objectTreeElement, contextMenu)
    {
        // Only add our watch expression context menus to the top level ObjectTree elements.
        if (objectTreeElement.parent !== objectTreeElement.treeOutline)
            return;

        contextMenu.appendItem(WebInspector.UIString("Remove Watch Expression"), function() {
            var expression = objectTreeElement.property.name;
            this._removeWatchExpression(expression);
        }.bind(this));
    }

    _propertyPathIdentifierForTreeElement(identifier, objectPropertyTreeElement)
    {
        if (!objectPropertyTreeElement.property)
            return null;

        var propertyPath = objectPropertyTreeElement.thisPropertyPath();
        if (propertyPath.isFullPathImpossible())
            return null;

        return identifier + "-" + propertyPath.fullPath;
    }

    _objectTreeAddHandler(identifier, treeElement)
    {
        var propertyPathIdentifier = this._propertyPathIdentifierForTreeElement(identifier, treeElement);
        if (!propertyPathIdentifier)
            return;

        if (WebInspector.ScopeChainDetailsSidebarPanel._autoExpandProperties.has(propertyPathIdentifier))
            treeElement.expand();
    }

    _objectTreeExpandHandler(identifier, treeElement)
    {
        var propertyPathIdentifier = this._propertyPathIdentifierForTreeElement(identifier, treeElement);
        if (!propertyPathIdentifier)
            return;

        WebInspector.ScopeChainDetailsSidebarPanel._autoExpandProperties.add(propertyPathIdentifier);
    }

    _objectTreeCollapseHandler(identifier, treeElement)
    {
        var propertyPathIdentifier = this._propertyPathIdentifierForTreeElement(identifier, treeElement);
        if (!propertyPathIdentifier)
            return;

        WebInspector.ScopeChainDetailsSidebarPanel._autoExpandProperties.delete(propertyPathIdentifier);
    }

    _updateWatchExpressionsNavigationBar()
    {
        var enabled = this._watchExpressionsSetting.value.length;
        this._refreshAllWatchExpressionButton.enabled = enabled;
        this._clearAllWatchExpressionButton.enabled = enabled;
    }
};

WebInspector.ScopeChainDetailsSidebarPanel._autoExpandProperties = new Set;
WebInspector.ScopeChainDetailsSidebarPanel.WatchExpressionsObjectGroupName = "watch-expressions";
