/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2003, 2006, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLBRElement_h
#define HTMLBRElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLBRElement final : public HTMLElement {
public:
    static Ref<HTMLBRElement> create(Document&);
    static Ref<HTMLBRElement> create(const QualifiedName&, Document&);

    virtual bool canContainRangeEndPoint() const override { return false; }

private:
    HTMLBRElement(const QualifiedName&, Document&);

    virtual bool isPresentationAttribute(const QualifiedName&) const override;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStyleProperties&) override;

    virtual RenderPtr<RenderElement> createElementRenderer(Ref<RenderStyle>&&, const RenderTreePosition&) override;
};

} // namespace

#endif
