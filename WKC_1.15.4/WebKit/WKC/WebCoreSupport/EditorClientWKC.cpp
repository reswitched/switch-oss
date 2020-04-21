/*
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2009 Diego Escalante Urrelo <diegoe@gnome.org>
 *  Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *  Copyright (C) 2009, Igalia S.L.
 *  Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "CSSStyleDeclaration.h"
#include "EditorClientWKC.h"
#include "EditCommand.h"
#include "Element.h"
#include "Frame.h"
#include "HTMLElement.h"
#include "KeyboardEvent.h"
#include "Node.h"
#include "Range.h"
#include "WTFString.h"

#include "WKCWebViewPrivate.h"
#include "WKCPlatformEvents.h"

#include "helpers/EditorClientIf.h"
#include "helpers/WKCString.h"

#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCHTMLElementPrivate.h"
#include "helpers/privates/WKCKeyboardEventPrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCRangePrivate.h"

#include "NotImplemented.h"

// implementations

#define WKCPOBJ(a) ((a) ? ((a)->platformObj()) : 0)

namespace WKC {

EditorClientWKC::EditorClientWKC(WKCWebViewPrivate* view)
     : m_view(view),
       m_appClient(0)
{
}

EditorClientWKC::~EditorClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteEditorClient(m_appClient);
        m_appClient = 0;
    }
}

EditorClientWKC*
EditorClientWKC::create(WKCWebViewPrivate* view)
{
    EditorClientWKC* self = 0;
    self = new EditorClientWKC(view);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
EditorClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createEditorClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void
EditorClientWKC::pageDestroyed()
{
    delete this;
}


bool
EditorClientWKC::shouldDeleteRange(WebCore::Range* range)
{
    RangePrivate r(range);
    return m_appClient->shouldDeleteRange(&r.wkc());
}

bool
EditorClientWKC::smartInsertDeleteEnabled()
{
    return m_appClient->smartInsertDeleteEnabled();
}

bool
EditorClientWKC::isSelectTrailingWhitespaceEnabled() const
{
    return m_appClient->isSelectTrailingWhitespaceEnabled();
}

bool
EditorClientWKC::isContinuousSpellCheckingEnabled()
{
    return m_appClient->isContinuousSpellCheckingEnabled();
}

void
EditorClientWKC::toggleContinuousSpellChecking()
{
    m_appClient->toggleContinuousSpellChecking();
}

bool
EditorClientWKC::isGrammarCheckingEnabled()
{
    return m_appClient->isGrammarCheckingEnabled();
}

void
EditorClientWKC::toggleGrammarChecking()
{
    m_appClient->toggleGrammarChecking();
}

int
EditorClientWKC::spellCheckerDocumentTag()
{
    return m_appClient->spellCheckerDocumentTag();
}


bool
EditorClientWKC::shouldBeginEditing(WebCore::Range* range)
{
    RangePrivate r(range);
    return m_appClient->shouldBeginEditing(&r.wkc());
}

bool
EditorClientWKC::shouldEndEditing(WebCore::Range* range)
{
    RangePrivate r(range);
    return m_appClient->shouldEndEditing(&r.wkc());
}

bool
EditorClientWKC::shouldInsertNode(WebCore::Node* node, WebCore::Range* range, WebCore::EditorInsertAction action)
{
    RangePrivate r(range);

    NodePrivate* n = 0;
    if (node) {
        n = NodePrivate::create(node);
    }
    bool ret = m_appClient->shouldInsertNode(&n->wkc(), &r.wkc(), toWKCEditorInsertAction(action));
    delete n;
    return ret;
}

bool
EditorClientWKC::shouldInsertText(const WTF::String& string, WebCore::Range* range, WebCore::EditorInsertAction action)
{
    RangePrivate r(range);
    return m_appClient->shouldInsertText(string, &r.wkc(), toWKCEditorInsertAction(action));
}

bool
EditorClientWKC::shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity affinity, bool stillSelecting)
{
    RangePrivate fr(fromRange);
    RangePrivate tr(toRange);
    return m_appClient->shouldChangeSelectedRange(&fr.wkc(), &tr.wkc(), toWKCEAffinity(affinity), stillSelecting);
}


bool
EditorClientWKC::shouldApplyStyle(WebCore::StyleProperties* decl, WebCore::Range* range)
{
    notImplemented();
    return false;
#if 0
    RangePrivate r(range);
    CSSStyleDeclarationPrivate c(decl);
    return m_appClient->shouldApplyStyle(&c.wkc(), &r.wkc());
#endif
}

void
EditorClientWKC::didApplyStyle()
{
    notImplemented();
}

bool
EditorClientWKC::shouldMoveRangeAfterDelete(WebCore::Range* range1, WebCore::Range* range2)
{
    RangePrivate r1(range1);
    RangePrivate r2(range2);
    return m_appClient->shouldMoveRangeAfterDelete(&r1.wkc(), &r2.wkc());
}


void
EditorClientWKC::didBeginEditing()
{
    m_appClient->didBeginEditing();
}

void
EditorClientWKC::respondToChangedContents()
{
    m_appClient->respondToChangedContents();
}

void
EditorClientWKC::respondToChangedSelection(WebCore::Frame* frame)
{
    WKC::FramePrivate f(frame);
    m_appClient->respondToChangedSelection(&f.wkc());
}

void
EditorClientWKC::didEndUserTriggeredSelectionChanges()
{
    notImplemented();
}

void
EditorClientWKC::didEndEditing()
{
    m_appClient->didEndEditing();
}

void
EditorClientWKC::didWriteSelectionToPasteboard()
{
    m_appClient->didWriteSelectionToPasteboard();
}

void
EditorClientWKC::registerUndoStep(WebCore::UndoStep& command)
{
    notImplemented();
#if 0
    EditCommandPrivate e(&command);
    m_appClient->registerUndoStep(&e.wkc());
#endif
}

void
EditorClientWKC::registerRedoStep(WebCore::UndoStep& command)
{
#if 0
    EditCommandPrivate e(&command);
    m_appClient->registerRedoStep(&e.wkc());
#endif
}

void
EditorClientWKC::clearUndoRedoOperations()
{
    m_appClient->clearUndoRedoOperations();
}


bool
EditorClientWKC::canUndo() const
{
    return m_appClient->canUndo();
}

bool
EditorClientWKC::canRedo() const
{
    return m_appClient->canRedo();
}

bool
EditorClientWKC::canCopyCut(WebCore::Frame* frame, bool defaultValue) const
{
    WKC::FramePrivate f(frame);
    return m_appClient->canCopyCut(&f.wkc(), defaultValue);
}

bool
EditorClientWKC::canPaste(WebCore::Frame* frame, bool defaultValue) const
{
    WKC::FramePrivate f(frame);
    return m_appClient->canPaste(&f.wkc(), defaultValue);
}

void
EditorClientWKC::undo()
{
    m_appClient->undo();
}

void
EditorClientWKC::redo()
{
    m_appClient->redo();
}

void
EditorClientWKC::handleKeyboardEvent(WebCore::KeyboardEvent* event)
{
    WKC::KeyboardEventPrivate wev(event);
    m_appClient->handleKeyboardEvent(&wev.wkc());
}

void
EditorClientWKC::handleInputMethodKeydown(WebCore::KeyboardEvent* event)
{
    WKC::KeyboardEventPrivate wev(event);
    m_appClient->handleInputMethodKeydown(&wev.wkc());
}


void
EditorClientWKC::textFieldDidBeginEditing(WebCore::Element* element)
{
    ElementPrivate e(element);
    m_appClient->textFieldDidBeginEditing(&e.wkc());
}

void
EditorClientWKC::textFieldDidEndEditing(WebCore::Element* element)
{
    ElementPrivate e(element);
    m_appClient->textFieldDidEndEditing(&e.wkc());
}

void
EditorClientWKC::textDidChangeInTextField(WebCore::Element* element)
{
    ElementPrivate e(element);
    m_appClient->textDidChangeInTextField(&e.wkc());
}

bool
EditorClientWKC::doTextFieldCommandFromEvent(WebCore::Element* element, WebCore::KeyboardEvent* event)
{
    ElementPrivate e(element);
    WKC::KeyboardEventPrivate wev(event);
    return m_appClient->doTextFieldCommandFromEvent(&e.wkc(), &wev.wkc());
}

void
EditorClientWKC::textWillBeDeletedInTextField(WebCore::Element* element)
{
    ElementPrivate e(element);
    m_appClient->textWillBeDeletedInTextField(&e.wkc());
}

void
EditorClientWKC::textDidChangeInTextArea(WebCore::Element* element)
{
    ElementPrivate e(element);
    m_appClient->textDidChangeInTextArea(&e.wkc());
}

void
EditorClientWKC::overflowScrollPositionChanged()
{
    notImplemented();
}

void
EditorClientWKC::updateSpellingUIWithGrammarString(const WTF::String& string, const WebCore::GrammarDetail& detail)
{
    // Ugh!: support this feature!
    // 110128 ACCESS Co.,Ltd.
//    m_appClient->updateSpellingUIWithGrammarString(string, detail);
}

void
EditorClientWKC::updateSpellingUIWithMisspelledWord(const WTF::String& string)
{
    m_appClient->updateSpellingUIWithMisspelledWord(string);
}

void
EditorClientWKC::showSpellingUI(bool show)
{
    m_appClient->showSpellingUI(show);
}

bool
EditorClientWKC::spellingUIIsShowing()
{
    return m_appClient->spellingUIIsShowing();
}

void
EditorClientWKC::willSetInputMethodState()
{
    m_appClient->willSetInputMethodState();
}

void
EditorClientWKC::setInputMethodState(bool enabled)
{
    m_appClient->setInputMethodState(enabled);
}


WebCore::TextCheckerClient*
EditorClientWKC::textChecker()
{
    // Ugh!: implement it!
    // 110620 ACCESS Co.,Ltd.
    notImplemented();
    return 0;
}

void
EditorClientWKC::willWriteSelectionToPasteboard(WebCore::Range*)
{
    notImplemented();
}

void
EditorClientWKC::getClientPasteboardDataForRange(WebCore::Range*, WTF::Vector<WTF::String> &, WTF::Vector<WTF::RefPtr<WebCore::SharedBuffer> > &)
{
    notImplemented();
}

WTF::String
EditorClientWKC::replacementURLForResource(Ref<WebCore::SharedBuffer>&& resourceData, const WTF::String& mimeType)
{
    notImplemented();
    return "";
}

void
EditorClientWKC::discardedComposition(WebCore::Frame*)
{
    notImplemented();
}

void
EditorClientWKC::canceledComposition()
{
    notImplemented();
}

void
EditorClientWKC::didUpdateComposition()
{
    notImplemented();
}

void
EditorClientWKC::updateEditorStateAfterLayoutIfEditabilityChanged()
{
    notImplemented();
}


bool
EditorClientWKC::performTwoStepDrop(WebCore::DocumentFragment &, WebCore::Range & destination, bool isMove)
{
    notImplemented();
    return false;
}

} // namespace

