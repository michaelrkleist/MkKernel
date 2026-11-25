////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Framework\MkFrameworkPCH.h"
#include "Process\MkFrameworkApplication.h"

// Mk includes.
#include "Basic\MkString.h"
#include "Process\MkTask.h"

// Static initializations.
namespace
{
std::map<MkTask::Priority, mkuint> sThreadCntMap = {
#if 1
        {MkTask::eLow, 4},
        {MkTask::eNormal, 4},
        {MkTask::eHigh, 2}};
#else
        {MkTask::eLow, 1},
        {MkTask::eNormal, 1},
        {MkTask::eHigh, 1}};
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkFrameworkApplication
////////////////////////////////////////////////////////////////////////////////////////////////////

void MkFrameworkApplication::Start()
{
    m_rTimeScheduler = std::make_shared<MkTimeScheduler>("Framework");
    m_rEventScheduler = std::make_shared<MkEventScheduler>("Framework");
    m_rSocketEventScheduler = std::make_shared<MkEventScheduler>("Socket");
    m_rTaskPool = std::make_shared<MkTaskPool>("Framework", sThreadCntMap, m_rTimeScheduler);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkFrameworkApplication::Stop()
{
    m_rTimeScheduler.reset();
    m_rEventScheduler.reset();
    m_rSocketEventScheduler.reset();
    m_rTaskPool.reset();
}