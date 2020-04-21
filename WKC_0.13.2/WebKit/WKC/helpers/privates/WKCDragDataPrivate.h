/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_DRAGDATA_H_
#define _WKC_HELPERS_PRIVATE_DRAGDATA_H_

#include "helpers/WKCDragData.h"

namespace WebCore {
class DragData;
} // namespace

namespace WKC {

class DragDataWrap : public DragData {
WTF_MAKE_FAST_ALLOCATED;
public:
    DragDataWrap(DragDataPrivate& parent) : DragData(parent) {}
    ~DragDataWrap() {}
};

class DragDataPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    DragDataPrivate(WebCore::DragData*);
    ~DragDataPrivate();

    WebCore::DragData* webcore() const { return m_webcore; }
    DragData& wkc() { return m_wkc; }

private:
    WebCore::DragData* m_webcore;
    DragDataWrap m_wkc;

};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_DRAGDATA_H_

