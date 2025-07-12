/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include "HTTPRequest.h"

using namespace WebCore;

namespace {
enum HTTPVersion { Unknown, HTTP_1_0, HTTP_1_1 };
// HTTP/1.1 - RFC 2616
// http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1
// Request-Line = Method SP Request-URI SP HTTP-Version CRLF
size_t parseHTTPRequestLine(const char* data, size_t length, String& failureReason, String& method, String& url, HTTPVersion& httpVersion)
{
    method = String();
    url = String();
    httpVersion = Unknown;

    const char* space1 = 0;
    const char* space2 = 0;
    const char* p;
    size_t consumedLength;

    for (p = data, consumedLength = 0; consumedLength < length; p++, consumedLength++) {
        if (*p == ' ') {
            if (!space1)
                space1 = p;
            else if (!space2)
                space2 = p;
        }
        else if (*p == '\n')
            break;
    }

    // Haven't finished header line.
    if (consumedLength == length) {
        failureReason = "Incomplete Request Line"_s;
        return 0;
    }

    // RequestLine does not contain 3 parts.
    if (!space1 || !space2) {
        failureReason = "Request Line does not appear to contain: <Method> <Url> <HTTPVersion>."_s;
        return 0;
    }

    // The line must end with "\r\n".
    const char* end = p + 1;
    if (*(end - 2) != '\r') {
        failureReason = "Request line does not end with CRLF"_s;
        return 0;
    }

    // Request Method.
    method = String(data, space1 - data); // For length subtract 1 for space, but add 1 for data being the first character.

    // Request URI.
    url = String(space1 + 1, space2 - space1 - 1); // For length subtract 1 for space.

    // HTTP Version.
    String httpVersionString(space2 + 1, end - space2 - 3); // For length subtract 1 for space, and 2 for "\r\n".
    if (httpVersionString.length() != 8 || !httpVersionString.startsWith("HTTP/1."))
        httpVersion = Unknown;
    else if (httpVersionString[7] == '0')
        httpVersion = HTTP_1_0;
    else if (httpVersionString[7] == '1')
        httpVersion = HTTP_1_1;
    else
        httpVersion = Unknown;

    return end - data;
}
}

namespace WKC {

RefPtr<HTTPRequest> HTTPRequest::parseHTTPRequestFromBuffer(const uint8_t* data, size_t length, WTF::String& failureReason)
{
    if (!length) {
        failureReason = "No data to parse.";
        return nullptr;
    }

    // Request we will be building.
    RefPtr<HTTPRequest> request = HTTPRequest::create();

    // Advance a pointer through the data as needed.
    const uint8_t* pos = data;
    size_t remainingLength = length;

    // 1. Parse Method + URL.
    size_t requestLineLength = request->parseRequestLine(pos, remainingLength, failureReason);
    if (!requestLineLength)
        return nullptr;
    pos += requestLineLength;
    remainingLength -= requestLineLength;

    // 2. Parse HTTP Headers.
    size_t headersLength = request->parseHeaders(pos, remainingLength, failureReason);
    if (!headersLength)
        return nullptr;
    pos += headersLength;
    remainingLength -= headersLength;

    // 3. Parse HTTP Data.
    size_t dataLength = request->parseRequestBody(pos, remainingLength);
    pos += dataLength;
    remainingLength -= dataLength;

    // We should have processed the entire input.
    ASSERT(!remainingLength);
    return request;
}

size_t HTTPRequest::parseRequestLine(const uint8_t* data, size_t length, WTF::String& failureReason)
{
    WTF::String url;
    HTTPVersion version;
    size_t result = parseHTTPRequestLine(reinterpret_cast<const char*>(data), length, failureReason, m_requestMethod, url, version);
    m_url = URL(URL(), url);
    return result;
}

size_t HTTPRequest::parseHeaders(const uint8_t* data, size_t length, WTF::String& failureReason)
{
    const uint8_t* p = data;
    const uint8_t* end = data + length;
    WTF::StringView name;
    WTF::String value;
    for (; p < data + length; p++) {
        size_t consumedLength = parseHTTPHeader(p, end - p, failureReason, name, value);
        if (!consumedLength)
            return 0;
        p += consumedLength;
        if (name.isEmpty())
            break;
        m_headerFields.add(name.toString(), value);
    }

    // If we got here and "name" is empty, it means the headers are valid and ended with a
    // blank line (parseHTTPHeader returns "name" as empty if parsing a blank line), otherwise
    // the headers didn't end with a blank line and we have an invalid request.
    // See also http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
    if (!name.isEmpty())
        return 0;
    return p - data;
}

size_t HTTPRequest::parseRequestBody(const uint8_t* data, size_t length)
{
    return parseHTTPRequestBody(data, length, m_body);
}

HTTPRequest::HTTPRequest()
{
}

HTTPRequest::HTTPRequest(const WTF::String& requestMethod, const URL& url)
    : m_url(url)
    , m_requestMethod(requestMethod)
{
}

HTTPRequest::~HTTPRequest()
{
}

} // namespace WebCore
