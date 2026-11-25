////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Framework\MkFrameworkPCH.h"
#include "Process\MkScheduler.h"

// Mk includes.
#include "Basic\MkException.h"
#include "Process\MkFrameworkApplication.h"
#include "Process\MkTaskPool.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif

// Static initializations.
namespace
{
MkDuration sMillisecond = MkDuration(1ll, MkDuration::eMilliseconds);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTimeScheduler
////////////////////////////////////////////////////////////////////////////////////////////////////

MkTimeScheduler::MkTimeScheduler(const MkAString& name) :
    m_name(name),
    m_rThread(std::make_shared<MkTimeScheduler::Thread>(this))
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTimeScheduler::Functor
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTimeScheduler::Functor::ScheduleSelf()
{
    Framework::TaskPool().Schedule(shared_from_this(), MkTask::eNormal);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTimeScheduler::Thread
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkTimeScheduler::Thread::Execute()
{
    MkMultiOSEvent multiOSEvent(MkConstAString("TimeScheduler"));

    multiOSEvent.AddOSEvent(m_rExitEvent);          // Event 0.
    multiOSEvent.AddOSEvent(m_rUpdateEvent);        // Event 1.

    while (true)
    {
        // Set the time out to the next functor to execute.
        MkDuration waitDuration;
        if (m_functorMultiMap.empty())
        {
            waitDuration.SetToForever();
        }
        else
        {
            MkCPUTime now;
            waitDuration = m_functorMultiMap.begin()->first - now + sMillisecond;
        }

        // If the next functor's time has already come, execute it, rinse and repeat.
        if (waitDuration <= sMillisecond)
        {
            (*m_functorMultiMap.begin()->second)();
            m_functorMultiMap.erase(m_functorMultiMap.begin());
            continue;
        }

        // NOTE: The point of this entire thread, method, etc.  MRK.
        auto waitStatus = multiOSEvent.WaitForObjects(waitDuration);

#if DEBUG
        { 
            MkDebugSystemText() << "Wait for " << m_pTimeScheduler->GetName().GetCString() <<
                    " TimeScheduler triggered with " << waitStatus << ".\n";
        }
#endif

        switch (waitStatus)
        {
        case MkWaitStatus::eFailed :
        case MkWaitStatus::eAbandoned :
            throw MkException(eOSFailure, "WaitForObjects failed.");
            break;

        case MkWaitStatus::eTimeOut :
        {
            // Execute the functor at the head of the list and remove it.
            auto rFunctor = m_functorMultiMap.begin()->second;

            // Reschedule it if reqested.
            if (rFunctor->Reschedule())
            {
                MkCriticalSection cs(m_pendingFunctorsLock);
                m_pendingFunctors.push_back(
                        std::make_pair(MkCPUTime() + rFunctor->GetRepeatDuration(), rFunctor));
                m_rUpdateEvent->Signal();
            }

            (*rFunctor)();
            m_functorMultiMap.erase(m_functorMultiMap.begin());
            break;
        }

        default :
        {
            switch (waitStatus - MkWaitStatus::eEvent0)
            {
            case 0 :
                // Exit the thread.
                return;

            case 1:
            {
                m_rUpdateEvent->Reset();

                MkCriticalSection cs(m_pendingFunctorsLock);
                while (!m_pendingFunctors.empty())
                {
                    m_functorMultiMap.insert(m_pendingFunctors.back());
                    m_pendingFunctors.pop_back();
                    break;
                }
                break;
            }

            default :
                // An undefined event.
                throw MkException(eOSFailure, "Undefined event.");
                break;   
            }

            break;
        }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkTimeScheduler::Thread::Schedule(const MkCPUTime cpuTime, const std::shared_ptr<Functor>& rFunctor)
{
    // First check if the time has already arrived.
    MkCPUTime now;
    if (cpuTime <= now)
    {
        (*rFunctor)();
        return true;
    }

    // Otherwise set the functor up to be added to the functor multi-map.
    {
        MkCriticalSection cs(m_pendingFunctorsLock);
        m_pendingFunctors.push_back(std::make_pair(cpuTime, rFunctor));
    }

    m_rUpdateEvent->Signal();

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkEventScheduler
////////////////////////////////////////////////////////////////////////////////////////////////////

MkEventScheduler::MkEventScheduler(const MkAString& name) :
    m_name(name)
{
    AddThread();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::Schedule(
        const std::shared_ptr<MkOSEvent>& rEvent, 
        const std::shared_ptr<Functor>& rFunctor, 
        const bool persistent)
{
    {
        MkCriticalSection cs(m_pendingFunctorsLock);
        m_pendingFunctors.push_back(std::make_tuple(rEvent, rFunctor, persistent));
    }

    // NOTE: A bit inefficient to always start with the first thread as empty slots
    // are more likely in the most recently added thread.  MRK.
    m_threads[0]->Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::Cancel(const std::shared_ptr<MkOSEvent>& rEvent)
{
    {
        MkCriticalSection cs(m_pendingFunctorsLock);
        m_pendingCancels.push_back(std::make_pair(rEvent, 0));
    }
    m_threads[0]->Update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::AddThread()
{
    m_threads.push_back(std::make_shared<MkEventScheduler::Thread>(this, m_threads.size()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::ProcessFunctors(const mkuint threadIdx)
{
    MkCriticalSection cs(m_threadsLock);

    if (threadIdx >= GetThreadCount())
    {
        AddThread();
    }

    m_threads[threadIdx]->Update();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkEventScheduler::Functor
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::Functor::ScheduleSelf()
{
#if DEBUG
    { 
        MkDebugSystemText() << "    " << shared_from_this()->GetName().GetCString() <<
                " is scheduling itself.\n";
    }
#endif

    Framework::TaskPool().Schedule(shared_from_this(), MkTask::eNormal);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkEventScheduler::Thread
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::Thread::Execute()
{
    MkMultiOSEvent multiOSEvent(MkConstAString("EventScheduler"));

    multiOSEvent.AddOSEvent(m_rExitEvent);          // Event 0.
    multiOSEvent.AddOSEvent(m_rUpdateEvent);        // Event 1.

    while (true)
    {
        // For simplicity, remove all events except the fixed ones and refill.
        multiOSEvent.Resize(2);
        for (auto& eventIter : m_eventFunctors)
        {
            multiOSEvent.AddOSEvent(std::get<0>(eventIter));
        }

        // NOTE: The point of this entire thread, method, etc.  MRK.
        auto waitStatus = multiOSEvent.WaitForObjects(MkForever());

#if DEBUG
        { 
            MkDebugSystemText() << "Wait for " << m_pEventScheduler->GetName().GetCString() <<
                    " EventScheduler triggered with " << waitStatus << ".\n";
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
            {
                m_rUpdateEvent->Reset();
                UpdateFunctors();
                break;
            }

            default :
                mkuint eventIdx = waitStatus - MkWaitStatus::eEvent0 - 2;
                if (eventIdx < m_eventFunctors.size())
                {
#if DEBUG
                    { 
                        MkDebugSystemText() << "    Execute " << 
                                std::get<1>(m_eventFunctors[eventIdx])->GetName().GetCString() <<
                                " and " << (std::get<2>(m_eventFunctors[eventIdx]) ? "keep" : "erase") << 
                                ".\n"; }
#endif
                    // Reset the functor event.
                    std::get<0>(m_eventFunctors[eventIdx])->Reset();

                    // Execute the indicated functor.
                    (*std::get<1>(m_eventFunctors[eventIdx]))();

                    // Remove it if appropriate.
                    if (!std::get<2>(m_eventFunctors[eventIdx]))
                    {
                        m_eventFunctors.erase(m_eventFunctors.begin() + eventIdx);
                    }
                }
                else
                {
                    throw MkException(eLogicError, "Undefined event.");
                }

                break;   
            }

            break;
        }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkEventScheduler::Thread::UpdateFunctors()
{
    MkCriticalSection cs(m_pEventScheduler->m_pendingFunctorsLock);

    // Process cancels.
    std::vector<std::pair<std::shared_ptr<MkOSEvent>, mkuint>> failedCancels;
    for (auto& cancelIter : m_pEventScheduler->m_pendingCancels)
    {
        auto eventIter = m_eventFunctors.begin();
        while (eventIter != m_eventFunctors.end())
        {
            if (*cancelIter.first == *std::get<0>(*eventIter))
            {
                // NOTE: This sequence is awkward, resetting to the beginning of the
                // vector when there should be only one event to remove.  Is there a
                // better way?  MRK.
                m_eventFunctors.erase(eventIter);
                eventIter = m_eventFunctors.begin();
                continue;
            }
        }

        // If all of the threads have had a shot at the cancel don't propagate it.
        if (cancelIter.second < m_pEventScheduler->GetThreadCount())
        {
            failedCancels.push_back(std::make_pair(cancelIter.first, cancelIter.second + 1));
        }
    }
    m_pEventScheduler->m_pendingCancels.swap(failedCancels);

    // Process adds.
    std::vector<std::tuple<std::shared_ptr<MkOSEvent>, std::shared_ptr<Functor>, bool>> failedAdds;
    for (auto& addIter : m_pEventScheduler->m_pendingFunctors)
    {
        if (m_eventFunctors.size() < MkMultiOSEvent::GetMaximumEvents() - 2)
        {
            m_eventFunctors.push_back(addIter);
            continue;
        }
        failedAdds.push_back(addIter);
    }
    m_pEventScheduler->m_pendingFunctors.swap(failedAdds);

    if (!m_pEventScheduler->m_pendingCancels.empty() || !m_pEventScheduler->m_pendingFunctors.empty())
    {
        m_pEventScheduler->ProcessFunctors(m_threadIdx + 1);
    }
}