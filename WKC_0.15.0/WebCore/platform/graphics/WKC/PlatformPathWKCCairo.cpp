/*
 * Copyright (c) 2012 ACCESS CO., LTD. All rights reserved.
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
 */

#include "config.h"

#if USE(WKC_CAIRO)
#include "PlatformPathCairo.h"

#include <cairo.h>

#include <wkc/wkcgpeer.h>

namespace WebCore {

CairoPath::CairoPath()
    : m_cr(0)
{
    WKC_DEFINE_STATIC_PTR(cairo_surface_t*, gPathSurface, cairo_image_surface_create(CAIRO_FORMAT_A8, 1,1));

    m_cr = adoptRef(cairo_create(gPathSurface));

    WKC_CAIRO_ADD_OBJECT(m_cr.get(), cairo);
}

}
#endif
