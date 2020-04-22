/*
 * Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "helpers/WKCFrameTree.h"
#include "helpers/privates/WKCFrameTreePrivate.h"

#include "Frame.h"
#include "FrameTree.h"

#include "helpers/privates/WKCFramePrivate.h"

namespace WKC {

FrameTreePrivate::FrameTreePrivate(WebCore::FrameTree* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_top(0)
    , m_parent(0)
    , m_frame(0)
{
}

FrameTreePrivate::~FrameTreePrivate()
{
    delete m_top;
    delete m_parent;
    delete m_frame;
}

Frame*
FrameTreePrivate::top()
{
    const WebCore::Frame* f = &m_webcore->top();
    if (!f)
        return 0;
    if (!m_top || m_top->webcore()!=f) {
        delete m_top;
        m_top = new FramePrivate(f);
    }
    return &m_top->wkc();
}

Frame*
FrameTreePrivate::parent()
{
    WebCore::Frame* f = m_webcore->parent();
    if (!f)
        return 0;
    if (!m_parent || m_parent->webcore()!=f) {
        delete m_parent;
        m_parent = new FramePrivate(f);
    }
    return &m_parent->wkc();
}

Frame*
FrameTreePrivate::traverseNext(const Frame* stayWithin)
{
    WebCore::Frame* f = m_webcore->traverseNext(stayWithin ? const_cast<Frame*>(stayWithin)->priv().webcore() : 0);
    if (!f)
        return 0;
    if (!m_frame || m_frame->webcore()!=f) {
        delete m_frame;
        m_frame = new FramePrivate(f);
    }
    return &m_frame->wkc();
}
 
////////////////////////////////////////////////////////////////////////////////

FrameTree::FrameTree(FrameTreePrivate& parent)
    : m_private(parent)
{
}

FrameTree::~FrameTree()
{
}

Frame*
FrameTree::top() const
{
    return m_private.top();
}

Frame*
FrameTree::parent() const
{
    return m_private.parent();
}

Frame*
FrameTree::traverseNext(const Frame* stayWithin) const
{
    return m_private.traverseNext(stayWithin);
}

} // namespace
