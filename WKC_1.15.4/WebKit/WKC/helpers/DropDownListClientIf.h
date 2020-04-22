/*
 * WKCDropDownListClient.h
 *
 * Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCDropDownListClient_h
#define WKCDropDownListClient_h

#include <wkc/wkcbase.h>

namespace WKC {

class FrameView;
class PopupMenuClient;

/*@{*/

/** @brief Class that notifies a drop-down list */
class WKC_API DropDownListClientIf {
public:
    /**
       @brief Destructor
       @details
       Destructor
    */
    virtual ~DropDownListClientIf() {}

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual void dropDownListDestroyed() = 0;
    /**
       @brief Notifies of displaying drop-down list
       @param r Coordinates of drop-down list
       @param view Pointer to FrameView
       @param index Index of selected item
       @param client Pointer to PopupMenuClient
       @return None
       @details
       Gets information required for generating a drop-down list through WKC::PopupMenuClient.@n
    */
    virtual void show(const WKCRect& r, WKC::FrameView* view, int index, WKC::PopupMenuClient *client) = 0;
    /**
       @brief Notifies of hiding drop-down list
       @param client Pointer to PopupMenuClient
       @return None
       @details
       Notification is given when the select element is deleted due to page transition or DOM operation while the drop-down list is displayed.@n
    */
    virtual void hide(WKC::PopupMenuClient *client) = 0;
    /**
       @brief Notifies of drop-down list state change
       @param client Pointer to PopupMenuClient
       @return None
       @details
       Notification is given when the select element details are changed due to DOM operation while the drop-down list is displayed.@n
    */
    virtual void updateFromElement(WKC::PopupMenuClient *client) = 0;
};

/*@}*/

} // namespace

#endif // WKCDropDownListClient_h
