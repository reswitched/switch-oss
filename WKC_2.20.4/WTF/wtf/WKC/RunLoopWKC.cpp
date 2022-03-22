/*
* Copyright (C) 2015-2021 ACCESS CO., LTD. All rights reserved.
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
#include "RunLoop.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/Vector.h>

#include <wkc/wkcpeer.h>

namespace WTF {

// copy from RunLoopGeneric.cpp

typedef bool(*CycleProc)(void*);
WKC_DEFINE_GLOBAL_TYPE_ZERO(CycleProc, gCycleProc);
WKC_DEFINE_GLOBAL_TYPE_ZERO(void*, gCycleOpaque);

class RunLoop::TimerBase::ScheduledTask : public ThreadSafeRefCounted<ScheduledTask> {
WTF_MAKE_NONCOPYABLE(ScheduledTask);
public:
    static Ref<ScheduledTask> create(Function<void()>&& function, Seconds interval, bool repeating)
    {
        return adoptRef(*new ScheduledTask(WTFMove(function), interval, repeating));
    }

    ScheduledTask(Function<void()>&& function, Seconds interval, bool repeating)
        : m_function(WTFMove(function))
        , m_fireInterval(interval)
        , m_isRepeating(repeating)
    {
        updateReadyTime();
    }

    bool fired()
    {
        if (!isActive())
            return false;

        m_function();

        if (!m_isRepeating)
            return false;

        updateReadyTime();
        return isActive();
    }

    MonotonicTime scheduledTimePoint() const
    {
        return m_scheduledTimePoint;
    }

    void updateReadyTime()
    {
        m_scheduledTimePoint = MonotonicTime::now();
        if (!m_fireInterval)
            return;
        m_scheduledTimePoint += m_fireInterval;
    }

    struct EarliestSchedule {
        bool operator()(const RefPtr<ScheduledTask>& lhs, const RefPtr<ScheduledTask>& rhs)
        {
            return lhs->scheduledTimePoint() > rhs->scheduledTimePoint();
        }
    };

    bool isActive() const
    {
        return m_isActive.load();
    }

    void deactivate()
    {
        m_isActive.store(false);
    }

private:
    Function<void ()> m_function;
    MonotonicTime m_scheduledTimePoint;
    Seconds m_fireInterval;
    std::atomic<bool> m_isActive { true };
    bool m_isRepeating;
};

RunLoop::RunLoop()
{
}

RunLoop::~RunLoop()
{
    LockHolder locker(m_loopLock);
    m_shutdown = true;
    m_readyToRun.notifyOne();
    
    // Here is running main loops. Wait until all the main loops are destroyed.
    if (!m_mainLoops.isEmpty())
    m_stopCondition.wait(m_loopLock);
}

inline bool RunLoop::populateTasks(RunMode runMode, Status& statusOfThisLoop, Deque<RefPtr<TimerBase::ScheduledTask>>& firedTimers)
{
    LockHolder locker(m_loopLock);

    if (runMode == RunMode::Drain) {
        MonotonicTime sleepUntil = MonotonicTime::infinity();
        if (!m_schedules.isEmpty())
            sleepUntil = m_schedules.first()->scheduledTimePoint();

        m_readyToRun.waitUntil(m_loopLock, sleepUntil, [&] {
            return m_shutdown || m_pendingTasks || statusOfThisLoop == Status::Stopping;
        });
    }

    if (statusOfThisLoop == Status::Stopping || m_shutdown) {
        m_mainLoops.removeLast();
        if (m_mainLoops.isEmpty())
            m_stopCondition.notifyOne();
        return false;
    }
    m_pendingTasks = false;
    if (runMode == RunMode::Iterate)
        statusOfThisLoop = Status::Stopping;

    // Check expired timers.
    MonotonicTime now = MonotonicTime::now();
    while (!m_schedules.isEmpty()) {
        RefPtr<TimerBase::ScheduledTask> earliest = m_schedules.first();
        if (earliest->scheduledTimePoint() > now)
            break;
        std::pop_heap(m_schedules.begin(), m_schedules.end(), TimerBase::ScheduledTask::EarliestSchedule());
        m_schedules.removeLast();
        firedTimers.append(WTFMove(earliest));
    }

    return true;
}

void RunLoop::runImpl(RunMode runMode)
{
    ASSERT(this == &RunLoop::current());

    Status statusOfThisLoop = Status::Clear;
    {
        LockHolder locker(m_loopLock);
        m_mainLoops.append(&statusOfThisLoop);
    }

    Deque<RefPtr<TimerBase::ScheduledTask>> firedTimers;
    while (true) {
        if (!populateTasks(runMode, statusOfThisLoop, firedTimers))
            return;

        // Dispatch scheduled timers.
        while (!firedTimers.isEmpty()) {
            RefPtr<TimerBase::ScheduledTask> task = firedTimers.takeFirst();
            if (task->fired()) {
                // Reschedule because the timer requires repeating.
                // Since we will query the timers' time points before sleeping,
                // we do not call wakeUp() here.
                schedule(*task);
            }
        }
        performWork();

        // Avoid busy loop, just in case.
        wkcThreadTimedSleepPeer(1);
    }
}

void RunLoop::run()
{
    RunLoop::current().runImpl(RunMode::Drain);
}

void RunLoop::iterate()
{
    RunLoop::current().runImpl(RunMode::Iterate);
}

// RunLoop operations are thread-safe. These operations can be called from outside of the RunLoop's thread.
// For example, WorkQueue::{dispatch, dispatchAfter} call the operations of the WorkQueue thread's RunLoop
// from the caller's thread.

void RunLoop::stop()
{
    LockHolder locker(m_loopLock);
    if (m_mainLoops.isEmpty())
        return;

    Status* status = m_mainLoops.last();
    if (*status != Status::Stopping) {
        *status = Status::Stopping;
        m_readyToRun.notifyOne();
    }
}

void RunLoop::wakeUp(const AbstractLocker&)
{
    m_pendingTasks = true;
    m_readyToRun.notifyOne();
}

void RunLoop::wakeUp()
{
    LockHolder locker(m_loopLock);
    wakeUp(locker);
}

void RunLoop::schedule(const AbstractLocker&, Ref<TimerBase::ScheduledTask>&& task)
{
    m_schedules.append(task.ptr());
    std::push_heap(m_schedules.begin(), m_schedules.end(), TimerBase::ScheduledTask::EarliestSchedule());
}

void RunLoop::schedule(Ref<TimerBase::ScheduledTask>&& task)
{
    LockHolder locker(m_loopLock);
    schedule(locker, WTFMove(task));
}

void RunLoop::scheduleAndWakeUp(const AbstractLocker& locker, Ref<TimerBase::ScheduledTask>&& task)
{
    schedule(locker, WTFMove(task));
    wakeUp(locker);
}

void RunLoop::dispatchAfter(Seconds delay, Function<void()>&& function)
{
    LockHolder locker(m_loopLock);
    bool repeating = false;
    schedule(locker, TimerBase::ScheduledTask::create(WTFMove(function), delay, repeating));
    wakeUp(locker);
}

// RunLoop::Timer

class RunLoop::TimerBase::Protector {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static int create(RunLoop::TimerBase* timer)
    {
        if (s_protectors.isNull()) {
            s_protectors.construct(64);
            s_index = 0;
        }

        int startIndex = s_index;

        while (1) {
            int index = s_index++;
            if (s_index == s_protectors->size())
                s_index = 0;
            Protector& protector = s_protectors->at(index);
            if (!protector.m_isAlive) {
                protector.m_timer = timer;
                protector.m_id = index;
                protector.m_isAlive = true;
                return index;
            }
            if (s_index == startIndex) {
                s_index = s_protectors->size();
                s_protectors->grow(s_index + 16);
            }
        }

        ASSERT_NOT_REACHED();
        return -1;
    }

    static Protector* get(size_t id)
    {
        RELEASE_ASSERT(!s_protectors.isNull());
        return &s_protectors->at(id);
    }

    Protector() = default;
    ~Protector() = default;

    RunLoop::TimerBase* timer() const { return m_timer; }

    int id() const { return m_id; }

    bool isAlive() const { return m_isAlive; }
    void setIsAlive(bool alive) { m_isAlive = alive; }

private:
    RunLoop::TimerBase* m_timer { nullptr };
    int m_id { 0 };
    bool m_isAlive { false };

    static LazyNeverDestroyed<Vector<RunLoop::TimerBase::Protector>> s_protectors;
    static int s_index;
};

LazyNeverDestroyed<Vector<RunLoop::TimerBase::Protector>> RunLoop::TimerBase::Protector::s_protectors;
int RunLoop::TimerBase::Protector::s_index;

bool RunLoop::TimerBase::timerFired(void* data)
{
    Protector* protector = Protector::get(reinterpret_cast<uintptr_t>(data));
    if (!protector->isAlive()) {
        return false;
    }

    TimerBase* self = protector->timer();
    self->fired();

    if (self->m_isRepeating)
        wkcTimerStartOneShotPeer(self->m_timer, self->m_nextFireInterval.value(), timerFired, reinterpret_cast<void*>(protector->id()));
    else
        self->m_isRunning = false;

    return false;
}

RunLoop::TimerBase::TimerBase(RunLoop& runLoop)
    : m_runLoop(runLoop)
    , m_isRunning(false)
    , m_isRepeating(false)
    , m_nextFireInterval()
{
    m_timer = wkcTimerNewPeer();
    if (!m_timer)
        CRASH();
    m_protectorID = Protector::create(this);
}

RunLoop::TimerBase::~TimerBase()
{
    Protector* protector = Protector::get(m_protectorID);
    protector->setIsAlive(false);
    stop();
    wkcTimerDeletePeer(m_timer);
}

void RunLoop::TimerBase::start(Seconds nextFireInterval, bool repeat)
{
    m_isRepeating = repeat;
    m_isRunning = true;
    m_nextFireInterval = nextFireInterval;
    m_nextFireDate = MonotonicTime::now() + m_nextFireInterval;
    wkcTimerStartOneShotPeer(m_timer, m_nextFireInterval.value(), timerFired, reinterpret_cast<void*>(m_protectorID));
}

void RunLoop::TimerBase::stop()
{
    wkcTimerCancelPeer(m_timer);
    m_isRunning = false;
}

bool RunLoop::TimerBase::isActive() const
{
    return m_isRunning;
}

Seconds RunLoop::TimerBase::secondsUntilFire() const
{
    if (m_isRunning)
        return std::max<Seconds>(m_nextFireDate - MonotonicTime::now(), Seconds(0));
    else 
        return Seconds(0);
}

void RunLoop_setCycleProc(bool(*proc)(void*), void* opaque)
{
    gCycleProc = proc;
    gCycleOpaque = opaque;
}

RunLoop::CycleResult RunLoop::cycle(RunLoopMode mode)
{
    if (!gCycleProc || !gCycleProc(gCycleOpaque)) {
        return RunLoop::CycleResult::Stop;
    }

    return RunLoop::CycleResult::Continue;
}

} // namespace
