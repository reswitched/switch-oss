/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2015 Apple Inc. All rights reserved.
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

#ifndef ContainerNode_h
#define ContainerNode_h

#include "CollectionType.h"
#include "ExceptionCodePlaceholder.h"
#include "Node.h"

namespace WebCore {

class HTMLCollection;
class NodeOrString;
class QualifiedName;
class RenderElement;

typedef void (*NodeCallback)(Node&, unsigned);

namespace Private { 
    template<class GenericNode, class GenericNodeContainer>
    void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer&);
};

class ContainerNode : public Node {
public:
    virtual ~ContainerNode();

    Node* firstChild() const { return m_firstChild; }
    static ptrdiff_t firstChildMemoryOffset() { return OBJECT_OFFSETOF(ContainerNode, m_firstChild); }
    Node* lastChild() const { return m_lastChild; }
    bool hasChildNodes() const { return m_firstChild; }
    bool hasOneChild() const { return m_firstChild && !m_firstChild->nextSibling(); }

    bool directChildNeedsStyleRecalc() const { return getFlag(DirectChildNeedsStyleRecalcFlag); }
    void setDirectChildNeedsStyleRecalc() { setFlag(DirectChildNeedsStyleRecalcFlag); }

    WEBCORE_EXPORT unsigned countChildNodes() const;
    WEBCORE_EXPORT Node* traverseToChildAt(unsigned) const;

    bool insertBefore(Ref<Node>&& newChild, Node* refChild, ExceptionCode& = ASSERT_NO_EXCEPTION);
    bool replaceChild(Ref<Node>&& newChild, Node& oldChild, ExceptionCode& = ASSERT_NO_EXCEPTION);
    WEBCORE_EXPORT bool removeChild(Node& child, ExceptionCode& = ASSERT_NO_EXCEPTION);
    WEBCORE_EXPORT bool appendChild(Ref<Node>&& newChild, ExceptionCode& = ASSERT_NO_EXCEPTION);

    // These methods are only used during parsing.
    // They don't send DOM mutation events or handle reparenting.
    // However, arbitrary code may be run by beforeload handlers.
    void parserAppendChild(PassRefPtr<Node>);
    void parserRemoveChild(Node&);
    void parserInsertBefore(PassRefPtr<Node> newChild, Node* refChild);

    void removeChildren();
    void takeAllChildrenFrom(ContainerNode*);

    void cloneChildNodes(ContainerNode& clone);

    enum ChildChangeType { ElementInserted, ElementRemoved, TextInserted, TextRemoved, TextChanged, AllChildrenRemoved, NonContentsChildRemoved, NonContentsChildInserted, AllChildrenReplaced };
    enum ChildChangeSource { ChildChangeSourceParser, ChildChangeSourceAPI };
    struct ChildChange {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
    public:
#endif
        ChildChangeType type;
        Element* previousSiblingElement;
        Element* nextSiblingElement;
        ChildChangeSource source;

        bool isInsertion() const
        {
            switch (type) {
            case ElementInserted:
            case TextInserted:
            case NonContentsChildInserted:
            case AllChildrenReplaced:
                return true;
            case ElementRemoved:
            case TextRemoved:
            case TextChanged:
            case AllChildrenRemoved:
            case NonContentsChildRemoved:
                return false;
            }
            ASSERT_NOT_REACHED();
            return false;
        }
    };
    virtual void childrenChanged(const ChildChange&);

    void disconnectDescendantFrames();

    using Node::setAttributeEventListener;
    void setAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value);

    RenderElement* renderer() const;

    // Return a bounding box in absolute coordinates enclosing this node and all its descendants.
    // This gives the area within which events may get handled by a hander registered on this node.
    virtual LayoutRect absoluteEventHandlerBounds(bool& /* includesFixedPositionElements */) { return LayoutRect(); }

    Element* querySelector(const String& selectors, ExceptionCode&);
    RefPtr<NodeList> querySelectorAll(const String& selectors, ExceptionCode&);

    RefPtr<NodeList> getElementsByTagName(const AtomicString&);
    RefPtr<NodeList> getElementsByTagNameNS(const AtomicString& namespaceURI, const AtomicString& localName);
    RefPtr<NodeList> getElementsByName(const String& elementName);
    RefPtr<NodeList> getElementsByClassName(const AtomicString& classNames);
    RefPtr<RadioNodeList> radioNodeList(const AtomicString&);

    // From the ParentNode interface - https://dom.spec.whatwg.org/#interface-parentnode
    Ref<HTMLCollection> children();
    Element* firstElementChild() const;
    Element* lastElementChild() const;
    unsigned childElementCount() const;
    void append(Vector<NodeOrString>&&, ExceptionCode&);
    void prepend(Vector<NodeOrString>&&, ExceptionCode&);

    bool ensurePreInsertionValidity(Node& newChild, Node* refChild, ExceptionCode&);

protected:
    explicit ContainerNode(Document&, ConstructionType = CreateContainer);

    template<class GenericNode, class GenericNodeContainer>
    friend void appendChildToContainer(GenericNode* child, GenericNodeContainer&);

    template<class GenericNode, class GenericNodeContainer>
    friend void Private::addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer&);

    void removeDetachedChildren();
    void setFirstChild(Node* child) { m_firstChild = child; }
    void setLastChild(Node* child) { m_lastChild = child; }

    Ref<HTMLCollection> ensureCachedHTMLCollection(CollectionType);
    HTMLCollection* cachedHTMLCollection(CollectionType);

private:
    void removeBetween(Node* previousChild, Node* nextChild, Node& oldChild);
    bool appendChildWithoutPreInsertionValidityCheck(Ref<Node>&&, ExceptionCode&);
    void insertBeforeCommon(Node& nextChild, Node& oldChild);

    void notifyChildInserted(Node& child, ChildChangeSource);
    void notifyChildRemoved(Node& child, Node* previousSibling, Node* nextSibling, ChildChangeSource);

    void updateTreeAfterInsertion(Node& child);

    bool isContainerNode() const  = delete;

    Node* m_firstChild;
    Node* m_lastChild;
};

inline ContainerNode::ContainerNode(Document& document, ConstructionType type)
    : Node(document, type)
    , m_firstChild(0)
    , m_lastChild(0)
{
}

inline unsigned Node::countChildNodes() const
{
    if (!is<ContainerNode>(*this))
        return 0;
    return downcast<ContainerNode>(*this).countChildNodes();
}

inline Node* Node::traverseToChildAt(unsigned index) const
{
    if (!is<ContainerNode>(*this))
        return nullptr;
    return downcast<ContainerNode>(*this).traverseToChildAt(index);
}

inline Node* Node::firstChild() const
{
    if (!is<ContainerNode>(*this))
        return nullptr;
    return downcast<ContainerNode>(*this).firstChild();
}

inline Node* Node::lastChild() const
{
    if (!is<ContainerNode>(*this))
        return nullptr;
    return downcast<ContainerNode>(*this).lastChild();
}

inline Node* Node::highestAncestor() const
{
    Node* node = const_cast<Node*>(this);
    Node* highest = node;
    for (; node; node = node->parentNode())
        highest = node;
    return highest;
}

inline bool Node::needsNodeRenderingTraversalSlowPath() const
{
    if (getFlag(NeedsNodeRenderingTraversalSlowPathFlag))
        return true;
    ContainerNode* parent = parentOrShadowHostNode();
    return parent && parent->getFlag(NeedsNodeRenderingTraversalSlowPathFlag);
}

inline bool Node::isTreeScope() const
{
    return &treeScope().rootNode() == this;
}

// This constant controls how much buffer is initially allocated
// for a Node Vector that is used to store child Nodes of a given Node.
// FIXME: Optimize the value.
const int initialNodeVectorSize = 11;
typedef Vector<Ref<Node>, initialNodeVectorSize> NodeVector;

inline void getChildNodes(Node& node, NodeVector& nodes)
{
    ASSERT(nodes.isEmpty());
    for (Node* child = node.firstChild(); child; child = child->nextSibling())
        nodes.append(*child);
}

class ChildNodesLazySnapshot {
    WTF_MAKE_NONCOPYABLE(ChildNodesLazySnapshot);
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit ChildNodesLazySnapshot(Node& parentNode)
        : m_currentNode(parentNode.firstChild())
        , m_currentIndex(0)
        , m_hasSnapshot(false)
    {
        m_nextSnapshot = latestSnapshot;
        latestSnapshot = this;
    }

    ALWAYS_INLINE ~ChildNodesLazySnapshot()
    {
        latestSnapshot = m_nextSnapshot;
    }

    // Returns 0 if there is no next Node.
    RefPtr<Node> nextNode()
    {
        if (LIKELY(!hasSnapshot())) {
            RefPtr<Node> node = m_currentNode.release();
            if (node)
                m_currentNode = node->nextSibling();
            return node.release();
        }
        if (m_currentIndex >= m_snapshot.size())
            return 0;
        return m_snapshot[m_currentIndex++];
    }

    void takeSnapshot()
    {
        if (hasSnapshot())
            return;
        m_hasSnapshot = true;
        Node* node = m_currentNode.get();
        while (node) {
            m_snapshot.append(node);
            node = node->nextSibling();
        }
    }

    ChildNodesLazySnapshot* nextSnapshot() { return m_nextSnapshot; }
    bool hasSnapshot() { return m_hasSnapshot; }

    static void takeChildNodesLazySnapshot()
    {
        ChildNodesLazySnapshot* snapshot = latestSnapshot;
        while (snapshot && !snapshot->hasSnapshot()) {
            snapshot->takeSnapshot();
            snapshot = snapshot->nextSnapshot();
        }
    }

private:
#if !PLATFORM(WKC)
    static ChildNodesLazySnapshot* latestSnapshot;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(ChildNodesLazySnapshot*, latestSnapshot);
#endif

    RefPtr<Node> m_currentNode;
    unsigned m_currentIndex;
    bool m_hasSnapshot;
    Vector<RefPtr<Node>> m_snapshot; // Lazily instantiated.
    ChildNodesLazySnapshot* m_nextSnapshot;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ContainerNode)
    static bool isType(const WebCore::Node& node) { return node.isContainerNode(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // ContainerNode_h
