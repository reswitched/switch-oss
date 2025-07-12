/*
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

#include "config.h"
#include "WorkQueue.h"

namespace WTF {

WKC_DEFINE_GLOBAL_TYPE_ZERO(RunLoop*, gRunLoop);

void WorkQueue::initialize()
{
    auto thread = Thread::create("WKC: WorkQueue", [] {
        gRunLoop = &RunLoop::current();
        gRunLoop->run();
    }).ptr();

    thread->detach();
}

WorkQueue::WorkQueue(RunLoop& loop)
    : WorkQueueBase(loop)
{
}

Ref<WorkQueue> WorkQueue::constructMainWorkQueue()
{
    return adoptRef(*new WorkQueue(RunLoop::main()));
}

WorkQueueBase::WorkQueueBase(RunLoop& loop)
    : m_runLoop(&loop)
{
}

void WorkQueueBase::platformInitialize(const char*, Type, QOS)
{
}

void WorkQueueBase::platformInvalidate()
{
}

void WorkQueueBase::dispatch(Function<void()>&& function)
{
    if (!gRunLoop)
        return;

    gRunLoop->dispatch([function = WTFMove(function)]{
        function();
        });
}

void WorkQueueBase::dispatchAfter(Seconds delay, Function<void()>&& function)
{
    if (!gRunLoop)
        return;

    gRunLoop->dispatchAfter(delay, [function = WTFMove(function)]{
        function();
        });
}

} // namespace
