/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThreadedScrollingTree.h"

#if ENABLE(ASYNC_SCROLLING)

#include "AsyncScrollingCoordinator.h"
#include "PlatformWheelEvent.h"
#include "ScrollingThread.h"
#include "ScrollingTreeFixedNode.h"
#include "ScrollingTreeNode.h"
#include "ScrollingTreeScrollingNode.h"
#include "ScrollingTreeStickyNode.h"
#include <wtf/RunLoop.h>
#include <wtf/TemporaryChange.h>

namespace WebCore {

ThreadedScrollingTree::ThreadedScrollingTree(AsyncScrollingCoordinator* scrollingCoordinator)
    : m_scrollingCoordinator(scrollingCoordinator)
{
}

ThreadedScrollingTree::~ThreadedScrollingTree()
{
    // invalidate() should have cleared m_scrollingCoordinator.
    ASSERT(!m_scrollingCoordinator);
}

ScrollingTree::EventResult ThreadedScrollingTree::tryToHandleWheelEvent(const PlatformWheelEvent& wheelEvent)
{
    if (shouldHandleWheelEventSynchronously(wheelEvent))
        return SendToMainThread;

    if (willWheelEventStartSwipeGesture(wheelEvent))
        return DidNotHandleEvent;

    RefPtr<ThreadedScrollingTree> threadedScrollingTree(this);
    ScrollingThread::dispatch([threadedScrollingTree, wheelEvent] {
        threadedScrollingTree->handleWheelEvent(wheelEvent);
    });
    
    return DidHandleEvent;
}

void ThreadedScrollingTree::handleWheelEvent(const PlatformWheelEvent& wheelEvent)
{
    ASSERT(ScrollingThread::isCurrentThread());
    ScrollingTree::handleWheelEvent(wheelEvent);
}

void ThreadedScrollingTree::invalidate()
{
    // Invalidate is dispatched by the ScrollingCoordinator class on the ScrollingThread
    // to break the reference cycle between ScrollingTree and ScrollingCoordinator when the
    // ScrollingCoordinator's page is destroyed.
    ASSERT(ScrollingThread::isCurrentThread());

    // Since this can potentially be the last reference to the scrolling coordinator,
    // we need to release it on the main thread since it has member variables (such as timers)
    // that expect to be destroyed from the main thread.
    ScrollingCoordinator* scrollingCoordinator = m_scrollingCoordinator.release().leakRef();
    RunLoop::main().dispatch([scrollingCoordinator] {
        scrollingCoordinator->deref();
    });
}

void ThreadedScrollingTree::commitNewTreeState(std::unique_ptr<ScrollingStateTree> scrollingStateTree)
{
    ASSERT(ScrollingThread::isCurrentThread());
    ScrollingTree::commitNewTreeState(WTF::move(scrollingStateTree));
}

void ThreadedScrollingTree::scrollingTreeNodeDidScroll(ScrollingNodeID nodeID, const FloatPoint& scrollPosition, SetOrSyncScrollingLayerPosition scrollingLayerPositionAction)
{
    if (!m_scrollingCoordinator)
        return;

    if (nodeID == rootNode()->scrollingNodeID())
        setMainFrameScrollPosition(scrollPosition);

    RefPtr<AsyncScrollingCoordinator> scrollingCoordinator = m_scrollingCoordinator;
    bool localIsHandlingProgrammaticScroll = isHandlingProgrammaticScroll();
    
    RunLoop::main().dispatch([scrollingCoordinator, nodeID, scrollPosition, localIsHandlingProgrammaticScroll, scrollingLayerPositionAction] {
        scrollingCoordinator->scheduleUpdateScrollPositionAfterAsyncScroll(nodeID, scrollPosition, localIsHandlingProgrammaticScroll, scrollingLayerPositionAction);
    });
}

void ThreadedScrollingTree::currentSnapPointIndicesDidChange(ScrollingNodeID nodeID, unsigned horizontal, unsigned vertical)
{
    if (!m_scrollingCoordinator)
        return;

    RefPtr<AsyncScrollingCoordinator> scrollingCoordinator = m_scrollingCoordinator;
    RunLoop::main().dispatch([scrollingCoordinator, nodeID, horizontal, vertical] {
        scrollingCoordinator->setActiveScrollSnapIndices(nodeID, horizontal, vertical);
    });
}

#if PLATFORM(MAC)
void ThreadedScrollingTree::handleWheelEventPhase(PlatformWheelEventPhase phase)
{
    if (!m_scrollingCoordinator)
        return;

    RefPtr<AsyncScrollingCoordinator> scrollingCoordinator = m_scrollingCoordinator;
    RunLoop::main().dispatch([scrollingCoordinator, phase] {
        scrollingCoordinator->handleWheelEventPhase(phase);
    });
}

void ThreadedScrollingTree::setActiveScrollSnapIndices(ScrollingNodeID nodeID, unsigned horizontalIndex, unsigned verticalIndex)
{
    if (!m_scrollingCoordinator)
        return;
    
    RefPtr<AsyncScrollingCoordinator> scrollingCoordinator = m_scrollingCoordinator;
    RunLoop::main().dispatch([scrollingCoordinator, nodeID, horizontalIndex, verticalIndex] {
        scrollingCoordinator->setActiveScrollSnapIndices(nodeID, horizontalIndex, verticalIndex);
    });
}

void ThreadedScrollingTree::deferTestsForReason(WheelEventTestTrigger::ScrollableAreaIdentifier identifier, WheelEventTestTrigger::DeferTestTriggerReason reason)
{
    if (!m_scrollingCoordinator)
        return;

    RefPtr<AsyncScrollingCoordinator> scrollingCoordinator = m_scrollingCoordinator;
    RunLoop::main().dispatch([scrollingCoordinator, identifier, reason] {
        scrollingCoordinator->deferTestsForReason(identifier, reason);
    });
}

void ThreadedScrollingTree::removeTestDeferralForReason(WheelEventTestTrigger::ScrollableAreaIdentifier identifier, WheelEventTestTrigger::DeferTestTriggerReason reason)
{
    if (!m_scrollingCoordinator)
        return;
    
    RefPtr<AsyncScrollingCoordinator> scrollingCoordinator = m_scrollingCoordinator;
    RunLoop::main().dispatch([scrollingCoordinator, identifier, reason] {
        scrollingCoordinator->removeTestDeferralForReason(identifier, reason);
    });
}

#endif

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING)
