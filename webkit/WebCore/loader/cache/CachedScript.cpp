/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedScript.h"

#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "HTTPHeaderNames.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "MemoryCache.h"
#include "RuntimeApplicationChecks.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include <wtf/Vector.h>

#if PLATFORM(WKC)
#include "TextCodecWKC.h"
#include "TextEncodingDetector.h"
#include "TextEncodingRegistry.h"
#include <wkc/wkcmpeer.h>
#endif

namespace WebCore {

CachedScript::CachedScript(const ResourceRequest& resourceRequest, const String& charset, SessionID sessionID)
    : CachedResource(resourceRequest, Script, sessionID)
    , m_decoder(TextResourceDecoder::create(ASCIILiteral("application/javascript"), charset))
{
    // It's javascript we want.
    // But some websites think their scripts are <some wrong mimetype here>
    // and refuse to serve them if we only accept application/x-javascript.
    setAccept("*/*");
}

CachedScript::~CachedScript()
{
}

void CachedScript::setEncoding(const String& chs)
{
    m_decoder->setEncoding(chs, TextResourceDecoder::EncodingFromHTTPHeader);
}

String CachedScript::encoding() const
{
    return m_decoder->encoding().name();
}

String CachedScript::mimeType() const
{
    return extractMIMETypeFromMediaType(m_response.httpHeaderField(HTTPHeaderName::ContentType)).lower();
}

const String& CachedScript::script()
{
    if (!m_script && m_data) {
#if PLATFORM(WKC)
        unsigned int bytesToBeAllocated;
        const char* data;
        unsigned int encodedLength;
        int decodedLength;
        std::unique_ptr<TextCodec> codecPtr;
        if (m_data->needsReallocationToMergeBuffers(&bytesToBeAllocated)
            && !wkcMemoryCheckMemoryAllocatablePeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT)) {
            // Failed to allocate continuous memory area for encoded script.
            // The area is used for a text decoder to detect encoding of the script.
            wkcMemoryNotifyMemoryAllocationErrorPeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT);
            goto setEmptyScript;
        }
        data = m_data->data();
        encodedLength = encodedSize();
        if (!data || !encodedLength) {
            goto setEmptyScript;
        }

        decodedLength = 0;
        codecPtr = newTextCodec(m_decoder->encoding());
        if (codecPtr->isTextCodecWKC()) {
            TextCodecWKC* codec = static_cast<TextCodecWKC*>(codecPtr.get());
            decodedLength = codec->getDecodedTextLength(data, encodedLength);
        } else {
            // encoding is Latin1 or UserDefined
            decodedLength = encodedLength;
            //How about the case where encoding is UTF-16???
            //decodedLength = encodedLength / 2;
        }

        bytesToBeAllocated = decodedLength * sizeof(UChar) * 2 + 128; // 128 for sizeof(StringImpl) and sizeof(TextCodecUTF8) and so forth.
        if (!wkcMemoryCheckMemoryAllocatablePeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT)) {
            // Failed to allocate continuous memory area for decoded script.
            wkcMemoryNotifyMemoryAllocationErrorPeer(bytesToBeAllocated, WKC_MEMORYALLOC_TYPE_JAVASCRIPT);
            goto setEmptyScript;
        } else {
#endif
            m_script = m_decoder->decodeAndFlush(m_data->data(), encodedSize());
            setDecodedSize(m_script.sizeInBytes());
#if PLATFORM(WKC)
        }
        goto junction;

setEmptyScript:
        m_script = emptyString();
        setDecodedSize(0);
        setStatus(DecodeError);

junction:
        ;
#endif
    }
    m_decodedDataDeletionTimer.restart();
    return m_script;
}

void CachedScript::finishLoading(SharedBuffer* data)
{
    m_data = data;
    setEncodedSize(data ? data->size() : 0);
    CachedResource::finishLoading(data);
}

void CachedScript::destroyDecodedData()
{
    m_script = String();
    setDecodedSize(0);
}

#if ENABLE(NOSNIFF)
bool CachedScript::mimeTypeAllowedByNosniff() const
{
    return parseContentTypeOptionsHeader(m_response.httpHeaderField(HTTPHeaderName::XContentTypeOptions)) != ContentTypeOptionsNosniff || MIMETypeRegistry::isSupportedJavaScriptMIMEType(mimeType());
}
#endif

bool CachedScript::shouldIgnoreHTTPStatusCodeErrors() const
{
    // This is a workaround for <rdar://problem/13916291>
    // REGRESSION (r119759): Adobe Flash Player "smaller" installer relies on the incorrect firing
    // of a load event and needs an app-specific hack for compatibility.
    // The installer in question tries to load .js file that doesn't exist, causing the server to
    // return a 404 response. Normally, this would trigger an error event to be dispatched, but the
    // installer expects a load event instead so we work around it here.
    if (applicationIsSolidStateNetworksDownloader())
        return true;

    return CachedResource::shouldIgnoreHTTPStatusCodeErrors();
}

} // namespace WebCore
