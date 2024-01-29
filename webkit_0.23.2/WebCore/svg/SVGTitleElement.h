/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGTitleElement_h
#define SVGTitleElement_h

#include "SVGElement.h"
#include "SVGNames.h"

namespace WebCore {

class SVGTitleElement final : public SVGElement {
public:
    static Ref<SVGTitleElement> create(const QualifiedName&, Document&);

private:
    SVGTitleElement(const QualifiedName&, Document&);

    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;
    virtual void removedFrom(ContainerNode&) override;
    virtual void childrenChanged(const ChildChange&) override;

    virtual bool rendererIsNeeded(const RenderStyle&) override { return false; }
};

} // namespace WebCore

#endif
