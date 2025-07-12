/*
 * Copyright (c) 2015, 2016 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(WKC_WEB_NFC)

#include "WKCEnums.h"

#include "helpers/WKCWebNfcMessage.h"
#include "helpers/privates/WKCWebNfcMessagePrivate.h"
#include "helpers/WKCString.h"

#include "WebNfcMessage.h"

namespace WKC {

// Private Implementation

WebNfcMessagePrivate::WebNfcMessagePrivate(WebNfcMessage* parent, WebNfcMessageDataType dataType,
                                           const unsigned short* scope, const unsigned short** datas, int len)
    : m_webcore(0)
    , m_wkc(parent)
    , m_dataType(dataType)
    , m_scope(String())
    , m_stringData(0)
    , m_stringDataItems(0)
    , m_stringDataCharacters(0)
    , m_stringDataLengths(0)
    , m_byteData(0)
    , m_byteDataLengths(0)
    , m_contentTypes(0)
    , m_contentTypeCharacters(0)
    , m_contentTypeLengths(0)
    , m_length(0)
{
    switch (dataType) {
    case EWebNfcMessageDataTypeString:
        {
            WTF::Vector<WTF::String> vector;
            for (int i = 0; i < len; ++i) {
                vector.append(WTF::String(datas[i]));
            }
            m_webcore = WebCore::WebNfcMessage::create(*&WTF::String(scope), *&vector, WebCore::WebNfcMessageDataTypeString);
            break;
        }
    case EWebNfcMessageDataTypeURL:
        {
            WTF::Vector<RefPtr<WebCore::DOMURL> > vector;
            for (int i = 0; i < len; ++i) {
                WebCore::ExceptionCode ec = 0;
                RefPtr<WebCore::DOMURL> object = WebCore::DOMURL::create(datas[i], ec);
                if (ec) {
                    continue;
                }
                vector.append(object);
            }
            m_webcore = WebCore::WebNfcMessage::create(*&WTF::String(scope), *&vector);
            break;
        }
    case EWebNfcMessageDataTypeJSON:
        {
            WTF::Vector<WTF::String> vector;
            for (int i = 0; i < len; ++i) {
                vector.append(WTF::String(datas[i]));
            }
            m_webcore = WebCore::WebNfcMessage::create(*&WTF::String(scope), *&vector, WebCore::WebNfcMessageDataTypeJSON);
            break;
        }
    default:
        break;
    }
}

WebNfcMessagePrivate::WebNfcMessagePrivate(WebNfcMessage* parent,
                                           const unsigned short* scope, const char** datas, const unsigned long long* dataLengths, const unsigned short** contentTypes, int len)
    : m_webcore(0)
    , m_wkc(parent)
    , m_dataType(EWebNfcMessageDataTypeBlob)
    , m_scope(String())
    , m_stringData(0)
    , m_stringDataItems(0)
    , m_stringDataCharacters(0)
    , m_stringDataLengths(0)
    , m_byteData(0)
    , m_byteDataLengths(0)
    , m_contentTypes(0)
    , m_contentTypeCharacters(0)
    , m_contentTypeLengths(0)
    , m_length(0)
{
    WTF::Vector<Vector<char> > dataVector;
    WTF::Vector<WTF::String> contenTypeVector;
    for (int i = 0; i < len; ++i) {
        WTF::Vector<char> data;
        for (int j = 0; j < dataLengths[i]; ++j) {
            data.append(datas[i][j]);
        }
        dataVector.append(data);

        contenTypeVector.append(contentTypes[i]);
    }
    m_webcore = WebCore::WebNfcMessage::create(*&WTF::String(scope), *&dataVector, *&contenTypeVector);
}

WebNfcMessagePrivate::WebNfcMessagePrivate(PassRefPtr<WebCore::WebNfcMessage> parent)
    : m_webcore(parent)
    , m_wkc(0)
    , m_dataType(EWebNfcMessageDataTypeString)
    , m_scope(String())
    , m_stringData(0)
    , m_stringDataItems(0)
    , m_stringDataCharacters(0)
    , m_stringDataLengths(0)
    , m_byteData(0)
    , m_byteDataLengths(0)
    , m_contentTypes(0)
    , m_contentTypeCharacters(0)
    , m_contentTypeLengths(0)
    , m_length(0)
{
    switch (webcore()->dataType()) {
    case WebCore::WebNfcMessageDataTypeString:
        m_dataType = EWebNfcMessageDataTypeString;
        break;
    case WebCore::WebNfcMessageDataTypeURL:
        m_dataType = EWebNfcMessageDataTypeURL;
        break;
    case WebCore::WebNfcMessageDataTypeBlob:
        m_dataType = EWebNfcMessageDataTypeBlob;
        break;
    case WebCore::WebNfcMessageDataTypeJSON:
        m_dataType = EWebNfcMessageDataTypeJSON;
        break;
    default:
        return;
    }

    m_scope = String(webcore()->scope());

    switch (m_dataType) {
    case EWebNfcMessageDataTypeString:
    case EWebNfcMessageDataTypeJSON:
        {
            WTF::Vector<WTF::String> vector = webcore()->stringDatas();
            m_stringDataItems = vector.size();
            void* p = WTF::fastMalloc(sizeof(String)*m_stringDataItems);
            m_stringData = new (p) String[m_stringDataItems];
            m_stringDataCharacters = (const unsigned short **)WTF::fastMalloc(sizeof(unsigned short*)*vector.size());
            m_stringDataLengths = (int *)WTF::fastMalloc(sizeof(int) * vector.size());
            for (int i = 0; i < vector.size(); ++i) {
                m_stringData[i] = String(vector.at(i));
                m_stringDataCharacters[i] = m_stringData[i].characters();
                m_stringDataLengths[i] = m_stringData[i].length();
            }
            m_length = vector.size();
            break;
        }
    case EWebNfcMessageDataTypeURL:
        {
            WTF::Vector<RefPtr<WebCore::DOMURL> > vector = webcore()->urlDatas();
            m_stringDataItems = vector.size();
            void* p = WTF::fastMalloc(sizeof(String)*m_stringDataItems);
            m_stringData = new (p) String[m_stringDataItems];
            m_stringDataCharacters = (const unsigned short **)WTF::fastMalloc(sizeof(unsigned short*)*vector.size());
            m_stringDataLengths = (int *)WTF::fastMalloc(sizeof(int)*vector.size());
            for (int i = 0; i < vector.size(); ++i) {
                m_stringData[i] = String(vector.at(i).get()->href().string());
                m_stringDataCharacters[i] = m_stringData[i].characters();
                m_stringDataLengths[i] = m_stringData[i].length();
            }
            m_length = vector.size();
            break;
        }
    case EWebNfcMessageDataTypeBlob:
        {
            WTF::Vector<Vector<char> > dataVector = webcore()->blobDatas();
            WTF::Vector<WTF::String> contenTypeVector = webcore()->blobContentTypes();
            m_byteData = (const char **)WTF::fastMalloc(sizeof(char*) * dataVector.size());
            m_byteDataLengths = (unsigned long long *)WTF::fastMalloc(sizeof(unsigned long long)*dataVector.size());
            m_stringDataItems = contenTypeVector.size();
            void* p = WTF::fastMalloc(sizeof(String)*m_stringDataItems);
            m_contentTypes = new (p) String[m_stringDataItems];
            m_contentTypeCharacters = (const unsigned short **)WTF::fastMalloc(sizeof(unsigned short*)*contenTypeVector.size());
            m_contentTypeLengths = (int *)WTF::fastMalloc(sizeof(int)*contenTypeVector.size());
            for (int i = 0; i < dataVector.size(); ++i) {
                char* tmp = (char *)WTF::fastMalloc(dataVector.at(i).size());
                for (int j = 0; j < dataVector.at(i).size(); ++j) {
                    tmp[j] = dataVector.at(i).at(j);
                }
                m_byteData[i] = tmp;
                m_byteDataLengths[i] = dataVector.at(i).size();

                m_contentTypes[i] = String(contenTypeVector.at(i));
                m_contentTypeCharacters[i] = m_contentTypes[i].characters();
                m_contentTypeLengths[i] = m_contentTypes[i].length();
            }
            m_length = dataVector.size();
            break;
        }
    default:
        break;
    }
}

WebNfcMessagePrivate::~WebNfcMessagePrivate()
{
    if (m_stringData) {
        for (size_t i=0; i<m_stringDataItems; i++)
            m_stringData[i].~String();
        WTF::fastFree((void *)m_stringData);
    }
    if (m_stringDataCharacters) {
        WTF::fastFree((void *)m_stringDataCharacters);
    }
    if (m_stringDataLengths) {
        WTF::fastFree((void *)m_stringDataLengths);
    }
    if (m_byteData) {
        WTF::fastFree((void *)m_byteData);
    }
    if (m_byteDataLengths) {
        WTF::fastFree((void *)m_byteDataLengths);
    }
    if (m_contentTypes) {
        WTF::fastFree((void *)m_contentTypes);
    }
    if (m_contentTypeCharacters) {
        WTF::fastFree((void *)m_contentTypeCharacters);
    }
    if (m_contentTypeLengths) {
        WTF::fastFree((void *)m_contentTypeLengths);
    }
}


// Implementation

WebNfcMessage::WebNfcMessage(WebNfcMessageDataType dataType, const unsigned short* scope, const unsigned short** datas, int len)
    : m_private(new WebNfcMessagePrivate(this, dataType, scope, datas, len))
{
}

WebNfcMessage::WebNfcMessage(const unsigned short* scope, const char** datas, const unsigned long long* dataLengths, const unsigned short** contentTypes, int len)
    : m_private(new WebNfcMessagePrivate(this, scope, datas, dataLengths, contentTypes, len))
{
}

WebNfcMessage::~WebNfcMessage()
{
    delete m_private;
}

void
WebNfcMessage::destroy(WebNfcMessage* instance)
{
    delete instance;
}

} // namespace WKC

#endif // ENABLE(WKC_WEB_NFC)
