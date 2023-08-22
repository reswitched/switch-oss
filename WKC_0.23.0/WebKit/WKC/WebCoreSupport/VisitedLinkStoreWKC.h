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

#ifndef VisitedLinkStoreWKC_h
#define VisitedLinkStoreWKC_h

#include "VisitedLinkStore.h"
#include "Vector.h"

namespace WKC {

class WKCWebViewPrivate;

class VisitedLinkStoreWKC : public WebCore::VisitedLinkStore {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static WTF::PassRefPtr<VisitedLinkStoreWKC> create(WKCWebViewPrivate*);
    virtual ~VisitedLinkStoreWKC() override;

    virtual bool isLinkVisited(WebCore::Page&, WebCore::LinkHash, const WebCore::URL& baseURL, const WTF::AtomicString& attributeURL) override;
    virtual void addVisitedLink(WebCore::Page&, WebCore::LinkHash) override;

private:
    VisitedLinkStoreWKC(WKCWebViewPrivate*);

private:
    WKCWebViewPrivate* m_view;
    WTF::Vector<WebCore::LinkHash> m_links;
};

} // namespace

#endif // VisitedLinkStoreWKC_h
