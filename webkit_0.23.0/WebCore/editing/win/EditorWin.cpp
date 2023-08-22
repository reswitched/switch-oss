/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "Editor.h"

#include "ClipboardUtilitiesWin.h"
#include "DocumentFragment.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "Pasteboard.h"
#include "windows.h"

namespace WebCore {

void Editor::pasteWithPasteboard(Pasteboard* pasteboard, bool allowPlainText, MailBlockquoteHandling mailBlockquoteHandling)
{
    RefPtr<Range> range = selectedRange();
    if (!range)
        return;

    bool chosePlainText;
    RefPtr<DocumentFragment> fragment = pasteboard->documentFragment(m_frame, *range, allowPlainText, chosePlainText);
    if (fragment && shouldInsertFragment(fragment, range, EditorInsertActionPasted))
        pasteAsFragment(fragment, canSmartReplaceWithPasteboard(*pasteboard), chosePlainText, mailBlockquoteHandling);
}

template <typename PlatformDragData>
static PassRefPtr<DocumentFragment> createFragmentFromPlatformData(PlatformDragData& platformDragData, Frame& frame)
{
    if (containsFilenames(&platformDragData)) {
        if (PassRefPtr<DocumentFragment> fragment = fragmentFromFilenames(frame.document(), &platformDragData))
            return fragment;
    }

    if (containsHTML(&platformDragData)) {
        if (PassRefPtr<DocumentFragment> fragment = fragmentFromHTML(frame.document(), &platformDragData))
            return fragment;
    }
    return nullptr;
}

PassRefPtr<DocumentFragment> Editor::webContentFromPasteboard(Pasteboard& pasteboard, Range&, bool /*allowPlainText*/, bool& /*chosePlainText*/)
{
    if (COMPtr<IDataObject> platformDragData = pasteboard.dataObject())
        return createFragmentFromPlatformData(*platformDragData, m_frame);

    return createFragmentFromPlatformData(pasteboard.dragDataMap(), m_frame);
}

} // namespace WebCore
