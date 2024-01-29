/*
 * Copyright (c) 2010-2020 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "SearchPopupMenuWKC.h"
#include "PopupMenuWKC.h"

#include "NotImplemented.h"

namespace WebCore {

SearchPopupMenuWKC::SearchPopupMenuWKC(PopupMenuClient* client)
    : m_popupMenu(adoptRef(new PopupMenuWKC(client)))
{
}

SearchPopupMenuWKC::~SearchPopupMenuWKC()
{
}

PopupMenu*
SearchPopupMenuWKC::popupMenu()
{
    return m_popupMenu.get();
}

void
SearchPopupMenuWKC::saveRecentSearches(const AtomString&, const Vector<RecentSearch>&)
{
    notImplemented();
}

void
SearchPopupMenuWKC::loadRecentSearches(const AtomString&, Vector<RecentSearch>&)
{
    notImplemented();
}

bool
SearchPopupMenuWKC::enabled()
{
    notImplemented();
    return false;
}

}
