/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * All rights reserved.
 * Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCEditorClient_h
#define WKCEditorClient_h

#include <wkc/wkcbase.h>
#include "WKCHelpersEnums.h"

namespace WKC {

class Range;
class HTMLElement;
class Frame;
class KeyboardEvent;
class Node;
class Page;
class String;
class CSSStyleDeclaration;
class EditCommand;
class Element;
struct GrammarDetail;
typedef struct WKCKeyEvent_ WKCKeyEvent;

/*@{*/

/** @brief Class that notifies of edit event for page. In this description, only those functions that were extended by ACCESS for NetFront Browser NX are described, and those inherited from WebCore::EditorClient are not described. */
class WKC_API EditorClientIf {
public:
    // from EditorClient
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void pageDestroyed() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Range* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldDeleteRange(WKC::Range*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool smartInsertDeleteEnabled() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool isSelectTrailingWhitespaceEnabled() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool isContinuousSpellCheckingEnabled() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void toggleContinuousSpellChecking() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool isGrammarCheckingEnabled() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void toggleGrammarChecking() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual int spellCheckerDocumentTag() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Range* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldBeginEditing(WKC::Range*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Range* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldEndEditing(WKC::Range*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Node* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldInsertNode(WKC::Node*, WKC::Range*, WKC::EditorInsertAction) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::String& (TBD) implement description
       @param  WKC::Range* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldInsertText(const WKC::String&, WKC::Range*, WKC::EditorInsertAction) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks if selection range can be changed
       @param fromRange Selection range before change
       @param toRange New selection range
       @param affinity Direction of range change
       @param stillSelecting Selection state
       @retval !false Can change
       @retval false Cannot change
       @details
       Returning !false calls ::WKC::EditorClientIf::respondToChangedSelection() after the selection range changes.
       @endcond
    */
    virtual bool shouldChangeSelectedRange(WKC::Range* fromRange, WKC::Range* toRange, WKC::EAffinity, bool stillSelecting) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::CSSStyleDeclaration* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldApplyStyle(WKC::CSSStyleDeclaration*, WKC::Range*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Range* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool shouldMoveRangeAfterDelete(WKC::Range*, WKC::Range*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void didBeginEditing() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void respondToChangedContents() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that selection range changes
       @return None
       @details
       Notification is given when the text selection range changes or when the caret position of text input changes.
       If inline text input is not performed, the text input application needs to be started within this function.
       For more information, see @ref bbb-textinput.
       @attention
       This function is not called unless !false is specified in the return value of WKC::EditorClientIf::shouldChangeSelectedRange().
       @endcond
    */
    virtual void respondToChangedSelection(WKC::Frame*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void didEndEditing() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void didWriteSelectionToPasteboard() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::EditCommand* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void registerUndoStep(WKC::EditCommand*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @fn  virtual void WKC::EditorClientIf::registerCommandForRedo(WTF::PassRefPtr<WKC::EditCommand>)
       @brief (TBD) implement description
       @param WKC::EditCommand* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void registerRedoStep(WKC::EditCommand*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void clearUndoRedoOperations() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks whether copy and cut can be used in frame
       @param frame Pointer to WKC::Frame
       @param defaultValue Default settings
       @return 
       - !false Can copy and cut
       - false Cannot copy and cut
       @endcond
    */
    virtual bool canCopyCut(Frame*, bool) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks whether paste can be used in frame
       @param frame Pointer to WKC::Frame
       @param defaultValue Default settings
       @return 
       - !false Can paste
       - false Cannot paste
       @endcond
    */
    virtual bool canPaste(Frame*, bool) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool canUndo() const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool canRedo() const = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void undo() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void redo() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::WKCKeyEvent* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void handleKeyboardEvent(WKC::KeyboardEvent*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::WKCKeyEvent* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void handleInputMethodKeydown(WKC::KeyboardEvent*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Element* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void textFieldDidBeginEditing(WKC::Element*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Element* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void textFieldDidEndEditing(WKC::Element*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Element* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void textDidChangeInTextField(WKC::Element*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Element* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool doTextFieldCommandFromEvent(WKC::Element*, WKC::KeyboardEvent*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Element* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void textWillBeDeletedInTextField(WKC::Element*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::Element* (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void textDidChangeInTextArea(WKC::Element*) = 0;



    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::String& (TBD) implement description
       @param WKC::GrammarDetail& (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void updateSpellingUIWithGrammarString(const WKC::String&, const WKC::GrammarDetail&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::String& (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void updateSpellingUIWithMisspelledWord(const WKC::String&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param show (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void showSpellingUI(bool show) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual bool spellingUIIsShowing() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return None
       @endcond
    */
    virtual void willSetInputMethodState() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param enabled (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void setInputMethodState(bool enabled) = 0;
};

/*@}*/

} // namespace

#endif // WKCEditorClient_h
