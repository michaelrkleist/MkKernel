////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCriticalSectionLock
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkCriticalSectionLock
{
public:
    MkCriticalSectionLock();
    ~MkCriticalSectionLock() = default;

    mkuint Acquire();
    bool TryAcquire();

    mkuint Release();

    mkuint GetReferenceCount() const
    { return m_referenceCnt; }

private:
    MkCriticalSectionLock(const MkCriticalSectionLock& other) = delete;
    MkCriticalSectionLock& operator=(const MkCriticalSectionLock& rhs) = delete;

    mkuint32 m_ownerThreadId;
    mkuint m_referenceCnt;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkCriticalSection
////////////////////////////////////////////////////////////////////////////////////////////////////

class MK_EXPORT_FOUNDATION MkCriticalSection
{
public:
    MkCriticalSection(MkCriticalSectionLock& lock);
    ~MkCriticalSection();

private:
    MkCriticalSection(const MkCriticalSection& other) = delete;
    MkCriticalSection operator=(const MkCriticalSection& rhs) = delete;

    MkCriticalSectionLock& m_lock;
};
