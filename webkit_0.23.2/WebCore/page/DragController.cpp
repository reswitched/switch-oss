/*
 * Copyright (C) 2007, 2009-2010, 2013, 2015-2016 Apple Inc. All rights reserved.
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
#include "DragController.h"

#include "HTMLAnchorElement.h"
#include "SVGAElement.h"

#if ENABLE(DRAG_SUPPORT)
#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "DataTransfer.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DragActions.h"
#include "DragClient.h"
#include "DragData.h"
#include "DragImage.h"
#include "DragState.h"
#include "Editor.h"
#include "EditorClient.h"
#include "EventHandler.h"
#include "ExceptionCodePlaceholder.h"
#include "FloatRect.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLAttachmentElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "Image.h"
#include "ImageOrientation.h"
#include "MainFrame.h"
#include "MoveSelectionCommand.h"
#include "Page.h"
#include "Pasteboard.h"
#include "PlatformKeyboardEvent.h"
#include "PluginDocument.h"
#include "PluginViewBase.h"
#include "RenderFileUploadControl.h"
#include "RenderImage.h"
#include "RenderView.h"
#include "ReplaceSelectionCommand.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleProperties.h"
#include "Text.h"
#include "TextEvent.h"
#include "htmlediting.h"
#include "markup.h"
#include <wtf/CurrentTime.h>
#include <wtf/RefPtr.h>
#endif

namespace WebCore {

bool isDraggableLink(const Element& element)
{
    if (is<HTMLAnchorElement>(element))
        return downcast<HTMLAnchorElement>(element).isLiveLink();
    if (is<SVGAElement>(element))
        return element.isLink();
    return false;
}

#if ENABLE(DRAG_SUPPORT)
    
static PlatformMouseEvent createMouseEvent(DragData& dragData)
{
    int keyState = dragData.modifierKeyState();
    bool shiftKey = static_cast<bool>(keyState & PlatformEvent::ShiftKey);
    bool ctrlKey = static_cast<bool>(keyState & PlatformEvent::CtrlKey);
    bool altKey = static_cast<bool>(keyState & PlatformEvent::AltKey);
    bool metaKey = static_cast<bool>(keyState & PlatformEvent::MetaKey);

    return PlatformMouseEvent(dragData.clientPosition(), dragData.globalPosition(),
                              LeftButton, PlatformEvent::MouseMoved, 0, shiftKey, ctrlKey, altKey,
                              metaKey, currentTime(), ForceAtClick);
}

DragController::DragController(Page& page, DragClient& client)
    : m_page(page)
    , m_client(client)
    , m_numberOfItemsToBeAccepted(0)
    , m_documentIsHandlingDrag(false)
    , m_dragDestinationAction(DragDestinationActionNone)
    , m_dragSourceAction(DragSourceActionNone)
    , m_didInitiateDrag(false)
    , m_sourceDragOperation(DragOperationNone)
{
}

DragController::~DragController()
{
    m_client.dragControllerDestroyed();
}

static PassRefPtr<DocumentFragment> documentFragmentFromDragData(DragData& dragData, Frame& frame, Range& context, bool allowPlainText, bool& chosePlainText)
{
    chosePlainText = false;

    Document& document = context.ownerDocument();
    if (dragData.containsCompatibleContent()) {
        if (PassRefPtr<DocumentFragment> fragment = frame.editor().webContentFromPasteboard(*Pasteboard::createForDragAndDrop(dragData), context, allowPlainText, chosePlainText))
            return fragment;

        if (dragData.containsURL(DragData::DoNotConvertFilenames)) {
            String title;
            String url = dragData.asURL(DragData::DoNotConvertFilenames, &title);
            if (!url.isEmpty()) {
                Ref<HTMLAnchorElement> anchor = HTMLAnchorElement::create(document);
                anchor->setHref(url);
                if (title.isEmpty()) {
                    // Try the plain text first because the url might be normalized or escaped.
                    if (dragData.containsPlainText())
                        title = dragData.asPlainText();
                    if (title.isEmpty())
                        title = url;
                }
                anchor->appendChild(document.createTextNode(title), IGNORE_EXCEPTION);
                Ref<DocumentFragment> fragment = document.createDocumentFragment();
                fragment->appendChild(WTF::move(anchor), IGNORE_EXCEPTION);
                return fragment.ptr();
            }
        }
    }
    if (allowPlainText && dragData.containsPlainText()) {
        chosePlainText = true;
        return createFragmentFromText(context, dragData.asPlainText()).ptr();
    }

    return nullptr;
}

bool DragController::dragIsMove(FrameSelection& selection, DragData& dragData)
{
    const VisibleSelection& visibleSelection = selection.selection();
    return m_documentUnderMouse == m_dragInitiator && visibleSelection.isContentEditable() && visibleSelection.isRange() && !isCopyKeyDown(dragData);
}

void DragController::clearDragCaret()
{
    m_page.dragCaretController().clear();
}

void DragController::dragEnded()
{
    m_dragInitiator = nullptr;
    m_didInitiateDrag = false;
    clearDragCaret();
    
    m_client.dragEnded();
}

DragOperation DragController::dragEntered(DragData& dragData)
{
    return dragEnteredOrUpdated(dragData);
}

void DragController::dragExited(DragData& dragData)
{
    if (RefPtr<FrameView> v = m_page.mainFrame().view()) {
#if ENABLE(DASHBOARD_SUPPORT)
        DataTransferAccessPolicy policy = (m_page.mainFrame().settings().usesDashboardBackwardCompatibilityMode() && (!m_documentUnderMouse || m_documentUnderMouse->securityOrigin().isLocal()))
            ? DataTransferAccessPolicy::Readable : DataTransferAccessPolicy::TypesReadable;
#else
        DataTransferAccessPolicy policy = DataTransferAccessPolicy::TypesReadable;
#endif
        RefPtr<DataTransfer> dataTransfer = DataTransfer::createForDragAndDrop(policy, dragData);
        dataTransfer->setSourceOperation(dragData.draggingSourceOperationMask());
        m_page.mainFrame().eventHandler().cancelDragAndDrop(createMouseEvent(dragData), dataTransfer.get());
        dataTransfer->setAccessPolicy(DataTransferAccessPolicy::Numb); // Invalidate dataTransfer here for security.
    }
    mouseMovedIntoDocument(nullptr);
    if (m_fileInputElementUnderMouse)
        m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
    m_fileInputElementUnderMouse = nullptr;
}

DragOperation DragController::dragUpdated(DragData& dragData)
{
    return dragEnteredOrUpdated(dragData);
}

bool DragController::performDragOperation(DragData& dragData)
{
    m_documentUnderMouse = m_page.mainFrame().documentAtPoint(dragData.clientPosition());

    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicy = ShouldOpenExternalURLsPolicy::ShouldNotAllow;
    if (m_documentUnderMouse)
        shouldOpenExternalURLsPolicy = m_documentUnderMouse->shouldOpenExternalURLsPolicyToPropagate();

    if ((m_dragDestinationAction & DragDestinationActionDHTML) && m_documentIsHandlingDrag) {
        m_client.willPerformDragDestinationAction(DragDestinationActionDHTML, dragData);
        Ref<MainFrame> mainFrame(m_page.mainFrame());
        bool preventedDefault = false;
        if (mainFrame->view()) {
            // Sending an event can result in the destruction of the view and part.
            RefPtr<DataTransfer> dataTransfer = DataTransfer::createForDragAndDrop(DataTransferAccessPolicy::Readable, dragData);
            dataTransfer->setSourceOperation(dragData.draggingSourceOperationMask());
            preventedDefault = mainFrame->eventHandler().performDragAndDrop(createMouseEvent(dragData), dataTransfer.get());
            dataTransfer->setAccessPolicy(DataTransferAccessPolicy::Numb); // Invalidate dataTransfer here for security.
        }
        if (preventedDefault) {
            clearDragCaret();
            m_documentUnderMouse = nullptr;
            return true;
        }
    }

    if ((m_dragDestinationAction & DragDestinationActionEdit) && concludeEditDrag(dragData)) {
        m_documentUnderMouse = nullptr;
        return true;
    }

    m_documentUnderMouse = nullptr;

    if (operationForLoad(dragData) == DragOperationNone)
        return false;

    m_client.willPerformDragDestinationAction(DragDestinationActionLoad, dragData);
    m_page.mainFrame().loader().load(FrameLoadRequest(&m_page.mainFrame(), ResourceRequest(dragData.asURL()), shouldOpenExternalURLsPolicy));
    return true;
}

void DragController::mouseMovedIntoDocument(Document* newDocument)
{
    if (m_documentUnderMouse == newDocument)
        return;

    // If we were over another document clear the selection
    if (m_documentUnderMouse)
        clearDragCaret();
    m_documentUnderMouse = newDocument;
}

DragOperation DragController::dragEnteredOrUpdated(DragData& dragData)
{
    mouseMovedIntoDocument(m_page.mainFrame().documentAtPoint(dragData.clientPosition()));

    m_dragDestinationAction = m_client.actionMaskForDrag(dragData);
    if (m_dragDestinationAction == DragDestinationActionNone) {
        clearDragCaret(); // FIXME: Why not call mouseMovedIntoDocument(nullptr)?
        return DragOperationNone;
    }

    DragOperation dragOperation = DragOperationNone;
    m_documentIsHandlingDrag = tryDocumentDrag(dragData, m_dragDestinationAction, dragOperation);
    if (!m_documentIsHandlingDrag && (m_dragDestinationAction & DragDestinationActionLoad))
        dragOperation = operationForLoad(dragData);
    return dragOperation;
}

static HTMLInputElement* asFileInput(Node* node)
{
    ASSERT(node);

    HTMLInputElement* inputElement = node->toInputElement();

    // If this is a button inside of the a file input, move up to the file input.
    if (inputElement && inputElement->isTextButton() && is<ShadowRoot>(inputElement->treeScope().rootNode()))
        inputElement = downcast<ShadowRoot>(inputElement->treeScope().rootNode()).hostElement()->toInputElement();

    return inputElement && inputElement->isFileUpload() ? inputElement : 0;
}

// This can return null if an empty document is loaded.
static Element* elementUnderMouse(Document* documentUnderMouse, const IntPoint& p)
{
    Frame* frame = documentUnderMouse->frame();
    float zoomFactor = frame ? frame->pageZoomFactor() : 1;
    LayoutPoint point(p.x() * zoomFactor, p.y() * zoomFactor);

    HitTestResult result(point);
    documentUnderMouse->renderView()->hitTest(HitTestRequest(), result);

    Node* node = result.innerNode();
    while (node && !is<Element>(*node))
        node = node->parentNode();
    if (node)
        node = node->deprecatedShadowAncestorNode();

    return downcast<Element>(node);
}

bool DragController::tryDocumentDrag(DragData& dragData, DragDestinationAction actionMask, DragOperation& dragOperation)
{
    if (!m_documentUnderMouse)
        return false;

    if (m_dragInitiator && !m_documentUnderMouse->securityOrigin().canReceiveDragData(m_dragInitiator->securityOrigin()))
        return false;

    bool isHandlingDrag = false;
    if (actionMask & DragDestinationActionDHTML) {
        isHandlingDrag = tryDHTMLDrag(dragData, dragOperation);
        // Do not continue if m_documentUnderMouse has been reset by tryDHTMLDrag.
        // tryDHTMLDrag fires dragenter event. The event listener that listens
        // to this event may create a nested message loop (open a modal dialog),
        // which could process dragleave event and reset m_documentUnderMouse in
        // dragExited.
        if (!m_documentUnderMouse)
            return false;
    }

    // It's unclear why this check is after tryDHTMLDrag.
    // We send drag events in tryDHTMLDrag and that may be the reason.
    RefPtr<FrameView> frameView = m_documentUnderMouse->view();
    if (!frameView)
        return false;

    if (isHandlingDrag) {
        clearDragCaret();
        return true;
    }

    if ((actionMask & DragDestinationActionEdit) && canProcessDrag(dragData)) {
        if (dragData.containsColor()) {
            dragOperation = DragOperationGeneric;
            return true;
        }

        IntPoint point = frameView->windowToContents(dragData.clientPosition());
        Element* element = elementUnderMouse(m_documentUnderMouse.get(), point);
        if (!element)
            return false;
        
        HTMLInputElement* elementAsFileInput = asFileInput(element);
        if (m_fileInputElementUnderMouse != elementAsFileInput) {
            if (m_fileInputElementUnderMouse)
                m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
            m_fileInputElementUnderMouse = elementAsFileInput;
        }
        
        if (!m_fileInputElementUnderMouse)
            m_page.dragCaretController().setCaretPosition(m_documentUnderMouse->frame()->visiblePositionForPoint(point));
        else
            clearDragCaret();

        Frame* innerFrame = element->document().frame();
        dragOperation = dragIsMove(innerFrame->selection(), dragData) ? DragOperationMove : DragOperationCopy;
        m_numberOfItemsToBeAccepted = 0;

        unsigned numberOfFiles = dragData.numberOfFiles();
        if (m_fileInputElementUnderMouse) {
            if (m_fileInputElementUnderMouse->isDisabledFormControl())
                m_numberOfItemsToBeAccepted = 0;
            else if (m_fileInputElementUnderMouse->multiple())
                m_numberOfItemsToBeAccepted = numberOfFiles;
            else if (numberOfFiles > 1)
                m_numberOfItemsToBeAccepted = 0;
            else
                m_numberOfItemsToBeAccepted = 1;
            
            if (!m_numberOfItemsToBeAccepted)
                dragOperation = DragOperationNone;
            m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(m_numberOfItemsToBeAccepted);
        } else {
            // We are not over a file input element. The dragged item(s) will only
            // be loaded into the view the number of dragged items is 1.
            m_numberOfItemsToBeAccepted = numberOfFiles != 1 ? 0 : 1;
        }
        
        return true;
    }
    
    // We are not over an editable region. Make sure we're clearing any prior drag cursor.
    clearDragCaret();
    if (m_fileInputElementUnderMouse)
        m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
    m_fileInputElementUnderMouse = nullptr;
    return false;
}

DragSourceAction DragController::delegateDragSourceAction(const IntPoint& rootViewPoint)
{
    m_dragSourceAction = m_client.dragSourceActionMaskForPoint(rootViewPoint);
    return m_dragSourceAction;
}

DragOperation DragController::operationForLoad(DragData& dragData)
{
    Document* document = m_page.mainFrame().documentAtPoint(dragData.clientPosition());

    bool pluginDocumentAcceptsDrags = false;

    if (is<PluginDocument>(document)) {
        const Widget* widget = downcast<PluginDocument>(*document).pluginWidget();
        const PluginViewBase* pluginView = is<PluginViewBase>(widget) ? downcast<PluginViewBase>(widget) : nullptr;

        if (pluginView)
            pluginDocumentAcceptsDrags = pluginView->shouldAllowNavigationFromDrags();
    }

    if (document && (m_didInitiateDrag || (is<PluginDocument>(*document) && !pluginDocumentAcceptsDrags) || document->hasEditableStyle()))
        return DragOperationNone;
    return dragOperation(dragData);
}

static bool setSelectionToDragCaret(Frame* frame, VisibleSelection& dragCaret, RefPtr<Range>& range, const IntPoint& point)
{
    Ref<Frame> protector(*frame);
    frame->selection().setSelection(dragCaret);
    if (frame->selection().selection().isNone()) {
        dragCaret = frame->visiblePositionForPoint(point);
        frame->selection().setSelection(dragCaret);
        range = dragCaret.toNormalizedRange();
    }
    return !frame->selection().isNone() && frame->selection().selection().isContentEditable();
}

bool DragController::dispatchTextInputEventFor(Frame* innerFrame, DragData& dragData)
{
    ASSERT(m_page.dragCaretController().hasCaret());
    String text = m_page.dragCaretController().isContentRichlyEditable() ? emptyString() : dragData.asPlainText();
    Node* target = innerFrame->editor().findEventTargetFrom(m_page.dragCaretController().caretPosition());
    return target->dispatchEvent(TextEvent::createForDrop(innerFrame->document()->domWindow(), text), IGNORE_EXCEPTION);
}

bool DragController::concludeEditDrag(DragData& dragData)
{
    RefPtr<HTMLInputElement> fileInput = m_fileInputElementUnderMouse;
    if (m_fileInputElementUnderMouse) {
        m_fileInputElementUnderMouse->setCanReceiveDroppedFiles(false);
        m_fileInputElementUnderMouse = nullptr;
    }

    if (!m_documentUnderMouse)
        return false;

    IntPoint point = m_documentUnderMouse->view()->windowToContents(dragData.clientPosition());
    Element* element = elementUnderMouse(m_documentUnderMouse.get(), point);
    if (!element)
        return false;
    RefPtr<Frame> innerFrame = element->document().frame();
    ASSERT(innerFrame);

    if (m_page.dragCaretController().hasCaret() && !dispatchTextInputEventFor(innerFrame.get(), dragData))
        return true;

    if (dragData.containsColor()) {
        Color color = dragData.asColor();
        if (!color.isValid())
            return false;
        RefPtr<Range> innerRange = innerFrame->selection().toNormalizedRange();
        if (!innerRange)
            return false;
        RefPtr<MutableStyleProperties> style = MutableStyleProperties::create();
        style->setProperty(CSSPropertyColor, color.serialized(), false);
        if (!innerFrame->editor().shouldApplyStyle(style.get(), innerRange.get()))
            return false;
        m_client.willPerformDragDestinationAction(DragDestinationActionEdit, dragData);
        innerFrame->editor().applyStyle(style.get(), EditActionSetColor);
        return true;
    }

    if (dragData.containsFiles() && fileInput) {
        // fileInput should be the element we hit tested for, unless it was made
        // display:none in a drop event handler.
        ASSERT(fileInput == element || !fileInput->renderer());
        if (fileInput->isDisabledFormControl())
            return false;

        return fileInput->receiveDroppedFiles(dragData);
    }

    if (!m_page.dragController().canProcessDrag(dragData)) {
        clearDragCaret();
        return false;
    }

    VisibleSelection dragCaret = m_page.dragCaretController().caretPosition();
    clearDragCaret();
    RefPtr<Range> range = dragCaret.toNormalizedRange();
    RefPtr<Element> rootEditableElement = innerFrame->selection().selection().rootEditableElement();

    // For range to be null a WebKit client must have done something bad while
    // manually controlling drag behaviour
    if (!range)
        return false;

    ResourceCacheValidationSuppressor validationSuppressor(range->ownerDocument().cachedResourceLoader());
    if (dragIsMove(innerFrame->selection(), dragData) || dragCaret.isContentRichlyEditable()) {
        bool chosePlainText = false;
        RefPtr<DocumentFragment> fragment = documentFragmentFromDragData(dragData, *innerFrame, *range, true, chosePlainText);
        if (!fragment || !innerFrame->editor().shouldInsertFragment(fragment, range, EditorInsertActionDropped)) {
            return false;
        }

        m_client.willPerformDragDestinationAction(DragDestinationActionEdit, dragData);
        if (dragIsMove(innerFrame->selection(), dragData)) {
            // NSTextView behavior is to always smart delete on moving a selection,
            // but only to smart insert if the selection granularity is word granularity.
            bool smartDelete = innerFrame->editor().smartInsertDeleteEnabled();
            bool smartInsert = smartDelete && innerFrame->selection().granularity() == WordGranularity && dragData.canSmartReplace();
            applyCommand(MoveSelectionCommand::create(fragment, dragCaret.base(), smartInsert, smartDelete));
        } else {
            if (setSelectionToDragCaret(innerFrame.get(), dragCaret, range, point)) {
                ReplaceSelectionCommand::CommandOptions options = ReplaceSelectionCommand::SelectReplacement | ReplaceSelectionCommand::PreventNesting;
                if (dragData.canSmartReplace())
                    options |= ReplaceSelectionCommand::SmartReplace;
                if (chosePlainText)
                    options |= ReplaceSelectionCommand::MatchStyle;
                applyCommand(ReplaceSelectionCommand::create(*m_documentUnderMouse, WTF::move(fragment), options));
            }
        }
    } else {
        String text = dragData.asPlainText();
        if (text.isEmpty() || !innerFrame->editor().shouldInsertText(text, range.get(), EditorInsertActionDropped)) {
            return false;
        }

        m_client.willPerformDragDestinationAction(DragDestinationActionEdit, dragData);
        if (setSelectionToDragCaret(innerFrame.get(), dragCaret, range, point))
            applyCommand(ReplaceSelectionCommand::create(*m_documentUnderMouse, createFragmentFromText(*range, text),  ReplaceSelectionCommand::SelectReplacement | ReplaceSelectionCommand::MatchStyle | ReplaceSelectionCommand::PreventNesting));
    }

    if (rootEditableElement) {
        if (Frame* frame = rootEditableElement->document().frame())
            frame->eventHandler().updateDragStateAfterEditDragIfNeeded(rootEditableElement.get());
    }

    return true;
}

bool DragController::canProcessDrag(DragData& dragData)
{
    if (!dragData.containsCompatibleContent())
        return false;

    IntPoint point = m_page.mainFrame().view()->windowToContents(dragData.clientPosition());
    HitTestResult result = HitTestResult(point);
    if (!m_page.mainFrame().contentRenderer())
        return false;

    result = m_page.mainFrame().eventHandler().hitTestResultAtPoint(point, HitTestRequest::ReadOnly | HitTestRequest::Active);

    if (!result.innerNonSharedNode())
        return false;

    if (dragData.containsFiles() && asFileInput(result.innerNonSharedNode()))
        return true;

    if (is<HTMLPlugInElement>(*result.innerNonSharedNode())) {
        if (!downcast<HTMLPlugInElement>(result.innerNonSharedNode())->canProcessDrag() && !result.innerNonSharedNode()->hasEditableStyle())
            return false;
    } else if (!result.innerNonSharedNode()->hasEditableStyle())
        return false;

    if (m_didInitiateDrag && m_documentUnderMouse == m_dragInitiator && result.isSelected())
        return false;

    return true;
}

static DragOperation defaultOperationForDrag(DragOperation srcOpMask)
{
    // This is designed to match IE's operation fallback for the case where
    // the page calls preventDefault() in a drag event but doesn't set dropEffect.
    if (srcOpMask == DragOperationEvery)
        return DragOperationCopy;
    if (srcOpMask == DragOperationNone)
        return DragOperationNone;
    if (srcOpMask & DragOperationMove || srcOpMask & DragOperationGeneric)
        return DragOperationMove;
    if (srcOpMask & DragOperationCopy)
        return DragOperationCopy;
    if (srcOpMask & DragOperationLink)
        return DragOperationLink;
    
    // FIXME: Does IE really return "generic" even if no operations were allowed by the source?
    return DragOperationGeneric;
}

bool DragController::tryDHTMLDrag(DragData& dragData, DragOperation& operation)
{
    ASSERT(m_documentUnderMouse);
    Ref<MainFrame> mainFrame(m_page.mainFrame());
    RefPtr<FrameView> viewProtector = mainFrame->view();
    if (!viewProtector)
        return false;

#if ENABLE(DASHBOARD_SUPPORT)
    DataTransferAccessPolicy policy = (mainFrame->settings().usesDashboardBackwardCompatibilityMode() && m_documentUnderMouse->securityOrigin().isLocal()) ?
        DataTransferAccessPolicy::Readable : DataTransferAccessPolicy::TypesReadable;
#else
    DataTransferAccessPolicy policy = DataTransferAccessPolicy::TypesReadable;
#endif
    RefPtr<DataTransfer> dataTransfer = DataTransfer::createForDragAndDrop(policy, dragData);
    DragOperation srcOpMask = dragData.draggingSourceOperationMask();
    dataTransfer->setSourceOperation(srcOpMask);

    PlatformMouseEvent event = createMouseEvent(dragData);
    if (!mainFrame->eventHandler().updateDragAndDrop(event, dataTransfer.get())) {
        dataTransfer->setAccessPolicy(DataTransferAccessPolicy::Numb); // Invalidate dataTransfer here for security.
        return false;
    }

    operation = dataTransfer->destinationOperation();
    if (dataTransfer->dropEffectIsUninitialized())
        operation = defaultOperationForDrag(srcOpMask);
    else if (!(srcOpMask & operation)) {
        // The element picked an operation which is not supported by the source
        operation = DragOperationNone;
    }

    dataTransfer->setAccessPolicy(DataTransferAccessPolicy::Numb); // Invalidate dataTransfer here for security.
    return true;
}

Element* DragController::draggableElement(const Frame* sourceFrame, Element* startElement, const IntPoint& dragOrigin, DragState& state) const
{
    state.type = (sourceFrame->selection().contains(dragOrigin)) ? DragSourceActionSelection : DragSourceActionNone;
    if (!startElement)
        return nullptr;
#if ENABLE(ATTACHMENT_ELEMENT)
    // Unlike image elements, attachment elements are immediately selected upon mouse down,
    // but for those elements we still want to use the single element drag behavior as long as
    // the element is the only content of the selection.
    const VisibleSelection& selection = sourceFrame->selection().selection();
    if (selection.isRange() && is<HTMLAttachmentElement>(selection.start().anchorNode()) && selection.start().anchorNode() == selection.end().anchorNode())
        state.type = DragSourceActionNone;
#endif

    for (auto renderer = startElement->renderer(); renderer; renderer = renderer->parent()) {
        Element* element = renderer->nonPseudoElement();
        if (!element) {
            // Anonymous render blocks don't correspond to actual DOM elements, so we skip over them
            // for the purposes of finding a draggable element.
            continue;
        }
        EUserDrag dragMode = renderer->style().userDrag();
        if ((m_dragSourceAction & DragSourceActionDHTML) && dragMode == DRAG_ELEMENT) {
            state.type = static_cast<DragSourceAction>(state.type | DragSourceActionDHTML);
            return element;
        }
        if (dragMode == DRAG_AUTO) {
            if ((m_dragSourceAction & DragSourceActionImage)
                && is<HTMLImageElement>(*element)
                && sourceFrame->settings().loadsImagesAutomatically()) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionImage);
                return element;
            }
            if ((m_dragSourceAction & DragSourceActionLink) && isDraggableLink(*element)) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionLink);
                return element;
            }
#if ENABLE(ATTACHMENT_ELEMENT)
            if ((m_dragSourceAction & DragSourceActionAttachment)
                && is<HTMLAttachmentElement>(*element)
                && downcast<HTMLAttachmentElement>(*element).file()) {
                state.type = static_cast<DragSourceAction>(state.type | DragSourceActionAttachment);
                return element;
            }
#endif
        }
    }

    // We either have nothing to drag or we have a selection and we're not over a draggable element.
    return (state.type & DragSourceActionSelection) ? startElement : nullptr;
}

static CachedImage* getCachedImage(Element& element)
{
    RenderObject* renderer = element.renderer();
    if (!is<RenderImage>(renderer))
        return nullptr;
    auto& image = downcast<RenderImage>(*renderer);
    return image.cachedImage();
}

static Image* getImage(Element& element)
{
    CachedImage* cachedImage = getCachedImage(element);
    // Don't use cachedImage->imageForRenderer() here as that may return BitmapImages for cached SVG Images.
    // Users of getImage() want access to the SVGImage, in order to figure out the filename extensions,
    // which would be empty when asking the cached BitmapImages.
    return (cachedImage && !cachedImage->errorOccurred()) ?
        cachedImage->image() : nullptr;
}

static void selectElement(Element& element)
{
    RefPtr<Range> range = element.document().createRange();
    range->selectNode(&element);
    element.document().frame()->selection().setSelection(VisibleSelection(*range, DOWNSTREAM));
}

static IntPoint dragLocForDHTMLDrag(const IntPoint& mouseDraggedPoint, const IntPoint& dragOrigin, const IntPoint& dragImageOffset, bool isLinkImage)
{
    // dragImageOffset is the cursor position relative to the lower-left corner of the image.
#if PLATFORM(COCOA)
    // We add in the Y dimension because we are a flipped view, so adding moves the image down.
    const int yOffset = dragImageOffset.y();
#else
    const int yOffset = -dragImageOffset.y();
#endif

    if (isLinkImage)
        return IntPoint(mouseDraggedPoint.x() - dragImageOffset.x(), mouseDraggedPoint.y() + yOffset);

    return IntPoint(dragOrigin.x() - dragImageOffset.x(), dragOrigin.y() + yOffset);
}

static IntPoint dragLocForSelectionDrag(Frame& src)
{
    IntRect draggingRect = enclosingIntRect(src.selection().selectionBounds());
    int xpos = draggingRect.maxX();
    xpos = draggingRect.x() < xpos ? draggingRect.x() : xpos;
    int ypos = draggingRect.maxY();
#if PLATFORM(COCOA)
    // Deal with flipped coordinates on Mac
    ypos = draggingRect.y() > ypos ? draggingRect.y() : ypos;
#else
    ypos = draggingRect.y() < ypos ? draggingRect.y() : ypos;
#endif
    return IntPoint(xpos, ypos);
}

bool DragController::startDrag(Frame& src, const DragState& state, DragOperation srcOp, const PlatformMouseEvent& dragEvent, const IntPoint& dragOrigin)
{
    if (!src.view() || !src.contentRenderer() || !state.source)
        return false;

    Ref<Frame> protector(src);
    HitTestResult hitTestResult = src.eventHandler().hitTestResultAtPoint(dragOrigin, HitTestRequest::ReadOnly | HitTestRequest::Active);

    // FIXME(136836): Investigate whether all elements should use the containsIncludingShadowDOM() path here.
    bool includeShadowDOM = false;
#if ENABLE(VIDEO)
    includeShadowDOM = state.source->isMediaElement();
#endif
    bool sourceContainsHitNode;
    if (!includeShadowDOM)
        sourceContainsHitNode = state.source->contains(hitTestResult.innerNode());
    else
        sourceContainsHitNode = state.source->containsIncludingShadowDOM(hitTestResult.innerNode());

    if (!sourceContainsHitNode)
        // The original node being dragged isn't under the drag origin anymore... maybe it was
        // hidden or moved out from under the cursor. Regardless, we don't want to start a drag on
        // something that's not actually under the drag origin.
        return false;
    URL linkURL = hitTestResult.absoluteLinkURL();
    URL imageURL = hitTestResult.absoluteImageURL();
#if ENABLE(ATTACHMENT_ELEMENT)
    URL attachmentURL = hitTestResult.absoluteAttachmentURL();
    m_draggingAttachmentURL = URL();
#endif

    IntPoint mouseDraggedPoint = src.view()->windowToContents(dragEvent.position());

    m_draggingImageURL = URL();
    m_sourceDragOperation = srcOp;

    DragImageRef dragImage = nullptr;
    IntPoint dragLoc(0, 0);
    IntPoint dragImageOffset(0, 0);

    ASSERT(state.dataTransfer);

    DataTransfer& dataTransfer = *state.dataTransfer;
    if (state.type == DragSourceActionDHTML)
        dragImage = dataTransfer.createDragImage(dragImageOffset);
    if (state.type == DragSourceActionSelection || !imageURL.isEmpty() || !linkURL.isEmpty())
        // Selection, image, and link drags receive a default set of allowed drag operations that
        // follows from:
        // http://trac.webkit.org/browser/trunk/WebKit/mac/WebView/WebHTMLView.mm?rev=48526#L3430
        m_sourceDragOperation = static_cast<DragOperation>(m_sourceDragOperation | DragOperationGeneric | DragOperationCopy);

    // We allow DHTML/JS to set the drag image, even if its a link, image or text we're dragging.
    // This is in the spirit of the IE API, which allows overriding of pasteboard data and DragOp.
    if (dragImage) {
        dragLoc = dragLocForDHTMLDrag(mouseDraggedPoint, dragOrigin, dragImageOffset, !linkURL.isEmpty());
        m_dragOffset = dragImageOffset;
    }

    bool startedDrag = true; // optimism - we almost always manage to start the drag

    ASSERT(state.source);
    Element& element = *state.source;

    Image* image = getImage(element);
    if (state.type == DragSourceActionSelection) {
        if (!dataTransfer.pasteboard().hasData()) {
            // FIXME: This entire block is almost identical to the code in Editor::copy, and the code should be shared.

            RefPtr<Range> selectionRange = src.selection().toNormalizedRange();
            ASSERT(selectionRange);

            src.editor().willWriteSelectionToPasteboard(selectionRange.get());

            if (enclosingTextFormControl(src.selection().selection().start()))
                dataTransfer.pasteboard().writePlainText(src.editor().selectedTextForDataTransfer(), Pasteboard::CannotSmartReplace);
            else {
#if PLATFORM(COCOA) || PLATFORM(EFL) || PLATFORM(GTK) || PLATFORM(WKC)
                src.editor().writeSelectionToPasteboard(dataTransfer.pasteboard());
#else
                // FIXME: Convert all other platforms to match Mac and delete this.
                dataTransfer.pasteboard().writeSelection(*selectionRange, src.editor().canSmartCopyOrDelete(), src, IncludeImageAltTextForDataTransfer);
#endif
            }

            src.editor().didWriteSelectionToPasteboard();
        }
        m_client.willPerformDragSourceAction(DragSourceActionSelection, dragOrigin, dataTransfer);
        if (!dragImage) {
            dragImage = dissolveDragImageToFraction(createDragImageForSelection(src), DragImageAlpha);
            dragLoc = dragLocForSelectionDrag(src);
            m_dragOffset = IntPoint(dragOrigin.x() - dragLoc.x(), dragOrigin.y() - dragLoc.y());
        }
        doSystemDrag(dragImage, dragLoc, dragOrigin, dataTransfer, src, false);
    } else if (!src.document()->securityOrigin().canDisplay(linkURL)) {
        src.document()->addConsoleMessage(MessageSource::Security, MessageLevel::Error, "Not allowed to drag local resource: " + linkURL.stringCenterEllipsizedToLength());
        startedDrag = false;
    } else if (!imageURL.isEmpty() && image && !image->isNull() && (m_dragSourceAction & DragSourceActionImage)) {
        // We shouldn't be starting a drag for an image that can't provide an extension.
        // This is an early detection for problems encountered later upon drop.
        ASSERT(!image->filenameExtension().isEmpty());
        if (!dataTransfer.pasteboard().hasData()) {
            m_draggingImageURL = imageURL;
            if (element.isContentRichlyEditable())
                selectElement(element);
            declareAndWriteDragImage(dataTransfer, element, !linkURL.isEmpty() ? linkURL : imageURL, hitTestResult.altDisplayString());
        }

        m_client.willPerformDragSourceAction(DragSourceActionImage, dragOrigin, dataTransfer);

        if (!dragImage) {
            IntRect imageRect = hitTestResult.imageRect();
            imageRect.setLocation(m_page.mainFrame().view()->rootViewToContents(src.view()->contentsToRootView(imageRect.location())));
            doImageDrag(element, dragOrigin, hitTestResult.imageRect(), dataTransfer, src, m_dragOffset);
        } else {
            // DHTML defined drag image
            doSystemDrag(dragImage, dragLoc, dragOrigin, dataTransfer, src, false);
        }
    } else if (!linkURL.isEmpty() && (m_dragSourceAction & DragSourceActionLink)) {
        if (!dataTransfer.pasteboard().hasData()) {
            // Simplify whitespace so the title put on the dataTransfer resembles what the user sees
            // on the web page. This includes replacing newlines with spaces.
            src.editor().copyURL(linkURL, hitTestResult.textContent().simplifyWhiteSpace(), dataTransfer.pasteboard());
        } else {
            // Make sure the pasteboard also contains trustworthy link data
            // but don't overwrite more general pasteboard types.
            PasteboardURL pasteboardURL;
            pasteboardURL.url = linkURL;
            pasteboardURL.title = hitTestResult.textContent();
            dataTransfer.pasteboard().writeTrustworthyWebURLsPboardType(pasteboardURL);
        }

        const VisibleSelection& sourceSelection = src.selection().selection();
        if (sourceSelection.isCaret() && sourceSelection.isContentEditable()) {
            // a user can initiate a drag on a link without having any text
            // selected.  In this case, we should expand the selection to
            // the enclosing anchor element
            Position pos = sourceSelection.base();
            Node* node = enclosingAnchorElement(pos);
            if (node)
                src.selection().setSelection(VisibleSelection::selectionFromContentsOfNode(node));
        }

        m_client.willPerformDragSourceAction(DragSourceActionLink, dragOrigin, dataTransfer);
        if (!dragImage) {
            dragImage = createDragImageForLink(linkURL, hitTestResult.textContent(), src.settings().fontRenderingMode());
            IntSize size = dragImageSize(dragImage);
            m_dragOffset = IntPoint(-size.width() / 2, -LinkDragBorderInset);
            dragLoc = IntPoint(mouseDraggedPoint.x() + m_dragOffset.x(), mouseDraggedPoint.y() + m_dragOffset.y());
            // Later code expects the drag image to be scaled by device's scale factor.
            dragImage = scaleDragImage(dragImage, FloatSize(m_page.deviceScaleFactor(), m_page.deviceScaleFactor()));
        }
        doSystemDrag(dragImage, dragLoc, mouseDraggedPoint, dataTransfer, src, true);
#if ENABLE(ATTACHMENT_ELEMENT)
    } else if (!attachmentURL.isEmpty() && (m_dragSourceAction & DragSourceActionAttachment)) {
        if (!dataTransfer.pasteboard().hasData()) {
            m_draggingAttachmentURL = attachmentURL;
            selectElement(element);
            declareAndWriteAttachment(dataTransfer, element, attachmentURL);
        }
        
        m_client.willPerformDragSourceAction(DragSourceActionAttachment, dragOrigin, dataTransfer);
        
        if (!dragImage) {
            dragImage = dissolveDragImageToFraction(createDragImageForSelection(src), DragImageAlpha);
            dragLoc = dragLocForSelectionDrag(src);
            m_dragOffset = IntPoint(dragOrigin.x() - dragLoc.x(), dragOrigin.y() - dragLoc.y());
        }
        doSystemDrag(dragImage, dragLoc, dragOrigin, dataTransfer, src, false);
#endif
    } else if (state.type == DragSourceActionDHTML) {
        if (dragImage) {
            ASSERT(m_dragSourceAction & DragSourceActionDHTML);
            m_client.willPerformDragSourceAction(DragSourceActionDHTML, dragOrigin, dataTransfer);
            doSystemDrag(dragImage, dragLoc, dragOrigin, dataTransfer, src, false);
        } else
            startedDrag = false;
    } else {
        // draggableElement() determined an image or link node was draggable, but it turns out the
        // image or link had no URL, so there is nothing to drag.
        startedDrag = false;
    }

    if (dragImage)
        deleteDragImage(dragImage);
    return startedDrag;
}

void DragController::doImageDrag(Element& element, const IntPoint& dragOrigin, const IntRect& layoutRect, DataTransfer& dataTransfer, Frame& frame, IntPoint& dragImageOffset)
{
    IntPoint mouseDownPoint = dragOrigin;
    DragImageRef dragImage = nullptr;
    IntPoint scaledOrigin;

    if (!element.renderer())
        return;

    ImageOrientationDescription orientationDescription(element.renderer()->shouldRespectImageOrientation());
#if ENABLE(CSS_IMAGE_ORIENTATION)
    orientationDescription.setImageOrientationEnum(element.renderer()->style().imageOrientation());
#endif

    Image* image = getImage(element);
    if (image && image->size().height() * image->size().width() <= MaxOriginalImageArea
        && (dragImage = createDragImageFromImage(image, element.renderer() ? orientationDescription : ImageOrientationDescription()))) {

        dragImage = fitDragImageToMaxSize(dragImage, layoutRect.size(), maxDragImageSize());
        IntSize fittedSize = dragImageSize(dragImage);

        dragImage = scaleDragImage(dragImage, FloatSize(m_page.deviceScaleFactor(), m_page.deviceScaleFactor()));
        dragImage = dissolveDragImageToFraction(dragImage, DragImageAlpha);

        // Properly orient the drag image and orient it differently if it's smaller than the original.
        float scale = fittedSize.width() / (float)layoutRect.width();
        float dx = scale * (layoutRect.x() - mouseDownPoint.x());
        float originY = layoutRect.y();
#if PLATFORM(COCOA)
        // Compensate for accursed flipped coordinates in Cocoa.
        originY += layoutRect.height();
#endif
        float dy = scale * (originY - mouseDownPoint.y());
        scaledOrigin = IntPoint((int)(dx + 0.5), (int)(dy + 0.5));
    } else {
        if (CachedImage* cachedImage = getCachedImage(element)) {
            dragImage = createDragImageIconForCachedImageFilename(cachedImage->response().suggestedFilename());
            if (dragImage)
                scaledOrigin = IntPoint(DragIconRightInset - dragImageSize(dragImage).width(), DragIconBottomInset);
        }
    }

    dragImageOffset = mouseDownPoint + scaledOrigin;
    doSystemDrag(dragImage, dragImageOffset, dragOrigin, dataTransfer, frame, false);

    deleteDragImage(dragImage);
}

void DragController::doSystemDrag(DragImageRef image, const IntPoint& dragLoc, const IntPoint& eventPos, DataTransfer& dataTransfer, Frame& frame, bool forLink)
{
    m_didInitiateDrag = true;
    m_dragInitiator = frame.document();
    // Protect this frame and view, as a load may occur mid drag and attempt to unload this frame
    Ref<MainFrame> frameProtector(m_page.mainFrame());
    RefPtr<FrameView> viewProtector = frameProtector->view();
    m_client.startDrag(image, viewProtector->rootViewToContents(frame.view()->contentsToRootView(dragLoc)),
        viewProtector->rootViewToContents(frame.view()->contentsToRootView(eventPos)), dataTransfer, frameProtector.get(), forLink);
    // DragClient::startDrag can cause our Page to dispear, deallocating |this|.
    if (!frameProtector->page())
        return;

    cleanupAfterSystemDrag();
}

// Manual drag caret manipulation
void DragController::placeDragCaret(const IntPoint& windowPoint)
{
    mouseMovedIntoDocument(m_page.mainFrame().documentAtPoint(windowPoint));
    if (!m_documentUnderMouse)
        return;
    Frame* frame = m_documentUnderMouse->frame();
    FrameView* frameView = frame->view();
    if (!frameView)
        return;
    IntPoint framePoint = frameView->windowToContents(windowPoint);

    m_page.dragCaretController().setCaretPosition(frame->visiblePositionForPoint(framePoint));
}

#endif // ENABLE(DRAG_SUPPORT)

} // namespace WebCore
