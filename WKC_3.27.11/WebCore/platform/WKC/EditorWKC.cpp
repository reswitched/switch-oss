/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include "Editor.h"

#include "DocumentFragment.h"
#include "NotImplemented.h"

namespace WebCore {

void Editor::pasteWithPasteboard(Pasteboard* pasteboard, OptionSet<PasteOption>)
{
    notImplemented();
}

template <typename PlatformDragData>
static RefPtr<DocumentFragment> createFragmentFromPlatformData(PlatformDragData& platformDragData, Frame& frame)
{
    notImplemented();
    return nullptr;
}

RefPtr<DocumentFragment> Editor::webContentFromPasteboard(Pasteboard& pasteboard, const SimpleRange&, bool /*allowPlainText*/, bool& /*chosePlainText*/)
{
    notImplemented();
    return nullptr;
}

void Editor::writeSelectionToPasteboard(Pasteboard &)
{
    notImplemented();
}

void Editor::writeImageToPasteboard(Pasteboard&, Element& imageElement, const URL&, const String& title)
{
    notImplemented();
}

void Editor::platformCopyFont()
{
    notImplemented();
}

void Editor::platformPasteFont()
{
    notImplemented();
}

} // namespace
