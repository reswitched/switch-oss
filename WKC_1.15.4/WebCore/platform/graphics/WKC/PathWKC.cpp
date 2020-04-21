/*
 *  Copyright (C) 2007-2009 Torch Mobile, Inc.
 *  Copyright (c) 2010-2012 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if !USE(WKC_CAIRO)

#include "Path.h"

#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Vector.h"
#include "PlatformPathWKC.h"
#include "WTFString.h"
#include "StrokeStyleApplier.h"
#include "TransformationMatrix.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

Path::Path()
    : m_path(new PlatformPathWKC())
{
}

Path::Path(const Path& other)
    : m_path(new PlatformPathWKC((const PlatformPathWKC *)other.m_path))
{
}

Path::~Path()
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    delete path;
}

Path& Path::operator=(const Path& other)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    if (&other != this) {
        delete path;
        m_path = new PlatformPathWKC((PlatformPathWKC *)other.m_path);
    }
    return *this;
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    return path->contains(point, rule);
}

void Path::translate(const FloatSize& size)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->translate(size);
}

FloatRect Path::boundingRect() const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    return path->boundingRect();
}

void Path::moveTo(const FloatPoint& point)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->moveTo(point);
}

void Path::addLineTo(const FloatPoint& point)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addLineTo(point);
}

void Path::addQuadCurveTo(const FloatPoint& cp, const FloatPoint& p)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addQuadCurveTo(cp, p);
}

void Path::addBezierCurveTo(const FloatPoint& cp1, const FloatPoint& cp2, const FloatPoint& p)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addBezierCurveTo(cp1, cp2, p);
}

void Path::addArcTo(const FloatPoint& p1, const FloatPoint& p2, float radius)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addArcTo(p1, p2, radius);
}

void Path::closeSubpath()
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->closeSubpath();
}

void Path::addArc(const FloatPoint& p, float r, float sar, float ear, bool anticlockwise)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addEllipse(p, r, r, sar, ear, anticlockwise);
}

void Path::addRect(const FloatRect& r)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addRect(r);
}

void Path::addEllipse(const FloatRect& r)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->addEllipse(r);
}

void Path::clear()
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->clear();
}

bool Path::isEmpty() const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    return path->isEmpty();
}

void Path::apply(void* info, PathApplierFunction function) const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->apply(info, function);
}

void Path::transform(const AffineTransform& t)
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    path->transform(t);
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;

    GraphicsContext gc(0);
    applier->strokeStyle(&gc);

    return path->boundingRectWithStyle(&gc);
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;

    GraphicsContext gc(0);
    applier->strokeStyle(&gc);

    return path->strokeContains(&gc, point);
}

bool Path::hasCurrentPoint() const
{
    // Not sure if this is correct. At the meantime, we do what other ports
    // do.
    // See https://bugs.webkit.org/show_bug.cgi?id=27266,
    // https://bugs.webkit.org/show_bug.cgi?id=27187, and 
    // http://trac.webkit.org/changeset/45873
    return !isEmpty();
}

FloatPoint Path::currentPoint() const
{
    PlatformPathWKC* path = (PlatformPathWKC *)m_path;
    return path->lastPoint();
}

}

#endif // USE(WKC_CAIRO)
