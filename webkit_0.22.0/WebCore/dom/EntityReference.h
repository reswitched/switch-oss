/*
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef EntityReference_h
#define EntityReference_h

#include "ContainerNode.h"

namespace WebCore {

class EntityReference final : public ContainerNode {
public:
    static Ref<EntityReference> create(Document&, const String& entityName);

private:
    EntityReference(Document&, const String& entityName);

    virtual String nodeName() const override;
    virtual NodeType nodeType() const override;
    virtual Ref<Node> cloneNodeInternal(Document&, CloningOperation) override;

    String m_entityName;
};

} //namespace

#endif
