/*
 *  Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_POPUPMENUCLIENT_H_
#define _WKC_HELPERS_PRIVATE_POPUPMENUCLIENT_H_

#include "helpers/WKCPopupMenuClient.h"

namespace WebCore {
class PopupMenuClient;
} // namespace

namespace WKC {

class String;

class PopupMenuClientWrap : public PopupMenuClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PopupMenuClientWrap(PopupMenuClientPrivate& parent) : PopupMenuClient(parent) {}
    ~PopupMenuClientWrap() {}
};

class PopupMenuClientPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PopupMenuClientPrivate(WebCore::PopupMenuClient*);
    ~PopupMenuClientPrivate();

    WebCore::PopupMenuClient* webcore() const { return m_webcore; }
    PopupMenuClient& wkc() { return m_wkc; }

    void valueChanged(unsigned listIndex, bool fireEvents = true);

    String itemText(unsigned listIndex) const;
    String itemToolTip(unsigned listIndex) const;
    bool itemIsEnabled(unsigned listIndex) const;
    bool itemIsEnabledRespectingGroup(unsigned listIndex) const; // Custom API
    int menuStyle_font_height() const;
    int clientInsetLeft() const;
    int clientInsetRight() const;
    int clientPaddingLeft() const;
    int clientPaddingRight() const;
    int listSize() const;
    int selectedIndex() const;
    void popupDidHide();
    bool itemIsSeparator(unsigned listIndex) const;
    bool itemIsLabel(unsigned listIndex) const;
    bool itemIsSelected(unsigned listIndex) const;
    bool shouldPopOver() const;
    bool valueShouldChangeOnHotTrack() const;
    void setTextFromItem(unsigned listIndex);
    void listBoxSelectItem(int listIndex, bool allowMultiplySelections, bool shift, bool fireOnChangeNow = true);
    bool multiple();

private:
    WebCore::PopupMenuClient* m_webcore;
    PopupMenuClientWrap m_wkc;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_POPUPMENUCLIENT_H_

