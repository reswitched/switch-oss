/*
 * Copyright (C) 2006 Apple Inc.
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

#ifndef FrameTree_h
#define FrameTree_h

#include <wtf/text/AtomicString.h>

namespace WebCore {

    class Frame;
    class TreeScope;

    class FrameTree {
        WTF_MAKE_NONCOPYABLE(FrameTree);
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:
        const static unsigned invalidCount = static_cast<unsigned>(-1);

        FrameTree(Frame& thisFrame, Frame* parentFrame)
            : m_thisFrame(thisFrame)
            , m_parent(parentFrame)
            , m_previousSibling(nullptr)
            , m_lastChild(nullptr)
            , m_scopedChildCount(invalidCount)
        {
        }

        ~FrameTree();

        const AtomicString& name() const { return m_name; }
        const AtomicString& uniqueName() const { return m_uniqueName; }
        WEBCORE_EXPORT void setName(const AtomicString&);
        WEBCORE_EXPORT void clearName();
        WEBCORE_EXPORT Frame* parent() const;
        void setParent(Frame* parent) { m_parent = parent; }
        
        Frame* nextSibling() const { return m_nextSibling.get(); }
        Frame* previousSibling() const { return m_previousSibling; }
        Frame* firstChild() const { return m_firstChild.get(); }
        Frame* lastChild() const { return m_lastChild; }

        Frame* firstRenderedChild() const;
        Frame* nextRenderedSibling() const;

        WEBCORE_EXPORT bool isDescendantOf(const Frame* ancestor) const;
        
        WEBCORE_EXPORT Frame* traverseNext(const Frame* stayWithin = nullptr) const;
        // Rendered means being the main frame or having an ownerRenderer. It may not have been parented in the Widget tree yet (see WidgetHierarchyUpdatesSuspensionScope).
        WEBCORE_EXPORT Frame* traverseNextRendered(const Frame* stayWithin = nullptr) const;
        WEBCORE_EXPORT Frame* traverseNextWithWrap(bool) const;
        WEBCORE_EXPORT Frame* traversePreviousWithWrap(bool) const;
        
        WEBCORE_EXPORT void appendChild(PassRefPtr<Frame>);
        bool transferChild(PassRefPtr<Frame>);

        Frame* traverseNextInPostOrderWithWrap(bool) const;

        void detachFromParent() { m_parent = nullptr; }
        void removeChild(Frame*);

        Frame* child(unsigned index) const;
        Frame* child(const AtomicString& name) const;
        WEBCORE_EXPORT Frame* find(const AtomicString& name) const;
        WEBCORE_EXPORT unsigned childCount() const;

        AtomicString uniqueChildName(const AtomicString& requestedName) const;

        WEBCORE_EXPORT Frame& top() const;

        Frame* scopedChild(unsigned index) const;
        Frame* scopedChild(const AtomicString& name) const;
        unsigned scopedChildCount() const;

    private:
        Frame* deepFirstChild() const;
        Frame* deepLastChild() const;
        void actuallyAppendChild(PassRefPtr<Frame>);

        bool scopedBy(TreeScope*) const;
        Frame* scopedChild(unsigned index, TreeScope*) const;
        Frame* scopedChild(const AtomicString& name, TreeScope*) const;
        unsigned scopedChildCount(TreeScope*) const;

        Frame& m_thisFrame;

        Frame* m_parent;
        AtomicString m_name; // The actual frame name (may be empty).
        AtomicString m_uniqueName;

        RefPtr<Frame> m_nextSibling;
        Frame* m_previousSibling;
        RefPtr<Frame> m_firstChild;
        Frame* m_lastChild;
        mutable unsigned m_scopedChildCount;
    };

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
WEBCORE_EXPORT void showFrameTree(const WebCore::Frame*);
#endif

#endif // FrameTree_h
