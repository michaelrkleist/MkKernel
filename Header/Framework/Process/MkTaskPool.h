 ////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#pragma warning(disable : 4251)

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkTime.h"
#include "Process\MkCriticalSection.h"
#include "Process\MkScheduler.h"
#include "Process\MkThread.h"
#include "Process\MkTask.h"

// C++ includes.
#include <memory>

// STL includes.
#include <map>
#include <vector>


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTaskPool
////////////////////////////////////////////////////////////////////////////////////////////////////

// A task pool consists of MkThreads that execute tasks, a list of pending tasks and a
// variety of other support data structures.
class MK_EXPORT_FRAMEWORK MkTaskPool
{
public:
    MkTaskPool(
            const MkAString& name,
            const std::map<MkTask::Priority, mkuint>& threadCntMap,
            const std::shared_ptr<MkTimeScheduler>& rTimeScheduler);
    ~MkTaskPool() = default;

    void Schedule(const std::shared_ptr<MkTask>& rTask, MkTask::Priority priority);

private:
    class TaskKey
    {
    public:
        TaskKey() :
            m_priority(MkTask::eLow),
            m_submitted(MkCPUTime::EndOfTime())
        { }

        TaskKey(const MkTask::Priority priority, const MkCPUTime submitted) :
            m_priority(priority),
            m_submitted(submitted)
        { }

        ~TaskKey() = default;

        bool operator<(const TaskKey& rhs) const
        { return std::tuple(m_priority, m_submitted) < std::tuple(rhs.m_priority, rhs.m_submitted); }

        MkTask::Priority GetPriority() const
        { return m_priority; }

    private:
        MkTask::Priority m_priority;
        MkCPUTime m_submitted;
    };

    // Task pool threads are used to execute the tasks.
    class Thread : public MkThread
    {
    public:
        Thread(
                MkTaskPool* pTaskPool,
                const MkAString& poolStr, 
                const mkuint threadIdx, 
                const MkTask::Priority priority);
        ~Thread() override = default;

        void Execute() override;

        bool IsRunning() const
        { return m_isRunning; }

        bool IsExecutingTask() const
        { return m_isExecutingTask; }

        const std::shared_ptr<MkOSEvent>& GetTaskEvent() const
        { return m_rTaskEvent; }

        MkTask::Priority GetPriority() const
        { return m_priority; }

    private:
        void SelectTask();
        void ExecuteTask(const std::pair<TaskKey, std::shared_ptr<MkTask>>& taskPair);
        
        friend class MkTaskPool;

        MkTaskPool* m_pTaskPool;
        std::shared_ptr<MkOSEvent> m_rTaskEvent;
        MkTask::Priority m_priority;
        volatile bool m_isRunning;
        volatile bool m_isExecutingTask;
        MkCriticalSectionLock m_lock;
    };

    // The reschedule functor reschedules incomplete tasks.
    class RescheduleFunctor : public MkTimeScheduler::Functor
    {
    public:
        RescheduleFunctor(MkTaskPool* pTaskPool, const std::shared_ptr<MkTask>& rTask) :
            MkTimeScheduler::Functor(MkAString("RescheduleTask")),
            m_pTaskPool(pTaskPool),
            m_rTask(rTask)
        { }
        ~RescheduleFunctor() override = default;

        void operator()() override
        { m_pTaskPool->Schedule(m_rTask, m_rTask->GetPriority()); }

        MkTask::Status Execute() override
        { return eComplete; }

    private:
        MkTaskPool* m_pTaskPool;
        const std::shared_ptr<MkTask> m_rTask;
    };

    MkAString m_name;

    std::shared_ptr<MkTimeScheduler> m_rTimeScheduler;

    // NOTE: The threads are in priority order, lowest to highest.  MRK.
    std::vector<std::shared_ptr<Thread>> m_threads;

    // The pending tasks.  The tasks are ordered such that the next task to execute
    // is at the back.
    // NOTE: I will probably replace this with a Fibonacci heap, just because.  For
    // now, I will do the simple thing.  MRK.
    std::map<TaskKey, std::shared_ptr<MkTask>> m_pendingTaskMap;

    // NOTE: This lock must be held to do any task scheduling activity.  MRK.
    MkCriticalSectionLock m_taskLock;
};