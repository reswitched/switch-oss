/*
 * This file is part of the popup menu implementation for <select> elements in WebCore.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (c) 2010, 2016 ACCESS CO., LTD. All rights reserved.
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
 *
 */

#ifndef PopupMenuWKC_h
#define PopupMenuWKC_h

#include "PopupMenu.h"

#include "helpers/privates/WKCPopupMenuClientPrivate.h"

namespace WebCore {

class PopupMenuClient;
class PopupMenu;

class PopupMenuWKC : public PopupMenu {
public:
    PopupMenuWKC(PopupMenuClient*);
    virtual ~PopupMenuWKC() override;
    virtual void show(const IntRect&, FrameView*, int index) override;
    virtual void hide() override;
    virtual void updateFromElement() override;
    virtual void disconnectClient() override;

private:
    PopupMenuClient* client() const { return m_popupClient; }

private:
    PopupMenuClient* m_popupClient;
    WKC::PopupMenuClientPrivate* m_wkc;
    bool m_visible;
};

}

#endif
