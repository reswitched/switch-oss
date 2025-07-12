/*
   Copyright (C) 1997 Martin Jones (mjones@kde.org)
             (C) 1998 Waldo Bastian (bastian@kde.org)
             (C) 1998, 1999 Torben Weis (weis@kde.org)
             (C) 1999 Lars Knoll (knoll@kde.org)
             (C) 1999 Antti Koivisto (koivisto@kde.org)
   Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
   Copyright (c) 2010-2017 ACCESS CO., LTD. All rights reserved.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _WKC_HELPER_WKCFRAMEVIEW_H_
#define _WKC_HELPER_WKCFRAMEVIEW_H_

#include <wkc/wkcbase.h>

namespace WKC {

class FrameViewPrivate;
class Scrollbar;
class Color;

class WKC_API FrameView {
public:
    void scrollPositionChanged();
    void setCannotBlitToWindow();
    void updateLayoutAndStyleIfNeededRecursive();
    void forceLayout();
    bool needsLayout() const;

    WKCPoint contentsToWindow(const WKCPoint&);
    WKCRect contentsToWindow(const WKCRect&);
    WKCRect convertToContainingWindow(const WKCRect&) const;

    WKCPoint windowToContents(const WKCPoint&);
    WKCRect windowToContents(const WKCRect&);

    Scrollbar* verticalScrollbar() const;
    Scrollbar* horizontalScrollbar() const;

    Color documentBackgroundColor() const;

    void setWasScrolledByUser(bool);

    bool flushCompositingStateIncludingSubframes();

    FrameViewPrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    FrameView(FrameViewPrivate&);
    ~FrameView();

private:
    FrameView(const FrameView&);
    FrameView& operator=(const FrameView&);

    FrameViewPrivate& m_private;
};
} // namespace

#endif // _WKC_HELPER_WKCFRAMEVIEW_H_
