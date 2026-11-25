////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkString.h"

// C++ includes.
#include <thread>

// Windows includes.
#include <windows.h>

// Forward declarations.
class MkOSEvent;


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkThread
////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: The thread model assumes that threads are primarily used to support task pools
// and therefore have long lifetimes that do not involve "joining".  MRK.
// NOTE: It is required that derived leaf classes call StartThread at the end of their
// constructor to assure full construction before the thread is turned loose.  MRK.
// NOTE: A thread always has an exit event available, just because.  MRK.
class MkThread
{
public:
    MK_EXPORT_FOUNDATION MkThread(const MkAString& name);
    MK_EXPORT_FOUNDATION virtual ~MkThread();

    MK_EXPORT_FOUNDATION static mkuint32 GetCurrentThreadId()
    { return (mkuint32)::GetCurrentThreadId(); }

    const MkAString& GetName() const
    { return m_name; }

protected:
    MK_EXPORT_FOUNDATION void StartThread();

    virtual void Execute() = 0;

    std::shared_ptr<MkOSEvent> m_rExitEvent;
    
private:
    void StartExecution();

    std::thread m_thread;
    MkAString m_name;
    mkuint32 m_threadId;
    HANDLE m_threadHandle;
};