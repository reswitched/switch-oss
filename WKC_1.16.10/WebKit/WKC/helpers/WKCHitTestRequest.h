/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_HITTESTREQUEST_H_
#define _WKC_HELPERS_WKC_HITTESTREQUEST_H_

#include <wkc/wkcbase.h>

namespace WKC {
class Node;
class HitTestRequestPrivate;

class WKC_API HitTestRequest {
public:
    enum RequestType {
        ReadOnly = 1 << 1,
        Active = 1 << 2,
        MouseMove = 1 << 3,
        MouseUp = 1 << 4,
        IgnoreClipping = 1 << 5
    };

    HitTestRequest(int);
    ~HitTestRequest();

    HitTestRequest(const HitTestRequest&);
    HitTestRequest& operator=(const HitTestRequest&);

    HitTestRequestPrivate* priv() const { return m_private; }

private:
    HitTestRequestPrivate* m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_HITTESTREQUEST_H_
