/*
 * Copyright (c) 2015,2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_WEBNFCMESSAGEPRIVATE_H_
#define _WKC_HELPERS_WKC_WEBNFCMESSAGEPRIVATE_H_

#if ENABLE(WKC_WEB_NFC)

#include "helpers/WKCWebNfcMessage.h"
#include "helpers/WKCString.h"

#include "WKCEnums.h"
#include "WebNfcMessage.h"

namespace WKC {

class WebNfcMessagePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    WebNfcMessagePrivate(WebNfcMessage* parent, WebNfcMessageDataType dataType,
                         const unsigned short* scope, const unsigned short** datas, int len);
    WebNfcMessagePrivate(WebNfcMessage* parent,
                         const unsigned short* scope, const char** datas, const unsigned long long* dataLengths, const unsigned short** contentTypes, int len);
    WebNfcMessagePrivate(PassRefPtr<WebCore::WebNfcMessage> parent);
    ~WebNfcMessagePrivate();

    PassRefPtr<WebCore::WebNfcMessage> webcore() const { return m_webcore; }
    WebNfcMessage* wkc() { return m_wkc; }

    WebNfcMessageDataType dataType() { return m_dataType; }
    const unsigned short* scope() { return m_scope.characters(); }
    int scopeLength() { return m_scope.length(); }
    const unsigned short** stringDatas() { return m_stringDataCharacters; }
    int* stringDataLengths() { return m_stringDataLengths; }
    const char** byteDatas() { return m_byteData; }
    unsigned long long* byteDataLengths() { return m_byteDataLengths; }
    const unsigned short** contentTypes() { return m_contentTypeCharacters; }
    int* contentTypeLengths() { return m_contentTypeLengths; }
    int length() { return m_length; }

private:
    RefPtr<WebCore::WebNfcMessage> m_webcore;
    WebNfcMessage* m_wkc;

    WebNfcMessageDataType m_dataType;
    String m_scope;
    String* m_stringData;
    size_t m_stringDataItems;
    const unsigned short** m_stringDataCharacters;
    int* m_stringDataLengths;
    const char** m_byteData;
    unsigned long long* m_byteDataLengths;
    String* m_contentTypes;
    const unsigned short** m_contentTypeCharacters;
    int* m_contentTypeLengths;
    int m_length;
};

} // namespace WKC

#endif // ENABLE(WKC_WEB_NFC)

#endif // _WKC_HELPERS_WKC_WEBNFCMESSAGEPRIVATE_H_
