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

WebInspector.isBeingEdited = function(element)
{
    while (element) {
        if (element.__editing)
            return true;
        element = element.parentNode;
    }

    return false;
};

WebInspector.markBeingEdited = function(element, value)
{
    if (value) {
        if (element.__editing)
            return false;
        element.__editing = true;
        WebInspector.__editingCount = (WebInspector.__editingCount || 0) + 1;
    } else {
        if (!element.__editing)
            return false;
        delete element.__editing;
        --WebInspector.__editingCount;
    }
    return true;
};

WebInspector.isEditingAnyField = function()
{
    return !!WebInspector.__editingCount;
};

WebInspector.isEventTargetAnEditableField = function(event)
{
    var textInputTypes = {"text": true, "search": true, "tel": true, "url": true, "email": true, "password": true};
    if (event.target instanceof HTMLInputElement)
        return event.target.type in textInputTypes;

    var codeMirrorEditorElement = event.target.enclosingNodeOrSelfWithClass("CodeMirror");
    if (codeMirrorEditorElement && codeMirrorEditorElement.CodeMirror)
        return !codeMirrorEditorElement.CodeMirror.getOption("readOnly");

    if (event.target instanceof HTMLTextAreaElement)
        return true;

    if (event.target.enclosingNodeOrSelfWithClass("text-prompt"))
        return true;

    return false;
};

WebInspector.EditingConfig = class EditingConfig
{
    constructor(commitHandler, cancelHandler, context)
    {
        this.commitHandler = commitHandler;
        this.cancelHandler = cancelHandler;
        this.context = context;
        this.spellcheck = false;
    }

    setPasteHandler(pasteHandler)
    {
        this.pasteHandler = pasteHandler;
    }

    setMultiline(multiline)
    {
        this.multiline = multiline;
    }

    setCustomFinishHandler(customFinishHandler)
    {
        this.customFinishHandler = customFinishHandler;
    }

    setNumberCommitHandler(numberCommitHandler)
    {
        this.numberCommitHandler = numberCommitHandler;
    }
};

WebInspector.startEditing = function(element, config)
{
    if (!WebInspector.markBeingEdited(element, true))
        return null;

    config = config || new WebInspector.EditingConfig(function() {}, function() {});
    var committedCallback = config.commitHandler;
    var cancelledCallback = config.cancelHandler;
    var pasteCallback = config.pasteHandler;
    var context = config.context;
    var oldText = getContent(element);
    var moveDirection = "";

    element.classList.add("editing");

    var oldSpellCheck = element.hasAttribute("spellcheck") ? element.spellcheck : undefined;
    element.spellcheck = config.spellcheck;

    if (config.multiline)
        element.classList.add("multiline");

    var oldTabIndex = element.tabIndex;
    if (element.tabIndex < 0)
        element.tabIndex = 0;

    function blurEventListener() {
        editingCommitted.call(element);
    }

    function getContent(element) {
        if (element.tagName === "INPUT" && element.type === "text")
            return element.value;
        else
            return element.textContent;
    }

    function cleanUpAfterEditing()
    {
        WebInspector.markBeingEdited(element, false);

        this.classList.remove("editing");
        this.scrollTop = 0;
        this.scrollLeft = 0;

        if (oldSpellCheck === undefined)
            element.removeAttribute("spellcheck");
        else
            element.spellcheck = oldSpellCheck;

        if (oldTabIndex === -1)
            this.removeAttribute("tabindex");
        else
            this.tabIndex = oldTabIndex;

        element.removeEventListener("blur", blurEventListener, false);
        element.removeEventListener("keydown", keyDownEventListener, true);
        if (pasteCallback)
            element.removeEventListener("paste", pasteEventListener, true);

        WebInspector.restoreFocusFromElement(element);
    }

    function editingCancelled()
    {
        if (this.tagName === "INPUT" && this.type === "text")
            this.value = oldText;
        else
            this.textContent = oldText;

        cleanUpAfterEditing.call(this);

        cancelledCallback(this, context);
    }

    function editingCommitted()
    {
        cleanUpAfterEditing.call(this);

        committedCallback(this, getContent(this), oldText, context, moveDirection);
    }

    function defaultFinishHandler(event)
    {
        var hasOnlyMetaModifierKey = event.metaKey && !event.shiftKey && !event.ctrlKey && !event.altKey;
        if (isEnterKey(event) && (!config.multiline || hasOnlyMetaModifierKey))
            return "commit";
        else if (event.keyCode === WebInspector.KeyboardShortcut.Key.Escape.keyCode || event.keyIdentifier === "U+001B")
            return "cancel";
        else if (event.keyIdentifier === "U+0009") // Tab key
            return "move-" + (event.shiftKey ? "backward" : "forward");
        else if (event.altKey) {
            if (event.keyIdentifier === "Up" || event.keyIdentifier === "Down")
                return "modify-" + (event.keyIdentifier === "Up" ? "up" : "down");
            if (event.keyIdentifier === "PageUp" || event.keyIdentifier === "PageDown")
                return "modify-" + (event.keyIdentifier === "PageUp" ? "up-big" : "down-big");
        }
    }

    function handleEditingResult(result, event)
    {
        if (result === "commit") {
            editingCommitted.call(element);
            event.preventDefault();
            event.stopPropagation();
        } else if (result === "cancel") {
            editingCancelled.call(element);
            event.preventDefault();
            event.stopPropagation();
        } else if (result && result.startsWith("move-")) {
            moveDirection = result.substring(5);
            if (event.keyIdentifier !== "U+0009")
                blurEventListener();
        } else if (result && result.startsWith("modify-")) {
            var direction = result.substring(7);
            var modifyValue = direction.startsWith("up") ? 1 : -1;
            if (direction.endsWith("big"))
                modifyValue *= 10;

            if (event.shiftKey)
                modifyValue *= 10;
            else if (event.ctrlKey)
                modifyValue /= 10;

            var selection = element.ownerDocument.defaultView.getSelection();
            if (!selection.rangeCount)
                return;

            var range = selection.getRangeAt(0);
            if (!range.commonAncestorContainer.isSelfOrDescendant(element))
                return false;

            var wordRange = range.startContainer.rangeOfWord(range.startOffset, WebInspector.EditingSupport.StyleValueDelimiters, element);
            var word = wordRange.toString();
            var wordPrefix = "";
            var wordSuffix = "";
            var nonNumberInWord = /[^\d-\.]+/.exec(word);
            if (nonNumberInWord) {
                var nonNumberEndOffset = nonNumberInWord.index + nonNumberInWord[0].length;
                if (range.startOffset > wordRange.startOffset + nonNumberInWord.index && nonNumberEndOffset < word.length && range.startOffset !== wordRange.startOffset) {
                    wordPrefix = word.substring(0, nonNumberEndOffset);
                    word = word.substring(nonNumberEndOffset);
                } else {
                    wordSuffix = word.substring(nonNumberInWord.index);
                    word = word.substring(0, nonNumberInWord.index);
                }
            }

            var matches = WebInspector.EditingSupport.CSSNumberRegex.exec(word);
            if (!matches || matches.length !== 4)
                return;

            var replacement = matches[1] + (Math.round((parseFloat(matches[2]) + modifyValue) * 100) / 100) + matches[3];

            selection.removeAllRanges();
            selection.addRange(wordRange);
            document.execCommand("insertText", false, wordPrefix + replacement + wordSuffix);

            var container = range.commonAncestorContainer;
            var startOffset = range.startOffset;
            // This check is for the situation when the cursor is in the space between the
            // opening quote of the attribute and the first character. In that spot, the
            // commonAncestorContainer is actually the entire attribute node since `="` is
            // added as a simple text node. Since the opening quote is immediately before
            // the attribute, the node for that attribute must be the next sibling and the
            // text of the attribute's value must be the first child of that sibling.
            if (container.parentNode.classList.contains("editing")) {
                container = container.nextSibling.firstChild;
                startOffset = 0;
            }
            startOffset += wordPrefix.length;

            if (!container)
                return;

            var replacementSelectionRange = document.createRange();
            replacementSelectionRange.setStart(container, startOffset);
            replacementSelectionRange.setEnd(container, startOffset + replacement.length);

            selection.removeAllRanges();
            selection.addRange(replacementSelectionRange);

            if (typeof config.numberCommitHandler === "function")
                config.numberCommitHandler(element, getContent(element), oldText, context, moveDirection);

            event.preventDefault();
        }
    }

    function pasteEventListener(event)
    {
        var result = pasteCallback(event);
        handleEditingResult(result, event);
    }

    function keyDownEventListener(event)
    {
        var handler = config.customFinishHandler || defaultFinishHandler;
        var result = handler(event);
        handleEditingResult(result, event);
    }

    element.addEventListener("blur", blurEventListener, false);
    element.addEventListener("keydown", keyDownEventListener, true);
    if (pasteCallback)
        element.addEventListener("paste", pasteEventListener, true);

    element.focus();

    return {
        cancel: editingCancelled.bind(element),
        commit: editingCommitted.bind(element)
    };
};

WebInspector.EditingSupport = {
    StyleValueDelimiters: " \xA0\t\n\"':;,/()",
    CSSNumberRegex: /(.*?)(-?(?:\d+(?:\.\d+)?|\.\d+))(.*)/,
    NumberRegex: /^(-?(?:\d+(?:\.\d+)?|\.\d+))$/
};
