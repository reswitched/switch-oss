/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "NavigatorContentUtils.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)

#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "Navigator.h"
#include "Page.h"
#include <wtf/HashSet.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

#if !PLATFORM(WKC)
static HashSet<String>* protocolWhitelist;
#else
WKC_DEFINE_GLOBAL_PTR(HashSet<String>*, protocolWhitelist, 0);
#endif

static void initProtocolHandlerWhitelist()
{
    protocolWhitelist = new HashSet<String>;
    for (auto* protocol : { "bitcoin", "geo", "im", "irc", "ircs", "magnet", "mailto", "mms", "news", "nntp", "sip", "sms", "smsto", "ssh", "tel", "urn", "webcal", "wtai", "xmpp" })
        protocolWhitelist->add(protocol);
}

static bool verifyCustomHandlerURL(const URL& baseURL, const String& url, ExceptionCode& ec)
{
    // The specification requires that it is a SYNTAX_ERR if the "%s" token is
    // not present.
    static const char token[] = "%s";
    int index = url.find(token);
    if (-1 == index) {
        ec = SYNTAX_ERR;
        return false;
    }

    // It is also a SYNTAX_ERR if the custom handler URL, as created by removing
    // the "%s" token and prepending the base url, does not resolve.
    String newURL = url;
    newURL.remove(index, WTF_ARRAY_LENGTH(token) - 1);

    URL kurl(baseURL, newURL);

    if (kurl.isEmpty() || !kurl.isValid()) {
        ec = SYNTAX_ERR;
        return false;
    }

    return true;
}

static bool isProtocolWhitelisted(const String& scheme)
{
    if (!protocolWhitelist)
        initProtocolHandlerWhitelist();
    return protocolWhitelist->contains(scheme.convertToASCIILowercase());
}

static bool verifyProtocolHandlerScheme(const String& scheme, ExceptionCode& ec)
{
    if (isProtocolWhitelisted(scheme))
        return true;

    if (scheme.startsWith("web+")) {
        // The specification requires that the length of scheme is at least five characters (including 'web+' prefix).
        if (scheme.length() >= 5 && isValidProtocol(scheme))
            return true;
    }

    ec = SECURITY_ERR;
    return false;
}

NavigatorContentUtils* NavigatorContentUtils::from(Page* page)
{
    return static_cast<NavigatorContentUtils*>(Supplement<Page>::from(page, supplementName()));
}

NavigatorContentUtils::~NavigatorContentUtils()
{
}

void NavigatorContentUtils::registerProtocolHandler(Navigator* navigator, const String& scheme, const String& url, const String& title, ExceptionCode& ec)
{
    if (!navigator->frame())
        return;

    URL baseURL = navigator->frame()->document()->baseURL();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return;

    if (!verifyProtocolHandlerScheme(scheme, ec))
        return;

    NavigatorContentUtils::from(navigator->frame()->page())->client()->registerProtocolHandler(scheme, baseURL, URL(ParsedURLString, url), navigator->frame()->displayStringModifiedByEncoding(title));
}

#if ENABLE(CUSTOM_SCHEME_HANDLER)
static String customHandlersStateString(const NavigatorContentUtilsClient::CustomHandlersState state)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<String> newHandler(ASCIILiteral("new"));
    static NeverDestroyed<String> registeredHandler(ASCIILiteral("registered"));
    static NeverDestroyed<String> declinedHandler(ASCIILiteral("declined"));

    switch (state) {
    case NavigatorContentUtilsClient::CustomHandlersNew:
        return newHandler;
    case NavigatorContentUtilsClient::CustomHandlersRegistered:
        return registeredHandler;
    case NavigatorContentUtilsClient::CustomHandlersDeclined:
        return declinedHandler;
    }
#else
    WKC_DEFINE_STATIC_PTR(String*, newHandler, new String(ASCIILiteral("new")));
    WKC_DEFINE_STATIC_PTR(String*, registeredHandler, new String(ASCIILiteral("registered")));
    WKC_DEFINE_STATIC_PTR(String*, declinedHandler, new String(ASCIILiteral("declined")));

    switch (state) {
    case NavigatorContentUtilsClient::CustomHandlersNew:
        return *newHandler;
    case NavigatorContentUtilsClient::CustomHandlersRegistered:
        return *registeredHandler;
    case NavigatorContentUtilsClient::CustomHandlersDeclined:
        return *declinedHandler;
    }
#endif

    ASSERT_NOT_REACHED();
    return String();
}

String NavigatorContentUtils::isProtocolHandlerRegistered(Navigator* navigator, const String& scheme, const String& url, ExceptionCode& ec)
{
#if !PLATFORM(WKC)
    static NeverDestroyed<String> declined(ASCIILiteral("declined"));
#else
    WKC_DEFINE_STATIC_PTR(String*, declined_, new String(ASCIILiteral("declined")));
    String& declined = *declined_;
#endif

    if (!navigator->frame())
        return declined;

    URL baseURL = navigator->frame()->document()->baseURL();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return declined;

    if (!verifyProtocolHandlerScheme(scheme, ec))
        return declined;

    return customHandlersStateString(NavigatorContentUtils::from(navigator->frame()->page())->client()->isProtocolHandlerRegistered(scheme, baseURL, URL(ParsedURLString, url)));
}

void NavigatorContentUtils::unregisterProtocolHandler(Navigator* navigator, const String& scheme, const String& url, ExceptionCode& ec)
{
    if (!navigator->frame())
        return;

    URL baseURL = navigator->frame()->document()->baseURL();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return;

    if (!verifyProtocolHandlerScheme(scheme, ec))
        return;

    NavigatorContentUtils::from(navigator->frame()->page())->client()->unregisterProtocolHandler(scheme, baseURL, URL(ParsedURLString, url));
}
#endif

const char* NavigatorContentUtils::supplementName()
{
    return "NavigatorContentUtils";
}

void provideNavigatorContentUtilsTo(Page* page, std::unique_ptr<NavigatorContentUtilsClient> client)
{
    NavigatorContentUtils::provideTo(page, NavigatorContentUtils::supplementName(), std::make_unique<NavigatorContentUtils>(WTF::move(client)));
}

} // namespace WebCore

#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)

