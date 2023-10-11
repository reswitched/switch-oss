/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013, 2014 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GridCoordinate_h
#define GridCoordinate_h

#if ENABLE(CSS_GRID_LAYOUT)

#include "GridResolvedPosition.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Recommended maximum size for both explicit and implicit grids.
const unsigned kGridMaxTracks = 1000000;

// A span in a single direction (either rows or columns). Note that |resolvedInitialPosition|
// and |resolvedFinalPosition| are grid areas' indexes, NOT grid lines'. Iterating over the
// span should include both |resolvedInitialPosition| and |resolvedFinalPosition| to be correct.
class GridSpan {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    GridSpan(const GridResolvedPosition& resolvedInitialPosition, const GridResolvedPosition& resolvedFinalPosition)
        : resolvedInitialPosition(std::min(resolvedInitialPosition.toInt(), kGridMaxTracks - 1))
        , resolvedFinalPosition(std::min(resolvedFinalPosition.toInt(), kGridMaxTracks))
    {
        ASSERT(resolvedInitialPosition <= resolvedFinalPosition);
    }

    bool operator==(const GridSpan& o) const
    {
        return resolvedInitialPosition == o.resolvedInitialPosition && resolvedFinalPosition == o.resolvedFinalPosition;
    }

    unsigned integerSpan() const
    {
        return resolvedFinalPosition.toInt() - resolvedInitialPosition.toInt() + 1;
    }

    GridResolvedPosition resolvedInitialPosition;
    GridResolvedPosition resolvedFinalPosition;

    typedef GridResolvedPosition iterator;

    iterator begin() const
    {
        return resolvedInitialPosition;
    }

    iterator end() const
    {
        return resolvedFinalPosition.next();
    }
};

// This represents a grid area that spans in both rows' and columns' direction.
class GridCoordinate {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    // HashMap requires a default constuctor.
    GridCoordinate()
        : columns(0, 0)
        , rows(0, 0)
    {
    }

    GridCoordinate(const GridSpan& r, const GridSpan& c)
        : columns(c)
        , rows(r)
    {
    }

    bool operator==(const GridCoordinate& o) const
    {
        return columns == o.columns && rows == o.rows;
    }

    bool operator!=(const GridCoordinate& o) const
    {
        return !(*this == o);
    }

    GridSpan columns;
    GridSpan rows;
};

typedef HashMap<String, GridCoordinate> NamedGridAreaMap;

} // namespace WebCore

#endif /* ENABLE(CSS_GRID_LAYOUT) */

#endif // GridCoordinate_h
