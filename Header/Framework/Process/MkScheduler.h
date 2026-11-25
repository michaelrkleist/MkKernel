////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkTime.h"
#include "Process\MkCriticalSection.h"
#include "Process\MkOSEvent.h"
#include "Process\MkSingleton.h"
#include "Process\MkTask.h"
#include "Process\MkThread.h"

// NOTE: The thread(s) for  the schedulers must not be blocked for any appreciable
// time (see comments below).  Therefore it is usually the case that a task must be
// scheduled to do any real work.  This situation sets up the need for both a functor
// and a task to be created to respond to an event.  Since doing so is a collosal
// pain, the functors derive from tasks so that they can (usually) schedule themselves
// to do the real work.  The () operator is used for functor execution and the 
// Execute() method is used for task execution.  


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTimeScheduler
////////////////////////////////////////////////////////////////////////////////////////////////////

// The MkScheduler is for scheduling a functor to be executed at some time in the 
// future using a single, high priority thread.  The functor MUST execute quickly.
// See NOTE above.
// NOTE: Even with these considerations, it is likely that is possible to vapor lock
// the scheduler with poor programming practices.  Caveat emptor.  MRK.
class MkTimeScheduler
{
public:
    MK_EXPORT_FRAMEWORK MkTimeScheduler(const MkAString& name);
    MK_EXPORT_FRAMEWORK ~MkTimeScheduler() = default;

    // Client functors must inherit from MkTimeScheduler::Functor.
    class Functor : public MkTask, public std::enable_shared_from_this<Functor>
    {
    public:
        Functor(
                const MkAString& name, 
                Priority priority = MkTask::GetDefaultPriority(),
                const mkuint repeatCnt = 0,
                const MkDuration repeatDuration = MkDuration()) :
            MkTask(name, priority),
            m_repeatCnt(repeatCnt),
            m_repeatDuration(repeatDuration)
        { }
        virtual ~Functor() = default;

        virtual void operator()()
        { }

        bool Reschedule()
        {
            if (m_repeatCnt == 0) return false;
            --m_repeatCnt;
            return true;
        }

        const MkDuration& GetRepeatDuration() const
        { return m_repeatDuration; }

    protected:
        void ScheduleSelf();

        mkuint m_repeatCnt = 0;
        MkDuration m_repeatDuration = MkDuration();
    };

    bool MK_EXPORT_FRAMEWORK Schedule(const MkCPUTime cpuTime, const std::shared_ptr<Functor>& rFunctor)
    { return m_rThread->Schedule(cpuTime, rFunctor); }

    bool MK_EXPORT_FRAMEWORK Schedule(const MkDuration duration, const std::shared_ptr<Functor>& rFunctor)
    { return m_rThread->Schedule(MkCPUTime() + duration, rFunctor); }

    const MkAString& GetName() const
    { return m_name; }

private:
    class Thread : public MkThread
    {
    public:
        Thread(MkTimeScheduler* pTimeScheduler) :
            MkThread("TimeSchedulerThread"),
            m_pTimeScheduler(pTimeScheduler),
            m_rUpdateEvent(std::make_shared<MkOSEvent>("TimeScheduler", true))
        { StartThread(); }

        ~Thread() override = default;

        bool Schedule(const MkCPUTime cpuTime, const std::shared_ptr<Functor>& rFunctor);

    protected:
        void Execute() override;

    private:
        MkTimeScheduler* m_pTimeScheduler;

        std::multimap<MkCPUTime, std::shared_ptr<Functor>> m_functorMultiMap;

        std::shared_ptr<MkOSEvent> m_rUpdateEvent;
        std::vector<std::pair<MkCPUTime, std::shared_ptr<Functor>>> m_pendingFunctors;
        MkCriticalSectionLock m_pendingFunctorsLock;
    };

    MkAString m_name;

    std::shared_ptr<Thread> m_rThread;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkEventScheduler
////////////////////////////////////////////////////////////////////////////////////////////////////

// The MkEventScheduler is for scheduling a functor to be executed in response to an
// OS event  using a single, high priority thread.  The functor MUST execute quickly.
// See NOTE above.
// NOTE: Even with these considerations, it is likely that is possible to vapor lock
// the scheduler with poor programming practices.  Caveat emptor.  MRK.
// NOTE: If there are sufficient events to wait on, this class can be extended to
// multiple waiting threads.  It is a bit complicated so it will not be done until
// needed.  MRK.
class MkEventScheduler
{
public:
    MK_EXPORT_FRAMEWORK MkEventScheduler(const MkAString& name);
    MK_EXPORT_FRAMEWORK ~MkEventScheduler() = default;

    // Client functors must inherit from MkEventScheduler::Functor.
    class Functor : public MkTask, public std::enable_shared_from_this<Functor>
    {
    public:
        Functor(const MkAString& name, Priority priority = MkTask::GetDefaultPriority()) :
            MkTask(name, priority)
        { }
        virtual ~Functor() = default;

        virtual void operator()()
        { }

    protected:
        void ScheduleSelf();
    };

    void MK_EXPORT_FRAMEWORK Schedule(
            const std::shared_ptr<MkOSEvent>& rEvent, 
            const std::shared_ptr<Functor>& rFunctor,
            const bool persistent);

    void MK_EXPORT_FRAMEWORK Cancel(const std::shared_ptr<MkOSEvent>& rEvent);

    const MkAString& GetName() const
    { return m_name; }

private:
    class Thread : public MkThread
    {
    public:
        Thread(MkEventScheduler* pEventScheduler, const mkuint threadIdx) :
            MkThread("EventSchedulerThread"),
            m_pEventScheduler(pEventScheduler),
            m_rUpdateEvent(std::make_shared<MkOSEvent>("EventSchedulerUpdate", true)),
            m_threadIdx(threadIdx)
        { StartThread(); }

        ~Thread() override = default;

        void Update()
        { m_rUpdateEvent->Signal(); }

    protected:
        void Execute() override;

    private:
        void UpdateFunctors();

        MkEventScheduler* m_pEventScheduler;
        std::shared_ptr<MkOSEvent> m_rUpdateEvent;
        mkuint m_threadIdx;

        std::vector<std::tuple<std::shared_ptr<MkOSEvent>, std::shared_ptr<Functor>, bool>> m_eventFunctors;
    };

    friend class Thread;

    void AddThread();

    mkuint GetThreadCount() const
    { return m_threads.size(); }

    void ProcessFunctors(const mkuint threadIdx);

    MkAString m_name;

    std::vector<std::shared_ptr<Thread>> m_threads;
    MkCriticalSectionLock m_threadsLock;

    std::vector<std::tuple<std::shared_ptr<MkOSEvent>, std::shared_ptr<Functor>, bool>> m_pendingFunctors;
    std::vector<std::pair<std::shared_ptr<MkOSEvent>, mkuint>> m_pendingCancels;
    MkCriticalSectionLock m_pendingFunctorsLock;
};

