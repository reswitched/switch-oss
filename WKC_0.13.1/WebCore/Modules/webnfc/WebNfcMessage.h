/*
 *  WebNfcMessage.h
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

#ifndef WebNfcMessage_h
#define WebNfcMessage_h

#if ENABLE(WKC_WEB_NFC)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/PassRefPtr.h>

#include "DOMURL.h"

namespace WebCore {

enum WebNfcMessageDataType {
    WebNfcMessageDataTypeString,
    WebNfcMessageDataTypeURL,
    WebNfcMessageDataTypeBlob,
    WebNfcMessageDataTypeJSON,
};

class WebNfcMessage : public RefCounted<WebNfcMessage>{
public:
    virtual ~WebNfcMessage();
    static PassRefPtr<WebNfcMessage> create(String&, Vector<String>&, WebNfcMessageDataType);
    static PassRefPtr<WebNfcMessage> create(String&, Vector<RefPtr<DOMURL> >&);
    static PassRefPtr<WebNfcMessage> create(String&, Vector<Vector<char> >&, Vector<String>&);

    String& scope() { return m_scope; }

    WebNfcMessageDataType dataType() { return m_dataType; }

    Vector<String>& stringDatas() { return m_stringDatas; }
    Vector<RefPtr<DOMURL> >& urlDatas() { return m_urlDatas; }
    Vector<Vector<char> >& blobDatas() { return m_blobDatas; }
    Vector<String>& blobContentTypes() { return m_blobContentTypes; }

private:
    WebNfcMessage(String&, Vector<String>&, WebNfcMessageDataType);
    WebNfcMessage(String&, Vector<RefPtr<DOMURL> >&);
    WebNfcMessage(String&, Vector<Vector<char> >&, Vector<String>&);

    String m_scope;

    WebNfcMessageDataType m_dataType;

    Vector<String> m_stringDatas;
    Vector<RefPtr<DOMURL> > m_urlDatas;
    Vector<Vector<char> > m_blobDatas;
    Vector<String> m_blobContentTypes;
};

}// namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfcMessage_h
