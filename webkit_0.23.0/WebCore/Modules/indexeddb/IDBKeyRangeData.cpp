/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBKeyRangeData.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

IDBKeyRangeData IDBKeyRangeData::isolatedCopy() const
{
    IDBKeyRangeData result;

    result.isNull = isNull;
    result.lowerKey = lowerKey.isolatedCopy();
    result.upperKey = upperKey.isolatedCopy();
    result.lowerOpen = lowerOpen;
    result.upperOpen = upperOpen;

    return result;
}

PassRefPtr<IDBKeyRange> IDBKeyRangeData::maybeCreateIDBKeyRange() const
{
    if (isNull)
        return nullptr;

    return IDBKeyRange::create(lowerKey.maybeCreateIDBKey(), upperKey.maybeCreateIDBKey(), lowerOpen ? IDBKeyRange::LowerBoundOpen : IDBKeyRange::LowerBoundClosed, upperOpen ? IDBKeyRange::UpperBoundOpen : IDBKeyRange::UpperBoundClosed);
}

bool IDBKeyRangeData::isExactlyOneKey() const
{
    if (isNull || lowerOpen || upperOpen)
        return false;

    return !lowerKey.compare(upperKey);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
