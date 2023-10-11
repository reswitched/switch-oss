/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderFieldset_h
#define RenderFieldset_h

#include "HTMLFieldSetElement.h"
#include "RenderBlockFlow.h"

namespace WebCore {

class HTMLFieldSetElement;

class RenderFieldset final : public RenderBlockFlow {
public:
    RenderFieldset(HTMLFieldSetElement&, Ref<RenderStyle>&&);

    enum FindLegendOption { IgnoreFloatingOrOutOfFlow, IncludeFloatingOrOutOfFlow };
    RenderBox* findLegend(FindLegendOption = IgnoreFloatingOrOutOfFlow) const;

    HTMLFieldSetElement& fieldSetElement() const { return downcast<HTMLFieldSetElement>(nodeForNonAnonymous()); }

private:
    void element() const  = delete;

    virtual const char* renderName() const override { return "RenderFieldSet"; }
    virtual bool isFieldset() const override { return true; }

    virtual RenderObject* layoutSpecialExcludedChild(bool relayoutChildren) override;

    virtual void computePreferredLogicalWidths() override;
    virtual bool avoidsFloats() const override { return true; }

    virtual void paintBoxDecorations(PaintInfo&, const LayoutPoint&) override;
    virtual void paintMask(PaintInfo&, const LayoutPoint&) override;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderFieldset, isFieldset())

#endif // RenderFieldset_h
