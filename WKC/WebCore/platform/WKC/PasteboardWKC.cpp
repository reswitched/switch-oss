/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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

Pasteboard::Pasteboard()
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


static void readHTML(String& html, String& url)
{
    int len = 0;
    char* buf = 0;

    // html
    len = wkcPasteboardReadHTMLPeer(0,0);
    if (len<=0) {
        html = String();
    } else {
        CString str = CString::newUninitialized(len+1, buf);
        len = wkcPasteboardReadHTMLPeer(buf, len+1);
        html = String::fromUTF8(str.data(), len);
    }

    // url
    len = wkcPasteboardReadHTMLURIPeer(0,0);
    if (len<=0) {
        url = String();
    } else {
        CString str = CString::newUninitialized(len+1, buf);
        len = wkcPasteboardReadHTMLURIPeer(buf, len+1);
        url = String::fromUTF8(str.data(), len);
    }
}

static String readPlainText()
{
    int len = wkcPasteboardReadPlainTextPeer(0,0);
    if (len<=0)
        return String();

    char* buf = 0;
    CString str = CString::newUninitialized(len+1, buf);
    len = wkcPasteboardReadPlainTextPeer(buf, len+1);

    return String::fromUTF8(str.data(), len);
}

void Pasteboard::write(const PasteboardImage& in_image)
{
    clear();

    Image* image = in_image.image.get();
    
    ImageWKC* bitmap = reinterpret_cast<ImageWKC*>(image->nativeImageForCurrentFrame());
    if (!bitmap)
        return;

    int type = WKC_IMAGETYPE_ARGB8888;
    switch (bitmap->type()) {
    case ImageWKC::EColorARGB8888:
        type = WKC_IMAGETYPE_ARGB8888;
        break;
    case ImageWKC::EColorRGB565:
        type = WKC_IMAGETYPE_RGB565;
        break;
    default:
        return;
    }
    if (bitmap->hasAlpha()) {
        type |= WKC_IMAGETYPE_FLAG_HASALPHA;
    }

    const WKCSize size = { bitmap->size().width(), bitmap->size().height() };
    if (bitmap->isBitmapAvail())
        wkcPasteboardWriteImagePeer(type, bitmap->bitmap(), bitmap->rowbytes(), &size);
}

bool Pasteboard::canSmartReplace()
{
    return wkcPasteboardIsFormatAvailablePeer(WKC_PASTEBOARD_FORMAT_SMART_PASTE);
}

#if 0
PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame& frame, Range& range,
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

std::unique_ptr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    notImplemented();
    return std::make_unique<Pasteboard>();
}

std::unique_ptr<Pasteboard> Pasteboard::createPrivate()
{
    notImplemented();
    return std::make_unique<Pasteboard>();
}

#if ENABLE(DRAG_SUPPORT)
std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop()
{
    notImplemented();
    return std::make_unique<Pasteboard>();
}

std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop(const DragData&)
{
    notImplemented();
    return std::make_unique<Pasteboard>();
}
#endif

bool Pasteboard::hasData()
{
    notImplemented();
    return false;
}

Vector<String> Pasteboard::types()
{
    notImplemented();
    Vector<String> t;
    return t;
}

String Pasteboard::readString(const String& type)
{
    notImplemented();
    return String();
}

void Pasteboard::read(PasteboardPlainText& text)
{
    notImplemented();
}

void read(PasteboardWebContentReader& reader)
{
    notImplemented();
}

Vector<String> Pasteboard::readFilenames()
{
    notImplemented();
    Vector<String> names;
    return names;
}

void Pasteboard::writeString(const String& type, const String& data)
{
    notImplemented();
}

void Pasteboard::writePasteboard(const Pasteboard& sourcePasteboard)
{
    notImplemented();
}

void Pasteboard::write(const PasteboardWebContent&)
{
    notImplemented();
}

#if ENABLE(DRAG_SUPPORT)
void Pasteboard::setDragImage(DragImageRef, const IntPoint& hotSpot)
{
    notImplemented();
}
#endif

}
