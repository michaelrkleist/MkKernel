////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Process\MkOSEvent.h"

#define DEBUG 0
#if DEBUG
#include "InputOutput\MkDebugText.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkOSEvent
////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkOSEvent::Signal()
{ 
#if DEBUG
    { MkDebugSystemText() << "Setting event " << m_osHandle << " called " << m_name.GetCString() << ".\n"; }
#endif

    if (::SetEvent(m_osHandle) == TRUE) return true;

    DWORD lastError = GetLastError();
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkOSEvent::Reset()
{
    if (::ResetEvent(m_osHandle) == TRUE) return true;

    DWORD lastError = GetLastError();
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkOSEvent::IsEventSignalled() const
{
    DWORD waitStatus = ::WaitForSingleObject(m_osHandle, 0);

    return bool(waitStatus == WAIT_OBJECT_0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkWaitStatus MkOSEvent::WaitForObject(const MkDuration& timeOut)
{
    mkuint32 milliseconds = 
            timeOut.IsForever() ? INFINITE : timeOut.GetScalar(MkDuration::eMilliseconds);

    DWORD waitStatus = ::WaitForSingleObject(m_osHandle, milliseconds);

    switch (waitStatus)
    {
    case WAIT_TIMEOUT : return eTimeOut;
    case WAIT_FAILED : return eFailed;
    case WAIT_ABANDONED : return eAbandoned;
    default : return eEvent0;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMultiOSEvent
////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkMultiOSEvent::AddOSEvent(const std::shared_ptr<MkOSEvent>& rOSEvent)
{
    MkCriticalSection cs(m_osEventsLock);
    if (sizeof(m_osEvents) >= MAXIMUM_WAIT_OBJECTS) return false;

    m_osEvents.push_back(rOSEvent);
    m_osHandles.push_back(rOSEvent->GetHandle());

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkMultiOSEvent::RemoveOSEvent(const std::shared_ptr<MkOSEvent>& rOSEvent)
{
    MkCriticalSection cs(m_osEventsLock);

    auto eventIter = m_osEvents.begin();
    auto handleIter = m_osHandles.begin();

    for ( ; eventIter != m_osEvents.end(); ++eventIter, ++handleIter)
    {
        if (*eventIter == rOSEvent)
        {
            m_osEvents.erase(eventIter);
            m_osHandles.erase(handleIter);
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MkMultiOSEvent::Resize(const mkuint eventCnt)
{
    m_osEvents.resize(eventCnt);
    m_osHandles.resize(eventCnt);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkWaitStatus MkMultiOSEvent::WaitForObjects(const MkDuration& timeOut)
{
    mkuint32 milliseconds = 
            timeOut.IsForever() ? INFINITE : timeOut.GetScalar(MkDuration::eMilliseconds);

    DWORD waitStatus = 
            ::WaitForMultipleObjects((DWORD)m_osHandles.size(), m_osHandles.data(), FALSE, milliseconds);

#if DEBUG
    { 
        MkDebugSystemText() << "Wait for " << m_name.GetCString() <<
                " multiple objects triggered with " << waitStatus << ".\n";
    }
#endif

    // NOTE: Abandoned needs to be treated differently if mutex's (mutices?) are involved.  MRK.
    switch (waitStatus)
    {
    case WAIT_TIMEOUT : return eTimeOut;
    case WAIT_FAILED : return eFailed;
    case WAIT_ABANDONED : return eAbandoned;
    default : return (MkWaitStatus)(waitStatus - WAIT_OBJECT_0);
    }
}

