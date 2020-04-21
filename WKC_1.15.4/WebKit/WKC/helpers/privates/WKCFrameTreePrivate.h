/*
 *  Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_FRAMETREE_H_
#define _WKC_HELPERS_PRIVATE_FRAMETREE_H_

#include "helpers/WKCFrameTree.h"

namespace WebCore {
class FrameTree;
} // namespace

namespace WKC {

class Frame;
class FramePrivate;

class FrameTreeWrap : public FrameTree {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FrameTreeWrap(FrameTreePrivate& parent) : FrameTree(parent) {}
    ~FrameTreeWrap() {}
};

class FrameTreePrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FrameTreePrivate(WebCore::FrameTree*);
    ~FrameTreePrivate();

    WebCore::FrameTree* webcore() const { return m_webcore; }
    FrameTree& wkc() { return m_wkc; }

    Frame* top();
    Frame* parent();

    Frame* traverseNext(const Frame* stayWithin = 0);

private:
    WebCore::FrameTree* m_webcore;
    FrameTreeWrap m_wkc;

    FramePrivate* m_top;
    FramePrivate* m_parent;
    FramePrivate* m_frame;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_FRAMETREE_H_

