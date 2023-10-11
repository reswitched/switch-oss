/*
 * Copyright (c) 2011-2022 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCRenderObject.h"
#include "helpers/privates/WKCRenderObjectPrivate.h"
#include "helpers/privates/WKCRenderStylePrivate.h"

#include "RenderObject.h"
#include "RenderElement.h"
#include "RenderStyle.h"

namespace WKC {
RenderObjectPrivate::RenderObjectPrivate(WebCore::RenderObject* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_renderStyle(0)
    , m_parent(0)
{
}

RenderObjectPrivate::~RenderObjectPrivate()
{
    delete m_renderStyle;
    delete m_parent;
}
 
bool
RenderObjectPrivate::isTextControl() const
{
    return m_webcore->isTextControl();
}

bool
RenderObjectPrivate::isTextArea() const
{
    return m_webcore->isTextArea();
}

bool
RenderObjectPrivate::isInline() const
{
    return m_webcore->isInline();
}

bool
RenderObjectPrivate::isRenderFullScreen() const
{
    return m_webcore->isRenderFullScreen();
}

WKCRect
RenderObjectPrivate::absoluteBoundingBoxRect(bool usetransform)
{
    return m_webcore->absoluteBoundingBoxRect(usetransform);
}

RenderObject*
RenderObjectPrivate::parent()
{
    WebCore::RenderObject* parent = m_webcore->parent();
    if (!parent)
        return 0;
    if (!m_parent || m_parent->webcore() != parent) {
        delete m_parent;
        m_parent = new RenderObjectPrivate(parent);
    }
    return &m_parent->wkc();
}

void
RenderObjectPrivate::focusRingRects(WTF::Vector<WKCFloatRect>& rects)
{
    WTF::Vector<WebCore::LayoutRect> layout_rects;
    m_webcore->focusRingRects(layout_rects);
    const size_t size = layout_rects.size();
    rects.grow(size);
    for (int i = 0; i < size; i++) {
        rects[i] = WebCore::FloatRect(layout_rects[i]);
    }
}

WKCFloatRect
RenderObjectPrivate::absoluteClippedOverflowRect()
{
    return WebCore::FloatRect(m_webcore->absoluteClippedOverflowRect());
}

bool
RenderObjectPrivate::hasOutline() const
{
    return m_webcore->container()->hasOutline();
}

RenderStyle*
RenderObjectPrivate::style()
{
    const WebCore::RenderStyle& renderStyle = m_webcore->style();
    if (!m_renderStyle || m_renderStyle->webcore() != &renderStyle) {
        delete m_renderStyle;
        m_renderStyle = new RenderStylePrivate(const_cast<WebCore::RenderStyle*>(&renderStyle));
    }
    return &m_renderStyle->wkc();
}
 
////////////////////////////////////////////////////////////////////////////////

RenderObject::RenderObject(RenderObjectPrivate& parent)
    : m_private(parent)
{
}

RenderObject::~RenderObject()
{
}


bool
RenderObject::isTextControl() const
{
    return m_private.isTextControl();
}

bool
RenderObject::isTextArea() const
{
    return m_private.isTextArea();
}

bool
RenderObject::isInline() const
{
    return m_private.isInline();
}

bool
RenderObject::isRenderFullScreen() const
{
    return m_private.isRenderFullScreen();
}

WKCRect
RenderObject::absoluteBoundingBoxRect(bool usetransform) const
{
    return m_private.absoluteBoundingBoxRect(usetransform);
}

RenderObject*
RenderObject::parent() const
{
    return m_private.parent();
}

WKCRingRects *
RenderObject::focusRingRects() const
{
    WTF::Vector<WKCFloatRect> core_rects;
    m_private.focusRingRects(core_rects);
    WKCRingRectsPrivate *p = new WKCRingRectsPrivate(core_rects);
    void* b = WTF::fastMalloc(sizeof(WKCRingRects));
    return new (b) WKCRingRects(p);
}

WKCFloatRect
RenderObject::absoluteClippedOverflowRect()
{
    return m_private.absoluteClippedOverflowRect();
}

bool
RenderObject::hasOutline() const
{
    return m_private.hasOutline();
}

RenderStyle*
RenderObject::style() const
{
    return m_private.style();
}


WKCRingRectsPrivate::WKCRingRectsPrivate(WTF::Vector<WKCFloatRect>& rects)
    : m_rects(new WTF::Vector<WKCFloatRect>(rects))
{
}
WKCRingRectsPrivate::~WKCRingRectsPrivate()
{
    delete m_rects;
}
int
WKCRingRectsPrivate::length() const
{
    return m_rects->size();
}
WKCFloatRect
WKCRingRectsPrivate::getAt(int i) const
{
    return m_rects->at(i);
}

void
WKCRingRects::destroy(WKCRingRects *self)
{
    if (!self)
        return;
    self->~WKCRingRects();
    WTF::fastFree(self);
}

WKCRingRects::WKCRingRects(WKCRingRectsPrivate* p)
    : m_private(p)
{
}
WKCRingRects::~WKCRingRects()
{
    delete m_private;
}
int
WKCRingRects::length() const
{
    return m_private->length();
}
WKCFloatRect
WKCRingRects::getAt(int i) const
{
    return m_private->getAt(i);
}

} // namespace
