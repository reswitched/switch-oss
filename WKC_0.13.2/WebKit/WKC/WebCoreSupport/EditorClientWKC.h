/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * All rights reserved.
 * Copyright (c) 2010-2017 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EditorClientWKC_h
#define EditorClientWKC_h

#include "EditorClient.h"

#include <wtf/Deque.h>
#include <wtf/Forward.h>

namespace WebCore {
    class Page;
}

namespace WKC {
class EditorClientIf;
class WKCWebViewPrivate;

class EditorClientWKC : public WebCore::EditorClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static EditorClientWKC* create(WKCWebViewPrivate*);
    ~EditorClientWKC();

    // from EditorClient
    virtual void pageDestroyed();

    virtual bool shouldDeleteRange(WebCore::Range*) override;
    virtual bool smartInsertDeleteEnabled() override;
    virtual bool isSelectTrailingWhitespaceEnabled() override;
    virtual bool isContinuousSpellCheckingEnabled() override;
    virtual void toggleContinuousSpellChecking() override;
    virtual bool isGrammarCheckingEnabled() override;
    virtual void toggleGrammarChecking() override;
    virtual int spellCheckerDocumentTag() override;

    virtual bool shouldBeginEditing(WebCore::Range*) override;
    virtual bool shouldEndEditing(WebCore::Range*) override;
    virtual bool shouldInsertNode(WebCore::Node*, WebCore::Range*, WebCore::EditorInsertAction) override;
    virtual bool shouldInsertText(const WTF::String&, WebCore::Range*, WebCore::EditorInsertAction) override;
    virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting) override;

    virtual bool shouldApplyStyle(WebCore::StyleProperties*, WebCore::Range*) override;
    virtual void didApplyStyle() override;
    virtual bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*) override;

    virtual void didBeginEditing() override;
    virtual void respondToChangedContents() override;
    virtual void respondToChangedSelection(WebCore::Frame*) override;
    virtual void didChangeSelectionAndUpdateLayout() override;
    virtual void updateEditorStateAfterLayoutIfEditabilityChanged() override;
    virtual void didEndEditing() override;
    virtual void didWriteSelectionToPasteboard() override;

    virtual void registerUndoStep(WTF::PassRefPtr<WebCore::UndoStep>) override;
    virtual void registerRedoStep(WTF::PassRefPtr<WebCore::UndoStep>) override;
    virtual void clearUndoRedoOperations() override;

    virtual bool canCopyCut(WebCore::Frame*, bool defaultValue) const override;
    virtual bool canPaste(WebCore::Frame*, bool defaultValue) const override;
    virtual bool canUndo() const override;
    virtual bool canRedo() const override;

    virtual void undo() override;
    virtual void redo() override;

    virtual void handleKeyboardEvent(WebCore::KeyboardEvent*) override;
    virtual void handleInputMethodKeydown(WebCore::KeyboardEvent*) override;

    virtual void textFieldDidBeginEditing(WebCore::Element*) override;
    virtual void textFieldDidEndEditing(WebCore::Element*) override;
    virtual void textDidChangeInTextField(WebCore::Element*) override;
    virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*) override;
    virtual void textWillBeDeletedInTextField(WebCore::Element*) override;
    virtual void textDidChangeInTextArea(WebCore::Element*) override;
    virtual void overflowScrollPositionChanged() override;

    virtual WebCore::TextCheckerClient* textChecker() override;

    virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail&) override;
    virtual void updateSpellingUIWithMisspelledWord(const WTF::String&) override;
    virtual void showSpellingUI(bool show) override;
    virtual bool spellingUIIsShowing() override;
    virtual void willSetInputMethodState() override;
    virtual void setInputMethodState(bool enabled) override;

    virtual void willWriteSelectionToPasteboard(WebCore::Range*) override;
    virtual void getClientPasteboardDataForRange(WebCore::Range*, WTF::Vector<WTF::String> &, WTF::Vector<WTF::RefPtr<WebCore::SharedBuffer> > &) override;

    virtual void discardedComposition(WebCore::Frame*) override;

private:
   EditorClientWKC(WKCWebViewPrivate*);
   bool construct();

private:
   WKCWebViewPrivate* m_view;
   WKC::EditorClientIf* m_appClient;
};

} // namespace

#endif // EditorClientWKC_h
