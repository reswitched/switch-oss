/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
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
#include "Pasteboard.h"

#include "CachedImage.h"
#include "CString.h"
#include "DocumentFragment.h"
#include "Editor.h"
#include "Frame.h"
#include "NotImplemented.h"
#include "WTFString.h"
#include "TextResourceDecoder.h"
#include "Image.h"
#include "ImageWKC.h"
#include "RenderImage.h"
#include "URL.h"
#include "markup.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>
#include <wkc/wkcpasteboardpeer.h>

#include "NotImplemented.h"

namespace WebCore {

Pasteboard::Pasteboard(std::unique_ptr<PasteboardContext>&& context)
    : m_context(WTFMove(context))
{
}

void Pasteboard::clear()
{
    wkcPasteboardClearPeer();
}

void Pasteboard::clear(const String& type)
{
    wkcPasteboardClearPeer();
}

#if 0
void Pasteboard::writeSelection(Range& selectedRange, bool canSmartCopyOrDelete, Frame& frame, ShouldSerializeSelectedTextForDataTransfer)
{
    clear();

    String html = createMarkup(selectedRange, 0, AnnotateForInterchange, false, ResolveNonLocalURLs);
    ExceptionCode ec = 0;
    Node* node = selectedRange.startContainer(ec);
    if (!node)
        return;
    URL url = node->document().url();
    String plainText = frame.editor().selectedText();

    wkcPasteboardWriteHTMLPeer(html.utf8().data(), (int)html.utf8().length(),
                               url.string().utf8().data(), (int)url.string().utf8().length(),
                               plainText.utf8().data(), (int)plainText.utf8().length(),
                               canSmartCopyOrDelete);
}
#endif

void Pasteboard::writePlainText(const String& text, SmartReplaceOption)
{
    clear();
    wkcPasteboardWritePlainTextPeer(text.utf8().data(), (int)text.utf8().length());
}


void Pasteboard::write(const PasteboardURL& url)
{
    clear();
    wkcPasteboardWriteURIPeer(url.url.string().utf8().data(), (int)url.url.string().utf8().length(), url.title.utf8().data(), (int)url.title.utf8().length());
}

void Pasteboard::write(const Color& color)
{
    notImplemented();
}

void Pasteboard::write(const PasteboardImage& in_image)
{
    clear();

    Image* image = in_image.image.get();
    PlatformImagePtr platformImagePtr = image->nativeImageForCurrentFrame()->platformImage();
    if (!platformImagePtr)
        return;

    PlatformImageHelper helper(WTFMove(platformImagePtr));
    const WKCSize size = { static_cast<int>(image->size().width()), static_cast<int>(image->size().height()) };
    wkcPasteboardWriteImagePeer(WKC_IMAGETYPE_ARGB8888, helper.bitmap(), helper.rowbytes(), &size);
}

void Pasteboard::write(const PasteboardBuffer&)
{
    notImplemented();
}

bool Pasteboard::canSmartReplace()
{
    return wkcPasteboardIsFormatAvailablePeer(WKC_PASTEBOARD_FORMAT_SMART_PASTE);
}

#if 0
RefPtr<DocumentFragment> Pasteboard::documentFragment(Frame& frame, Range& range,
                                                          bool allowPlainText, bool& chosePlainText)
{
    chosePlainText = false;

    if (wkcPasteboardIsFormatAvailablePeer(WKC_PASTEBOARD_FORMAT_HTML)) {
        String html, url;
        readHTML(html, url);
        if (html.length()) {
            RefPtr<DocumentFragment> fm = createFragmentFromMarkup(*frame.document(), html, url, DisallowScriptingContent);
            if (fm)
                return fm.release();
        }
    }

    if (allowPlainText && wkcPasteboardIsFormatAvailablePeer(WKC_PASTEBOARD_FORMAT_TEXT)) {
        String text = readPlainText();
        if (text.length()) {
            RefPtr<DocumentFragment> fm = createFragmentFromText(range, text);
            if (fm) {
                chosePlainText = true;
                return fm.release();
            }
        }
    }

    return 0;
}
#endif

std::unique_ptr<Pasteboard> Pasteboard::createForCopyAndPaste(std::unique_ptr<PasteboardContext>&&)
{
    notImplemented();
    return std::make_unique<Pasteboard>(nullptr);
}

#if ENABLE(DRAG_SUPPORT)
std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop(std::unique_ptr<PasteboardContext>&&)
{
    notImplemented();
    return std::make_unique<Pasteboard>(nullptr);
}

std::unique_ptr<Pasteboard> Pasteboard::create(const DragData&)
{
    notImplemented();
    return std::make_unique<Pasteboard>(nullptr);
}
#endif

bool Pasteboard::hasData()
{
    notImplemented();
    return false;
}

Vector<String> Pasteboard::typesSafeForBindings(const String& origin)
{
    notImplemented();
    return {};
}

Vector<String> Pasteboard::typesForLegacyUnsafeBindings()
{
    notImplemented();
    return {};
}

String Pasteboard::readOrigin()
{
    notImplemented();
    return String();
}

String Pasteboard::readString(const String& type)
{
    notImplemented();
    return String();
}

String Pasteboard::readStringInCustomData(const String& type)
{
    notImplemented();
    return String();
}

void Pasteboard::read(PasteboardPlainText& text, PlainTextURLReadingPolicy, std::optional<size_t> itemIndex)
{
    notImplemented();
}

void Pasteboard::read(PasteboardWebContentReader& reader, WebContentReadingPolicy, std::optional<size_t> itemIndex)
{
    notImplemented();
}

void Pasteboard::read(PasteboardFileReader&, std::optional<size_t> itemIndex)
{
    notImplemented();
}

void Pasteboard::writeString(const String& type, const String& data)
{
    notImplemented();
}

void Pasteboard::write(const PasteboardWebContent&)
{
    notImplemented();
}

void Pasteboard::writeCustomData(const Vector<PasteboardCustomData>&)
{
    notImplemented();
}

Pasteboard::FileContentState Pasteboard::fileContentState()
{
    notImplemented();
    return FileContentState::NoFileOrImageData;
}


void Pasteboard::writeMarkup(const String& markup)
{
    notImplemented();
}

#if ENABLE(DRAG_SUPPORT)
void Pasteboard::setDragImage(DragImage, const IntPoint& hotSpot)
{
    notImplemented();
}
#endif

void Pasteboard::writeTrustworthyWebURLsPboardType(const PasteboardURL&)
{
    notImplemented();
}

}
