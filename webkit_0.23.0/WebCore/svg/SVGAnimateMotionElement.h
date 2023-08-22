/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef SVGAnimateMotionElement_h
#define SVGAnimateMotionElement_h
#include "Path.h"
#include "SVGAnimationElement.h"

namespace WebCore {

class AffineTransform;
            
class SVGAnimateMotionElement final : public SVGAnimationElement {
public:
    static Ref<SVGAnimateMotionElement> create(const QualifiedName&, Document&);
    void updateAnimationPath();

private:
    SVGAnimateMotionElement(const QualifiedName&, Document&);

    virtual bool hasValidAttributeType() override;
    virtual bool hasValidAttributeName() override;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;

    virtual void resetAnimatedType() override;
    virtual void clearAnimatedType(SVGElement* targetElement) override;
    virtual bool calculateToAtEndOfDurationValue(const String& toAtEndOfDurationString) override;
    virtual bool calculateFromAndToValues(const String& fromString, const String& toString) override;
    virtual bool calculateFromAndByValues(const String& fromString, const String& byString) override;
    virtual void calculateAnimatedValue(float percentage, unsigned repeatCount, SVGSMILElement* resultElement) override;
    virtual void applyResultsToTarget() override;
    virtual float calculateDistance(const String& fromString, const String& toString) override;

    enum RotateMode {
        RotateAngle,
        RotateAuto,
        RotateAutoReverse
    };
    RotateMode rotateMode() const;
    void buildTransformForProgress(AffineTransform*, float percentage);

    bool m_hasToPointAtEndOfDuration;

    virtual void updateAnimationMode() override;

    // Note: we do not support percentage values for to/from coords as the spec implies we should (opera doesn't either)
    FloatPoint m_fromPoint;
    FloatPoint m_toPoint;
    FloatPoint m_toPointAtEndOfDuration;

    Path m_path;
    Path m_animationPath;
};
    
} // namespace WebCore

#endif // SVGAnimateMotionElement_h
