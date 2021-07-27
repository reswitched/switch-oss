/*
 * Copyright (c) 2011-2017 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_FRAMELOADREQUEST_H_
#define _WKC_HELPERS_PRIVATE_FRAMELOADREQUEST_H_

#include "helpers/WKCFrameLoadRequest.h"

namespace WebCore {
struct FrameLoadRequest;
} // namespace


namespace WKC {

class FrameLoadRequestWrap : public FrameLoadRequest {
WTF_MAKE_FAST_ALLOCATED;
public:
    FrameLoadRequestWrap(FrameLoadRequestPrivate& parent) : FrameLoadRequest(parent) {}
    ~FrameLoadRequestWrap() {}
};

class FrameLoadRequestPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    FrameLoadRequestPrivate(const WebCore::FrameLoadRequest&);
    ~FrameLoadRequestPrivate();

    const WebCore::FrameLoadRequest& webcore() const { return m_webcore; }
    FrameLoadRequest& wkc() { return m_wkc; }

private:
    const WebCore::FrameLoadRequest& m_webcore;
    FrameLoadRequestWrap m_wkc;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_FRAMELOADREQUEST_H_
