/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#import "config.h"
#import "DragController.h"

#if ENABLE(DRAG_SUPPORT)

#import "DataTransfer.h"
#import "Document.h"
#import "DocumentFragment.h"
#import "DragClient.h"
#import "DragData.h"
#import "Editor.h"
#import "EditorClient.h"
#import "Element.h"
#import "File.h"
#import "FrameView.h"
#import "HTMLAttachmentElement.h"
#import "MainFrame.h"
#import "Page.h"
#import "Pasteboard.h"
#import "Range.h"

namespace WebCore {

const int DragController::LinkDragBorderInset = -2;

const int DragController::MaxOriginalImageArea = 1500 * 1500;
const int DragController::DragIconRightInset = 7;
const int DragController::DragIconBottomInset = 3;

const float DragController::DragImageAlpha = 0.75f;

bool DragController::isCopyKeyDown(DragData& dragData)
{
    return dragData.flags() & DragApplicationIsCopyKeyDown;
}
    
DragOperation DragController::dragOperation(DragData& dragData)
{
    if ((dragData.flags() & DragApplicationIsModal) || !dragData.containsURL())
        return DragOperationNone;

    if (!m_documentUnderMouse || (!(dragData.flags() & (DragApplicationHasAttachedSheet | DragApplicationIsSource))))
        return DragOperationCopy;

    return DragOperationNone;
}

const IntSize& DragController::maxDragImageSize()
{
    static const IntSize maxDragImageSize(400, 400);
    
    return maxDragImageSize;
}

void DragController::cleanupAfterSystemDrag()
{
    // Drag has ended, dragEnded *should* have been called, however it is possible
    // for the UIDelegate to take over the drag, and fail to send the appropriate
    // drag termination event.  As dragEnded just resets drag variables, we just
    // call it anyway to be on the safe side.
    // We don't want to do this for WebKit2, since the client call to start the drag
    // is asynchronous.
    if (m_page.mainFrame().view()->platformWidget())
        dragEnded();
}

#if ENABLE(ATTACHMENT_ELEMENT)
void DragController::declareAndWriteAttachment(DataTransfer& dataTransfer, Element& element, const URL& url)
{
    const HTMLAttachmentElement& attachment = downcast<HTMLAttachmentElement>(element);
    m_client.declareAndWriteAttachment(dataTransfer.pasteboard().name(), element, url, attachment.file()->path(), element.document().frame());
}
#endif
    
void DragController::declareAndWriteDragImage(DataTransfer& dataTransfer, Element& element, const URL& url, const String& label)
{
    m_client.declareAndWriteDragImage(dataTransfer.pasteboard().name(), element, url, label, element.document().frame());
}

} // namespace WebCore

#endif // ENABLE(DRAG_SUPPORT)
