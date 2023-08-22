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
#import "DragData.h"

#if ENABLE(DRAG_SUPPORT)
#import "MIMETypeRegistry.h"
#import "Pasteboard.h"
#import "PasteboardStrategy.h"
#import "PlatformStrategies.h"
#import "WebCoreNSURLExtras.h"

namespace WebCore {

DragData::DragData(DragDataRef data, const IntPoint& clientPosition, const IntPoint& globalPosition, 
    DragOperation sourceOperationMask, DragApplicationFlags flags)
    : m_clientPosition(clientPosition)
    , m_globalPosition(globalPosition)
    , m_platformDragData(data)
    , m_draggingSourceOperationMask(sourceOperationMask)
    , m_applicationFlags(flags)
    , m_pasteboardName([[m_platformDragData draggingPasteboard] name])
{
}

DragData::DragData(const String& dragStorageName, const IntPoint& clientPosition, const IntPoint& globalPosition,
    DragOperation sourceOperationMask, DragApplicationFlags flags)
    : m_clientPosition(clientPosition)
    , m_globalPosition(globalPosition)
    , m_platformDragData(0)
    , m_draggingSourceOperationMask(sourceOperationMask)
    , m_applicationFlags(flags)
    , m_pasteboardName(dragStorageName)
{
}
    
bool DragData::canSmartReplace() const
{
    return Pasteboard(m_pasteboardName).canSmartReplace();
}

bool DragData::containsColor() const
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    return types.contains(String(NSColorPboardType));
}

bool DragData::containsFiles() const
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    return types.contains(String(NSFilenamesPboardType));
}

unsigned DragData::numberOfFiles() const
{
    Vector<String> files;
    platformStrategies()->pasteboardStrategy()->getPathnamesForType(files, String(NSFilenamesPboardType), m_pasteboardName);
    return files.size();
}

void DragData::asFilenames(Vector<String>& result) const
{
    platformStrategies()->pasteboardStrategy()->getPathnamesForType(result, String(NSFilenamesPboardType), m_pasteboardName);
}

bool DragData::containsPlainText() const
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);

    return types.contains(String(NSStringPboardType))
        || types.contains(String(NSRTFDPboardType))
        || types.contains(String(NSRTFPboardType))
        || types.contains(String(NSFilenamesPboardType))
        || platformStrategies()->pasteboardStrategy()->stringForType(String(NSURLPboardType), m_pasteboardName).length();
}

String DragData::asPlainText() const
{
    Pasteboard pasteboard(m_pasteboardName);
    PasteboardPlainText text;
    pasteboard.read(text);
    String string = text.text;

    // FIXME: It's not clear this is 100% correct since we know -[NSURL URLWithString:] does not handle
    // all the same cases we handle well in the URL code for creating an NSURL.
    if (text.isURL)
        return userVisibleString([NSURL URLWithString:string]);

    // FIXME: WTF should offer a non-Mac-specific way to convert string to precomposed form so we can do it for all platforms.
    return [(NSString *)string precomposedStringWithCanonicalMapping];
}

Color DragData::asColor() const
{
    return platformStrategies()->pasteboardStrategy()->color(m_pasteboardName);
}

bool DragData::containsCompatibleContent() const
{
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);
    return types.contains(String(WebArchivePboardType))
        || types.contains(String(NSHTMLPboardType))
        || types.contains(String(NSFilenamesPboardType))
        || types.contains(String(NSTIFFPboardType))
        || types.contains(String(NSPDFPboardType))
        || types.contains(String(NSURLPboardType))
        || types.contains(String(NSRTFDPboardType))
        || types.contains(String(NSRTFPboardType))
        || types.contains(String(NSStringPboardType))
        || types.contains(String(NSColorPboardType))
        || types.contains(String(kUTTypePNG));
}
    
bool DragData::containsURL(FilenameConversionPolicy filenamePolicy) const
{
    return !asURL(filenamePolicy).isEmpty();
}

String DragData::asURL(FilenameConversionPolicy, String* title) const
{
    // FIXME: Use filenamePolicy.

    if (title) {
        String URLTitleString = platformStrategies()->pasteboardStrategy()->stringForType(String(WebURLNamePboardType), m_pasteboardName);
        if (!URLTitleString.isEmpty())
            *title = URLTitleString;
    }
    
    Vector<String> types;
    platformStrategies()->pasteboardStrategy()->getTypes(types, m_pasteboardName);

    if (types.contains(String(NSURLPboardType))) {
        NSURL *URLFromPasteboard = [NSURL URLWithString:platformStrategies()->pasteboardStrategy()->stringForType(String(NSURLPboardType), m_pasteboardName)];
        NSString *scheme = [URLFromPasteboard scheme];
        // Cannot drop other schemes unless <rdar://problem/10562662> and <rdar://problem/11187315> are fixed.
        if ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"])
            return [URLByCanonicalizingURL(URLFromPasteboard) absoluteString];
    }
    
    if (types.contains(String(NSStringPboardType))) {
        NSURL *URLFromPasteboard = [NSURL URLWithString:platformStrategies()->pasteboardStrategy()->stringForType(String(NSStringPboardType), m_pasteboardName)];
        NSString *scheme = [URLFromPasteboard scheme];
        // Pasteboard content is not trusted, because JavaScript code can modify it. We can sanitize it for URLs and other typed content, but not for strings.
        // The result of this function is used to initiate navigation, so we shouldn't allow arbitrary file URLs.
        // FIXME: Should we allow only http family schemes, or anything non-local?
        if ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"])
            return [URLByCanonicalizingURL(URLFromPasteboard) absoluteString];
    }
    
    if (types.contains(String(NSFilenamesPboardType))) {
        Vector<String> files;
        platformStrategies()->pasteboardStrategy()->getPathnamesForType(files, String(NSFilenamesPboardType), m_pasteboardName);
        if (files.size() == 1) {
            BOOL isDirectory;
            if ([[NSFileManager defaultManager] fileExistsAtPath:files[0] isDirectory:&isDirectory] && isDirectory)
                return String();
            return [URLByCanonicalizingURL([NSURL fileURLWithPath:files[0]]) absoluteString];
        }
    }
    
    return String();        
}

} // namespace WebCore

#endif // ENABLE(DRAG_SUPPORT)
