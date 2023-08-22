/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
 * Copyright (c) 2009,2010,2012 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityController.h"

#include "AccessibilityUIElement.h"
#include "DumpRenderTree.h"

AccessibilityController::AccessibilityController()
{
}

AccessibilityController::~AccessibilityController()
{
}

AccessibilityUIElement AccessibilityController::focusedElement()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS CO.,Ltd.
    return 0;
#else
    AtkObject* accessible =  webkit_web_frame_get_focused_accessible_element(mainFrame);
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
#endif
}

AccessibilityUIElement AccessibilityController::rootElement()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS CO.,Ltd.
    return 0;
#else
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);

    // The presumed, desired rootElement is the parent of the web view.
    GtkWidget* webViewParent = gtk_widget_get_parent(GTK_WIDGET(view));
    AtkObject* axObject = gtk_widget_get_accessible(webViewParent);

    return AccessibilityUIElement(axObject);
#endif
}

void AccessibilityController::setLogFocusEvents(bool)
{
}

void AccessibilityController::setLogScrollingStartEvents(bool)
{
}

void AccessibilityController::setLogValueChangeEvents(bool)
{
}

void AccessibilityController::setLogAccessibilityEvents(bool)
{
}

AccessibilityUIElement AccessibilityController::elementAtPoint(int x, int y)
{
    return 0;
}

bool AccessibilityController::addNotificationListener(JSObjectRef functionCallback)
{
    return false;
}

void AccessibilityController::removeNotificationListener()
{
}
