/*
 * Copyright (c) 2013, 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef BackForwardClientWKC_h
#define BackForwardClientWKC_h

#include "BackForwardClient.h"

namespace WebCore {
class HistoryItem;
}

namespace WKC {
class BackForwardClientIf;
class WKCWebViewPrivate;

class BackForwardClientWKC : public WebCore::BackForwardClient
{
public:
    static BackForwardClientWKC* create(WKCWebViewPrivate* view);
    virtual ~BackForwardClientWKC();

    virtual void addItem(Ref<WebCore::HistoryItem>&&) override;

    virtual void goToItem(WebCore::HistoryItem*) override;
        
    virtual WebCore::HistoryItem* itemAtIndex(int) override;
    virtual int backListCount() override;
    virtual int forwardListCount() override;

    virtual void close() override;

private:
    BackForwardClientWKC(WKCWebViewPrivate* view);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    WKC::BackForwardClientIf* m_appClient;
};

} // namespace

#endif // BackForwardClientWKC_h
