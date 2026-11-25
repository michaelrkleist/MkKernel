////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFramework.h"
#include "Process\MkSingleton.h"

// Mk includes.
#include "Process\MkScheduler.h"
#include "Process\MkTaskPool.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkFrameworkApplication
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FRAMEWORK MkFrameworkApplication : public MkSingleton<MkFrameworkApplication>
{
public:
    MkFrameworkApplication() = default;
    MkFrameworkApplication(const MkAString& name) :
        m_name(name)
    { }
    ~MkFrameworkApplication() = default;

    // TODO: Add feature identifiers to Start.  MRK.
    void Start();
    void Stop();

    MkTimeScheduler& GetTimeScheduler() const
    { return *m_rTimeScheduler; }

    MkEventScheduler& GetEventScheduler() const
    { return *m_rEventScheduler; }

    MkEventScheduler& GetSocketEventScheduler() const
    { return *m_rSocketEventScheduler; }

    MkTaskPool& GetTaskPool() const
    { return *m_rTaskPool; }

private:
    MkAString m_name;
    std::shared_ptr<MkTimeScheduler> m_rTimeScheduler = nullptr;
    std::shared_ptr<MkEventScheduler> m_rEventScheduler = nullptr;
    std::shared_ptr<MkEventScheduler> m_rSocketEventScheduler = nullptr;
    std::shared_ptr<MkTaskPool> m_rTaskPool = nullptr;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Namespace for a Framework Application
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Framework
{
    inline MkTimeScheduler& TimeScheduler()
    { return MkFrameworkApplication::Instance().GetTimeScheduler(); }

    inline MkEventScheduler& EventScheduler()
    { return MkFrameworkApplication::Instance().GetEventScheduler(); }

    inline MkTaskPool& TaskPool()
    { return MkFrameworkApplication::Instance().GetTaskPool(); }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Global Namespace for Socket Event Scheduler
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Socket
{
    inline MkEventScheduler& EventScheduler()
    { return MkFrameworkApplication::Instance().GetSocketEventScheduler(); }
}