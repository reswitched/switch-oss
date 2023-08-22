/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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
 *
 */

#ifndef ScaleTransformOperation_h
#define ScaleTransformOperation_h

#include "TransformOperation.h"
#include <wtf/Ref.h>

namespace WebCore {

class ScaleTransformOperation final : public TransformOperation {
public:
    static Ref<ScaleTransformOperation> create(double sx, double sy, OperationType type)
    {
        return adoptRef(*new ScaleTransformOperation(sx, sy, 1, type));
    }

    static Ref<ScaleTransformOperation> create(double sx, double sy, double sz, OperationType type)
    {
        return adoptRef(*new ScaleTransformOperation(sx, sy, sz, type));
    }

    virtual Ref<TransformOperation> clone() const override
    {
        return adoptRef(*new ScaleTransformOperation(m_x, m_y, m_z, m_type));
    }

    double x() const { return m_x; }
    double y() const { return m_y; }
    double z() const { return m_z; }

private:
    virtual bool isIdentity() const override { return m_x == 1 &&  m_y == 1 &&  m_z == 1; }
    virtual bool isAffectedByTransformOrigin() const override { return !isIdentity(); }

    virtual OperationType type() const override { return m_type; }
    virtual bool isSameType(const TransformOperation& o) const override { return o.type() == m_type; }

    virtual bool operator==(const TransformOperation&) const override;

    virtual bool apply(TransformationMatrix& transform, const FloatSize&) const override
    {
        transform.scale3d(m_x, m_y, m_z);
        return false;
    }

    virtual Ref<TransformOperation> blend(const TransformOperation* from, double progress, bool blendToIdentity = false) override;

    ScaleTransformOperation(double sx, double sy, double sz, OperationType type)
        : m_x(sx)
        , m_y(sy)
        , m_z(sz)
        , m_type(type)
    {
        ASSERT(isScaleTransformOperationType());
    }
        
    double m_x;
    double m_y;
    double m_z;
    OperationType m_type;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_TRANSFORMOPERATION(WebCore::ScaleTransformOperation, isScaleTransformOperationType())

#endif // ScaleTransformOperation_h
