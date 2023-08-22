/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGPathTraversalStateBuilder_h
#define SVGPathTraversalStateBuilder_h

#include "SVGPathConsumer.h"
#include "SVGPoint.h"

namespace WebCore {

class PathTraversalState;

class SVGPathTraversalStateBuilder : public SVGPathConsumer {
public:
    SVGPathTraversalStateBuilder();

    unsigned pathSegmentIndex() { return m_segmentIndex; }
    float totalLength();
    SVGPoint currentPoint();

    void setCurrentTraversalState(PathTraversalState* traversalState) { m_traversalState = traversalState; }
    void setDesiredLength(float);
    virtual void incrementPathSegmentCount() override { ++m_segmentIndex; }
    virtual bool continueConsuming() override;
    virtual void cleanup() override { m_traversalState = nullptr, m_segmentIndex = 0; }

private:
    // Used in UnalteredParsing/NormalizedParsing modes.
    virtual void moveTo(const FloatPoint&, bool closed, PathCoordinateMode) override;
    virtual void lineTo(const FloatPoint&, PathCoordinateMode) override;
    virtual void curveToCubic(const FloatPoint&, const FloatPoint&, const FloatPoint&, PathCoordinateMode) override;
    virtual void closePath() override;

private:
    // Not used for PathTraversalState.
    virtual void lineToHorizontal(float, PathCoordinateMode) override { ASSERT_NOT_REACHED(); }
    virtual void lineToVertical(float, PathCoordinateMode) override { ASSERT_NOT_REACHED(); }
    virtual void curveToCubicSmooth(const FloatPoint&, const FloatPoint&, PathCoordinateMode) override { ASSERT_NOT_REACHED(); }
    virtual void curveToQuadratic(const FloatPoint&, const FloatPoint&, PathCoordinateMode) override { ASSERT_NOT_REACHED(); }
    virtual void curveToQuadraticSmooth(const FloatPoint&, PathCoordinateMode) override { ASSERT_NOT_REACHED(); }
    virtual void arcTo(float, float, float, bool, bool, const FloatPoint&, PathCoordinateMode) override { ASSERT_NOT_REACHED(); }

    PathTraversalState* m_traversalState;
    unsigned m_segmentIndex;
};

} // namespace WebCore

#endif // SVGPathTraversalStateBuilder_h
