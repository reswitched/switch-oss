/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (c) 2019-2020 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTTPRequest_h
#define HTTPRequest_h

#include <HTTPHeaderMap.h>
#include <HTTPParsers.h>
#include <URL.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WKC {

class HTTPRequest : public RefCounted<HTTPRequest> {
public:
    static Ref<HTTPRequest> create() { return adoptRef(*new HTTPRequest()); }
    static Ref<HTTPRequest> create(const WTF::String& requestMethod, const URL& url, WebCore::HTTPVersion version) { return adoptRef(*new HTTPRequest(requestMethod, url, version)); }
    static RefPtr<HTTPRequest> parseHTTPRequestFromBuffer(const char* data, size_t length, WTF::String& failureReason);
    virtual ~HTTPRequest();

    WTF::String requestMethod() const { return m_requestMethod; }
    void setRequestMethod(const WTF::String& method) { m_requestMethod = method; }

    URL url() const { return m_url; }
    void setURL(const URL& url) { m_url = url; }

    const Vector<unsigned char>& body() const { return m_body; }

    const WebCore::HTTPHeaderMap& headerFields() const { return m_headerFields; }
    void addHeaderField(const AtomString& name, const WTF::String& value) { m_headerFields.add(name, value); }
    void addHeaderField(const char* name, const WTF::String& value) { m_headerFields.add(name, value); }

protected:
    HTTPRequest();
    HTTPRequest(const WTF::String& requestMethod, const URL&, WebCore::HTTPVersion);

    // Parsing helpers.
    size_t parseRequestLine(const char* data, size_t length, WTF::String& failureReason);
    size_t parseHeaders(const char* data, size_t length, WTF::String& failureReason);
    size_t parseRequestBody(const char* data, size_t length);

    URL m_url;
    WebCore::HTTPVersion m_httpVersion;
    WTF::String m_requestMethod;
    WebCore::HTTPHeaderMap m_headerFields;
    Vector<unsigned char> m_body;
};

} // namespace WebKit

#endif // HTTPRequest_h
