/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
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

WebInspector.FolderizedTreeElement = class FolderizedTreeElement extends WebInspector.GeneralTreeElement
{
    constructor(classNames, title, subtitle, representedObject, hasChildren)
    {
        super(classNames, title, subtitle, representedObject, hasChildren);

        this.shouldRefreshChildren = true;

        this._folderExpandedSettingMap = new Map;
        this._folderSettingsKey = "";
        this._folderTypeMap = new Map;
        this._folderizeSettingsMap = new Map;
        this._groupedIntoFolders = false;
        this._clearNewChildQueue();
    }

    // Public

    get groupedIntoFolders()
    {
        return this._groupedIntoFolders;
    }

    set folderSettingsKey(x)
    {
        this._folderSettingsKey = x;
    }

    registerFolderizeSettings(type, folderDisplayName, validateRepresentedObjectCallback, countChildrenCallback, treeElementConstructor)
    {
        console.assert(type);
        console.assert(folderDisplayName);
        console.assert(typeof validateRepresentedObjectCallback === "function");
        console.assert(typeof countChildrenCallback === "function");
        console.assert(typeof treeElementConstructor === "function");

        var settings = {
            type,
            folderDisplayName,
            validateRepresentedObjectCallback,
            countChildrenCallback,
            treeElementConstructor
        };

        this._folderizeSettingsMap.set(type, settings);
    }

    // Overrides from TreeElement (Private).

    removeChildren()
    {
        super.removeChildren();

        this._clearNewChildQueue();

        for (var folder of this._folderTypeMap.values())
            folder.removeChildren();

        this._folderExpandedSettingMap.clear();
        this._folderTypeMap.clear();

        this._groupedIntoFolders = false;
    }

    // Protected

    addChildForRepresentedObject(representedObject)
    {
        var settings = this._settingsForRepresentedObject(representedObject);
        console.assert(settings);
        if (!settings) {
            console.error("No settings for represented object", representedObject);
            return;
        }

        if (!this.treeOutline) {
            // Just mark as needing to update to avoid doing work that might not be needed.
            this.shouldRefreshChildren = true;
            return;
        }

        var childTreeElement = this.treeOutline.getCachedTreeElement(representedObject);
        if (!childTreeElement)
            childTreeElement = new settings.treeElementConstructor(representedObject);

        this._addTreeElement(childTreeElement);
    }

    addRepresentedObjectToNewChildQueue(representedObject)
    {
        // This queue reduces flashing as resources load and change folders when their type becomes known.

        this._newChildQueue.push(representedObject);
        if (!this._newChildQueueTimeoutIdentifier)
            this._newChildQueueTimeoutIdentifier = setTimeout(this._populateFromNewChildQueue.bind(this), WebInspector.FolderizedTreeElement.NewChildQueueUpdateInterval);
    }

    removeChildForRepresentedObject(representedObject)
    {
        this._removeRepresentedObjectFromNewChildQueue(representedObject);
        this.updateParentStatus();

        if (!this.treeOutline) {
            // Just mark as needing to update to avoid doing work that might not be needed.
            this.shouldRefreshChildren = true;
            return;
        }

        // Find the tree element for the frame by using getCachedTreeElement
        // to only get the item if it has been created already.
        var childTreeElement = this.treeOutline.getCachedTreeElement(representedObject);
        if (!childTreeElement || !childTreeElement.parent)
            return;

        this._removeTreeElement(childTreeElement);
    }

    compareChildTreeElements(a, b)
    {
        return this._compareTreeElementsByMainTitle(a, b);
    }

    updateParentStatus()
    {
        var hasChildren = false;
        for (var settings of this._folderizeSettingsMap.values()) {
            if (settings.countChildrenCallback()) {
                hasChildren = true;
                break;
            }
        }

        this.hasChildren = hasChildren;
        if (!this.hasChildren)
            this.removeChildren();
    }

    prepareToPopulate()
    {
        if (!this._groupedIntoFolders && this._shouldGroupIntoFolders()) {
            this._groupedIntoFolders = true;
            return true;
        }

        return false;
    }

    // Private

    _clearNewChildQueue()
    {
        this._newChildQueue = [];
        if (this._newChildQueueTimeoutIdentifier) {
            clearTimeout(this._newChildQueueTimeoutIdentifier);
            this._newChildQueueTimeoutIdentifier = null;
        }
    }

    _populateFromNewChildQueue()
    {
        if (!this.children.length) {
            this.updateParentStatus();
            this.shouldRefreshChildren = true;
            return;
        }

        if (this.prepareToPopulate()) {
            // Will now folderize, repopulate children.
            this._clearNewChildQueue();
            this.shouldRefreshChildren = true;
            return;
        }

        for (var i = 0; i < this._newChildQueue.length; ++i)
            this.addChildForRepresentedObject(this._newChildQueue[i]);

        this._clearNewChildQueue();
    }

    _removeRepresentedObjectFromNewChildQueue(representedObject)
    {
        this._newChildQueue.remove(representedObject);
    }

    _addTreeElement(childTreeElement)
    {
        console.assert(childTreeElement);
        if (!childTreeElement)
            return;

        var wasSelected = childTreeElement.selected;

        this._removeTreeElement(childTreeElement, true, true);

        var parentTreeElement = this._parentTreeElementForRepresentedObject(childTreeElement.representedObject);
        if (parentTreeElement !== this && !parentTreeElement.parent)
            this._insertFolderTreeElement(parentTreeElement);

        this._insertChildTreeElement(parentTreeElement, childTreeElement);

        if (wasSelected)
            childTreeElement.revealAndSelect(true, false, true, true);
    }

    _compareTreeElementsByMainTitle(a, b)
    {
        return a.mainTitle.localeCompare(b.mainTitle);
    }

    _insertFolderTreeElement(folderTreeElement)
    {
        console.assert(this._groupedIntoFolders);
        console.assert(!folderTreeElement.parent);
        this.insertChild(folderTreeElement, insertionIndexForObjectInListSortedByFunction(folderTreeElement, this.children, this._compareTreeElementsByMainTitle));
    }

    _insertChildTreeElement(parentTreeElement, childTreeElement)
    {
        console.assert(!childTreeElement.parent);
        parentTreeElement.insertChild(childTreeElement, insertionIndexForObjectInListSortedByFunction(childTreeElement, parentTreeElement.children, this.compareChildTreeElements.bind(this)));
    }

    _removeTreeElement(childTreeElement, suppressOnDeselect, suppressSelectSibling)
    {
        var oldParent = childTreeElement.parent;
        if (!oldParent)
            return;

        oldParent.removeChild(childTreeElement, suppressOnDeselect, suppressSelectSibling);

        if (oldParent === this)
            return;

        console.assert(oldParent instanceof WebInspector.FolderTreeElement);
        if (!(oldParent instanceof WebInspector.FolderTreeElement))
            return;

        // Remove the old parent folder if it is now empty.
        if (!oldParent.children.length)
            oldParent.parent.removeChild(oldParent);
    }

    _parentTreeElementForRepresentedObject(representedObject)
    {
        if (!this._groupedIntoFolders)
            return this;

        console.assert(this._folderSettingsKey !== "");

        function createFolderTreeElement(type, displayName)
        {
            var folderTreeElement = new WebInspector.FolderTreeElement(displayName);
            var folderExpandedSetting = new WebInspector.Setting(type + "-folder-expanded-" + this._folderSettingsKey, false);
            this._folderExpandedSettingMap.set(folderTreeElement, folderExpandedSetting);

            if (folderExpandedSetting.value)
                folderTreeElement.expand();

            folderTreeElement.onexpand = this._folderTreeElementExpandedStateChange.bind(this);
            folderTreeElement.oncollapse = this._folderTreeElementExpandedStateChange.bind(this);
            return folderTreeElement;
        }

        var settings = this._settingsForRepresentedObject(representedObject);
        if (!settings) {
            console.error("Unknown representedObject", representedObject);
            return this;
        }

        var folder = this._folderTypeMap.get(settings.type);
        if (folder)
            return folder;

        folder = createFolderTreeElement.call(this, settings.type, settings.folderDisplayName);
        this._folderTypeMap.set(settings.type, folder);
        return folder;
    }

    _folderTreeElementExpandedStateChange(folderTreeElement)
    {
        var expandedSetting = this._folderExpandedSettingMap.get(folderTreeElement);
        console.assert(expandedSetting, "No expanded setting for folderTreeElement", folderTreeElement);
        if (!expandedSetting)
            return;

        expandedSetting.value = folderTreeElement.expanded;
    }

    _settingsForRepresentedObject(representedObject)
    {
        for (var settings of this._folderizeSettingsMap.values()) {
            if (settings.validateRepresentedObjectCallback(representedObject))
                return settings;
        }
        return null;
    }

    _shouldGroupIntoFolders()
    {
        // Already grouped into folders, keep it that way.
        if (this._groupedIntoFolders)
            return true;

        // Child objects are grouped into folders if one of two thresholds are met:
        // 1) Once the number of medium categories passes NumberOfMediumCategoriesThreshold.
        // 2) When there is a category that passes LargeChildCountThreshold and there are
        //    any child objects in another category.

        // Folders are avoided when there is only one category or most categories are small.

        var numberOfSmallCategories = 0;
        var numberOfMediumCategories = 0;
        var foundLargeCategory = false;

        function pushCategory(childCount)
        {
            if (!childCount)
                return false;

            // If this type has any resources and there is a known large category, make folders.
            if (foundLargeCategory)
                return true;

            // If there are lots of this resource type, then count it as a large category.
            if (childCount >= WebInspector.FolderizedTreeElement.LargeChildCountThreshold) {
                // If we already have other resources in other small or medium categories, make folders.
                if (numberOfSmallCategories || numberOfMediumCategories)
                    return true;

                foundLargeCategory = true;
                return false;
            }

            // Check if this is a medium category.
            if (childCount >= WebInspector.FolderizedTreeElement.MediumChildCountThreshold) {
                // If this is the medium category that puts us over the maximum allowed, make folders.
                return ++numberOfMediumCategories >= WebInspector.FolderizedTreeElement.NumberOfMediumCategoriesThreshold;
            }

            // This is a small category.
            ++numberOfSmallCategories;
            return false;
        }

        // Iterate over all the available child object types.
        for (var settings of this._folderizeSettingsMap.values()) {
            if (pushCategory(settings.countChildrenCallback()))
                return true;
        }
        return false;
    }
};

WebInspector.FolderizedTreeElement.MediumChildCountThreshold = 5;
WebInspector.FolderizedTreeElement.LargeChildCountThreshold = 15;
WebInspector.FolderizedTreeElement.NumberOfMediumCategoriesThreshold = 2;
WebInspector.FolderizedTreeElement.NewChildQueueUpdateInterval = 500;
