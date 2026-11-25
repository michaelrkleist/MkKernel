////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Process\MkCriticalSection.h"

// Mk includes.
#include "Process\MkThread.h"

// Windows includes.
#include <windows.h>

// Static initializations.
mkuint32 sNullThreadId = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCriticalSectionLock
////////////////////////////////////////////////////////////////////////////////////////////////////

MkCriticalSectionLock::MkCriticalSectionLock() :
    m_ownerThreadId(sNullThreadId),
    m_referenceCnt(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint MkCriticalSectionLock::Acquire()
{
    mkuint32* pOwnerThreadId = &m_ownerThreadId;
    mkuint32 requestingThreadId = MkThread::GetCurrentThreadId();

    while (true)
    {
        mkuint32 freeThreadId = sNullThreadId;
        mkuint32 previousOwnerThreadId = (mkuint32)_InterlockedCompareExchange(
                (volatile long*)pOwnerThreadId, requestingThreadId, freeThreadId);

        if ((previousOwnerThreadId == sNullThreadId) || (previousOwnerThreadId == requestingThreadId))
        {
            return ++m_referenceCnt;
        }

        // Wait a bit before trying again.  What is the right way to do it?  Should it
        // happen in longer and longer stages?
        Sleep(1);
    }

    return m_referenceCnt;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkCriticalSectionLock::TryAcquire()
{
    mkuint32* pOwnerThreadId = &m_ownerThreadId;
    mkuint32 requestingThreadId = GetCurrentThreadId();

    mkuint32 freeThreadId = sNullThreadId;
    mkuint32 previousOwnerThreadId = (mkuint32)_InterlockedCompareExchange(
            (volatile long*)pOwnerThreadId, requestingThreadId, freeThreadId);

    if ((previousOwnerThreadId == sNullThreadId) || (previousOwnerThreadId == requestingThreadId))
    {
        ++m_referenceCnt;
    }

    return ((previousOwnerThreadId == sNullThreadId) || (previousOwnerThreadId == requestingThreadId));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint MkCriticalSectionLock::Release()
{
    --m_referenceCnt;

    if (m_referenceCnt == 0)
    {
        mkuint32* pOwnerThreadId = &m_ownerThreadId;
        mkuint32 freeThreadId = sNullThreadId;

        _InterlockedCompareExchange((volatile long*)pOwnerThreadId, freeThreadId, m_ownerThreadId);
    }

    return m_referenceCnt;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCriticalSection
////////////////////////////////////////////////////////////////////////////////////////////////////

MkCriticalSection::MkCriticalSection(MkCriticalSectionLock& lock) :
    m_lock(lock)
{
    m_lock.Acquire();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkCriticalSection::~MkCriticalSection()
{
    m_lock.Release();
}
