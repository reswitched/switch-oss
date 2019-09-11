/*
 *  WebNfcMessage.cpp
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "WebNfcMessage.h"

#if ENABLE(WKC_WEB_NFC)

#include "DOMURL.h"

namespace WebCore {

WebNfcMessage::WebNfcMessage(String& scope, Vector<String>& datas, WebNfcMessageDataType dataType)
    : m_scope(scope)
    , m_dataType(dataType)
    , m_stringDatas(datas)
    , m_urlDatas(0)
    , m_blobDatas(0)
    , m_blobContentTypes(0)
{
}

WebNfcMessage::WebNfcMessage(String& scope, Vector<RefPtr<DOMURL> >& datas)
    : m_scope(scope)
    , m_dataType(WebNfcMessageDataTypeURL)
    , m_stringDatas(0)
    , m_urlDatas(datas)
    , m_blobDatas(0)
    , m_blobContentTypes(0)
{
}

WebNfcMessage::WebNfcMessage(String& scope, Vector<Vector<char> >& datas, Vector<String>& contentTypes)
    : m_scope(scope)
    , m_dataType(WebNfcMessageDataTypeBlob)
    , m_stringDatas(0)
    , m_urlDatas(0)
    , m_blobDatas(datas)
    , m_blobContentTypes(contentTypes)
{
}

WebNfcMessage::~WebNfcMessage()
{
}

PassRefPtr<WebNfcMessage> WebNfcMessage::create(String& scope, Vector<String>& datas, WebNfcMessageDataType dataType)
{
    return adoptRef(*new WebNfcMessage(scope, datas, dataType));
}

PassRefPtr<WebNfcMessage> WebNfcMessage::create(String& scope, Vector<RefPtr<DOMURL> >& datas)
{
    return adoptRef(*new WebNfcMessage(scope, datas));
}

PassRefPtr<WebNfcMessage> WebNfcMessage::create(String& scope, Vector<Vector<char> >& datas, Vector<String>& contentTypes)
{
    return adoptRef(*new WebNfcMessage(scope, datas, contentTypes));
}

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
