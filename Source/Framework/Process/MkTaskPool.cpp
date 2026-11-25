////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework\MkFrameworkPCH.h"
#include "Process\MkTaskPool.h"

// Mk includes.
#include "Basic\MkException.h"
#include "Basic\MkString.h"
#include "Process\MkOSEvent.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif

// Static initializations.
namespace
{
const MkAString sPriorityStr[] = {"Low", "Medium", "High"};

const MkDuration sEventDuration(1.0, MkDuration::eMinutes);
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTaskPool
////////////////////////////////////////////////////////////////////////////////////////////////////

MkTaskPool::MkTaskPool(
        const MkAString& name,
        const std::map<MkTask::Priority, mkuint>& threadCntMap,
        const std::shared_ptr<MkTimeScheduler>& rTimeScheduler) :
    m_name(name),
    m_rTimeScheduler(rTimeScheduler)
{
    MkCriticalSection cs(m_taskLock);

    // Need to assure each task thread is running before exiting this constructor.
    mkuint32 priorityIdx = 0;
    for (auto& threadCntIter : threadCntMap)
    {
        for (mkuint threadIdx = 0; threadIdx < threadCntIter.second; ++threadIdx)
        {
            auto rThread = std::make_shared<Thread>(
                    this, sPriorityStr[priorityIdx], threadIdx, threadCntIter.first);

            while (!rThread->IsRunning()) Sleep(1);

            m_threads.push_back(rThread);
        }
        ++priorityIdx;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTaskPool::Schedule(const std::shared_ptr<MkTask>& rTask, MkTask::Priority priority)
{
    rTask->SetPriority(priority);
    auto taskPair = std::make_pair(MkTaskPool::TaskKey(priority, MkCPUTime()), rTask);

    {
        MkCriticalSection cs(m_taskLock);
        m_pendingTaskMap.insert(taskPair);
    
        // If there is an idle task thread that is allowed to execute this task,
        // wake it up.
        for (auto& threadIter : m_threads)
        {
            if (threadIter->GetPriority() <= priority)
            {
                if (!threadIter->IsExecutingTask())
                {
                    threadIter->m_isExecutingTask = true;
                    threadIter->GetTaskEvent()->Signal();
                    return;
                }
            }
            else
            {
                // Short circuit when the thread priority is higher than the task.
                // NOTE: This logic depends on the threads being in order from low
                // to high priority.  MRK.
                break;
            }
        }

#if DEBUG
        { MkDebugSystemText() << "All eligible task threads executing, task queued.\n"; }
#endif
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTaskPool::Thread
////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: The execute task OS event for task pool threads is a manual reset event.
// Something, somewhere was consuming task OS event auto-reset signals.  MRK.
// TODO: Figure out what is happening.  MRK.
MkTaskPool::Thread::Thread(
        MkTaskPool* pTaskPool,
        const MkAString& poolStr, 
        const mkuint threadIdx, 
        const MkTask::Priority priority) :
    MkThread(MkAString("%sPoolThread%3d", poolStr.GetCString(), threadIdx)),
    m_pTaskPool(pTaskPool),
    m_rTaskEvent(std::make_shared<MkOSEvent>(
            MkAString("%sPoolThread%3d", poolStr.GetCString(), threadIdx), true)),
    m_priority(priority),
    m_isRunning(false),
    m_isExecutingTask(false)
{
    StartThread();
#if DEBUG
    { 
        MkDebugSystemText() << "Creating " << GetName().GetCString() <<
                " with event " << m_rTaskEvent->GetHandle() << ".\n";
    }
#endif
}


#if 1
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTaskPool::Thread::Execute()
{
    m_isRunning = true;

    MkMultiOSEvent multiOSEvent(GetName());

    multiOSEvent.AddOSEvent(m_rExitEvent);      // Event 0.
    multiOSEvent.AddOSEvent(m_rTaskEvent);      // Event 1.

    while (true)
    {
#if DEBUG
        {
            MkDebugSystemText() << "Waiting in '" << GetName().GetCString() <<
                    "' for primary event " << m_rTaskEvent->GetHandle() << ".\n"; 
        }
#endif
        // NOTE: The point of this entire thread, method, etc.  MRK.
        auto waitStatus = multiOSEvent.WaitForObjects(MkForever());
        m_rTaskEvent->Reset();
#if DEBUG
        { 
            MkDebugSystemText() << "'" << GetName().GetCString() << "' with primary event " <<
                    m_rTaskEvent->GetHandle() << " got activated.\n";
        }
#endif
        switch (waitStatus)
        {
        case MkWaitStatus::eFailed :
        case MkWaitStatus::eAbandoned :
        case MkWaitStatus::eTimeOut :
            throw MkException(eOSFailure, "WaitForObjects failed.");
            break;

        default :
        {
            switch (waitStatus - MkWaitStatus::eEvent0)
            {
            case 0 :
                // Exit the thread.
                return;

            case 1:
                // Find a task to execute.
                SelectTask();
                break;

            default :
                // An undefined event.
                throw MkException(eLogicError, "Undefined event.");
                break;   
            }

            break;
        }
        }
    }
}
#endif


#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTaskPool::Thread::Execute()
{
    m_isRunning = true;

    while (true)
    {
#if DEBUG
        { MkDebugSystemText() << "Waiting in task thread for single event " <<
                m_rTaskEvent->GetHandle() << "\n"; }
#endif
        // NOTE: The point of this entire thread, method, etc.  MRK.
        auto waitStatus = m_rTaskEvent->WaitForObject(MkForever());
#if DEBUG
        { 
            MkDebugSystemText() << "Task thread with single event " <<
                    m_rTaskEvent->GetHandle() << " got activated.\n";
        }
#endif

        switch (waitStatus)
        {
        case MkWaitStatus::eFailed :
        case MkWaitStatus::eAbandoned :
        case MkWaitStatus::eTimeOut :
            throw MkException(eOSFailure, "WaitForObjects failed.");
            break;

        default :
            // Find a task to execute.
            SelectTask();
            break;
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTaskPool::Thread::SelectTask()
{
    while (true)
    {
        // Look for a task for us to execute.
        std::pair<MkTaskPool::TaskKey, std::shared_ptr<MkTask>> taskPair;
        {
            {
                MkCriticalSection cs(m_pTaskPool->m_taskLock);
                if (m_pTaskPool->m_pendingTaskMap.empty())
                {
                    m_isExecutingTask = false;
                    return;
                }

                // NOTE: The highest priority task is at the back of the pending
                // tasks.  If, for example, a high priority thread cannot execute
                // it, then we want that thread to remain idle.  MRK.
                taskPair = *m_pTaskPool->m_pendingTaskMap.rbegin();
                if (m_isExecutingTask = (m_priority <= taskPair.first.GetPriority()))
                {
                    m_pTaskPool->m_pendingTaskMap.erase(taskPair.first);
                }
            }

            if (m_isExecutingTask)
            {
                ExecuteTask(taskPair);
                continue;
            }

            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTaskPool::Thread::ExecuteTask(
        const std::pair<MkTaskPool::TaskKey, std::shared_ptr<MkTask>>& taskPair)
{
#if DEBUG
    { MkDebugSystemText() << "    Execute task " << taskPair.second->GetName().GetCString() << ".\n"; }
#endif

    auto status = taskPair.second->Execute();

    switch (status)
    {
    case MkTask::eComplete :
#if DEBUG
        { MkDebugSystemText() << "    Task " << taskPair.second->GetName().GetCString() << " completed.\n"; }
#endif
        break;

    case MkTask::eIncomplete:
#if DEBUG
        { MkDebugSystemText() << "    Task " << taskPair.second->GetName().GetCString() << " incomplete.\n"; }
#endif
        m_pTaskPool->m_rTimeScheduler->Schedule(
                taskPair.second->GetRescheduleDuration(),
                std::make_shared<RescheduleFunctor>(m_pTaskPool, taskPair.second));
        break;
    
    case MkTask::eContinue:
#if DEBUG
        { 
            MkDebugSystemText() << "    Task " << taskPair.second->GetName().GetCString() <<
                    " has continuation.\n";
        }
#endif
        // TODO: Introduce continuation tasks.  MRK.
        break;
    }
}