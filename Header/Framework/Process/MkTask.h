////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFramework.h"
#include "Basic\MkDuration.h"
#include "Basic\MkString.h"

#define DEBUG_TASK 0


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkTask
////////////////////////////////////////////////////////////////////////////////////////////////////

// A task encapsulates some work to be done in a thread.
// NOTE:  By convention, a task must not indefinitely block (as opposed to blocking to
// read a disk file, for example).  If there is a need to wait for something to happen,
// the task must check if it has happened and reschedule if it has not happened.  MRK.

class MK_EXPORT_FRAMEWORK MkTask
{
public:
    friend class MkTaskPoolManager;
    friend class MkTaskThread;

    enum Status
    {
        eComplete,
        eIncomplete,     // Reschedule the task.
        eContinue      // Run the continuation task, if any.
    };

    enum Priority
    {
        eLow,
        eNormal,
        eHigh,
        ePriorityCnt
    };

    virtual Status Execute() = 0;

    const MkAString& GetName() const
    { return m_name; }

    void SetPriority(const Priority priority)
    { m_priority = priority; }

    Priority GetPriority() const
    { return m_priority; }

    static Priority GetDefaultPriority()
    { return m_sDefaultPriority; }

    void SetRescheduleDuration(const MkDuration& rescheduleDuration)
    { m_rescheduleDuration = rescheduleDuration; }

    MkDuration GetRescheduleDuration() const
    { return m_rescheduleDuration; }

    // NOTE: Only compare priority.  MRK.
    bool operator<(const std::shared_ptr<MkTask>& rOtherTask) const;

protected:
    MkTask(const MkAString& name, Priority priority = m_sDefaultPriority);
    virtual ~MkTask() = default;

#if DEBUG_TASK
    void SetCallStackString(const MkAString& callStackStr);

    const MkAString& GetCallStackString() const
    { return m_callStackStr; }
#endif

private:
    MkAString m_name;
    mutable Priority m_priority;
    MkDuration m_rescheduleDuration = MkDuration();

    static const MkAString m_sDefaultName;
    static const Priority m_sDefaultPriority = eLow;

#if DEBUG_TASK
    MkAString m_callStackStr;
#endif
};


#if 0
////////////////////////////////////////////////////////////////////////////////////////////////////
// MkSleepTask
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkSleepTask : public MkTask
{
public:
    MkSleepTask(const MkAString& name) :
        MkTask(name)
    { }
    ~MkSleepTask() override = default;
    void Execute() override
    {
        Sleep(10);
        { MkDebugSystemText() << "Executed sleep task '" << GetName().GetCString() << "'.\n"; }
    }
};
#endif