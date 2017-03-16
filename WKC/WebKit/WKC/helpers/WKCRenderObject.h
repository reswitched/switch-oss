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

#ifndef _WKC_HELPERS_WKC_RENDEROBJECT_H_
#define _WKC_HELPERS_WKC_RENDEROBJECT_H_

#include <wkc/wkcbase.h>

namespace WKC {

class RenderStyle;
class WKCRingRectsPrivate;

class WKC_API WKCRingRects {
public:
    WKCRingRects(WKCRingRectsPrivate*);
    ~WKCRingRects();
    int length() const;
    WKCRect getAt(int i) const;
    static void destroy(WKCRingRects *self);
private:
    WKCRingRects(const WKCRingRects&);
    WKCRingRects& operator=(const WKCRingRects&);
private:
    WKCRingRectsPrivate* m_private;
};

class RenderObjectPrivate;

class WKC_API RenderObject {
public:
    bool isTextControl() const;
    bool isTextArea() const;
    WKCRect absoluteBoundingBoxRect(bool usetransform = false) const;
    WKCRingRects* focusRingRects() const;
    WKCRect absoluteClippedOverflowRect();
    bool hasOutline() const;
    RenderStyle* style() const;

    RenderObjectPrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    RenderObject(RenderObjectPrivate&);
    ~RenderObject();

private:
    RenderObject(const RenderObject&);
    RenderObject& operator=(const RenderObject&);

    RenderObjectPrivate& m_private;
};
}

#endif // _WKC_HELPERS_WKC_RENDEROBJECT_H_
