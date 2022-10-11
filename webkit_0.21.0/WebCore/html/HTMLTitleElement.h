/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2010 Apple Inc. All rights reserved.
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
#ifndef HTMLTitleElement_h
#define HTMLTitleElement_h

#include "HTMLElement.h"
#include "StringWithDirection.h"

namespace WebCore {

class HTMLTitleElement final : public HTMLElement {
public:
    static Ref<HTMLTitleElement> create(const QualifiedName&, Document&);

    String text() const;
    void setText(const String&);

    StringWithDirection textWithDirection();

private:
    HTMLTitleElement(const QualifiedName&, Document&);

    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;
    virtual void removedFrom(ContainerNode&) override;
    virtual void childrenChanged(const ChildChange&) override;

    StringWithDirection m_title;
};

} //namespace

#endif
