/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 *
 */

#include "config.h"
#include "ClientRectList.h"

#include "ExceptionCode.h"
#include "ClientRect.h"

namespace WebCore {

ClientRectList::ClientRectList()
{
}

ClientRectList::ClientRectList(const Vector<FloatQuad>& quads)
{
    m_list.reserveInitialCapacity(quads.size());
    for (size_t i = 0; i < quads.size(); ++i)
        m_list.append(ClientRect::create(quads[i].enclosingBoundingBox()));
}

ClientRectList::~ClientRectList()
{
}

unsigned ClientRectList::length() const
{
    return m_list.size();
}

ClientRect* ClientRectList::item(unsigned index)
{
    if (index >= m_list.size()) {
        // FIXME: this should throw an exception.
        // ec = INDEX_SIZE_ERR;
        return nullptr;
    }

    return m_list[index].ptr();
}

} // namespace WebCore
