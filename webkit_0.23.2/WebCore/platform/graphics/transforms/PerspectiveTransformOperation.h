/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PerspectiveTransformOperation_h
#define PerspectiveTransformOperation_h

#include "Length.h"
#include "LengthFunctions.h"
#include "TransformOperation.h"
#include <wtf/Ref.h>

namespace WebCore {

class PerspectiveTransformOperation final : public TransformOperation {
public:
    static Ref<PerspectiveTransformOperation> create(const Length& p)
    {
        return adoptRef(*new PerspectiveTransformOperation(p));
    }

    virtual Ref<TransformOperation> clone() const override
    {
        return adoptRef(*new PerspectiveTransformOperation(m_p));
    }

    Length perspective() const { return m_p; }
    
private:
    virtual bool isIdentity() const override { return !floatValueForLength(m_p, 1); }
    virtual bool isAffectedByTransformOrigin() const override { return !isIdentity(); }

    virtual OperationType type() const override { return PERSPECTIVE; }
    virtual bool isSameType(const TransformOperation& o) const override { return o.type() == PERSPECTIVE; }

    virtual bool operator==(const TransformOperation&) const override;

    virtual bool apply(TransformationMatrix& transform, const FloatSize&) const override
    {
        transform.applyPerspective(floatValueForLength(m_p, 1));
        return false;
    }

    virtual Ref<TransformOperation> blend(const TransformOperation* from, double progress, bool blendToIdentity = false) override;

    PerspectiveTransformOperation(const Length& p)
        : m_p(p)
    {
        ASSERT(p.isFixed());
    }

    Length m_p;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_TRANSFORMOPERATION(WebCore::PerspectiveTransformOperation, type() == WebCore::TransformOperation::PERSPECTIVE)

#endif // PerspectiveTransformOperation_h
