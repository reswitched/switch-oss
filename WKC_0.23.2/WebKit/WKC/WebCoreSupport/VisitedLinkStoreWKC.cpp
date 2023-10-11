/*
 * Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
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

#include "VisitedLinkStoreWKC.h"

namespace WKC {

VisitedLinkStoreWKC::VisitedLinkStoreWKC(WKCWebViewPrivate* view)
    : VisitedLinkStore()
    , m_view(view)
{
}

VisitedLinkStoreWKC::~VisitedLinkStoreWKC()
{
}

WTF::PassRefPtr<VisitedLinkStoreWKC>
VisitedLinkStoreWKC::create(WKCWebViewPrivate* view)
{
    return adoptRef(new VisitedLinkStoreWKC(view));
}

bool
VisitedLinkStoreWKC::isLinkVisited(WebCore::Page&, WebCore::LinkHash hash, const WebCore::URL& baseURL, const WTF::AtomicString& attributeURL)
{
    return m_links.contains(hash);
}

void
VisitedLinkStoreWKC::addVisitedLink(WebCore::Page&, WebCore::LinkHash hash)
{
    m_links.append(hash);
}

} // namespace
