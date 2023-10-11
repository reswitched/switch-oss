/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"
#include "TreeScopeAdopter.h"

#include "Attr.h"
#include "ElementRareData.h"
#include "NodeTraversal.h"

namespace WebCore {

// FIXME: Do we ever change tree scopes except between documents?
void TreeScopeAdopter::moveTreeToNewScope(Node* root) const
{
    ASSERT(needsScopeChange());

    // If an element is moved from a document and then eventually back again the collection cache for
    // that element may contain stale data as changes made to it will have updated the DOMTreeVersion
    // of the document it was moved to. By increasing the DOMTreeVersion of the donating document here
    // we ensure that the collection cache will be invalidated as needed when the element is moved back.
    Document& oldDocument = m_oldScope.documentScope();
    Document& newDocument = m_newScope.documentScope();
    bool willMoveToNewDocument = &oldDocument != &newDocument;
    if (willMoveToNewDocument) {
        oldDocument.incrementReferencingNodeCount();
        oldDocument.incDOMTreeVersion();
    }

    for (Node* node = root; node; node = NodeTraversal::next(*node, root)) {
        updateTreeScope(node);

        if (willMoveToNewDocument)
            moveNodeToNewDocument(node, &oldDocument, &newDocument);
        else if (node->hasRareData()) {
            NodeRareData* rareData = node->rareData();
            if (rareData->nodeLists())
                rareData->nodeLists()->adoptTreeScope();
        }

        if (!is<Element>(*node))
            continue;

        if (node->hasSyntheticAttrChildNodes()) {
            const Vector<RefPtr<Attr>>& attrs = downcast<Element>(*node).attrNodeList();
            for (unsigned i = 0; i < attrs.size(); ++i)
                moveTreeToNewScope(attrs[i].get());
        }

        if (ShadowRoot* shadow = node->shadowRoot()) {
            shadow->setParentTreeScope(&m_newScope);
            if (willMoveToNewDocument)
                moveShadowTreeToNewDocument(shadow, &oldDocument, &newDocument);
        }
    }

    if (willMoveToNewDocument)
        oldDocument.decrementReferencingNodeCount();
}

void TreeScopeAdopter::moveShadowTreeToNewDocument(ShadowRoot* shadowRoot, Document* oldDocument, Document* newDocument) const
{
    for (Node* node = shadowRoot; node; node = NodeTraversal::next(*node, shadowRoot)) {
        moveNodeToNewDocument(node, oldDocument, newDocument);
        if (ShadowRoot* shadow = node->shadowRoot())
            moveShadowTreeToNewDocument(shadow, oldDocument, newDocument);
    }
}

#ifndef NDEBUG
#if !PLATFORM(WKC)
static bool didMoveToNewDocumentWasCalled = false;
static Document* oldDocumentDidMoveToNewDocumentWasCalledWith = nullptr;
#else
WKC_DEFINE_GLOBAL_BOOL(didMoveToNewDocumentWasCalled, false);
WKC_DEFINE_GLOBAL_PTR(Document*, oldDocumentDidMoveToNewDocumentWasCalledWith, 0);
#endif

void TreeScopeAdopter::ensureDidMoveToNewDocumentWasCalled(Document* oldDocument)
{
    ASSERT(!didMoveToNewDocumentWasCalled);
    ASSERT_UNUSED(oldDocument, oldDocument == oldDocumentDidMoveToNewDocumentWasCalledWith);
    didMoveToNewDocumentWasCalled = true;
}
#endif

inline void TreeScopeAdopter::updateTreeScope(Node* node) const
{
    ASSERT(!node->isTreeScope());
    ASSERT(&node->treeScope() == &m_oldScope);
    node->setTreeScope(m_newScope);
}

inline void TreeScopeAdopter::moveNodeToNewDocument(Node* node, Document* oldDocument, Document* newDocument) const
{
    ASSERT(!node->inDocument() || oldDocument != newDocument);

    newDocument->incrementReferencingNodeCount();
    oldDocument->decrementReferencingNodeCount();

    if (node->hasRareData()) {
        NodeRareData* rareData = node->rareData();
        if (rareData->nodeLists())
            rareData->nodeLists()->adoptDocument(oldDocument, newDocument);
    }

    if (oldDocument)
        oldDocument->moveNodeIteratorsToNewDocument(node, newDocument);

    if (is<ShadowRoot>(*node))
        downcast<ShadowRoot>(*node).setDocumentScope(newDocument);

#ifndef NDEBUG
    didMoveToNewDocumentWasCalled = false;
    oldDocumentDidMoveToNewDocumentWasCalledWith = oldDocument;
#endif

    node->didMoveToNewDocument(oldDocument);
    ASSERT(didMoveToNewDocumentWasCalled);
}

}
