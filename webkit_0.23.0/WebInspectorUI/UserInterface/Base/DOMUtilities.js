/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2007, 2008, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2009 Joseph Pecoraro
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.displayNameForNode = function(node)
{
    var title = node.nodeNameInCorrectCase();

    var idAttribute = node.getAttribute("id");
    if (idAttribute) {
        if (/[\s'"]/.test(idAttribute)) {
            idAttribute = idAttribute.replace(/\\/g, "\\\\").replace(/\"/g, "\\\"");
            title += "[id=\"" + idAttribute + "\"]";
        } else
            title += "#" + idAttribute;
    }

    var classAttribute = node.getAttribute("class");
    if (classAttribute) {
        var classes = classAttribute.trim().split(/\s+/);
        var foundClasses = {};

        for (var i = 0; i < classes.length; ++i) {
            var className = classes[i];
            if (className && !(className in foundClasses)) {
                title += "." + className;
                foundClasses[className] = true;
            }
        }
    }

    return title;
};

WebInspector.roleSelectorForNode = function(node)
{
    // This is proposed syntax for CSS 4 computed role selector :role(foo) and subject to change.
    // See http://lists.w3.org/Archives/Public/www-style/2013Jul/0104.html
    var title = "";
    var role = node.computedRole();
    if (role)
        title = ":role(" + role + ")";
    return title;
};

WebInspector.linkifyAccessibilityNodeReference = function(node)
{
    if (!node)
        return null;
    // Same as linkifyNodeReference except the link text has the classnames removed...
    // ...for list brevity, and both text and title have roleSelectorForNode appended.
    var link = WebInspector.linkifyNodeReference(node);
    var tagIdSelector = link.title;
    var classSelectorIndex = tagIdSelector.indexOf(".");
    if (classSelectorIndex > -1)
        tagIdSelector = tagIdSelector.substring(0, classSelectorIndex);
    var roleSelector = WebInspector.roleSelectorForNode(node);
    link.textContent = tagIdSelector + roleSelector;
    link.title += roleSelector;
    return link;
};

WebInspector.linkifyNodeReference = function(node)
{
    var displayName = WebInspector.displayNameForNode(node);

    var link = document.createElement("span");
    link.appendChild(document.createTextNode(displayName));
    link.setAttribute("role", "link");
    link.className = "node-link";
    link.title = displayName;

    link.addEventListener("click", WebInspector.domTreeManager.inspectElement.bind(WebInspector.domTreeManager, node.id));
    link.addEventListener("mouseover", WebInspector.domTreeManager.highlightDOMNode.bind(WebInspector.domTreeManager, node.id, ""));
    link.addEventListener("mouseout", WebInspector.domTreeManager.hideDOMNodeHighlight.bind(WebInspector.domTreeManager));

    return link;
};

function createSVGElement(tagName)
{
    return document.createElementNS("http://www.w3.org/2000/svg", tagName);
}
