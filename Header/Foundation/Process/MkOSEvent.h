////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic\MkDuration.h"
#include "Basic\MkString.h"
#include "Process\MkCriticalSection.h"

// Windows includes.
#include <windows.h>

// NOTE: For whatever reason it just feels right for this enumeration to be in the
// global name space.  MRK. 
enum MkWaitStatus
{
    eFailed = -1,
    eEvent0 = 0,
    eTimeOut = 255,
    eAbandoned = 1000
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkOSEvent
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkOSEvent
{
public:
    MkOSEvent(const MkAString& eventName, bool manualReset) :
        m_osHandle(CreateEvent(nullptr, manualReset, FALSE, eventName.GetWString().get())),
        m_name(eventName)
    { }

    ~MkOSEvent()
    { CloseHandle(m_osHandle); }

    bool operator==(const MkOSEvent& rhs) const
    { return m_osHandle == rhs.m_osHandle; }

    bool Signal();
    bool Reset();

    MkWaitStatus WaitForObject(const MkDuration& timeOut);

    const MkAString& GetName() const
    { return m_name; }

    HANDLE GetHandle() const
    { return m_osHandle; }

    bool IsEventSignalled() const;

private:
    HANDLE m_osHandle;
    MkAString m_name;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkMultiOSEvent
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkMultiOSEvent
{
public:
    MK_EXPORT_FOUNDATION MkMultiOSEvent(const MkAString& name) :
        m_name(name)
    { }
    MK_EXPORT_FOUNDATION ~MkMultiOSEvent() = default;

    bool MK_EXPORT_FOUNDATION AddOSEvent(const std::shared_ptr<MkOSEvent>& rOSEvent);
    bool MK_EXPORT_FOUNDATION RemoveOSEvent(const std::shared_ptr<MkOSEvent>& rOSEvent);

    void MK_EXPORT_FOUNDATION Resize(const mkuint eventCnt);

    MkWaitStatus MK_EXPORT_FOUNDATION WaitForObjects(const MkDuration& timeOut);

    static mkuint GetMaximumEvents()
    { return MAXIMUM_WAIT_OBJECTS; }

    mkuint32 GetEventCount() const
    { return (mkuint32)m_osEvents.size(); }


private:
    std::vector<std::shared_ptr<MkOSEvent>> m_osEvents;
    std::vector<HANDLE> m_osHandles;
    MkCriticalSectionLock m_osEventsLock;

    MkAString m_name;
};