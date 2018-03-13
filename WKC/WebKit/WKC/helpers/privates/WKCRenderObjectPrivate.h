/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_RENDEROBJECT_H_
#define _WKC_HELPERS_PRIVATE_RENDEROBJECT_H_

#include <wkc/wkcbase.h>
#include "helpers/WKCRenderObject.h"
#include "Vector.h"

namespace WebCore {
class RenderObject;
class RenderStyle;
} // namespace

namespace WKC {
class RenderStylePrivate;

class WKCRingRectsPrivate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WKCRingRectsPrivate(WTF::Vector<WKCRect>&);
    ~WKCRingRectsPrivate();
    int length() const;
    WKCRect getAt(int) const;
private:
    WKCRingRectsPrivate(const WKCRingRectsPrivate&);
    WKCRingRectsPrivate& operator=(const WKCRingRectsPrivate&);
private:
    WTF::Vector<WKCRect>* m_rects;
};
 
class RenderObjectWrap : public RenderObject {
WTF_MAKE_FAST_ALLOCATED;
public:
    RenderObjectWrap(RenderObjectPrivate& parent) : RenderObject(parent) {}
    ~RenderObjectWrap() {}
};

class RenderObjectPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    RenderObjectPrivate(WebCore::RenderObject*);
    ~RenderObjectPrivate();

    const WebCore::RenderObject* webcore() const { return m_webcore; }
    RenderObject& wkc() { return m_wkc; }

    bool isTextControl() const;
    bool isTextArea() const;
    bool isInline() const;
    bool isRenderFullScreen() const;
    WKCRect absoluteBoundingBoxRect(bool usetransform);
    void focusRingRects(WTF::Vector<WKCRect>&);
    WKCRect absoluteClippedOverflowRect();
    bool hasOutline() const;
    RenderStyle* style();
    RenderObject* parent();

private:
    WebCore::RenderObject* m_webcore;
    RenderObjectWrap m_wkc;
    RenderStylePrivate* m_renderStyle;
    RenderObjectPrivate* m_parent;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_RENDEROBJECT_H_
