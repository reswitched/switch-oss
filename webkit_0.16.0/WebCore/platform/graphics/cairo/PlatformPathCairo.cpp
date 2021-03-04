/*
 * Copyright (C) 2011 Collabora Ltd.
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
#include "PlatformPathCairo.h"

#if USE(CAIRO)

#include <cairo.h>

namespace WebCore {

#if USE(WKC_CAIRO)
static cairo_surface_t* getPathSurface() 
{
    WKC_DEFINE_STATIC_PTR(cairo_surface_t*, gPathSurface, cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1))
    return gPathSurface;
}
#else
static cairo_surface_t* getPathSurface() 
{
    return cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
}

static cairo_surface_t* gPathSurface = getPathSurface(); 
#endif

CairoPath::CairoPath()
#if USE(WKC_CAIRO)
    : m_cr(adoptRef(cairo_create(getPathSurface())))
#else
    : m_cr(adoptRef(cairo_create(gPathSurface)))
#endif
{
}

} // namespace WebCore

#endif // USE(CAIRO)
