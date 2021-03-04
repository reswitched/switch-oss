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

#ifndef WKCBackForwardClient_h
#define WKCBackForwardClient_h

#include <wkc/wkcbase.h>

namespace WKC {

class HistoryItem;

/*@{*/

/** @brief Class that notifies of an event during navigating in history. In this description, only those functions that were extended by ACCESS for NetFront Browser NX are described, and those inherited from WebCore::BackForwardList are not described. */
class WKC_API BackForwardClientIf {
public:
    // from BackForwardList
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @endcond
    */
    virtual ~BackForwardClientIf() {}

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::HistoryItem* (TBD) implement description
       @endcond
    */
    virtual void addItem(WKC::HistoryItem*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::HistoryItem* (TBD) implement description
       @endcond
    */
    virtual void goToItem(WKC::HistoryItem*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param int (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual WKC::HistoryItem* itemAtIndex(int) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual int backListCount() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description 
       @endcond
    */
    virtual int forwardListCount() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @endcond
    */
    virtual void close() = 0;
};

/*@}*/

} // namespace

#endif // WKCBackForwardClient_h
