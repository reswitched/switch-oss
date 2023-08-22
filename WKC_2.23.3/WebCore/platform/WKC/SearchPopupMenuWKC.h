/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef SearchPopupMenuWKC_h
#define SearchPopupMenuWKC_h

#include "SearchPopupMenu.h"

namespace WebCore {

class PopupMenuClient;
class PopupMenuWKC;

class SearchPopupMenuWKC : public SearchPopupMenu
{
public:
    SearchPopupMenuWKC(PopupMenuClient*);
    virtual ~SearchPopupMenuWKC() override final;
    virtual PopupMenu* popupMenu() override final;
    virtual void saveRecentSearches(const AtomString& name, const Vector<RecentSearch>& searchItems) override final;
    virtual void loadRecentSearches(const AtomString& name, Vector<RecentSearch>& searchItems) override final;
    virtual bool enabled() override final;

private:
    RefPtr<PopupMenuWKC> m_popupMenu;
};

} // namespace

#endif // SearchPopupMenuWKC_h
