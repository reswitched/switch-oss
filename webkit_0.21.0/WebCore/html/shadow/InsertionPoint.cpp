/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InsertionPoint.h"

#include "QualifiedName.h"
#include "StaticNodeList.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

InsertionPoint::InsertionPoint(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document, CreateInsertionPoint)
    , m_hasDistribution(false)
{
}

InsertionPoint::~InsertionPoint()
{
}

bool InsertionPoint::shouldUseFallbackElements() const
{
    return isActive() && !hasDistribution();
}

bool InsertionPoint::isActive() const
{
    if (!containingShadowRoot())
        return false;
    const Node* node = parentNode();
    while (node) {
        if (node->isInsertionPoint())
            return false;

        node = node->parentNode();
    }
    return true;
}

bool InsertionPoint::rendererIsNeeded(const RenderStyle& style)
{
    return !isActive() && HTMLElement::rendererIsNeeded(style);
}

void InsertionPoint::childrenChanged(const ChildChange& change)
{
    HTMLElement::childrenChanged(change);
    if (ShadowRoot* root = containingShadowRoot())
        root->invalidateDistribution();
}

Node::InsertionNotificationRequest InsertionPoint::insertedInto(ContainerNode& insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);

    if (ShadowRoot* root = containingShadowRoot()) {
        root->distributor().didShadowBoundaryChange(root->hostElement());
        root->distributor().invalidateInsertionPointList();
    }

    return InsertionDone;
}

void InsertionPoint::removedFrom(ContainerNode& insertionPoint)
{
    ShadowRoot* root = containingShadowRoot();
    if (!root)
        root = insertionPoint.containingShadowRoot();

    if (root && root->hostElement()) {
        root->invalidateDistribution();
        root->distributor().invalidateInsertionPointList();
    }

    // Since this insertion point is no longer visible from the shadow subtree, it need to clean itself up.
    clearDistribution();

    HTMLElement::removedFrom(insertionPoint);
}
    
Node* InsertionPoint::firstDistributed() const
{
    if (!m_hasDistribution)
        return 0;
    for (Node* current = shadowHost()->firstChild(); current; current = current->nextSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

Node* InsertionPoint::lastDistributed() const
{
    if (!m_hasDistribution)
        return 0;
    for (Node* current = shadowHost()->lastChild(); current; current = current->previousSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

Node* InsertionPoint::nextDistributedTo(const Node* node) const
{
    for (Node* current = node->nextSibling(); current; current = current->nextSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

Node* InsertionPoint::previousDistributedTo(const Node* node) const
{
    for (Node* current = node->previousSibling(); current; current = current->previousSibling()) {
        if (matchTypeFor(current) == InsertionPoint::AlwaysMatches)
            return current;
    }
    return 0;
}

InsertionPoint* findInsertionPointOf(const Node* projectedNode)
{
    if (ShadowRoot* shadowRoot = shadowRootOfParentForDistribution(projectedNode)) {
        if (ShadowRoot* root = projectedNode->containingShadowRoot())
            ContentDistributor::ensureDistribution(root);
        return shadowRoot->distributor().findInsertionPointFor(projectedNode);
    }
    return 0;
}

} // namespace WebCore
