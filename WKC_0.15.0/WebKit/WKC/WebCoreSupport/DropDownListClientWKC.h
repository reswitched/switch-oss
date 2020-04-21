/*
 * DropDownListClientWKC.h
 *
 * Copyright (c) 2010 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef DropDownListClientWKC_h
#define DropDownListClientWKC_h

namespace WebCore {
    class IntRect;
    class FrameView;
}

namespace WKC {

class DropDownListClientIf;
class WKCWebViewPrivate;
class PopupMenuClient;

class DropDownListClientWKC {
    WTF_MAKE_FAST_ALLOCATED;
public:
    DropDownListClientWKC()
       : m_view(0),
         m_appClient(0),
         m_destroyed(false)
    {};
    static DropDownListClientWKC* create(WKCWebViewPrivate* view);
    virtual ~DropDownListClientWKC();
    virtual void show(const WebCore::IntRect& r, WebCore::FrameView* view, int index, WKC::PopupMenuClient *client);
    virtual void hide(WKC::PopupMenuClient *client);
    virtual void updateFromElement(WKC::PopupMenuClient *client);

private:
    DropDownListClientWKC(WKCWebViewPrivate*);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    WKC::DropDownListClientIf* m_appClient;
    bool m_destroyed;
};

} // namespace

#endif // DropDownListClientWKC_h
