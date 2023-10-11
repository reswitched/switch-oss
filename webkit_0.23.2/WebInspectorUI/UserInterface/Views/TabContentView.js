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

WebInspector.TabContentView = class TabContentView extends WebInspector.ContentView
{
    constructor(identifier, styleClassNames, tabBarItem, navigationSidebarPanel, detailsSidebarPanels)
    {
        console.assert(typeof identifier === "string");
        console.assert(typeof styleClassNames === "string" || styleClassNames.every(function(className) { return typeof className === "string"; }));
        console.assert(tabBarItem instanceof WebInspector.TabBarItem);
        console.assert(!navigationSidebarPanel || navigationSidebarPanel instanceof WebInspector.NavigationSidebarPanel);
        console.assert(!detailsSidebarPanels || detailsSidebarPanels.every(function(detailsSidebarPanel) { return detailsSidebarPanel instanceof WebInspector.DetailsSidebarPanel; }));

        super(null);

        this.element.classList.add("tab");

        if (typeof styleClassNames === "string")
            styleClassNames = [styleClassNames];

        this.element.classList.add(...styleClassNames);

        this._identifier = identifier;
        this._tabBarItem = tabBarItem;
        this._navigationSidebarPanel = navigationSidebarPanel || null;
        this._detailsSidebarPanels = detailsSidebarPanels || [];

        this._navigationSidebarCollapsedSetting = new WebInspector.Setting(identifier + "-navigation-sidebar-collapsed", false);

        this._detailsSidebarCollapsedSetting = new WebInspector.Setting(identifier + "-details-sidebar-collapsed", true);
        this._detailsSidebarSelectedPanelSetting = new WebInspector.Setting(identifier + "-details-sidebar-selected-panel", null);

        this._cookieSetting = new WebInspector.Setting(identifier + "-tab-cookie", {});
    }

    // Public

    get type()
    {
        // Implemented by subclasses.
        return null;
    }

    get parentTabBrowser()
    {
        return this._parentTabBrowser;
    }

    set parentTabBrowser(tabBrowser)
    {
        this._parentTabBrowser = tabBrowser || null;
    }

    get identifier()
    {
        return this._identifier;
    }

    get tabBarItem()
    {
        return this._tabBarItem;
    }

    get managesDetailsSidebarPanels()
    {
        // Implemented by subclasses.
        return false;
    }

    showDetailsSidebarPanels()
    {
        // Implemented by subclasses.
    }

    showRepresentedObject(representedObject, cookie)
    {
        // Implemented by subclasses.
    }

    canShowRepresentedObject(representedObject)
    {
        // Implemented by subclasses.
        return false;
    }

    shown()
    {
        if (this._shouldRestoreStateWhenShown)
            this.restoreStateFromCookie(WebInspector.StateRestorationType.Delayed);
    }

    restoreStateFromCookie(restorationType)
    {
        if (!this.navigationSidebarPanel)
            return;

        if (!this.visible) {
            this._shouldRestoreStateWhenShown = true;
            return;
        }

        this._shouldRestoreStateWhenShown = false;

        var relaxMatchDelay = 0;
        if (restorationType === WebInspector.StateRestorationType.Load)
            relaxMatchDelay = 1000;
        else if (restorationType === WebInspector.StateRestorationType.Navigation)
            relaxMatchDelay = 2000;

        this.navigationSidebarPanel.restoreStateFromCookie(this._cookieSetting.value || {}, relaxMatchDelay);
    }

    saveStateToCookie()
    {
        if (!this.navigationSidebarPanel)
            return;

        if (this._shouldRestoreStateWhenShown)
            return;

        var cookie = {};
        this.navigationSidebarPanel.saveStateToCookie(cookie);
        this._cookieSetting.value = cookie;
    }

    get navigationSidebarPanel()
    {
        return this._navigationSidebarPanel;
    }

    get navigationSidebarCollapsedSetting()
    {
        return this._navigationSidebarCollapsedSetting;
    }

    get detailsSidebarPanels()
    {
        return this._detailsSidebarPanels;
    }

    get detailsSidebarCollapsedSetting()
    {
        return this._detailsSidebarCollapsedSetting;
    }

    get detailsSidebarSelectedPanelSetting()
    {
        return this._detailsSidebarSelectedPanelSetting;
    }
};
