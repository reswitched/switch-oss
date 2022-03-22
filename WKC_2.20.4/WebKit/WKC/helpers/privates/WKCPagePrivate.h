/*
 * Copyright (c) 2011-2021 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_PAGE_H_
#define _WKC_HELPERS_PRIVATE_PAGE_H_

#include "helpers/WKCPage.h"
#include "helpers/WKCString.h"

namespace WebCore {
class Page;
} // namespace

namespace WKC {

class FocusController;
class BackForwardList;
class Frame;
class FocusControllerPrivate;
class BackForwardListPrivate;
class FramePrivate;
class HistoryItem;

class PageWrap : public Page {
WTF_MAKE_FAST_ALLOCATED;
public:
    PageWrap(PagePrivate& parent) : Page(parent) {}
    ~PageWrap() {}
};

class PagePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    PagePrivate(WebCore::Page*);
    ~PagePrivate();

    WebCore::Page* webcore() const { return m_webcore; }
    Page& wkc() { return m_wkc; }

    FocusController* focusController();
    BackForwardList* backForwardList();
    Frame* mainFrame();
    void goToItem(HistoryItem*, FrameLoadType);
    void setGroupName(const String& name);
    const String& groupName();
    void setDeviceScaleFactor(float);
    void updateRendering();

    void clear();

private:
    WebCore::Page* m_webcore;
    PageWrap m_wkc;

    FramePrivate* m_mainFrame;
    FocusControllerPrivate* m_focusController;
    BackForwardListPrivate* m_backForwardList;
    String m_groupName;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_PAGE_H_

