/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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
{
}

RenderObjectPrivate::~RenderObjectPrivate()
{
    delete m_renderStyle;
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

WKCRect
RenderObjectPrivate::absoluteBoundingBoxRect(bool usetransform)
{
    return m_webcore->absoluteBoundingBoxRect(usetransform);
}

void
RenderObjectPrivate::focusRingRects(WTF::Vector<WKCRect>& rects)
{
    WTF::Vector<WebCore::IntRect> int_rects;
    m_webcore->focusRingRects(int_rects);
    const size_t size = int_rects.size();
    rects.grow(size);
    for (int i = 0; i < size; i++) {
        rects[i] = int_rects[i];
    }
}

WKCRect
RenderObjectPrivate::absoluteClippedOverflowRect()
{
    return m_webcore->absoluteClippedOverflowRect();
}

bool
RenderObjectPrivate::hasOutline() const
{
    return m_webcore->hasOutline();
}

RenderStyle*
RenderObjectPrivate::style()
{
    WebCore::RenderStyle* renderStyle = &m_webcore->style();
    if (!m_renderStyle || m_renderStyle->webcore() != renderStyle) {
        delete m_renderStyle;
        m_renderStyle = new RenderStylePrivate(renderStyle);
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

WKCRect
RenderObject::absoluteBoundingBoxRect(bool usetransform) const
{
    return m_private.absoluteBoundingBoxRect(usetransform);
}

WKCRingRects *
RenderObject::focusRingRects() const
{
    WTF::Vector<WKCRect> core_rects;
    m_private.focusRingRects(core_rects);
    WKCRingRectsPrivate *p = new WKCRingRectsPrivate(core_rects);
    void* b = WTF::fastMalloc(sizeof(WKCRingRects));
    return new (b) WKCRingRects(p);
}

WKCRect
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


WKCRingRectsPrivate::WKCRingRectsPrivate(WTF::Vector<WKCRect>& rects)
    : m_rects(new WTF::Vector<WKCRect>(rects))
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
WKCRect
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
WKCRect
WKCRingRects::getAt(int i) const
{
    return m_private->getAt(i);
}

} // namespace
