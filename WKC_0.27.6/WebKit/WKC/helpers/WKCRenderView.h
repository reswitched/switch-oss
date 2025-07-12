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

#ifndef _WKC_HELPERS_WKC_RENDERVIEW_H_
#define _WKC_HELPERS_WKC_RENDERVIEW_H_

#include <wkc/wkcbase.h>

namespace WKC {

class RenderLayer;
class RenderViewPrivate;

class WKC_API RenderView {
public:
    void setNeedsLayout(bool);
    WKCRect documentRect() const;

    RenderLayer* layer() const;

    RenderViewPrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    RenderView(RenderViewPrivate&);
    ~RenderView();

private:
    RenderView(const RenderView&);
    RenderView& operator=(const RenderView&);

    RenderViewPrivate& m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_RENDERVIEW_H_
